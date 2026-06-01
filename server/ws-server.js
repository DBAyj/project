"use strict";

var http = require("http");
var crypto = require("crypto");
var Game = require("../utils/game-core");

var PORT = Number(process.env.PORT || 8787);
var HOST = process.env.HOST || "127.0.0.1";
var TICK_RATE = 30;
var rooms = new Map();
var clients = new Map();

function makeId(prefix) {
  return prefix + "-" + crypto.randomBytes(4).toString("hex");
}

function encodeFrame(text) {
  var payload = Buffer.from(text);
  var length = payload.length;
  var header;

  if (length < 126) {
    header = Buffer.alloc(2);
    header[1] = length;
  } else if (length < 65536) {
    header = Buffer.alloc(4);
    header[1] = 126;
    header.writeUInt16BE(length, 2);
  } else {
    header = Buffer.alloc(10);
    header[1] = 127;
    header.writeBigUInt64BE(BigInt(length), 2);
  }

  header[0] = 0x81;
  return Buffer.concat([header, payload]);
}

function send(client, packet) {
  if (!client || client.socket.destroyed) {
    return;
  }

  try {
    client.socket.write(encodeFrame(JSON.stringify(packet)));
  } catch (err) {
    console.error("send failed:", err.message);
  }
}

function broadcast(room, packet) {
  room.clients.forEach(function (client) {
    send(client, packet);
  });
}

function parseFrame(buffer) {
  if (buffer.length < 2) {
    return null;
  }

  var first = buffer[0];
  var second = buffer[1];
  var opcode = first & 0x0f;
  var masked = (second & 0x80) === 0x80;
  var length = second & 0x7f;
  var offset = 2;

  if (length === 126) {
    if (buffer.length < offset + 2) {
      return null;
    }
    length = buffer.readUInt16BE(offset);
    offset += 2;
  } else if (length === 127) {
    if (buffer.length < offset + 8) {
      return null;
    }
    var bigLength = buffer.readBigUInt64BE(offset);
    if (bigLength > BigInt(Number.MAX_SAFE_INTEGER)) {
      throw new Error("Frame too large");
    }
    length = Number(bigLength);
    offset += 8;
  }

  var mask;
  if (masked) {
    if (buffer.length < offset + 4) {
      return null;
    }
    mask = buffer.slice(offset, offset + 4);
    offset += 4;
  }

  if (buffer.length < offset + length) {
    return null;
  }

  var payload = Buffer.from(buffer.slice(offset, offset + length));
  if (masked) {
    for (var i = 0; i < payload.length; i += 1) {
      payload[i] ^= mask[i % 4];
    }
  }

  return {
    opcode: opcode,
    text: payload.toString("utf8"),
    rest: buffer.slice(offset + length)
  };
}

function getRoom(roomId) {
  var id = String(roomId || "arena-1").trim() || "arena-1";
  var room = rooms.get(id);
  if (!room) {
    room = {
      id: id,
      state: Game.createGameState(),
      clients: new Map(),
      lastActiveAt: Date.now()
    };
    rooms.set(id, room);
  }
  return room;
}

function leaveRoom(client) {
  var room = client.room;
  if (!room) {
    return;
  }

  room.clients.delete(client.id);
  Game.removePlayer(room.state, client.id);
  room.lastActiveAt = Date.now();
  broadcast(room, {
    type: "notice",
    message: client.name + " 离开了房间"
  });
  broadcast(room, {
    type: "snapshot",
    state: Game.publicState(room.state)
  });
  client.room = null;
}

function handlePacket(client, packet) {
  if (packet.type === "join") {
    leaveRoom(client);

    var room = getRoom(packet.roomId);
    client.name = String(packet.name || "玩家").trim().slice(0, 12) || "玩家";
    client.room = room;
    room.clients.set(client.id, client);
    room.lastActiveAt = Date.now();
    Game.addPlayer(room.state, client.id, {
      name: client.name
    });

    send(client, {
      type: "welcome",
      playerId: client.id,
      state: Game.publicState(room.state)
    });
    broadcast(room, {
      type: "notice",
      message: client.name + " 加入了房间 " + room.id
    });
    return;
  }

  if (packet.type === "input" && client.room) {
    Game.setInput(client.room.state, client.id, packet.input);
    client.room.lastActiveAt = Date.now();
    return;
  }

  if (packet.type === "ping") {
    send(client, { type: "pong", time: Date.now() });
  }
}

function handleData(client, chunk) {
  client.buffer = Buffer.concat([client.buffer, chunk]);

  while (client.buffer.length > 0) {
    var frame = parseFrame(client.buffer);
    if (!frame) {
      return;
    }
    client.buffer = frame.rest;

    if (frame.opcode === 0x8) {
      client.socket.end();
      return;
    }

    if (frame.opcode !== 0x1) {
      continue;
    }

    try {
      handlePacket(client, JSON.parse(frame.text));
    } catch (err) {
      send(client, {
        type: "notice",
        message: "服务器无法解析消息"
      });
      console.error("bad packet:", err.message);
    }
  }
}

var server = http.createServer(function (req, res) {
  if (req.url === "/health") {
    res.writeHead(200, { "content-type": "application/json; charset=utf-8" });
    res.end(JSON.stringify({
      ok: true,
      rooms: rooms.size,
      clients: clients.size,
      uptime: Math.round(process.uptime())
    }) + "\n");
    return;
  }

  res.writeHead(200, { "content-type": "text/plain; charset=utf-8" });
  res.end("Neon Arcade Battle WebSocket server is running.\n");
});

server.on("upgrade", function (req, socket) {
  var key = req.headers["sec-websocket-key"];
  if (!key) {
    socket.destroy();
    return;
  }

  var accept = crypto
    .createHash("sha1")
    .update(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
    .digest("base64");

  socket.write([
    "HTTP/1.1 101 Switching Protocols",
    "Upgrade: websocket",
    "Connection: Upgrade",
    "Sec-WebSocket-Accept: " + accept,
    "",
    ""
  ].join("\r\n"));

  var client = {
    id: makeId("p"),
    name: "玩家",
    socket: socket,
    room: null,
    buffer: Buffer.alloc(0)
  };
  clients.set(client.id, client);

  socket.on("data", function (chunk) {
    try {
      handleData(client, chunk);
    } catch (err) {
      console.error("socket data error:", err.message);
      socket.destroy();
    }
  });

  socket.on("close", function () {
    leaveRoom(client);
    clients.delete(client.id);
  });

  socket.on("error", function () {
    leaveRoom(client);
    clients.delete(client.id);
  });
});

setInterval(function () {
  var now = Date.now();
  rooms.forEach(function (room, roomId) {
    if (room.clients.size === 0 && now - room.lastActiveAt > 60000) {
      rooms.delete(roomId);
      return;
    }

    if (room.clients.size > 0) {
      Game.stepGame(room.state, 1 / TICK_RATE);
      broadcast(room, {
        type: "snapshot",
        state: Game.publicState(room.state)
      });
    }
  });
}, 1000 / TICK_RATE);

server.listen(PORT, HOST, function () {
  console.log("Neon Arcade Battle server listening on ws://" + HOST + ":" + PORT);
});

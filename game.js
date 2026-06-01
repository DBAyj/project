"use strict";

var Game = require("./utils/game-core");
var Realtime = require("./utils/realtime");
var Config = require("./utils/config");

var PLAYER_ONE = "local-one";
var PLAYER_TWO = "local-two";
var MENU_HEIGHT = 76;

function safeName(value) {
  var text = String(value || "").trim();
  return text || "玩家";
}

function touchId(touch) {
  if (touch.identifier !== undefined) {
    return touch.identifier;
  }
  return touch.id;
}

function nowMs() {
  return Date.now ? Date.now() : new Date().getTime();
}

function ArcadeGame() {
  this.canvas = wx.createCanvas();
  this.ctx = this.canvas.getContext("2d");
  this.system = wx.getSystemInfoSync ? wx.getSystemInfoSync() : {};
  this.pixelRatio = this.system.pixelRatio || 1;
  this.viewWidth = this.system.windowWidth || this.canvas.width || 812;
  this.viewHeight = this.system.windowHeight || this.canvas.height || 375;
  this.buttons = [];
  this.leftStick = null;
  this.rightStick = null;
  this.localInput = { moveX: 0, moveY: 0, aimX: 1, aimY: 0, fire: false };
  this.localPlayerId = PLAYER_ONE;
  this.mode = "practice";
  this.statusLabel = "练习";
  this.hint = "左半屏移动，右半屏瞄准并开火。";
  this.roomId = Config.roomId || "arena-1";
  this.serverUrl = Config.serverUrl || "";
  this.connected = false;
  this.state = null;
  this.onlineState = null;
  this.realtime = null;
  this.lastFrameAt = 0;
  this.lastInputSentAt = 0;
  this.running = true;

  this.setupCanvas();
  this.bindTouches();
  this.startPractice();
  this.loop();

  if (wx.setKeepScreenOn) {
    wx.setKeepScreenOn({ keepScreenOn: true });
  }
}

ArcadeGame.prototype.setupCanvas = function () {
  this.canvas.width = this.viewWidth * this.pixelRatio;
  this.canvas.height = this.viewHeight * this.pixelRatio;
  this.ctx.scale(this.pixelRatio, this.pixelRatio);
};

ArcadeGame.prototype.scheduleFrame = function (callback) {
  if (this.canvas.requestAnimationFrame) {
    return this.canvas.requestAnimationFrame(callback);
  }
  if (typeof requestAnimationFrame === "function") {
    return requestAnimationFrame(callback);
  }
  return setTimeout(function () {
    callback(nowMs());
  }, 16);
};

ArcadeGame.prototype.bindTouches = function () {
  var self = this;
  wx.onTouchStart(function (event) {
    var touches = event.changedTouches || [];
    for (var i = 0; i < touches.length; i += 1) {
      self.handleTouchStart(touches[i]);
    }
  });
  wx.onTouchMove(function (event) {
    var touches = event.touches || [];
    for (var i = 0; i < touches.length; i += 1) {
      self.handleTouchMove(touches[i]);
    }
  });
  wx.onTouchEnd(function (event) {
    var touches = event.changedTouches || [];
    for (var i = 0; i < touches.length; i += 1) {
      self.handleTouchEnd(touches[i]);
    }
  });
  wx.onTouchCancel(function (event) {
    var touches = event.changedTouches || [];
    for (var i = 0; i < touches.length; i += 1) {
      self.handleTouchEnd(touches[i]);
    }
  });
};

ArcadeGame.prototype.startPractice = function () {
  this.closeRealtimeOnly();
  this.mode = "practice";
  this.statusLabel = "练习";
  this.connected = false;
  this.localPlayerId = PLAYER_ONE;
  this.state = Game.createGameState();
  Game.addPlayer(this.state, PLAYER_ONE, {
    name: safeName("玩家"),
    color: "#00d1c1"
  });
  Game.addPlayer(this.state, "bot-spark", { name: "Spark", color: "#ff6b4a", isBot: true });
  Game.addPlayer(this.state, "bot-byte", { name: "Byte", color: "#ffd166", isBot: true });
  this.onlineState = null;
  this.resetSticks();
  this.hint = "练习模式：移动、瞄准、开火，拾取绿色修复包和黄色超频芯片。";
};

ArcadeGame.prototype.startLocalDuel = function () {
  this.closeRealtimeOnly();
  this.mode = "local-duel";
  this.statusLabel = "双人";
  this.connected = false;
  this.localPlayerId = PLAYER_ONE;
  this.state = Game.createGameState();
  Game.addPlayer(this.state, PLAYER_ONE, { name: "左手", color: "#00d1c1" });
  Game.addPlayer(this.state, PLAYER_TWO, { name: "右手", color: "#ff6b4a" });
  this.onlineState = null;
  this.resetSticks();
  this.hint = "本机双人：左半屏控制青色玩家，右半屏控制橙色玩家。";
};

ArcadeGame.prototype.joinRoom = function () {
  this.closeRealtimeOnly();
  this.mode = "online";
  this.statusLabel = "连接中";
  this.connected = false;
  this.state = null;
  this.onlineState = Game.createGameState();
  this.resetSticks();
  this.hint = this.serverUrl ? "正在连接房间 " + this.roomId + "。" : "缺少 WebSocket 地址，请先配置线上 wss。";

  this.realtime = new Realtime.RealtimeClient({
    onEvent: this.handleRealtimeEvent.bind(this)
  });
  this.realtime.connect({
    url: this.serverUrl,
    roomId: this.roomId,
    name: safeName("玩家")
  });
};

ArcadeGame.prototype.closeRealtimeOnly = function () {
  if (this.realtime) {
    this.realtime.close();
    this.realtime = null;
  }
};

ArcadeGame.prototype.handleRealtimeEvent = function (type, payload) {
  if (type === "open") {
    this.connected = true;
    this.statusLabel = "在线";
    return;
  }
  if (type === "close") {
    if (this.mode === "online") {
      this.connected = false;
      this.statusLabel = "离线";
      this.hint = "连接已断开，可以重新进入在线对战。";
    }
    return;
  }
  if (type === "error") {
    this.connected = false;
    this.statusLabel = "异常";
    this.hint = payload.message || "多人连接异常，请检查服务地址和小游戏域名配置。";
    return;
  }

  var packet = payload;
  if (type !== "message" || !packet) {
    return;
  }
  if (packet.type === "welcome") {
    this.localPlayerId = packet.playerId;
    this.onlineState = packet.state || this.onlineState;
    this.connected = true;
    this.statusLabel = "在线";
    this.hint = "已进入房间 " + this.roomId + "，等待其他玩家即可开战。";
    return;
  }
  if (packet.type === "snapshot") {
    this.onlineState = packet.state;
    return;
  }
  if (packet.type === "notice") {
    this.hint = packet.message;
  }
};

ArcadeGame.prototype.loop = function () {
  var self = this;
  this.scheduleFrame(function (timestamp) {
    self.tick(timestamp || nowMs());
  });
};

ArcadeGame.prototype.tick = function (timestamp) {
  if (!this.running) {
    return;
  }

  var now = timestamp || nowMs();
  var dt = this.lastFrameAt ? (now - this.lastFrameAt) / 1000 : 0.016;
  this.lastFrameAt = now;
  this.updateInputs();

  if (this.mode === "online") {
    if (this.realtime && this.realtime.connected && now - this.lastInputSentAt > 45) {
      this.realtime.sendInput(this.localInput);
      this.lastInputSentAt = now;
    }
    this.draw(this.onlineState, true);
  } else if (this.state) {
    this.updateBots();
    Game.stepGame(this.state, dt);
    this.draw(this.state, false);
  }

  this.loop();
};

ArcadeGame.prototype.updateBots = function () {
  if (!this.state || this.mode !== "practice") {
    return;
  }
  Object.keys(this.state.players).forEach(function (id) {
    var player = this.state.players[id];
    if (player.isBot) {
      Game.setInput(this.state, id, Game.createBotInput(this.state, id));
    }
  }, this);
};

ArcadeGame.prototype.updateInputs = function () {
  var move = this.stickVector(this.leftStick);
  var aim = { x: this.localInput.aimX || 1, y: this.localInput.aimY || 0 };
  var stateForAim = this.mode === "online" ? this.onlineState : this.state;

  if (this.mode === "local-duel") {
    this.updateLocalDuelInputs();
    return;
  }

  if (this.rightStick && stateForAim) {
    var player = stateForAim.players && stateForAim.players[this.localPlayerId];
    var target = this.pointToWorld(this.rightStick.x, this.rightStick.y, stateForAim);
    if (player) {
      var vector = Game.normalize(target.x - player.x, target.y - player.y);
      if (vector.len > 0.01) {
        aim = { x: vector.x, y: vector.y };
      }
    }
  }

  this.localInput = {
    moveX: move.x,
    moveY: move.y,
    aimX: aim.x,
    aimY: aim.y,
    fire: !!this.rightStick
  };

  if (this.state && this.mode === "practice") {
    Game.setInput(this.state, this.localPlayerId, this.localInput);
  }
};

ArcadeGame.prototype.updateLocalDuelInputs = function () {
  if (!this.state) {
    return;
  }

  var one = this.state.players[PLAYER_ONE];
  var two = this.state.players[PLAYER_TWO];
  if (!one || !two) {
    return;
  }

  var leftMove = this.stickVector(this.leftStick);
  var rightMove = this.stickVector(this.rightStick);
  var oneAim = Game.normalize(two.x - one.x, two.y - one.y);
  var twoAim = Game.normalize(one.x - two.x, one.y - two.y);

  Game.setInput(this.state, PLAYER_ONE, {
    moveX: leftMove.x,
    moveY: leftMove.y,
    aimX: oneAim.x || 1,
    aimY: oneAim.y || 0,
    fire: !!this.leftStick
  });

  Game.setInput(this.state, PLAYER_TWO, {
    moveX: rightMove.x,
    moveY: rightMove.y,
    aimX: twoAim.x || -1,
    aimY: twoAim.y || 0,
    fire: !!this.rightStick
  });
};

ArcadeGame.prototype.handleTouchStart = function (touch) {
  var point = this.touchPoint(touch);
  if (this.hitButton(point.x, point.y)) {
    return;
  }

  var id = touchId(touch);
  var stick = {
    id: id,
    startX: point.x,
    startY: point.y,
    x: point.x,
    y: point.y
  };

  if (point.x < this.viewWidth * 0.5) {
    this.leftStick = stick;
  } else {
    this.rightStick = stick;
  }
};

ArcadeGame.prototype.handleTouchMove = function (touch) {
  var point = this.touchPoint(touch);
  var id = touchId(touch);
  if (this.leftStick && this.leftStick.id === id) {
    this.leftStick.x = point.x;
    this.leftStick.y = point.y;
  }
  if (this.rightStick && this.rightStick.id === id) {
    this.rightStick.x = point.x;
    this.rightStick.y = point.y;
  }
};

ArcadeGame.prototype.handleTouchEnd = function (touch) {
  var id = touchId(touch);
  if (this.leftStick && this.leftStick.id === id) {
    this.leftStick = null;
  }
  if (this.rightStick && this.rightStick.id === id) {
    this.rightStick = null;
  }
};

ArcadeGame.prototype.touchPoint = function (touch) {
  return {
    x: Number(touch.clientX !== undefined ? touch.clientX : touch.x) || 0,
    y: Number(touch.clientY !== undefined ? touch.clientY : touch.y) || 0
  };
};

ArcadeGame.prototype.hitButton = function (x, y) {
  for (var i = 0; i < this.buttons.length; i += 1) {
    var button = this.buttons[i];
    if (x >= button.x && x <= button.x + button.w && y >= button.y && y <= button.y + button.h) {
      button.action();
      return true;
    }
  }
  return false;
};

ArcadeGame.prototype.resetSticks = function () {
  this.leftStick = null;
  this.rightStick = null;
  this.localInput = { moveX: 0, moveY: 0, aimX: 1, aimY: 0, fire: false };
};

ArcadeGame.prototype.stickVector = function (stick) {
  if (!stick) {
    return { x: 0, y: 0 };
  }
  var dx = stick.x - stick.startX;
  var dy = stick.y - stick.startY;
  var max = 58;
  var vector = Game.normalize(dx, dy);
  var ratio = Math.min(1, vector.len / max);
  return {
    x: vector.x * ratio,
    y: vector.y * ratio
  };
};

ArcadeGame.prototype.pointToWorld = function (x, y, state) {
  state = state || { width: Game.ARENA_WIDTH, height: Game.ARENA_HEIGHT };
  return {
    x: x / this.viewWidth * state.width,
    y: y / Math.max(1, this.viewHeight - MENU_HEIGHT) * state.height
  };
};

ArcadeGame.prototype.draw = function (state, online) {
  var ctx = this.ctx;
  var width = this.viewWidth;
  var height = this.viewHeight;
  var playHeight = Math.max(1, height - MENU_HEIGHT);

  ctx.clearRect(0, 0, width, height);
  if (!state) {
    this.drawEmpty(ctx, width, playHeight, "等待竞技场数据");
  } else {
    ctx.save();
    ctx.scale(width / state.width, playHeight / state.height);
    this.drawWorld(ctx, state);
    this.drawPickups(ctx, state);
    this.drawBullets(ctx, state);
    this.drawPlayers(ctx, state);
    ctx.restore();
  }

  this.drawHud(ctx, state, width, height, playHeight, online);
  this.drawSticks(ctx);
  if (online && state && state.players && Object.keys(state.players).length < 2) {
    this.drawBanner(ctx, width, playHeight, "等待其他玩家加入");
  }
};

ArcadeGame.prototype.drawEmpty = function (ctx, width, height, message) {
  ctx.fillStyle = "#171a20";
  ctx.fillRect(0, 0, width, height);
  ctx.fillStyle = "#d9ddd5";
  ctx.font = "16px sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(message, width / 2, height / 2);
};

ArcadeGame.prototype.drawWorld = function (ctx, state) {
  var width = state.width;
  var height = state.height;
  var pulse = (Math.sin(state.elapsed * 1.8) + 1) * 0.5;

  ctx.fillStyle = "#171a20";
  ctx.fillRect(0, 0, width, height);

  ctx.strokeStyle = "rgba(247, 247, 242, 0.08)";
  ctx.lineWidth = 1;
  for (var x = 0; x <= width; x += 48) {
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, height);
    ctx.stroke();
  }
  for (var y = 0; y <= height; y += 48) {
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
  }

  ctx.strokeStyle = "rgba(0, 209, 193, " + (0.25 + pulse * 0.18) + ")";
  ctx.lineWidth = 4;
  ctx.beginPath();
  ctx.arc(width / 2, height / 2, 82, 0, Math.PI * 2);
  ctx.stroke();

  ctx.strokeStyle = "rgba(255, 107, 74, 0.34)";
  ctx.strokeRect(18, 18, width - 36, height - 36);
};

ArcadeGame.prototype.drawPickups = function (ctx, state) {
  (state.pickups || []).forEach(function (pickup) {
    ctx.save();
    ctx.fillStyle = pickup.color;
    ctx.shadowColor = pickup.color;
    ctx.shadowBlur = 16;
    ctx.beginPath();
    ctx.arc(pickup.x, pickup.y, 13, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = "#171a20";
    ctx.font = "bold 16px sans-serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(pickup.type === "heal" ? "+" : "x", pickup.x, pickup.y + 1);
    ctx.restore();
  });
};

ArcadeGame.prototype.drawBullets = function (ctx, state) {
  (state.bullets || []).forEach(function (bullet) {
    ctx.save();
    ctx.fillStyle = bullet.color;
    ctx.shadowColor = bullet.color;
    ctx.shadowBlur = 12;
    ctx.beginPath();
    ctx.arc(bullet.x, bullet.y, Game.BULLET_RADIUS, 0, Math.PI * 2);
    ctx.fill();
    ctx.restore();
  });
};

ArcadeGame.prototype.drawPlayers = function (ctx, state) {
  var self = this;
  Object.keys(state.players || {}).forEach(function (id) {
    var player = state.players[id];
    var dead = player.hp <= 0;
    var radius = Game.PLAYER_RADIUS;

    ctx.save();
    ctx.globalAlpha = dead ? 0.35 : 1;
    ctx.shadowColor = player.color;
    ctx.shadowBlur = dead ? 0 : 18;
    ctx.fillStyle = player.color;
    ctx.beginPath();
    ctx.arc(player.x, player.y, radius, 0, Math.PI * 2);
    ctx.fill();
    ctx.shadowBlur = 0;

    ctx.strokeStyle = player.id === self.localPlayerId ? "#f7f7f2" : "rgba(247,247,242,0.38)";
    ctx.lineWidth = player.id === self.localPlayerId ? 3 : 2;
    ctx.beginPath();
    ctx.arc(player.x, player.y, radius + 5, 0, Math.PI * 2);
    ctx.stroke();

    ctx.strokeStyle = "#111318";
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.moveTo(player.x, player.y);
    ctx.lineTo(player.x + Math.cos(player.angle) * 29, player.y + Math.sin(player.angle) * 29);
    ctx.stroke();

    ctx.fillStyle = "rgba(0, 0, 0, 0.42)";
    ctx.fillRect(player.x - 28, player.y - 38, 56, 6);
    ctx.fillStyle = player.hp > 35 ? "#4bd37b" : "#ff6b4a";
    ctx.fillRect(player.x - 28, player.y - 38, 56 * Math.max(0, player.hp / player.maxHp), 6);

    ctx.fillStyle = "#f7f7f2";
    ctx.font = "13px sans-serif";
    ctx.textAlign = "center";
    ctx.fillText(dead ? "复活中" : player.name, player.x, player.y - 48);

    if (player.powerTimer > 0 && !dead) {
      ctx.strokeStyle = "#ffd166";
      ctx.lineWidth = 3;
      ctx.beginPath();
      ctx.arc(player.x, player.y, radius + 10, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * (player.powerTimer / 5.2));
      ctx.stroke();
    }
    ctx.restore();
  });
};

ArcadeGame.prototype.drawHud = function (ctx, state, width, height, playHeight, online) {
  var panelTop = playHeight;
  var self = this;
  this.buttons = [];

  ctx.save();
  ctx.fillStyle = "#101318";
  ctx.fillRect(0, panelTop, width, height - panelTop);
  ctx.strokeStyle = "rgba(247, 247, 242, 0.12)";
  ctx.beginPath();
  ctx.moveTo(0, panelTop + 0.5);
  ctx.lineTo(width, panelTop + 0.5);
  ctx.stroke();

  ctx.fillStyle = "#f7f7f2";
  ctx.font = "bold 16px sans-serif";
  ctx.textAlign = "left";
  ctx.textBaseline = "top";
  ctx.fillText("霓虹街区", 14, panelTop + 10);

  ctx.fillStyle = online && this.connected ? "#4bd37b" : "#ffd166";
  ctx.font = "12px sans-serif";
  ctx.fillText(this.statusLabel + " / " + this.roomId, 14, panelTop + 34);

  this.addButton(ctx, width - 286, panelTop + 14, 82, 42, "练习", function () {
    self.startPractice();
  }, this.mode === "practice");
  this.addButton(ctx, width - 194, panelTop + 14, 82, 42, "双人", function () {
    self.startLocalDuel();
  }, this.mode === "local-duel");
  this.addButton(ctx, width - 102, panelTop + 14, 88, 42, "在线", function () {
    self.joinRoom();
  }, this.mode === "online");

  ctx.fillStyle = "rgba(247,247,242,0.76)";
  ctx.font = "12px sans-serif";
  ctx.textAlign = "center";
  ctx.fillText(this.hint, width / 2, Math.max(18, panelTop - 10));

  this.drawScoreboard(ctx, state, width);
  ctx.restore();
};

ArcadeGame.prototype.addButton = function (ctx, x, y, w, h, label, action, active) {
  ctx.save();
  ctx.fillStyle = active ? "#00d1c1" : "#222832";
  ctx.strokeStyle = active ? "#d9fff9" : "rgba(247,247,242,0.18)";
  ctx.lineWidth = 1;
  ctx.beginPath();
  ctx.rect(x, y, w, h);
  ctx.fill();
  ctx.stroke();
  ctx.fillStyle = active ? "#0e1116" : "#f7f7f2";
  ctx.font = "bold 14px sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(label, x + w / 2, y + h / 2);
  ctx.restore();
  this.buttons.push({ x: x, y: y, w: w, h: h, action: action });
};

ArcadeGame.prototype.drawScoreboard = function (ctx, state, width) {
  if (!state || !state.players) {
    return;
  }
  var board = Object.keys(state.players).map(function (id) {
    return state.players[id];
  }).sort(function (a, b) {
    return b.score - a.score;
  }).slice(0, 4);

  ctx.save();
  ctx.textAlign = "right";
  ctx.textBaseline = "top";
  board.forEach(function (player, index) {
    ctx.fillStyle = player.color;
    ctx.font = "bold 12px sans-serif";
    ctx.fillText(player.name + "  " + player.score + "  HP " + Math.ceil(player.hp), width - 14, 12 + index * 18);
  });
  ctx.restore();
};

ArcadeGame.prototype.drawSticks = function (ctx) {
  this.drawStick(ctx, this.leftStick, "#00d1c1");
  this.drawStick(ctx, this.rightStick, "#ff6b4a");
};

ArcadeGame.prototype.drawStick = function (ctx, stick, color) {
  if (!stick) {
    return;
  }
  var vector = this.stickVector(stick);
  var knobX = stick.startX + vector.x * 58;
  var knobY = stick.startY + vector.y * 58;

  ctx.save();
  ctx.strokeStyle = color;
  ctx.globalAlpha = 0.72;
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.arc(stick.startX, stick.startY, 58, 0, Math.PI * 2);
  ctx.stroke();
  ctx.fillStyle = color;
  ctx.globalAlpha = 0.88;
  ctx.beginPath();
  ctx.arc(knobX, knobY, 22, 0, Math.PI * 2);
  ctx.fill();
  ctx.restore();
};

ArcadeGame.prototype.drawBanner = function (ctx, width, height, text) {
  ctx.save();
  ctx.fillStyle = "rgba(17, 19, 24, 0.72)";
  ctx.fillRect(width * 0.25, height * 0.42, width * 0.5, 44);
  ctx.fillStyle = "#f7f7f2";
  ctx.font = "15px sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(text, width / 2, height * 0.42 + 22);
  ctx.restore();
};

new ArcadeGame();

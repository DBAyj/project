"use strict";

function RealtimeClient(options) {
  options = options || {};
  this.onEvent = options.onEvent || function () {};
  this.socket = null;
  this.connected = false;
  this.playerId = "";
  this.roomId = "";
  this.url = "";
}

RealtimeClient.prototype.emit = function (type, payload) {
  this.onEvent(type, payload || {});
};

RealtimeClient.prototype.connect = function (options) {
  var self = this;
  options = options || {};
  this.close();
  this.url = options.url;
  this.roomId = options.roomId || "arena-1";
  this.connected = false;

  if (!this.url) {
    this.emit("error", { message: "缺少 WebSocket 地址" });
    return;
  }

  var socket = wx.connectSocket({
    url: this.url,
    fail: function (err) {
      self.emit("error", { message: "连接失败", detail: err });
    }
  });

  this.socket = socket;

  socket.onOpen(function () {
    self.connected = true;
    self.emit("open", {});
    self.send({
      type: "join",
      roomId: self.roomId,
      name: options.name || "玩家"
    });
  });

  socket.onMessage(function (message) {
    var data = message.data;
    if (typeof data !== "string") {
      try {
        data = String.fromCharCode.apply(null, new Uint8Array(data));
      } catch (err) {
        self.emit("error", { message: "收到无法解析的消息" });
        return;
      }
    }

    try {
      var packet = JSON.parse(data);
      if (packet.type === "welcome") {
        self.playerId = packet.playerId;
      }
      self.emit("message", packet);
    } catch (err2) {
      self.emit("error", { message: "消息格式错误", detail: err2 });
    }
  });

  socket.onClose(function () {
    self.connected = false;
    self.emit("close", {});
  });

  socket.onError(function (err) {
    self.connected = false;
    self.emit("error", { message: "WebSocket 异常", detail: err });
  });
};

RealtimeClient.prototype.send = function (packet) {
  if (!this.socket || !this.connected) {
    return;
  }

  this.socket.send({
    data: JSON.stringify(packet)
  });
};

RealtimeClient.prototype.sendInput = function (input) {
  this.send({
    type: "input",
    input: input
  });
};

RealtimeClient.prototype.close = function () {
  if (this.socket) {
    try {
      this.socket.close({});
    } catch (err) {
      // WeChat throws if the task already closed; the page can ignore that.
    }
  }
  this.socket = null;
  this.connected = false;
};

module.exports = {
  RealtimeClient: RealtimeClient
};

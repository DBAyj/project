"use strict";

var Game = require("../../utils/game-core");
var Realtime = require("../../utils/realtime");

var WORLD_WIDTH = Game.ARENA_WIDTH;
var WORLD_HEIGHT = Game.ARENA_HEIGHT;
var PLAYER_ONE = "local-one";
var PLAYER_TWO = "local-two";

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

Page({
  data: {
    connected: false,
    statusLabel: "本地",
    playerName: "玩家",
    roomId: "arena-1",
    serverUrl: "",
    scoreboard: [],
    hint: "左半屏拖动移动，右半屏按住瞄准并开火。在线对战需要可用的 WebSocket 对战服务。"
  },

  onReady: function () {
    var app = getApp();
    this.setData({
      serverUrl: app.globalData.defaultServerUrl,
      roomId: app.globalData.defaultRoomId
    });

    this.mode = "practice";
    this.localPlayerId = PLAYER_ONE;
    this.leftStick = null;
    this.rightStick = null;
    this.localInput = { moveX: 0, moveY: 0, aimX: 1, aimY: 0, fire: false };
    this.lastFrameAt = 0;
    this.lastInputSentAt = 0;
    this.lastScoreSyncAt = 0;
    this.running = true;
    this.initCanvas();
  },

  onUnload: function () {
    this.running = false;
    if (this.realtime) {
      this.realtime.close();
    }
    if (this.canvas && this.canvas.cancelAnimationFrame && this.rafId) {
      this.canvas.cancelAnimationFrame(this.rafId);
    }
    if (this.frameTimer) {
      clearTimeout(this.frameTimer);
    }
  },

  initCanvas: function () {
    var self = this;
    wx.createSelectorQuery()
      .in(this)
      .select("#arenaCanvas")
      .fields({ node: true, size: true })
      .exec(function (res) {
        var canvasInfo = res && res[0];
        if (!canvasInfo || !canvasInfo.node) {
          self.setData({ hint: "Canvas 初始化失败，请确认微信基础库支持 type=2d Canvas。" });
          return;
        }

        var pixelRatio = wx.getSystemInfoSync().pixelRatio || 1;
        self.canvas = canvasInfo.node;
        self.ctx = self.canvas.getContext("2d");
        self.canvasSize = {
          width: canvasInfo.width,
          height: canvasInfo.height
        };
        self.canvas.width = canvasInfo.width * pixelRatio;
        self.canvas.height = canvasInfo.height * pixelRatio;
        self.ctx.scale(pixelRatio, pixelRatio);

        self.startPractice();
        self.scheduleFrame(function (timestamp) {
          self.tick(timestamp || Date.now());
        });
      });
  },

  scheduleFrame: function (callback) {
    var self = this;
    if (this.canvas && this.canvas.requestAnimationFrame) {
      this.rafId = this.canvas.requestAnimationFrame(callback);
      return;
    }
    this.frameTimer = setTimeout(function () {
      callback(Date.now());
    }, 16);
  },

  startPractice: function () {
    this.closeRealtimeOnly();
    this.mode = "practice";
    this.localPlayerId = PLAYER_ONE;
    this.state = Game.createGameState();
    Game.addPlayer(this.state, PLAYER_ONE, {
      name: safeName(this.data.playerName),
      color: "#00d1c1"
    });
    Game.addPlayer(this.state, "bot-spark", { name: "Spark", color: "#ff6b4a", isBot: true });
    Game.addPlayer(this.state, "bot-byte", { name: "Byte", color: "#ffd166", isBot: true });
    this.onlineState = null;
    this.resetSticks();
    this.setData({
      connected: false,
      statusLabel: "练习",
      hint: "练习模式：左半屏移动，右半屏按住瞄准并开火。拾取绿色修复包回血，黄色超频芯片提升射速。"
    });
    this.syncScoreboard(true);
  },

  startLocalDuel: function () {
    this.closeRealtimeOnly();
    this.mode = "local-duel";
    this.localPlayerId = PLAYER_ONE;
    this.state = Game.createGameState();
    Game.addPlayer(this.state, PLAYER_ONE, { name: "左手", color: "#00d1c1" });
    Game.addPlayer(this.state, PLAYER_TWO, { name: "右手", color: "#ff6b4a" });
    this.onlineState = null;
    this.resetSticks();
    this.setData({
      connected: false,
      statusLabel: "双人",
      hint: "本机双人：左半屏控制青色玩家，右半屏控制橙色玩家，两边都会自动瞄准对手并开火。"
    });
    this.syncScoreboard(true);
  },

  joinRoom: function () {
    this.closeRealtimeOnly();
    this.mode = "online";
    this.state = null;
    this.onlineState = Game.createGameState();
    this.resetSticks();
    this.setData({
      connected: false,
      statusLabel: "连接中",
      hint: "正在连接房间。真机预览时请使用已配置到小程序后台的 wss 域名；开发者工具可关闭域名校验。"
    });

    this.realtime = new Realtime.RealtimeClient({
      onEvent: this.handleRealtimeEvent.bind(this)
    });
    this.realtime.connect({
      url: this.data.serverUrl,
      roomId: this.data.roomId,
      name: safeName(this.data.playerName)
    });
  },

  leaveOnline: function () {
    this.closeRealtimeOnly();
    this.startPractice();
  },

  closeRealtimeOnly: function () {
    if (this.realtime) {
      this.realtime.close();
      this.realtime = null;
    }
  },

  handleRealtimeEvent: function (type, payload) {
    if (type === "open") {
      this.setData({ connected: true, statusLabel: "在线" });
      return;
    }

    if (type === "close") {
      if (this.mode === "online") {
        this.setData({
          connected: false,
          statusLabel: "离线",
          hint: "连接已断开，可以重新点击在线对战。"
        });
      }
      return;
    }

    if (type === "error") {
      this.setData({
        connected: false,
        statusLabel: "异常",
        hint: payload.message || "多人连接异常，请检查服务地址和小程序域名配置。"
      });
      return;
    }

    var packet = payload;
    if (type !== "message" || !packet) {
      return;
    }

    if (packet.type === "welcome") {
      this.localPlayerId = packet.playerId;
      this.onlineState = packet.state || this.onlineState;
      this.setData({
        connected: true,
        statusLabel: "在线",
        hint: "已进入房间 " + this.data.roomId + "。左半屏移动，右半屏开火；等待其他玩家加入即可对战。"
      });
      this.syncScoreboard(true);
      return;
    }

    if (packet.type === "snapshot") {
      this.onlineState = packet.state;
      this.syncScoreboard(false);
      return;
    }

    if (packet.type === "notice") {
      this.setData({ hint: packet.message });
    }
  },

  tick: function (timestamp) {
    if (!this.running) {
      return;
    }

    var now = timestamp || Date.now();
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
      this.syncScoreboard(false);
    }

    var self = this;
    this.scheduleFrame(function (nextTimestamp) {
      self.tick(nextTimestamp || Date.now());
    });
  },

  updateBots: function () {
    if (!this.state || this.mode !== "practice") {
      return;
    }
    Object.keys(this.state.players).forEach(function (id) {
      var player = this.state.players[id];
      if (player.isBot) {
        Game.setInput(this.state, id, Game.createBotInput(this.state, id));
      }
    }, this);
  },

  updateInputs: function () {
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
  },

  updateLocalDuelInputs: function () {
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
  },

  syncScoreboard: function (force) {
    var now = Date.now();
    if (!force && now - this.lastScoreSyncAt < 240) {
      return;
    }
    this.lastScoreSyncAt = now;

    var activeState = this.mode === "online" ? this.onlineState : this.state;
    if (!activeState || !activeState.players) {
      return;
    }

    var board = Object.keys(activeState.players)
      .map(function (id) {
        var player = activeState.players[id];
        return {
          id: player.id,
          name: player.name,
          color: player.color,
          hp: Math.ceil(player.hp),
          score: player.score
        };
      })
      .sort(function (a, b) {
        return b.score - a.score;
      });

    this.setData({ scoreboard: board });
  },

  onNameInput: function (event) {
    this.setData({ playerName: event.detail.value });
  },

  onRoomInput: function (event) {
    this.setData({ roomId: event.detail.value });
  },

  onServerUrlInput: function (event) {
    this.setData({ serverUrl: event.detail.value });
  },

  onCanvasTouchStart: function (event) {
    var touches = event.changedTouches || [];
    for (var i = 0; i < touches.length; i += 1) {
      this.assignTouch(touches[i]);
    }
  },

  onCanvasTouchMove: function (event) {
    var touches = event.touches || [];
    for (var i = 0; i < touches.length; i += 1) {
      this.moveTouch(touches[i]);
    }
  },

  onCanvasTouchEnd: function (event) {
    var touches = event.changedTouches || [];
    for (var i = 0; i < touches.length; i += 1) {
      var id = touchId(touches[i]);
      if (this.leftStick && this.leftStick.id === id) {
        this.leftStick = null;
      }
      if (this.rightStick && this.rightStick.id === id) {
        this.rightStick = null;
      }
    }
  },

  assignTouch: function (touch) {
    if (!this.canvasSize) {
      return;
    }

    var id = touchId(touch);
    var stick = {
      id: id,
      startX: touch.x,
      startY: touch.y,
      x: touch.x,
      y: touch.y
    };

    if (touch.x < this.canvasSize.width * 0.5) {
      this.leftStick = stick;
    } else {
      this.rightStick = stick;
    }
  },

  moveTouch: function (touch) {
    var id = touchId(touch);
    if (this.leftStick && this.leftStick.id === id) {
      this.leftStick.x = touch.x;
      this.leftStick.y = touch.y;
    }
    if (this.rightStick && this.rightStick.id === id) {
      this.rightStick.x = touch.x;
      this.rightStick.y = touch.y;
    }
  },

  resetSticks: function () {
    this.leftStick = null;
    this.rightStick = null;
    this.localInput = { moveX: 0, moveY: 0, aimX: 1, aimY: 0, fire: false };
  },

  stickVector: function (stick) {
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
  },

  pointToWorld: function (x, y, state) {
    state = state || { width: WORLD_WIDTH, height: WORLD_HEIGHT };
    return {
      x: x / this.canvasSize.width * state.width,
      y: y / this.canvasSize.height * state.height
    };
  },

  draw: function (state, online) {
    if (!this.ctx || !this.canvasSize) {
      return;
    }

    var ctx = this.ctx;
    var width = this.canvasSize.width;
    var height = this.canvasSize.height;
    ctx.clearRect(0, 0, width, height);

    if (!state) {
      this.drawEmpty(ctx, width, height, "等待竞技场数据");
      return;
    }

    ctx.save();
    ctx.scale(width / state.width, height / state.height);
    this.drawWorld(ctx, state);
    this.drawPickups(ctx, state);
    this.drawBullets(ctx, state);
    this.drawPlayers(ctx, state);
    ctx.restore();

    this.drawSticks(ctx);
    if (online && state.players && Object.keys(state.players).length < 2) {
      this.drawBanner(ctx, width, height, "等待其他玩家加入");
    }
  },

  drawEmpty: function (ctx, width, height, message) {
    ctx.fillStyle = "#171a20";
    ctx.fillRect(0, 0, width, height);
    ctx.fillStyle = "#d9ddd5";
    ctx.font = "16px sans-serif";
    ctx.textAlign = "center";
    ctx.fillText(message, width / 2, height / 2);
  },

  drawWorld: function (ctx, state) {
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
  },

  drawPickups: function (ctx, state) {
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
      ctx.fillText(pickup.type === "heal" ? "+" : "×", pickup.x, pickup.y + 1);
      ctx.restore();
    });
  },

  drawBullets: function (ctx, state) {
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
  },

  drawPlayers: function (ctx, state) {
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
  },

  drawSticks: function (ctx) {
    this.drawStick(ctx, this.leftStick, "#00d1c1");
    this.drawStick(ctx, this.rightStick, "#ff6b4a");
  },

  drawStick: function (ctx, stick, color) {
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
  },

  drawBanner: function (ctx, width, height, text) {
    ctx.save();
    ctx.fillStyle = "rgba(17, 19, 24, 0.72)";
    ctx.fillRect(width * 0.25, height * 0.42, width * 0.5, 44);
    ctx.fillStyle = "#f7f7f2";
    ctx.font = "15px sans-serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(text, width / 2, height * 0.42 + 22);
    ctx.restore();
  }
});

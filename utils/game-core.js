"use strict";

var ARENA_WIDTH = 960;
var ARENA_HEIGHT = 540;
var PLAYER_RADIUS = 18;
var BULLET_RADIUS = 5;
var PLAYER_SPEED = 225;
var BULLET_SPEED = 560;
var MAX_HP = 100;
var COLORS = ["#00d1c1", "#ff6b4a", "#ffd166", "#4bd37b", "#8bb7ff", "#f48fb1"];
var EMPTY_INPUT = { moveX: 0, moveY: 0, aimX: 1, aimY: 0, fire: false };

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function length(x, y) {
  return Math.sqrt(x * x + y * y);
}

function normalize(x, y) {
  var len = length(x, y);
  if (len <= 0.0001) {
    return { x: 0, y: 0, len: 0 };
  }
  return { x: x / len, y: y / len, len: len };
}

function distance(a, b) {
  var dx = a.x - b.x;
  var dy = a.y - b.y;
  return Math.sqrt(dx * dx + dy * dy);
}

function createGameState(options) {
  options = options || {};
  return {
    width: options.width || ARENA_WIDTH,
    height: options.height || ARENA_HEIGHT,
    elapsed: 0,
    nextBulletId: 1,
    nextPickupId: 1,
    nextPickupAt: 4,
    players: {},
    bullets: [],
    pickups: []
  };
}

function spawnPoint(state, slot) {
  var margin = 74;
  var points = [
    { x: margin, y: margin },
    { x: state.width - margin, y: state.height - margin },
    { x: state.width - margin, y: margin },
    { x: margin, y: state.height - margin },
    { x: state.width * 0.5, y: margin },
    { x: state.width * 0.5, y: state.height - margin }
  ];
  return points[slot % points.length];
}

function addPlayer(state, id, options) {
  options = options || {};
  if (!id || state.players[id]) {
    return state.players[id];
  }

  var slot = Object.keys(state.players).length;
  var point = options.spawn || spawnPoint(state, slot);
  var color = options.color || COLORS[slot % COLORS.length];
  var player = {
    id: id,
    name: options.name || ("玩家" + (slot + 1)),
    color: color,
    x: point.x,
    y: point.y,
    vx: 0,
    vy: 0,
    angle: slot % 2 === 0 ? 0 : Math.PI,
    hp: MAX_HP,
    maxHp: MAX_HP,
    score: 0,
    cooldown: 0,
    respawn: 0,
    powerTimer: 0,
    isBot: !!options.isBot,
    input: Object.assign({}, EMPTY_INPUT)
  };
  state.players[id] = player;
  return player;
}

function removePlayer(state, id) {
  delete state.players[id];
  state.bullets = state.bullets.filter(function (bullet) {
    return bullet.ownerId !== id;
  });
}

function setInput(state, id, input) {
  var player = state.players[id];
  if (!player) {
    return;
  }

  input = input || EMPTY_INPUT;
  player.input = {
    moveX: clamp(Number(input.moveX) || 0, -1, 1),
    moveY: clamp(Number(input.moveY) || 0, -1, 1),
    aimX: clamp(Number(input.aimX) || 0, -1, 1),
    aimY: clamp(Number(input.aimY) || 0, -1, 1),
    fire: !!input.fire
  };
}

function respawnPlayer(state, player) {
  var slot = Math.abs(hashCode(player.id)) % 6;
  var point = spawnPoint(state, slot);
  player.x = point.x;
  player.y = point.y;
  player.vx = 0;
  player.vy = 0;
  player.hp = player.maxHp;
  player.respawn = 0;
  player.cooldown = 0.45;
}

function hashCode(value) {
  var hash = 0;
  var text = String(value);
  for (var i = 0; i < text.length; i += 1) {
    hash = ((hash << 5) - hash) + text.charCodeAt(i);
    hash |= 0;
  }
  return hash;
}

function fireBullet(state, player) {
  var dir = normalize(Math.cos(player.angle), Math.sin(player.angle));
  state.bullets.push({
    id: state.nextBulletId++,
    ownerId: player.id,
    color: player.color,
    x: player.x + dir.x * (PLAYER_RADIUS + 6),
    y: player.y + dir.y * (PLAYER_RADIUS + 6),
    vx: dir.x * BULLET_SPEED,
    vy: dir.y * BULLET_SPEED,
    life: 0.92,
    damage: player.powerTimer > 0 ? 18 : 16
  });
}

function spawnPickup(state) {
  var type = Math.random() > 0.52 ? "rapid" : "heal";
  state.pickups.push({
    id: state.nextPickupId++,
    type: type,
    x: 110 + Math.random() * (state.width - 220),
    y: 85 + Math.random() * (state.height - 170),
    life: 12,
    color: type === "rapid" ? "#ffd166" : "#4bd37b"
  });
}

function stepGame(state, dt) {
  dt = clamp(dt || 0, 0, 0.05);
  state.elapsed += dt;
  state.nextPickupAt -= dt;

  if (state.nextPickupAt <= 0 && state.pickups.length < 3) {
    spawnPickup(state);
    state.nextPickupAt = 7 + Math.random() * 4;
  }

  Object.keys(state.players).forEach(function (id) {
    var player = state.players[id];
    var input = player.input || EMPTY_INPUT;

    if (player.hp <= 0) {
      player.respawn -= dt;
      if (player.respawn <= 0) {
        respawnPlayer(state, player);
      }
      return;
    }

    var move = normalize(input.moveX, input.moveY);
    player.vx = move.x * PLAYER_SPEED;
    player.vy = move.y * PLAYER_SPEED;
    player.x = clamp(player.x + player.vx * dt, PLAYER_RADIUS, state.width - PLAYER_RADIUS);
    player.y = clamp(player.y + player.vy * dt, PLAYER_RADIUS, state.height - PLAYER_RADIUS);

    var aim = normalize(input.aimX, input.aimY);
    if (aim.len > 0.02) {
      player.angle = Math.atan2(aim.y, aim.x);
    } else if (move.len > 0.02) {
      player.angle = Math.atan2(move.y, move.x);
    }

    player.cooldown = Math.max(0, player.cooldown - dt);
    player.powerTimer = Math.max(0, player.powerTimer - dt);

    var interval = player.powerTimer > 0 ? 0.105 : 0.22;
    if (input.fire && player.cooldown <= 0) {
      fireBullet(state, player);
      player.cooldown = interval;
    }
  });

  state.bullets.forEach(function (bullet) {
    bullet.x += bullet.vx * dt;
    bullet.y += bullet.vy * dt;
    bullet.life -= dt;
  });

  var remainingBullets = [];
  state.bullets.forEach(function (bullet) {
    if (
      bullet.life <= 0 ||
      bullet.x < -BULLET_RADIUS ||
      bullet.x > state.width + BULLET_RADIUS ||
      bullet.y < -BULLET_RADIUS ||
      bullet.y > state.height + BULLET_RADIUS
    ) {
      return;
    }

    var hit = false;
    Object.keys(state.players).forEach(function (id) {
      var target = state.players[id];
      if (hit || target.id === bullet.ownerId || target.hp <= 0) {
        return;
      }

      if (distance(bullet, target) <= PLAYER_RADIUS + BULLET_RADIUS) {
        target.hp = Math.max(0, target.hp - bullet.damage);
        hit = true;

        if (target.hp <= 0) {
          target.respawn = 2.2;
          var owner = state.players[bullet.ownerId];
          if (owner) {
            owner.score += 1;
          }
        }
      }
    });

    if (!hit) {
      remainingBullets.push(bullet);
    }
  });
  state.bullets = remainingBullets;

  var remainingPickups = [];
  state.pickups.forEach(function (pickup) {
    pickup.life -= dt;
    if (pickup.life <= 0) {
      return;
    }

    var consumed = false;
    Object.keys(state.players).forEach(function (id) {
      var player = state.players[id];
      if (consumed || player.hp <= 0) {
        return;
      }

      if (distance(pickup, player) <= PLAYER_RADIUS + 13) {
        consumed = true;
        if (pickup.type === "heal") {
          player.hp = Math.min(player.maxHp, player.hp + 30);
        } else {
          player.powerTimer = 5.2;
        }
      }
    });

    if (!consumed) {
      remainingPickups.push(pickup);
    }
  });
  state.pickups = remainingPickups;
}

function createBotInput(state, botId) {
  var bot = state.players[botId];
  if (!bot || bot.hp <= 0) {
    return Object.assign({}, EMPTY_INPUT);
  }

  var nearest = null;
  var nearestDistance = Infinity;
  Object.keys(state.players).forEach(function (id) {
    var player = state.players[id];
    if (player.id === botId || player.hp <= 0) {
      return;
    }

    var d = distance(bot, player);
    if (d < nearestDistance) {
      nearestDistance = d;
      nearest = player;
    }
  });

  if (!nearest) {
    return Object.assign({}, EMPTY_INPUT);
  }

  var aim = normalize(nearest.x - bot.x, nearest.y - bot.y);
  var desiredRange = nearestDistance < 155 ? -0.7 : 1;
  var strafe = Math.sin(state.elapsed * 1.7 + hashCode(botId)) * 0.42;
  return {
    moveX: clamp(aim.x * desiredRange + -aim.y * strafe, -1, 1),
    moveY: clamp(aim.y * desiredRange + aim.x * strafe, -1, 1),
    aimX: aim.x,
    aimY: aim.y,
    fire: nearestDistance < 470
  };
}

function publicState(state) {
  return {
    width: state.width,
    height: state.height,
    elapsed: state.elapsed,
    players: state.players,
    bullets: state.bullets,
    pickups: state.pickups
  };
}

module.exports = {
  ARENA_WIDTH: ARENA_WIDTH,
  ARENA_HEIGHT: ARENA_HEIGHT,
  PLAYER_RADIUS: PLAYER_RADIUS,
  BULLET_RADIUS: BULLET_RADIUS,
  MAX_HP: MAX_HP,
  COLORS: COLORS,
  createGameState: createGameState,
  addPlayer: addPlayer,
  removePlayer: removePlayer,
  setInput: setInput,
  stepGame: stepGame,
  createBotInput: createBotInput,
  publicState: publicState,
  normalize: normalize,
  clamp: clamp
};

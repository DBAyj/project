"use strict";

var ENV = "development";

var CONFIG = {
  development: {
    env: "development",
    serverUrl: "ws://127.0.0.1:8787",
    roomId: "arena-1"
  },
  production: {
    env: "production",
    serverUrl: "wss://YOUR_SOCKET_DOMAIN/ws",
    roomId: "arena-1"
  }
};

module.exports = CONFIG[ENV];

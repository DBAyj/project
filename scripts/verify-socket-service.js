"use strict";

var crypto = require("crypto");
var fs = require("fs");
var http = require("http");
var https = require("https");
var net = require("net");
var path = require("path");
var tls = require("tls");

var root = path.resolve(__dirname, "..");
var args = process.argv.slice(2);
var allowWs = args.indexOf("--allow-ws") !== -1;
var timeoutMs = Number(process.env.WECHAT_SOCKET_VERIFY_TIMEOUT_MS || 7000);

function fail(message) {
  console.error("对战服务验证失败：" + message);
  process.exit(1);
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(path.join(root, filePath), "utf8"));
}

function loadReleaseConfig() {
  var releasePath = path.join(root, "release.local.json");
  if (!fs.existsSync(releasePath)) {
    fail("缺少 release.local.json");
  }
  return readJson("release.local.json");
}

function requestHealth(healthUrl) {
  return new Promise(function (resolve, reject) {
    var parsed = new URL(healthUrl);
    var client = parsed.protocol === "https:" ? https : http;
    var req = client.get(parsed, function (res) {
      var body = "";
      res.setEncoding("utf8");
      res.on("data", function (chunk) {
        body += chunk;
      });
      res.on("end", function () {
        if (res.statusCode < 200 || res.statusCode >= 300) {
          reject(new Error("健康检查返回 HTTP " + res.statusCode));
          return;
        }
        resolve(body);
      });
    });

    req.setTimeout(timeoutMs, function () {
      req.destroy(new Error("健康检查超时"));
    });
    req.on("error", reject);
  });
}

function websocketHandshake(socketUrl) {
  return new Promise(function (resolve, reject) {
    var parsed = new URL(socketUrl);
    var secure = parsed.protocol === "wss:";
    var port = Number(parsed.port || (secure ? 443 : 80));
    var key = crypto.randomBytes(16).toString("base64");
    var pathWithQuery = parsed.pathname + (parsed.search || "");
    var socket;
    var done = false;
    var handshakeSent = false;
    var buffer = "";

    function finish(err) {
      if (done) {
        return;
      }
      done = true;
      clearTimeout(timer);
      if (socket) {
        socket.destroy();
      }
      if (err) {
        reject(err);
      } else {
        resolve();
      }
    }

    var timer = setTimeout(function () {
      finish(new Error("WebSocket 握手超时"));
    }, timeoutMs);

    var options = {
      host: parsed.hostname,
      port: port,
      servername: parsed.hostname
    };

    socket = secure ? tls.connect(options) : net.connect(options);

    function sendHandshake() {
      if (handshakeSent) {
        return;
      }
      handshakeSent = true;
      socket.write([
        "GET " + pathWithQuery + " HTTP/1.1",
        "Host: " + parsed.host,
        "Upgrade: websocket",
        "Connection: Upgrade",
        "Sec-WebSocket-Version: 13",
        "Sec-WebSocket-Key: " + key,
        "",
        ""
      ].join("\r\n"));
    }

    if (secure) {
      socket.on("secureConnect", sendHandshake);
    } else {
      socket.on("connect", sendHandshake);
    }

    socket.on("data", function (chunk) {
      buffer += chunk.toString("utf8");
      if (buffer.indexOf("\r\n\r\n") === -1) {
        return;
      }

      var firstLine = buffer.split("\r\n")[0];
      if (/^HTTP\/1\.1 101\b/.test(firstLine) || /^HTTP\/1\.0 101\b/.test(firstLine)) {
        finish();
      } else {
        finish(new Error("WebSocket 握手返回 " + firstLine));
      }
    });

    socket.on("error", finish);
  });
}

function healthUrlFromSocket(socketUrl, explicitHealthUrl) {
  if (explicitHealthUrl) {
    return explicitHealthUrl;
  }
  var parsed = new URL(socketUrl);
  parsed.protocol = parsed.protocol === "wss:" ? "https:" : "http:";
  parsed.pathname = "/health";
  parsed.search = "";
  parsed.hash = "";
  return parsed.toString();
}

async function main() {
  if (process.env.WECHAT_SKIP_SERVICE_VERIFY === "1") {
    console.log("已跳过对战服务连通性验证。");
    return;
  }

  var config = loadReleaseConfig();
  var socketUrl = config.socketServerUrl || "";
  if (!socketUrl) {
    fail("release.local.json 缺少 socketServerUrl");
  }

  var parsed = new URL(socketUrl);
  if (parsed.protocol !== "wss:" && !(allowWs && parsed.protocol === "ws:")) {
    fail("正式发布必须使用 wss:// 对战服务地址");
  }

  var healthUrl = healthUrlFromSocket(socketUrl, config.healthCheckUrl);
  await requestHealth(healthUrl);
  await websocketHandshake(socketUrl);

  console.log("对战服务验证通过：" + socketUrl + "，健康检查：" + healthUrl);
}

main().catch(function (err) {
  fail(err && err.message ? err.message : String(err));
});

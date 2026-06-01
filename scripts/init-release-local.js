"use strict";

var fs = require("fs");
var path = require("path");

var root = path.resolve(__dirname, "..");
var outputPath = path.join(root, "release.local.json");

function parseArgs(argv) {
  var result = {};
  for (var i = 0; i < argv.length; i += 1) {
    var arg = argv[i];
    if (arg.indexOf("--") !== 0) {
      continue;
    }
    var key = arg.slice(2);
    var value = "true";
    if (i + 1 < argv.length && argv[i + 1].indexOf("--") !== 0) {
      value = argv[i + 1];
      i += 1;
    }
    result[key] = value;
  }
  return result;
}

function env(name, fallback) {
  return process.env[name] || fallback || "";
}

function value(args, key, envName, fallback) {
  return args[key] || env(envName, fallback);
}

function validate(config) {
  var failures = [];
  if (!/^wx[a-fA-F0-9]{16}$/.test(config.appid || "")) {
    failures.push("appid 必须是真实的 wx 开头 18 位 AppID");
  }
  if (!/^wss:\/\//.test(config.socketServerUrl || "")) {
    failures.push("socketServerUrl 必须使用 wss://");
  }
  if (!/^https:\/\//.test(config.healthCheckUrl || "")) {
    failures.push("healthCheckUrl 必须使用 https://");
  }
  if (/your-domain|example\.com|127\.0\.0\.1|localhost/i.test(config.socketServerUrl || "")) {
    failures.push("socketServerUrl 不能是示例或本地地址");
  }
  if (/your-domain|example\.com|127\.0\.0\.1|localhost/i.test(config.healthCheckUrl || "")) {
    failures.push("healthCheckUrl 不能是示例或本地地址");
  }
  if (!/^\d+\.\d+\.\d+([.-][0-9A-Za-z]+)?$/.test(config.version || "")) {
    failures.push("version 建议使用 1.0.0 这样的版本号");
  }
  if (config.privateKeyPath) {
    var keyPath = path.isAbsolute(config.privateKeyPath)
      ? config.privateKeyPath
      : path.join(root, config.privateKeyPath);
    if (!fs.existsSync(keyPath)) {
      failures.push("privateKeyPath 指向的上传密钥文件不存在");
    }
  }
  return failures;
}

function main() {
  var args = parseArgs(process.argv.slice(2));
  var socketServerUrl = value(args, "socket", "WECHAT_SOCKET_SERVER_URL", "");
  var defaultHealth = socketServerUrl
    ? socketServerUrl.replace(/^wss:\/\//, "https://").replace(/^ws:\/\//, "http://").replace(/\/ws(\?.*)?$/, "/health")
    : "";

  var config = {
    appid: value(args, "appid", "WECHAT_APPID", ""),
    version: value(args, "version", "WECHAT_VERSION", "1.0.0"),
    desc: value(args, "desc", "WECHAT_RELEASE_DESC", "霓虹街区多人对战版本"),
    socketServerUrl: socketServerUrl,
    healthCheckUrl: value(args, "health", "WECHAT_HEALTH_CHECK_URL", defaultHealth),
    roomId: value(args, "room", "WECHAT_ROOM_ID", "arena-1"),
    reviewRoomId: value(args, "review-room", "WECHAT_REVIEW_ROOM_ID", "review-room"),
    robot: Number(value(args, "robot", "WECHAT_UPLOAD_ROBOT", "1")),
    privateKeyPath: value(args, "key", "WECHAT_PRIVATE_KEY_PATH", "private.wx.key"),
    compileOutputPath: value(args, "compile-output", "WECHAT_COMPILE_OUTPUT_PATH", "dist/compiled-result.zip"),
    previewQrcodePath: value(args, "qrcode", "WECHAT_PREVIEW_QRCODE_PATH", "dist/preview.jpg"),
    previewPagePath: value(args, "preview-page", "WECHAT_PREVIEW_PAGE_PATH", ""),
    previewSearchQuery: value(args, "preview-query", "WECHAT_PREVIEW_SEARCH_QUERY", "")
  };

  var failures = validate(config);
  if (failures.length > 0 && args.force !== "true") {
    console.error("release.local.json 未生成：");
    failures.forEach(function (message) {
      console.error("- " + message);
    });
    console.error("确认只是生成草稿时可追加 --force。");
    process.exit(1);
  }

  fs.writeFileSync(outputPath, JSON.stringify(config, null, 2) + "\n");
  console.log("已生成 " + outputPath);
  if (failures.length > 0) {
    console.log("注意：当前为草稿配置，仍需修复：");
    failures.forEach(function (message) {
      console.log("- " + message);
    });
  }
}

main();

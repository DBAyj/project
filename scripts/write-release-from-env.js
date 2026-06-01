"use strict";

var crypto = require("crypto");
var fs = require("fs");
var path = require("path");

var root = path.resolve(__dirname, "..");

function required(name) {
  var value = process.env[name];
  if (!value) {
    console.error("缺少环境变量 " + name);
    process.exit(1);
  }
  return value;
}

function optional(name, fallback) {
  return process.env[name] || fallback;
}

function normalizePrivateKey(source) {
  var key = String(source || "").trim();
  if (key.indexOf("\\n") !== -1 && key.indexOf("\n") === -1) {
    key = key.replace(/\\n/g, "\n");
  }
  return key.replace(/\r\n/g, "\n").replace(/\r/g, "\n").trim() + "\n";
}

function validatePrivateKey(source) {
  var key = normalizePrivateKey(source);
  var compact = key.trim();
  if (/^[a-f0-9]{32}$/i.test(compact)) {
    console.error("WECHAT_UPLOAD_PRIVATE_KEY 看起来是 AppSecret，不是小程序代码上传密钥。");
    process.exit(1);
  }
  if (!/-----BEGIN (RSA |EC |)PRIVATE KEY-----[\s\S]+-----END (RSA |EC |)PRIVATE KEY-----/.test(compact)) {
    console.error("WECHAT_UPLOAD_PRIVATE_KEY 必须是 PEM 私钥文本。");
    process.exit(1);
  }
  try {
    crypto.createPrivateKey(key);
  } catch (err) {
    console.error("WECHAT_UPLOAD_PRIVATE_KEY 不是可解析的私钥：" + (err && err.message ? err.message : err));
    process.exit(1);
  }
  return key;
}

function main() {
  var appid = required("WECHAT_APPID");
  var socketServerUrl = required("WECHAT_SOCKET_SERVER_URL");
  var privateKey = required("WECHAT_UPLOAD_PRIVATE_KEY");
  var version = optional("WECHAT_VERSION", optional("GITHUB_RUN_NUMBER", "1.0.0"));
  var desc = optional("WECHAT_RELEASE_DESC", "CI upload");
  var roomId = optional("WECHAT_ROOM_ID", "arena-1");
  var reviewRoomId = optional("WECHAT_REVIEW_ROOM_ID", "review-room");
  var robot = Number(optional("WECHAT_UPLOAD_ROBOT", "1"));
  var healthCheckUrl = optional("WECHAT_HEALTH_CHECK_URL", socketServerUrl.replace(/^wss:\/\//, "https://").replace(/^ws:\/\//, "http://").replace(/\/ws(\?.*)?$/, "/health"));
  var compileOutputPath = optional("WECHAT_COMPILE_OUTPUT_PATH", "dist/compiled-result.zip");
  var previewQrcodePath = optional("WECHAT_PREVIEW_QRCODE_PATH", "dist/preview.jpg");
  var previewPagePath = optional("WECHAT_PREVIEW_PAGE_PATH", "");
  var previewSearchQuery = optional("WECHAT_PREVIEW_SEARCH_QUERY", "");
  var privateKeyPath = "private.ci.key";

  if (/^\d+$/.test(version)) {
    version = "1.0." + version;
  }

  var normalizedPrivateKey = validatePrivateKey(privateKey);
  fs.writeFileSync(path.join(root, privateKeyPath), normalizedPrivateKey, { mode: 0o600 });
  fs.chmodSync(path.join(root, privateKeyPath), 0o600);
  fs.writeFileSync(path.join(root, "release.local.json"), JSON.stringify({
    appid: appid,
    version: version,
    desc: desc,
    socketServerUrl: socketServerUrl,
    healthCheckUrl: healthCheckUrl,
    roomId: roomId,
    reviewRoomId: reviewRoomId,
    robot: robot,
    privateKeyPath: privateKeyPath,
    compileOutputPath: compileOutputPath,
    previewQrcodePath: previewQrcodePath,
    previewPagePath: previewPagePath,
    previewSearchQuery: previewSearchQuery
  }, null, 2) + "\n");

  console.log("已从环境变量生成 release.local.json");
}

main();

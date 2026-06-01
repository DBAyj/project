"use strict";

var fs = require("fs");
var path = require("path");

var root = path.resolve(__dirname, "..");
var localConfigPath = path.join(root, "release.local.json");
var appConfigPath = path.join(root, "utils", "config.js");
var privateProjectPath = path.join(root, "project.private.config.json");

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function ensure(condition, message) {
  if (!condition) {
    throw new Error(message);
  }
}

function validate(config) {
  ensure(/^wx[a-fA-F0-9]{16}$/.test(config.appid || ""), "appid 必须是真实的 wx 开头 18 位 AppID");
  ensure(/^wss:\/\//.test(config.socketServerUrl || ""), "socketServerUrl 必须使用 wss://");
  ensure(!/YOUR_SOCKET_DOMAIN|example\.com|127\.0\.0\.1|localhost/.test(config.socketServerUrl), "socketServerUrl 不能是示例或本地地址");
  ensure(/^\d+\.\d+\.\d+([.-][0-9A-Za-z]+)?$/.test(config.version || ""), "version 建议使用 1.0.0 这样的版本号");
}

function jsString(value) {
  return JSON.stringify(String(value));
}

function writeRuntimeConfig(config) {
  var source = [
    "\"use strict\";",
    "",
    "var ENV = \"production\";",
    "",
    "var CONFIG = {",
    "  production: {",
    "    env: \"production\",",
    "    serverUrl: " + jsString(config.socketServerUrl) + ",",
    "    roomId: " + jsString(config.roomId || "arena-1"),
    "  }",
    "};",
    "",
    "module.exports = CONFIG[ENV];",
    ""
  ].join("\n");
  fs.writeFileSync(appConfigPath, source);
}

function writePrivateProjectConfig(config) {
  var privateConfig = {
    appid: config.appid,
    projectname: "wechat-arcade-battle",
    setting: {
      urlCheck: true,
      es6: true,
      enhance: true,
      postcss: true,
      minified: true,
      newFeature: true
    }
  };
  fs.writeFileSync(privateProjectPath, JSON.stringify(privateConfig, null, 2) + "\n");
}

function main() {
  ensure(fs.existsSync(localConfigPath), "请先从 release.config.example.json 复制出 release.local.json 并填入真实 AppID 和 wss 地址");
  var config = readJson(localConfigPath);
  validate(config);
  writeRuntimeConfig(config);
  writePrivateProjectConfig(config);
  console.log("Release config prepared for " + config.appid + " " + config.version);
}

main();

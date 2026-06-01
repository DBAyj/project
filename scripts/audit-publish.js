"use strict";

var fs = require("fs");
var path = require("path");
var childProcess = require("child_process");

var root = path.resolve(__dirname, "..");
var checks = [];

function exists(filePath) {
  return fs.existsSync(path.join(root, filePath));
}

function read(filePath) {
  return fs.readFileSync(path.join(root, filePath), "utf8");
}

function add(name, ok, detail) {
  checks.push({ name: name, ok: !!ok, detail: detail || "" });
}

function executableExists(filePath) {
  try {
    fs.accessSync(filePath, fs.constants.X_OK);
    return true;
  } catch (err) {
    return false;
  }
}

function findDevtoolsCli() {
  var candidates = [
    process.env.WECHAT_DEVTOOLS_CLI,
    "/Applications/wechatwebdevtools.app/Contents/MacOS/cli",
    "/Applications/微信开发者工具.app/Contents/MacOS/cli",
    "/Applications/微信web开发者工具.app/Contents/MacOS/cli"
  ].filter(Boolean);

  for (var i = 0; i < candidates.length; i += 1) {
    if (executableExists(candidates[i])) {
      return candidates[i];
    }
  }
  return "";
}

function hasCiPackage() {
  return exists("node_modules/miniprogram-ci/package.json");
}

function readReleaseLocal() {
  if (!exists("release.local.json")) {
    return null;
  }
  return JSON.parse(read("release.local.json"));
}

function runCheckRelease() {
  var result = childProcess.spawnSync(process.execPath, ["scripts/check-release.js"], {
    cwd: root,
    encoding: "utf8"
  });
  return {
    ok: result.status === 0,
    output: (result.stdout + result.stderr).trim()
  };
}

function main() {
  var cli = findDevtoolsCli();
  var releaseLocal = readReleaseLocal();
  var projectConfig = exists("project.config.json") ? JSON.parse(read("project.config.json")) : {};
  var runtimeSource = exists("utils/config.js") ? read("utils/config.js") : "";
  var releaseResult = runCheckRelease();
  var ciKeyPath = "";
  var ciKeyExists = false;
  if (releaseLocal && releaseLocal.privateKeyPath) {
    ciKeyPath = path.isAbsolute(releaseLocal.privateKeyPath) ? releaseLocal.privateKeyPath : path.join(root, releaseLocal.privateKeyPath);
    ciKeyExists = fs.existsSync(ciKeyPath);
  }
  var ciReady = hasCiPackage() && ciKeyExists;
  var uploadReady = !!cli || ciReady;

  add(
    "游戏源码",
    projectConfig.compileType === "game" ? (exists("game.json") && exists("game.js")) : (exists("app.json") && exists("pages/index/index.js")),
    projectConfig.compileType === "game" ? "game.json 和 game.js 存在" : "app.json 和首页脚本存在"
  );
  add("发布本地配置", exists("release.local.json"), exists("release.local.json") ? "已存在" : "缺少 release.local.json");
  add("真实 AppID 配置", exists("project.private.config.json"), exists("project.private.config.json") ? "已存在" : "缺少 project.private.config.json");
  add("生产 WebSocket 地址", /var ENV = "production"/.test(runtimeSource) && /wss:\/\//.test(runtimeSource), "utils/config.js 需要 production + wss");
  add("发布前检查", releaseResult.ok, releaseResult.output);
  add("上传通道", uploadReady, cli ? "微信开发者工具 CLI: " + cli : (ciReady ? "miniprogram-ci + 上传密钥已就绪" : "需要微信开发者工具 CLI，或安装 miniprogram-ci 并配置 privateKeyPath"));

  console.log("微信小程序发布审计：");
  checks.forEach(function (item) {
    console.log((item.ok ? "PASS " : "FAIL ") + item.name + " - " + item.detail);
  });

  var failed = checks.filter(function (item) {
    return !item.ok;
  });
  if (failed.length > 0) {
    process.exit(1);
  }
}

main();

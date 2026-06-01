"use strict";

var fs = require("fs");
var path = require("path");
var childProcess = require("child_process");

var root = path.resolve(__dirname, "..");
var localConfigPath = path.join(root, "release.local.json");

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function executableExists(filePath) {
  try {
    fs.accessSync(filePath, fs.constants.X_OK);
    return true;
  } catch (err) {
    return false;
  }
}

function findCli() {
  var fromEnv = process.env.WECHAT_DEVTOOLS_CLI;
  var candidates = [
    fromEnv,
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

function run(command, args) {
  var result = childProcess.spawnSync(command, args, {
    cwd: root,
    stdio: "inherit"
  });

  if (result.error) {
    throw result.error;
  }
  if (result.status !== 0) {
    process.exit(result.status || 1);
  }
}

function runPreflight() {
  run(process.execPath, [path.join(root, "scripts", "preflight-release.js")]);
}

function main() {
  if (!fs.existsSync(localConfigPath)) {
    console.error("缺少 release.local.json，请先复制 release.config.example.json 并填入真实发布信息。");
    process.exit(1);
  }

  var releaseConfig = readJson(localConfigPath);
  var cli = findCli();
  if (!cli) {
    console.error("未找到微信开发者工具 CLI。请安装微信开发者工具，或设置 WECHAT_DEVTOOLS_CLI=/path/to/cli。");
    process.exit(1);
  }

  runPreflight();
  run(cli, [
    "upload",
    "--project",
    root,
    "--version",
    releaseConfig.version,
    "--desc",
    releaseConfig.desc || "release",
    "--robot",
    String(releaseConfig.robot || 1)
  ]);
}

main();

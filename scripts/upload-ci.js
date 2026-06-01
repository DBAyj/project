"use strict";

var fs = require("fs");
var path = require("path");
var childProcess = require("child_process");

var root = path.resolve(__dirname, "..");
var localConfigPath = path.join(root, "release.local.json");

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function resolveFromRoot(filePath) {
  if (!filePath) {
    return "";
  }
  return path.isAbsolute(filePath) ? filePath : path.join(root, filePath);
}

function loadCiPackage() {
  try {
    return require("miniprogram-ci");
  } catch (err) {
    console.error("缺少 miniprogram-ci。请先运行：npm install --save-dev miniprogram-ci");
    process.exit(1);
  }
}

function ciProjectType() {
  var projectConfig = readJson(path.join(root, "project.config.json"));
  return projectConfig.compileType === "game" ? "miniGame" : "miniProgram";
}

function runPreflight() {
  var result = childProcess.spawnSync(process.execPath, ["scripts/preflight-release.js"], {
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

async function main() {
  if (!fs.existsSync(localConfigPath)) {
    console.error("缺少 release.local.json，请先复制 release.config.example.json 并填入真实发布信息。");
    process.exit(1);
  }

  var releaseConfig = readJson(localConfigPath);
  var privateKeyPath = resolveFromRoot(releaseConfig.privateKeyPath);
  if (!privateKeyPath || !fs.existsSync(privateKeyPath)) {
    console.error("缺少小程序上传密钥文件，请在 release.local.json 配置 privateKeyPath。");
    process.exit(1);
  }

  runPreflight();

  var ci = loadCiPackage();
  var project = new ci.Project({
    appid: releaseConfig.appid,
    type: ciProjectType(),
    projectPath: root,
    privateKeyPath: privateKeyPath,
    ignores: [
      "node_modules/**/*",
      "server/**/*",
      "release.local.json",
      "private.*.key",
      "*.private.key"
    ]
  });

  await ci.upload({
    project: project,
    version: releaseConfig.version,
    desc: releaseConfig.desc || "release",
    robot: releaseConfig.robot || 1,
    setting: {
      es6: true,
      es7: true,
      minify: true,
      codeProtect: true,
      minifyJS: true,
      minifyWXML: true,
      minifyWXSS: true,
      autoPrefixWXSS: true
    },
    onProgressUpdate: function (message) {
      if (message) {
        console.log(message);
      }
    }
  });

  console.log("CI 上传完成：" + releaseConfig.version);
}

main().then(function () {
  process.exit(0);
}).catch(function (err) {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});

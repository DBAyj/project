"use strict";

var fs = require("fs");
var path = require("path");

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
    console.error("缺少 miniprogram-ci。请先运行：npm ci");
    process.exit(1);
  }
}

function ciProjectType() {
  var projectConfig = readJson(path.join(root, "project.config.json"));
  return projectConfig.compileType === "game" ? "miniGame" : "miniProgram";
}

async function main() {
  if (process.env.WECHAT_SKIP_CI_COMPILE === "1") {
    console.log("已跳过微信 CI 编译检查。");
    return;
  }

  if (!fs.existsSync(localConfigPath)) {
    console.error("缺少 release.local.json，请先生成真实发布配置。");
    process.exit(1);
  }

  var releaseConfig = readJson(localConfigPath);
  var privateKeyPath = resolveFromRoot(releaseConfig.privateKeyPath);
  if (!privateKeyPath || !fs.existsSync(privateKeyPath)) {
    console.error("缺少小程序上传密钥文件，请在 release.local.json 配置 privateKeyPath。");
    process.exit(1);
  }

  var ci = loadCiPackage();
  var outputPath = resolveFromRoot(releaseConfig.compileOutputPath || "dist/compiled-result.zip");
  fs.mkdirSync(path.dirname(outputPath), { recursive: true });

  var project = new ci.Project({
    appid: releaseConfig.appid,
    type: ciProjectType(),
    projectPath: root,
    privateKeyPath: privateKeyPath,
    ignores: [
      "node_modules/**/*",
      "server/**/*",
      "docs/**/*",
      "scripts/**/*",
      ".github/**/*",
      "dist/**/*",
      "release.local.json",
      "private.*.key",
      "*.private.key"
    ]
  });

  var result = await ci.getCompiledResult({
    project: project,
    setting: {
      useProjectConfig: true
    },
    onProgressUpdate: function (message) {
      if (message) {
        console.log(message);
      }
    }
  }, outputPath);

  console.log("微信 CI 编译通过：" + Object.keys(result).length + " 个输出文件");
  console.log("编译产物：" + outputPath);
}

main().then(function () {
  process.exit(0);
}).catch(function (err) {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});

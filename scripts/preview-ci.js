"use strict";

var fs = require("fs");
var path = require("path");
var childProcess = require("child_process");

var root = path.resolve(__dirname, "..");
var localConfigPath = path.join(root, "release.local.json");
var dryRun = process.argv.indexOf("--dry-run") !== -1;

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

  var qrcodeOutputDest = resolveFromRoot(releaseConfig.previewQrcodePath || "dist/preview.jpg");
  fs.mkdirSync(path.dirname(qrcodeOutputDest), { recursive: true });

  if (dryRun) {
    console.log("CI 预览 dry-run 通过，二维码将输出到：" + qrcodeOutputDest);
    return;
  }

  var ci = loadCiPackage();
  var projectType = ciProjectType();
  var project = new ci.Project({
    appid: releaseConfig.appid,
    type: projectType,
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

  var previewOptions = {
    project: project,
    desc: releaseConfig.desc || "preview",
    robot: releaseConfig.robot || 1,
    setting: {
      useProjectConfig: true
    },
    qrcodeFormat: "image",
    qrcodeOutputDest: qrcodeOutputDest,
    onProgressUpdate: function (message) {
      if (message) {
        console.log(message);
      }
    }
  };

  if (projectType !== "miniGame") {
    previewOptions.pagePath = releaseConfig.previewPagePath || "pages/index/index";
    previewOptions.searchQuery = releaseConfig.previewSearchQuery || "";
  }

  var result = await ci.preview(previewOptions);

  console.log("CI 预览二维码已生成：" + qrcodeOutputDest);
  if (result) {
    console.log(JSON.stringify(result, null, 2));
  }
}

main().then(function () {
  process.exit(0);
}).catch(function (err) {
  console.error(err && err.stack ? err.stack : err);
  process.exit(1);
});

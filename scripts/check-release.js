"use strict";

var fs = require("fs");
var path = require("path");

var root = path.resolve(__dirname, "..");
var failures = [];

function fail(message) {
  failures.push(message);
}

function read(filePath) {
  return fs.readFileSync(path.join(root, filePath), "utf8");
}

function readJson(filePath) {
  return JSON.parse(read(filePath));
}

function fileExists(filePath) {
  return fs.existsSync(path.join(root, filePath));
}

function checkProjectConfig() {
  var config = readJson("project.config.json");
  if (["miniprogram", "game"].indexOf(config.compileType) === -1) {
    fail("project.config.json compileType 必须是 miniprogram 或 game");
  }
  if (!config.packOptions || !Array.isArray(config.packOptions.ignore)) {
    fail("project.config.json 需要配置 packOptions.ignore");
  } else {
    ["server", "docs", "scripts", "node_modules", ".github", "dist"].forEach(function (folder) {
      var ignoresFolder = config.packOptions.ignore.some(function (item) {
        return item.type === "folder" && item.value === folder;
      });
      if (!ignoresFolder) {
        fail("发布包应忽略 " + folder + " 目录");
      }
    });

    ["release.local.json", "project.private.config.json"].forEach(function (file) {
      var ignoresFile = config.packOptions.ignore.some(function (item) {
        return item.type === "file" && item.value === file;
      });
      if (!ignoresFile) {
        fail("发布包应忽略 " + file);
      }
    });

    var ignoresKeys = config.packOptions.ignore.some(function (item) {
      return item.type === "suffix" && item.value === ".key";
    });
    if (!ignoresKeys) {
      fail("发布包应忽略 .key 上传密钥文件");
    }
  }
  if (!config.setting || config.setting.urlCheck !== true) {
    fail("发布配置里 setting.urlCheck 应为 true");
  }
}

function checkPrivateConfig() {
  if (!fileExists("project.private.config.json")) {
    fail("缺少 project.private.config.json，请运行 scripts/prepare-release.js 生成真实 AppID 配置");
    return;
  }
  var config = readJson("project.private.config.json");
  if (!/^wx[a-fA-F0-9]{16}$/.test(config.appid || "")) {
    fail("project.private.config.json 里的 appid 不是有效真实 AppID");
  }
}

function checkReleaseLocalConfig() {
  if (!fileExists("release.local.json")) {
    fail("缺少 release.local.json，请从 release.config.example.json 复制并填入真实发布信息");
    return;
  }

  var config = readJson("release.local.json");
  if (!/^wx[a-fA-F0-9]{16}$/.test(config.appid || "")) {
    fail("release.local.json 里的 appid 不是有效真实 AppID");
  }
  if (!/^wss:\/\//.test(config.socketServerUrl || "")) {
    fail("release.local.json 里的 socketServerUrl 必须使用 wss://");
  }
  if (/your-domain|example\.com|127\.0\.0\.1|localhost/i.test(config.socketServerUrl || "")) {
    fail("release.local.json 里的 socketServerUrl 不能是示例或本地地址");
  }
  if (config.healthCheckUrl) {
    if (!/^https:\/\//.test(config.healthCheckUrl)) {
      fail("release.local.json 里的 healthCheckUrl 必须使用 https://");
    }
    if (/your-domain|example\.com|127\.0\.0\.1|localhost/i.test(config.healthCheckUrl)) {
      fail("release.local.json 里的 healthCheckUrl 不能是示例或本地地址");
    }
  }
  if (!/^\d+\.\d+\.\d+([.-][0-9A-Za-z]+)?$/.test(config.version || "")) {
    fail("release.local.json 里的 version 建议使用 1.0.0 这样的版本号");
  }
  if (config.privateKeyPath) {
    var keyPath = path.isAbsolute(config.privateKeyPath) ? config.privateKeyPath : path.join(root, config.privateKeyPath);
    if (!fs.existsSync(keyPath)) {
      fail("release.local.json 配置了 privateKeyPath，但密钥文件不存在");
    }
  }
}

function checkRuntimeConfig() {
  var source = read("utils/config.js");
  if (!/var ENV = "production"/.test(source)) {
    fail("utils/config.js 仍不是 production 环境");
  }
  if (!/wss:\/\//.test(source)) {
    fail("生产联机地址必须是 wss://");
  }
  if (/YOUR_SOCKET_DOMAIN|127\.0\.0\.1|localhost|example\.com/.test(source)) {
    fail("utils/config.js 仍包含示例或本地联机地址");
  }
}

function checkRequiredFiles() {
  var config = readJson("project.config.json");
  var files = config.compileType === "game"
    ? [
      "game.json",
      "game.js",
      "utils/config.js",
      "utils/game-core.js",
      "utils/realtime.js"
    ]
    : [
      "app.json",
      "app.js",
      "app.wxss",
      "sitemap.json",
      "pages/index/index.js",
      "pages/index/index.wxml",
      "pages/index/index.wxss",
      "utils/game-core.js",
      "utils/realtime.js"
    ];

  files.forEach(function (filePath) {
    if (!fileExists(filePath)) {
      fail("缺少必要文件 " + filePath);
    }
  });
}

function main() {
  checkProjectConfig();
  checkReleaseLocalConfig();
  checkPrivateConfig();
  checkRuntimeConfig();
  checkRequiredFiles();

  if (failures.length > 0) {
    console.error("发布前检查未通过：");
    failures.forEach(function (message) {
      console.error("- " + message);
    });
    process.exit(1);
  }

  console.log("发布前检查通过，可以用微信开发者工具上传代码。");
}

main();

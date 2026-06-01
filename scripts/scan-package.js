"use strict";

var fs = require("fs");
var path = require("path");

var root = path.resolve(__dirname, "..");
var MAX_PACKAGE_BYTES = 2 * 1024 * 1024;
var failures = [];

function fail(message) {
  failures.push(message);
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(path.join(root, filePath), "utf8"));
}

function normalize(filePath) {
  return filePath.split(path.sep).join("/");
}

function walk(dir, files) {
  files = files || [];
  fs.readdirSync(dir, { withFileTypes: true }).forEach(function (entry) {
    if (entry.name === ".git") {
      return;
    }

    var fullPath = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      walk(fullPath, files);
      return;
    }
    if (entry.isFile()) {
      files.push(normalize(path.relative(root, fullPath)));
    }
  });
  return files;
}

function isIgnored(filePath, ignoreRules) {
  return ignoreRules.some(function (rule) {
    var value = normalize(rule.value || "");
    if (!value) {
      return false;
    }
    if (rule.type === "folder") {
      return filePath === value || filePath.indexOf(value + "/") === 0;
    }
    if (rule.type === "file") {
      return filePath === value;
    }
    if (rule.type === "suffix") {
      return filePath.endsWith(value);
    }
    if (rule.type === "prefix") {
      return filePath.indexOf(value) === 0;
    }
    return false;
  });
}

function includedFiles() {
  var projectConfig = readJson("project.config.json");
  var ignoreRules = projectConfig.packOptions && Array.isArray(projectConfig.packOptions.ignore)
    ? projectConfig.packOptions.ignore
    : [];
  return walk(root).filter(function (filePath) {
    return !isIgnored(filePath, ignoreRules);
  });
}

function checkForbiddenFiles(files) {
  var forbidden = [
    /^server\//,
    /^docs\//,
    /^scripts\//,
    /^node_modules\//,
    /^\.github\//,
    /^dist\//,
    /^release\.local\.json$/,
    /^project\.private\.config\.json$/,
    /\.key$/,
    /\.DS_Store$/
  ];

  files.forEach(function (filePath) {
    if (forbidden.some(function (pattern) { return pattern.test(filePath); })) {
      fail("发布包不应包含 " + filePath);
    }
  });
}

function checkForbiddenContent(files) {
  var textFilePattern = /\.(js|json|wxml|wxss|md|txt|conf|example)$/;
  var forbiddenPatterns = [
    { pattern: /ws:\/\/127\.0\.0\.1:8787/, message: "本地 WebSocket 地址" },
    { pattern: /YOUR_SOCKET_DOMAIN|your-domain\.example\.com/i, message: "示例域名" },
    { pattern: /privateKeyPath|BEGIN PRIVATE KEY/, message: "上传密钥线索" }
  ];

  files.forEach(function (filePath) {
    if (!textFilePattern.test(filePath)) {
      return;
    }

    var source = fs.readFileSync(path.join(root, filePath), "utf8");
    forbiddenPatterns.forEach(function (item) {
      if (item.pattern.test(source)) {
        fail(filePath + " 包含" + item.message);
      }
    });
  });
}

function checkSize(files) {
  var total = files.reduce(function (sum, filePath) {
    return sum + fs.statSync(path.join(root, filePath)).size;
  }, 0);

  if (total > MAX_PACKAGE_BYTES) {
    fail("发布包估算大小超过 2MB，当前约 " + Math.round(total / 1024) + "KB");
  }
  return total;
}

function main() {
  var files = includedFiles();
  checkForbiddenFiles(files);
  checkForbiddenContent(files);
  var total = checkSize(files);

  if (failures.length > 0) {
    console.error("发布包扫描未通过：");
    failures.forEach(function (message) {
      console.error("- " + message);
    });
    process.exit(1);
  }

  console.log("发布包扫描通过：" + files.length + " 个文件，约 " + Math.round(total / 1024) + "KB");
}

main();

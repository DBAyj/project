"use strict";

var fs = require("fs");
var path = require("path");
var childProcess = require("child_process");

var root = path.resolve(__dirname, "..");
var outputRoot = path.join(root, "dist", "miniprogram");

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

function runScan() {
  var result = childProcess.spawnSync(process.execPath, ["scripts/scan-package.js"], {
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

function copyFile(filePath) {
  var source = path.join(root, filePath);
  var target = path.join(outputRoot, filePath);
  fs.mkdirSync(path.dirname(target), { recursive: true });
  fs.copyFileSync(source, target);
}

function main() {
  runScan();

  var files = includedFiles();
  fs.rmSync(outputRoot, { recursive: true, force: true });
  fs.mkdirSync(outputRoot, { recursive: true });
  files.forEach(copyFile);

  var totalBytes = files.reduce(function (sum, filePath) {
    return sum + fs.statSync(path.join(root, filePath)).size;
  }, 0);

  var manifest = {
    generatedAt: new Date().toISOString(),
    fileCount: files.length,
    totalBytes: totalBytes,
    totalKb: Math.round(totalBytes / 1024),
    files: files
  };

  fs.writeFileSync(path.join(outputRoot, "manifest.json"), JSON.stringify(manifest, null, 2) + "\n");
  console.log("发布包已导出：" + outputRoot);
  console.log("文件数：" + files.length + "，约 " + Math.round(totalBytes / 1024) + "KB");
}

main();

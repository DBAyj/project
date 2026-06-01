"use strict";

var fs = require("fs");
var path = require("path");
var childProcess = require("child_process");

var root = path.resolve(__dirname, "..");

function exists(filePath) {
  return fs.existsSync(path.join(root, filePath));
}

function read(filePath) {
  return fs.readFileSync(path.join(root, filePath), "utf8");
}

function readJson(filePath) {
  return JSON.parse(read(filePath));
}

function normalize(filePath) {
  return filePath.split(path.sep).join("/");
}

function runScript(scriptPath) {
  var result = childProcess.spawnSync(process.execPath, [scriptPath], {
    cwd: root,
    encoding: "utf8"
  });
  return {
    ok: result.status === 0,
    output: (result.stdout + result.stderr).trim()
  };
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

function packageFiles() {
  var projectConfig = readJson("project.config.json");
  var ignoreRules = projectConfig.packOptions && Array.isArray(projectConfig.packOptions.ignore)
    ? projectConfig.packOptions.ignore
    : [];
  var files = walk(root).filter(function (filePath) {
    return !isIgnored(filePath, ignoreRules);
  });
  var totalBytes = files.reduce(function (sum, filePath) {
    return sum + fs.statSync(path.join(root, filePath)).size;
  }, 0);
  return {
    files: files,
    totalBytes: totalBytes
  };
}

function releaseLocalInfo() {
  if (!exists("release.local.json")) {
    return {
      exists: false
    };
  }

  var config = readJson("release.local.json");
  var keyPath = config.privateKeyPath
    ? (path.isAbsolute(config.privateKeyPath) ? config.privateKeyPath : path.join(root, config.privateKeyPath))
    : "";

  return {
    exists: true,
    appid: config.appid || "",
    version: config.version || "",
    socketServerUrl: config.socketServerUrl || "",
    roomId: config.roomId || "",
    robot: config.robot || 1,
    privateKeyPath: config.privateKeyPath || "",
    privateKeyExists: keyPath ? fs.existsSync(keyPath) : false
  };
}

function reviewMaterialsInfo() {
  var required = [
    "docs/SUBMISSION_CHECKLIST.md",
    "docs/PRIVACY_AND_COMPLIANCE.md",
    "docs/REVIEW_SUBMISSION_TEXT.md"
  ];
  var missing = required.filter(function (filePath) {
    return !exists(filePath);
  });
  return {
    ok: missing.length === 0,
    required: required,
    missing: missing,
    generated: exists("dist/review-submission.md")
  };
}

function main() {
  var check = runScript("scripts/check-release.js");
  var scan = runScript("scripts/scan-package.js");
  var service = runScript("scripts/verify-socket-service.js");
  var cli = findDevtoolsCli();
  var pkg = packageFiles();
  var local = releaseLocalInfo();
  var review = reviewMaterialsInfo();
  var hasCi = exists("node_modules/miniprogram-ci/package.json");
  var runtimeSource = exists("utils/config.js") ? read("utils/config.js") : "";
  var uploadReady = !!cli || (hasCi && local.privateKeyExists);
  var ready = check.ok && scan.ok && service.ok && uploadReady;

  var report = {
    readyToUpload: ready,
    checks: {
      releaseConfig: check.ok,
      packageScan: scan.ok,
      socketService: service.ok,
      devtoolsCli: !!cli,
      miniprogramCi: hasCi,
      ciPrivateKey: !!local.privateKeyExists,
      productionRuntimeConfig: /var ENV = "production"/.test(runtimeSource) && /wss:\/\//.test(runtimeSource),
      reviewMaterials: review.ok
    },
    releaseLocal: local,
    upload: {
      method: cli ? "devtools-cli" : (hasCi && local.privateKeyExists ? "miniprogram-ci" : ""),
      devtoolsCliPath: cli,
      miniprogramCiInstalled: hasCi
    },
    package: {
      fileCount: pkg.files.length,
      totalKb: Math.round(pkg.totalBytes / 1024),
      files: pkg.files
    },
    reviewMaterials: review,
    diagnostics: {
      releaseCheck: check.output,
      packageScan: scan.output,
      socketService: service.output
    }
  };

  if (process.argv.indexOf("--json") !== -1) {
    console.log(JSON.stringify(report, null, 2));
    process.exit(ready ? 0 : 1);
  }

  console.log("微信小程序发布状态：" + (ready ? "READY" : "NOT READY"));
  console.log("");
  console.log("上传通道：" + (report.upload.method || "未就绪"));
  console.log("开发者工具 CLI：" + (cli || "未找到"));
  console.log("miniprogram-ci：" + (hasCi ? "已安装" : "未安装"));
  console.log("上传密钥：" + (local.privateKeyExists ? "已找到" : "未找到"));
  console.log("发布配置：" + (local.exists ? (local.appid + " / " + local.version + " / " + local.socketServerUrl) : "缺少 release.local.json"));
  console.log("提审材料：" + (review.ok ? "模板齐全" : ("缺少 " + review.missing.join(", "))) + (review.generated ? "，已生成当前版本材料" : ""));
  console.log("发布包：" + pkg.files.length + " 个文件，约 " + Math.round(pkg.totalBytes / 1024) + "KB");
  console.log("");
  console.log("发布包文件：");
  pkg.files.forEach(function (filePath) {
    console.log("- " + filePath);
  });
  console.log("");
  console.log("检查结果：");
  console.log(check.output || "(release check 无输出)");
  console.log("");
  console.log(scan.output || "(package scan 无输出)");
  console.log("");
  console.log(service.output || "(socket service verify 无输出)");

  process.exit(ready ? 0 : 1);
}

main();

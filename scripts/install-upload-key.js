"use strict";

var crypto = require("crypto");
var fs = require("fs");
var path = require("path");

var root = path.resolve(__dirname, "..");
var releaseConfigPath = path.join(root, "release.local.json");
var defaultEnvName = "WECHAT_UPLOAD_PRIVATE_KEY";

function parseArgs(argv) {
  var result = {};
  for (var i = 0; i < argv.length; i += 1) {
    var arg = argv[i];
    if (arg.indexOf("--") !== 0) {
      continue;
    }
    var key = arg.slice(2);
    var value = "true";
    if (i + 1 < argv.length && argv[i + 1].indexOf("--") !== 0) {
      value = argv[i + 1];
      i += 1;
    }
    result[key] = value;
  }
  return result;
}

function usage() {
  console.log([
    "微信小程序代码上传密钥安装工具",
    "",
    "用法：",
    "  npm run release:key:install -- --from-file /path/to/private.key",
    "  WECHAT_UPLOAD_PRIVATE_KEY='-----BEGIN RSA PRIVATE KEY-----...' npm run release:key:install",
    "  pbpaste | npm run release:key:install -- --overwrite",
    "  npm run release:key:check",
    "",
    "选项：",
    "  --path private.wx.key       写入路径，默认读取 release.local.json privateKeyPath",
    "  --from-file /path/key       从已有密钥文件读取",
    "  --env NAME                  从指定环境变量读取，默认 WECHAT_UPLOAD_PRIVATE_KEY",
    "  --overwrite                 允许覆盖已有密钥文件",
    "  --check                     只校验当前密钥文件，不写入",
    ""
  ].join("\n"));
}

function readJsonIfExists(filePath) {
  if (!fs.existsSync(filePath)) {
    return null;
  }
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function writeJson(filePath, value) {
  fs.writeFileSync(filePath, JSON.stringify(value, null, 2) + "\n");
}

function resolveFromRoot(filePath) {
  if (!filePath) {
    return "";
  }
  return path.isAbsolute(filePath) ? filePath : path.join(root, filePath);
}

function displayPath(filePath) {
  var relative = path.relative(root, filePath);
  return relative && relative.indexOf("..") !== 0 ? relative : filePath;
}

function configuredKeyPath(args, config) {
  if (args.path) {
    return args.path;
  }
  if (config && config.privateKeyPath) {
    return config.privateKeyPath;
  }
  return "private.wx.key";
}

function normalizeKey(source) {
  var key = String(source || "").trim();
  if (key.indexOf("\\n") !== -1 && key.indexOf("\n") === -1) {
    key = key.replace(/\\n/g, "\n");
  }
  return key.replace(/\r\n/g, "\n").replace(/\r/g, "\n").trim() + "\n";
}

function validateUploadKey(source) {
  var key = normalizeKey(source);
  var compact = key.trim();
  if (!compact) {
    throw new Error("上传密钥为空。");
  }
  if (/^[a-f0-9]{32}$/i.test(compact)) {
    throw new Error("这看起来是 AppSecret，不是“小程序代码上传密钥”。请到微信公众平台生成上传密钥文件。");
  }
  if (/^wx[a-f0-9]{16}$/i.test(compact)) {
    throw new Error("这看起来是 AppID，不是“小程序代码上传密钥”。");
  }
  if (!/-----BEGIN (RSA |EC |)PRIVATE KEY-----[\s\S]+-----END (RSA |EC |)PRIVATE KEY-----/.test(compact)) {
    throw new Error("上传密钥应是 PEM 私钥文本，例如 -----BEGIN RSA PRIVATE KEY----- 开头。");
  }
  try {
    crypto.createPrivateKey(key);
  } catch (err) {
    throw new Error("上传密钥不是可解析的私钥：" + (err && err.message ? err.message : err));
  }
  return key;
}

function readInputKey(args) {
  if (args["from-file"]) {
    return fs.readFileSync(resolveFromRoot(args["from-file"]), "utf8");
  }

  var envName = args.env && args.env !== "true" ? args.env : defaultEnvName;
  if (process.env[envName]) {
    return process.env[envName];
  }

  if (!process.stdin.isTTY) {
    return fs.readFileSync(0, "utf8");
  }

  throw new Error("没有读取到上传密钥。请使用 --from-file、环境变量 " + envName + "，或通过 stdin 传入。");
}

function installKey(args) {
  var config = readJsonIfExists(releaseConfigPath);
  var keyPathValue = configuredKeyPath(args, config);
  var keyPath = resolveFromRoot(keyPathValue);
  var key = validateUploadKey(readInputKey(args));

  if (fs.existsSync(keyPath) && args.overwrite !== "true") {
    throw new Error(displayPath(keyPath) + " 已存在。确认要覆盖时请追加 --overwrite。");
  }

  fs.mkdirSync(path.dirname(keyPath), { recursive: true });
  fs.writeFileSync(keyPath, key, { mode: 0o600 });
  fs.chmodSync(keyPath, 0o600);

  if (config) {
    config.privateKeyPath = keyPathValue;
    writeJson(releaseConfigPath, config);
  }

  console.log("上传密钥已安装：" + displayPath(keyPath));
  console.log("文件权限已设置为 0600。");
}

function tightenPermissions(filePath) {
  var stat = fs.statSync(filePath);
  if ((stat.mode & 0o077) !== 0) {
    fs.chmodSync(filePath, 0o600);
    console.log("文件权限已收紧为 0600。");
  }
}

function checkKey(args) {
  var config = readJsonIfExists(releaseConfigPath);
  var keyPathValue = configuredKeyPath(args, config);
  var keyPath = resolveFromRoot(keyPathValue);

  if (!fs.existsSync(keyPath)) {
    throw new Error("上传密钥文件不存在：" + displayPath(keyPath));
  }

  validateUploadKey(fs.readFileSync(keyPath, "utf8"));
  tightenPermissions(keyPath);
  console.log("上传密钥校验通过：" + displayPath(keyPath));
}

function main() {
  var args = parseArgs(process.argv.slice(2));
  if (args.help === "true" || args.h === "true") {
    usage();
    return;
  }
  if (args.check === "true") {
    checkKey(args);
    return;
  }
  installKey(args);
}

try {
  main();
} catch (err) {
  console.error(err && err.message ? err.message : err);
  console.error("");
  usage();
  process.exit(1);
}

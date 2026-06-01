"use strict";

var fs = require("fs");
var path = require("path");

var root = path.resolve(__dirname, "..");
var logPath = path.join(root, "dist", "release-log.json");
var allowedEvents = [
  "prepared",
  "preflight-passed",
  "preview-created",
  "uploaded",
  "submitted-review",
  "review-approved",
  "published"
];

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

function readJsonIfExists(filePath, fallback) {
  if (!fs.existsSync(filePath)) {
    return fallback;
  }
  return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function releaseConfig() {
  return readJsonIfExists(path.join(root, "release.local.json"), {});
}

function main() {
  var args = parseArgs(process.argv.slice(2));
  var event = args.event;
  if (!event || allowedEvents.indexOf(event) === -1) {
    console.error("请使用 --event 指定事件：" + allowedEvents.join(", "));
    process.exit(1);
  }

  var config = releaseConfig();
  var log = readJsonIfExists(logPath, []);
  var entry = {
    event: event,
    at: new Date().toISOString(),
    version: args.version || config.version || "",
    appid: args.appid || config.appid || "",
    desc: args.desc || config.desc || "",
    actor: args.actor || process.env.USER || "",
    evidence: args.evidence || "",
    note: args.note || ""
  };

  if (args.reviewId) {
    entry.reviewId = args.reviewId;
  }
  if (args.qrcode) {
    entry.qrcode = args.qrcode;
  }
  if (args.url) {
    entry.url = args.url;
  }

  log.push(entry);
  fs.mkdirSync(path.dirname(logPath), { recursive: true });
  fs.writeFileSync(logPath, JSON.stringify(log, null, 2) + "\n");
  console.log("发布事件已记录：" + event + " -> " + logPath);
}

main();

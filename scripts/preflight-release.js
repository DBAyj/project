"use strict";

var childProcess = require("child_process");
var path = require("path");

var root = path.resolve(__dirname, "..");

function run(label, args) {
  console.log("\n> " + label);
  var result = childProcess.spawnSync(process.execPath, args, {
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

function main() {
  run("release:check", ["scripts/check-release.js"]);
  run("release:scan", ["scripts/scan-package.js"]);
  run("release:compile", ["scripts/compile-ci.js"]);
  run("release:verify-service", ["scripts/verify-socket-service.js"]);
  run("release:audit", ["scripts/audit-publish.js"]);
  console.log("\n发布预检通过，可以上传开发版本。");
}

main();

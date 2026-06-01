"use strict";

var fs = require("fs");
var path = require("path");

var root = path.resolve(__dirname, "..");
var outputPath = path.join(root, "dist", "review-submission.md");

function exists(filePath) {
  return fs.existsSync(path.join(root, filePath));
}

function readJson(filePath) {
  return JSON.parse(fs.readFileSync(path.join(root, filePath), "utf8"));
}

function releaseConfig() {
  if (!exists("release.local.json")) {
    return {
      version: "待填写",
      desc: "霓虹街区多人对战版本",
      socketServerUrl: "wss://待填写/ws",
      healthCheckUrl: "https://待填写/health",
      roomId: "review-room"
    };
  }
  return readJson("release.local.json");
}

function main() {
  var config = releaseConfig();
  var roomId = config.reviewRoomId || config.roomId || "review-room";
  var lines = [
    "# 微信小程序提审材料",
    "",
    "## 版本信息",
    "",
    "- 版本号：" + (config.version || "待填写"),
    "- 版本描述：" + (config.desc || "霓虹街区多人对战版本"),
    "- 在线对战服务：" + (config.socketServerUrl || "wss://待填写/ws"),
    "- 健康检查地址：" + (config.healthCheckUrl || "https://待填写/health"),
    "- 审核测试房间号：" + roomId,
    "",
    "## 功能说明",
    "",
    "霓虹街区是一款俯视角街机对战小程序游戏。玩家通过左半屏移动、右半屏瞄准并开火，在竞技场中拾取回血和射速道具，击败对手获得分数。",
    "",
    "当前版本支持单人练习、本机双人和 WebSocket 房间多人对战。在线对战中，玩家手动输入昵称和房间号，使用同一房间号的客户端会进入同一局对战。",
    "",
    "## 审核测试步骤",
    "",
    "1. 打开小程序首页。",
    "2. 点击“单人练习”，验证移动、瞄准、开火、道具拾取和计分。",
    "3. 点击“本机双人”，验证左右半屏分别控制两名玩家。",
    "4. 输入昵称和测试房间号 `" + roomId + "`。",
    "5. 点击“在线对战”，两个客户端使用同一房间号可进入同一房间进行实时对战。",
    "",
    "## 审核账号",
    "",
    "当前版本不需要账号登录，也不读取微信头像、昵称、手机号、位置、相册、通讯录、录音或摄像头权限。",
    "",
    "## 隐私与数据说明",
    "",
    "当前版本不采集微信个人资料。多人对战只使用用户手动输入的昵称、房间号和对局内操作状态，用于房间内实时同步。服务端不需要持久化保存对局数据，房间无人后可清理内存状态。",
    "",
    "## 合规说明",
    "",
    "当前版本不包含付费、广告点击返利、抽奖、提现、赌博、诱导分享、诱导关注等内容。",
    ""
  ];

  fs.mkdirSync(path.dirname(outputPath), { recursive: true });
  fs.writeFileSync(outputPath, lines.join("\n"));
  console.log("提审材料已生成：" + outputPath);
}

main();

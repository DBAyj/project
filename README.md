# 霓虹街区：微信小游戏多人街机对战

这是一个可直接导入微信开发者工具的小游戏示例。玩法是俯视角街机竞技：移动、瞄准、开火、拾取道具、击败对手得分。

## 功能

- 单人练习：玩家对战两个 AI 对手。
- 本机双人：同一块屏幕左右两侧分别控制一名玩家。
- 在线对战：通过 WebSocket 房间同步多人状态。
- 道具系统：绿色修复包回血，黄色超频芯片提升射速。
- 零依赖本地服务器：`server/ws-server.js` 使用 Node 原生模块实现。

## 运行小游戏

1. 打开微信开发者工具。
2. 选择“导入项目”，目录选择 `wechat-arcade-battle`。
3. 使用小游戏 AppID，或开发者工具的小游戏游客模式。
4. 进入后点击底部“练习”即可开始本地游戏。

## 本地多人联调

先启动本地 WebSocket 房间服务器：

```bash
cd wechat-arcade-battle
node server/ws-server.js
```

开发者工具里保持服务地址为：

```text
ws://127.0.0.1:8787
```

两个模拟器或两台设备使用同一个房间号，点击“在线”即可进入同一房间。

如果要让同一局域网的真机访问这台电脑，可改成监听所有网卡，并把小游戏里的服务地址改成电脑的局域网 IP：

```bash
HOST=0.0.0.0 node server/ws-server.js
```

## 真机和上线注意

微信小游戏正式环境通常要求 WebSocket 使用 `wss://`，并且域名需要在小游戏后台配置到合法 socket 域名。开发者工具里可以先关闭域名校验用于本地调试；真机预览和线上版本建议部署 `server/ws-server.js` 等价服务到支持 TLS 的服务器，再把服务地址改为对应 `wss://` 地址。

## 发布

发布前先复制 `release.config.example.json` 为 `release.local.json`，填入真实 AppID 和线上 `wss://` 对战服务地址，然后运行：

```bash
npm run release:init -- --appid wx1234567890abcdef --socket wss://your-domain.example.com/ws --health https://your-domain.example.com/health --key private.wx.key --version 1.0.0
npm run release:prepare
npm run release:status
npm run release:review
npm run release:export
npm run release:preflight
npm run release:preview:ci
```

本机安装并登录微信开发者工具后，可以尝试：

```bash
npm run release:upload
```

如果使用微信后台生成的“代码上传密钥”，也可以安装 `miniprogram-ci` 后执行。AppSecret 不是上传密钥，不能写进小游戏前端或仓库：

```bash
npm ci
npm run release:key:install -- --from-file /path/to/downloaded-upload-key.key
npm run release:key:check
npm run release:upload:ci
```

完整步骤见 `PUBLISH.md`。

多人对战服务上线部署见 `docs/DEPLOY_WEBSOCKET.md`，GitHub Actions 上传见 `docs/GITHUB_ACTIONS_RELEASE.md`，提交审核材料见 `docs/SUBMISSION_CHECKLIST.md`，上传后发布流程见 `docs/POST_UPLOAD_RELEASE.md`。

## 目录

```text
wechat-arcade-battle/
  game.js             微信小游戏入口和 Canvas 渲染
  game.json           微信小游戏配置
  app.js/app.json     旧小程序入口，发布小游戏包时会忽略
  pages/index/        旧小程序页面，发布小游戏包时会忽略
  utils/game-core.js  可复用游戏规则核心
  utils/realtime.js   微信 WebSocket 客户端
  server/ws-server.js 本地多人对战服务器
  docs/               部署和提审清单
  scripts/            发布配置检查和上传脚本
```

# 发布到微信小游戏

这份项目已经按微信小游戏发布前状态整理：发布包会忽略本地 `server`、旧小程序 `pages` 和 `app.*` 文件，线上联机地址需要使用 `wss://`，并通过 `project.private.config.json` 放置真实 AppID。

## 1. 准备微信侧资源

- 一个已认证或可发布的微信小游戏账号。
- 微信小游戏 AppID，格式类似 `wx1234567890abcdef`。
- 已部署的 WebSocket 对战服务，地址必须是 `wss://`。
- 在微信公众平台后台把该 `wss` 域名配置到合法 socket 域名。
- 本机安装并登录微信开发者工具，或在后台生成“小程序代码上传密钥”用于 `miniprogram-ci`。

注意：AppSecret 不是代码上传密钥，不能用于上传小游戏代码；它也不能写入小游戏前端或提交到仓库。若后续服务端需要调用微信登录等接口，再把 AppSecret 放到服务端环境变量里。

## 2. 写入本地发布配置

可以用命令生成本地发布配置：

```bash
cd /Users/apple/Documents/C_Project/wechat-arcade-battle
npm run release:init -- --appid wx1234567890abcdef --socket wss://your-domain.example.com/ws --health https://your-domain.example.com/health --key private.wx.key --version 1.0.0
```

也可以手动复制示例配置：

```bash
cd /Users/apple/Documents/C_Project/wechat-arcade-battle
cp release.config.example.json release.local.json
```

编辑 `release.local.json`：

```json
{
  "appid": "wx1234567890abcdef",
  "version": "1.0.0",
  "desc": "霓虹街区首个多人对战版本",
  "socketServerUrl": "wss://your-domain.example.com/ws",
  "healthCheckUrl": "https://your-domain.example.com/health",
  "roomId": "arena-1",
  "reviewRoomId": "review-room",
  "robot": 1,
  "privateKeyPath": "private.wx.key",
  "compileOutputPath": "dist/compiled-result.zip",
  "previewQrcodePath": "dist/preview.jpg",
  "previewPagePath": "",
  "previewSearchQuery": ""
}
```

生成发布用小游戏配置：

```bash
npm run release:prepare
```

## 3. 发布前检查

```bash
npm run release:status
npm run release:review
npm run release:check
npm run release:scan
npm run release:export
npm run release:compile
npm run release:verify-service
npm run release:audit
```

检查通过后，说明项目已切换到生产环境，发布包干净，并且线上对战服务的健康检查和 WebSocket 握手可用。
`release:export` 会在 `dist/miniprogram` 生成一份可审查的发布包镜像和 `manifest.json`。
`release:review` 会在 `dist/review-submission.md` 生成当前版本的提审说明。
`release:compile` 会调用微信 `miniprogram-ci` 做正式小游戏编译检查，需要真实 AppID、上传密钥和微信服务网络。

也可以使用一键预检：

```bash
npm run release:preflight
```

## 4. 上传代码

### 方式 A：微信开发者工具 CLI

```bash
npm run release:upload
```

如果开发者工具不在默认位置，可指定 CLI 路径：

```bash
WECHAT_DEVTOOLS_CLI="/Applications/微信开发者工具.app/Contents/MacOS/cli" npm run release:upload
```

### 方式 B：miniprogram-ci

在微信公众平台生成“小程序代码上传密钥”，把密钥文件放到 `release.local.json` 的 `privateKeyPath` 指向的位置。密钥文件已被 `.gitignore` 忽略，不要提交到仓库。这里需要的是上传密钥文件，不是 AppSecret。

可以用脚本安全写入和校验本机密钥：

```bash
npm run release:key:install -- --from-file /path/to/downloaded-upload-key.key
npm run release:key:check
```

如果密钥内容已经复制到剪贴板，也可以：

```bash
pbpaste | npm run release:key:install -- --overwrite
npm run release:key:check
```

安装 CI 工具后上传：

```bash
npm ci
npm run release:preview:ci
npm run release:upload:ci
```

`release:preview:ci` 会生成预览二维码，默认路径是 `dist/preview.jpg`。建议先扫码验证体验版，再上传开发版本。

上传成功后，到微信公众平台管理后台把刚上传的开发版本提交审核。审核通过后再点击发布。
上传后的提审、审核通过、正式发布记录流程见 `docs/POST_UPLOAD_RELEASE.md`。

## 5. 常见卡点

- `game.json` 未找到：当前 AppID 是微信小游戏，项目必须保留 `game.json`、`game.js`，且 `project.config.json` 的 `compileType` 应为 `game`。
- `appid` 仍是 `touristappid`：没有生成 `project.private.config.json`，请运行 `npm run release:prepare`。
- 连接失败：线上必须使用 `wss://`，且域名必须配置到小游戏后台的 socket 合法域名。
- 找不到 CLI：本机没有安装微信开发者工具，或需要设置 `WECHAT_DEVTOOLS_CLI`。
- CI 上传失败：确认 `privateKeyPath` 指向真实上传密钥，并且后台已开启代码上传能力。
- 只有 AppSecret：仍不能上传代码，请到微信公众平台生成“小程序代码上传密钥”或改用已登录的微信开发者工具 CLI。
- 审核被拒：通常需要补齐小程序类目、隐私协议、用户信息用途说明、游戏内容合规说明。

## 6. GitHub Actions

如果想走远程 CI 上传，参考 `docs/GITHUB_ACTIONS_RELEASE.md` 配置仓库 Secrets。CI 会从环境变量生成发布配置，执行预检，生成预览二维码 artifact，然后使用 `miniprogram-ci` 上传开发版本。

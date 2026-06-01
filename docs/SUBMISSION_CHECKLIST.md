# 微信小程序提交审核清单

## 账号和基础资料

- 小程序账号已完成主体信息、类目和必要认证。
- 小程序名称、头像、简介、服务类目已填写。
- AppID 已写入本地 `release.local.json`，并通过 `npm run release:prepare` 生成 `project.private.config.json`。
- 微信开发者工具已安装并登录拥有该 AppID 开发权限的微信号，或已在后台生成代码上传密钥并配置 `privateKeyPath`。

## 网络和服务器

- 多人对战服务已部署到公网服务器。
- 对战服务对外地址使用 `wss://`，例如 `wss://battle.example.com/ws`。
- 证书有效，`https://battle.example.com/health` 可返回健康状态。
- 该域名已在微信公众平台配置为 socket 合法域名。
- `npm run release:check` 通过，确认发布包不含 `127.0.0.1`、`localhost` 或示例域名。
- `npm run release:compile` 通过，确认微信 CI 能正式编译小程序包。

## 内容合规

- 游戏内不包含赌博、抽奖提现、现金收益、诱导分享、诱导关注等内容。
- 当前版本不采集微信头像、昵称、手机号、位置、通讯录、相册等敏感信息。
- 若后续接入登录、排行榜或用户资料，需要在小程序后台补齐用户隐私保护指引。
- 对战昵称为用户手动输入，建议上线前加入敏感词过滤或服务端审核策略。
- 隐私与合规说明见 `docs/PRIVACY_AND_COMPLIANCE.md`。

## 提审素材

- 版本号：以 `release.local.json` 的 `version` 为准。
- 版本描述：以 `release.local.json` 的 `desc` 为准。
- 功能说明：俯视角街机对战游戏，支持单人练习、本机双人和 WebSocket 房间多人对战。
- 测试步骤：进入首页，点击“单人练习”验证基础玩法；输入房间号后点击“在线对战”，两个客户端进入同一房间进行对战。
- 审核账号：当前游戏不需要账号登录；如后台要求测试房间，可提供一个固定房间号，例如 `review-room`。
- 可复制提审文本见 `docs/REVIEW_SUBMISSION_TEXT.md`。
- 当前版本提审材料可运行 `npm run release:review` 生成到 `dist/review-submission.md`。

## 上传和发布

```bash
cd /Users/apple/Documents/C_Project/wechat-arcade-battle
npm run release:prepare
npm run release:review
npm run release:export
npm run release:preflight
npm run release:preview:ci
npm run release:upload
```

如果使用 CI 上传：

```bash
npm ci
npm run release:upload:ci
```

上传成功后，在微信公众平台把开发版本提交审核。审核通过后，回到版本管理页点击发布。
上传后的证据记录流程见 `docs/POST_UPLOAD_RELEASE.md`。

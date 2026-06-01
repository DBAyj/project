# GitHub Actions 上传微信小程序

本项目提供两份 workflow：

- 仓库根目录 `.github/workflows/wechat-release.yml`：适合当前 `C_Project` 作为仓库根目录，工作目录指向 `wechat-arcade-battle`。
- `wechat-arcade-battle/.github/workflows/wechat-release.yml`：适合只把 `wechat-arcade-battle` 单独作为仓库根目录。

## 需要配置的 Secrets

在 GitHub 仓库的 Settings -> Secrets and variables -> Actions 中添加：

- `WECHAT_APPID`：小程序真实 AppID，例如 `wx1234567890abcdef`。
- `WECHAT_SOCKET_SERVER_URL`：线上 WebSocket 地址，例如 `wss://battle.example.com/ws`。
- `WECHAT_UPLOAD_PRIVATE_KEY`：微信公众平台生成的小程序代码上传密钥内容。

不要把 AppSecret 配置成 `WECHAT_UPLOAD_PRIVATE_KEY`。AppSecret 只适合放在服务端环境变量中调用微信服务端接口，不能用于小程序代码上传。

本地可以先用同一份密钥跑一次格式校验：

```bash
npm run release:key:install -- --from-file /path/to/downloaded-upload-key.key
npm run release:key:check
```

可选 Variables：

- `WECHAT_UPLOAD_ROBOT`：上传机器人编号，默认 `1`。
- `WECHAT_ROOM_ID`：默认房间号，默认 `arena-1`。
- `WECHAT_REVIEW_ROOM_ID`：审核测试房间号，默认 `review-room`。
- `WECHAT_HEALTH_CHECK_URL`：健康检查地址，默认会从 `WECHAT_SOCKET_SERVER_URL` 推导为 `/health`。
- `WECHAT_COMPILE_OUTPUT_PATH`：微信 CI 编译产物路径，默认 `dist/compiled-result.zip`。
- `WECHAT_PREVIEW_SEARCH_QUERY`：预览二维码启动参数，默认空。

## 运行

进入 GitHub Actions，选择 `WeChat Mini Program Release`，手动触发 workflow，填写版本号和上传描述。

流程会自动执行：

```bash
npm ci
npm run release:from-env
npm run release:prepare
npm run release:preflight
npm run release:preview:ci
npm run release:upload:ci
```

Workflow 会把预览二维码作为 `wechat-preview-qrcode` artifact 保存。扫码验证体验版正常后，仍需到微信公众平台把开发版本提交审核。审核通过后再发布。

`release:preflight` 内含微信 CI 编译检查和线上 WebSocket 服务验证。除非只是离线烟测，不建议设置 `WECHAT_SKIP_CI_COMPILE=1` 或 `WECHAT_SKIP_SERVICE_VERIFY=1`。

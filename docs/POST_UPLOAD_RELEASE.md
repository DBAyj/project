# 上传后提交审核与发布

代码上传完成后，还需要在微信公众平台完成提交审核、等待审核、正式发布。建议每一步都记录证据，方便确认当前版本是否真的已经上线。

## 1. 预览验证

生成预览二维码：

```bash
npm run release:preview:ci
```

扫码验证以下路径：

- 首页可打开。
- 单人练习可移动、开火、得分。
- 本机双人可对战。
- 在线对战可连接正式 `wss://` 服务。

记录事件：

```bash
npm run release:record -- --event preview-created --qrcode dist/preview.jpg --note "预览二维码已验证"
```

## 2. 上传开发版本

```bash
npm run release:upload:ci
```

上传成功后记录：

```bash
npm run release:record -- --event uploaded --evidence "miniprogram-ci upload success"
```

## 3. 提交审核

到微信公众平台进入版本管理，选择刚上传的开发版本，提交审核。可复制：

- `dist/review-submission.md`
- `docs/REVIEW_SUBMISSION_TEXT.md`
- `docs/PRIVACY_AND_COMPLIANCE.md`

提交审核后记录审核单号或后台截图说明：

```bash
npm run release:record -- --event submitted-review --reviewId "填写审核单号" --note "已提交微信审核"
```

## 4. 审核通过

审核通过后记录：

```bash
npm run release:record -- --event review-approved --reviewId "填写审核单号"
```

## 5. 正式发布

在微信公众平台点击发布。发布后记录：

```bash
npm run release:record -- --event published --evidence "微信公众平台显示已发布"
```

最终证据文件位于：

```text
dist/release-log.json
```

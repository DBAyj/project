# 部署多人对战 WebSocket 服务

微信小程序正式环境需要使用 `wss://` 连接线上服务。本项目的小程序端默认连接 `/ws`，服务端本身监听普通 HTTP/WebSocket，推荐用 Nginx 或云厂商负载均衡终止 TLS。

## 直接启动

```bash
cd /opt/wechat-arcade-battle
HOST=127.0.0.1 PORT=8787 node server/ws-server.js
```

健康检查：

```bash
curl http://127.0.0.1:8787/health
```

## systemd

复制示例服务：

```bash
sudo cp server/deploy/systemd.service.example /etc/systemd/system/wechat-arcade-battle.service
sudo systemctl daemon-reload
sudo systemctl enable --now wechat-arcade-battle
sudo systemctl status wechat-arcade-battle
```

## Nginx 反向代理

将 `server/deploy/nginx.websocket.conf` 里的域名和证书路径替换为真实值，再放入 Nginx 配置目录。上线后检查：

```bash
curl https://your-domain.example.com/health
```

小程序发布配置里的地址应填写：

```text
wss://your-domain.example.com/ws
```

发布前可验证健康检查和 WebSocket 握手：

```bash
npm run release:verify-service
```

## Docker

从项目根目录构建：

```bash
docker build -f server/Dockerfile -t wechat-arcade-battle-server .
docker run -d --name wechat-arcade-battle-server -p 127.0.0.1:8787:8787 wechat-arcade-battle-server
```

随后仍然建议使用 Nginx 或云负载均衡提供 `wss://`。

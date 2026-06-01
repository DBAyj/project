FROM node:22-alpine

WORKDIR /app/server

COPY server/package.json /app/server/package.json
COPY server/ws-server.js /app/server/ws-server.js
COPY utils/game-core.js /app/utils/game-core.js

ENV NODE_ENV=production
ENV HOST=0.0.0.0
ENV PORT=8787

EXPOSE 8787

CMD ["node", "ws-server.js"]

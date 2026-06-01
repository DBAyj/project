var config = require("./utils/config");

App({
  globalData: {
    env: config.env,
    defaultServerUrl: config.serverUrl,
    defaultRoomId: config.roomId
  }
})

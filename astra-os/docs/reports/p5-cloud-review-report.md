# P5 云端代码审查修复报告与云下操作需求

- 日期：2026-09-24
- 分支：`claude/nice-franklin-taaz1b`（仓库 `DBAyj/project`，项目位于 `astra-os/` 子目录）
- 审查基线：提交 `4bfbca5 Add astra-os project`（本地 `/Users/apple/CodexProjects/astra-os` 的上传快照）
- 范围：只修复逻辑漏洞与缺陷，并补充对应回归测试；不新增功能，不改动冻结架构、协议 Schema 和错误码注册表。

## 1. 云端验证环境

| 项目 | 云端实际值 | 与 Mac 开发主机的差异 |
| --- | --- | --- |
| 系统 / 架构 | Ubuntu 24.04.4 LTS，x86_64 | Mac 为 Apple Silicon arm64 |
| Qt | 6.8.1（conda-forge：qt6-main、qt6-multimedia、qt6-quick3d） | Mac 使用 Homebrew Qt |
| OpenCV | 5.0.0（conda-forge headless） | Mac 使用 Homebrew OpenCV |
| 编译器 / CMake | GCC 13.3.0，CMake 3.28.3，Ninja | Mac 使用 Apple Clang |
| 图形 | `QT_QPA_PLATFORM=offscreen`，软件渲染，没有 Metal / RHI | Mac 使用 Metal |
| Python | 3.12 虚拟环境，含 jsonschema、PyYAML | 与 Mac 的 `.venv` 同类 |

仓库里的 `scripts/*.sh` 依赖 `brew`、`launchctl`、`route -n get`、`ipconfig getifaddr` 和 arm64 产物检查，在 Linux 上无法直接运行。所以云端没有调用这些脚本，而是用等价的 CMake、CTest 和 Python 命令完成验证。

## 2. 已修复的问题

| # | 严重度 | 位置 | 问题（已确认） | 修复 | 回归测试 |
| --- | --- | --- | --- | --- | --- |
| 1 | 高 | `CMakeLists.txt` | 约 40 个测试文件用 `assert()` 做校验，其中很多把有副作用的调用写在 `assert()` 里（如 `assert(server.listen(...))`）。P5 用 `RelWithDebInfo` 构建，CMake 会定义 `NDEBUG`，这些断言整行被删除。结果是测试要么什么都不校验就通过，要么崩溃。云端实测：`astra-shell-intent-client-tests`、`astra-shell-p2-controller-integration-tests` 出现 SegFault，性能阈值断言被跳过 | 对所有 `*-tests`、`*-test`、`*-benchmark` 可执行文件追加 `-UNDEBUG`，不影响产品二进制 | RelWithDebInfo 全量 CTest |
| 2 | 高 | `ProjectionService::listen` | Unix Socket 每次 `readyRead` 都把 `readAll()` 的结果按 `\n` 切分，没有逐连接缓冲。请求被拆成两段到达时会被当作非法 JSON 拒绝；连接断开后也没有释放 socket | 改为 `canReadLine()` 循环读取，断开时 `deleteLater`，与 `SpatialUIService` 的写法一致 | `projection_service_test.cpp`：同一请求分两次写入 |
| 3 | 高 | `SpatialUIService::listenHttp` | HTTP 缓冲区处理完后既不清空也不标记。同一连接上再到达的任何字节（尾随数据、WebSocket 客户端帧）都会让 `handleHttpRequest` 在旧缓冲区上再执行一次，可能导致 POST 被执行两次，或向已升级的 WebSocket 流里再写一个 `101` 响应。另外请求体大小没有上限 | 每个连接只处理一次请求，之后到达的字节直接丢弃；请求上限 1 MiB，超出返回 400 `request_too_large` | `spatial_ui_service_test.cpp`：升级完成后发送一个 WebSocket ping 帧 |
| 4 | 中 | Shell 的 4 个本地 Socket 客户端 | `waitForReadyRead()` 只等一次就 `readLine()` 或 `readAll()`。响应分多段到达时（如 `projection.output.frame` 返回的 PNG base64），客户端会误判“服务无响应”并进入降级流程 | 新增 `LocalSocketResponse.h`，在超时内读到完整一行为止 | `intent_service_client_test.cpp`：服务端分两段返回响应 |
| 5 | 高（隐私） | `ShellController::serviceIntent` | 意图服务从文本里提取出的 `privacy_level` 会直接覆盖用户选择的级别。用户选了 `PRIVATE_SCREEN_ONLY`，输入“把设备模型投到桌面上，公开展示”，就会按 `PUBLIC` 投影。这与“隐私级别只能由策略服务决定”的规则冲突 | 文本提取出的级别只能收紧、不能放宽用户选择，取两者中更严格的一个 | `p2_shell_controller_integration_test.cpp`：投影被拒绝（4301），隐私级别保持 `PRIVATE_SCREEN_ONLY` |
| 6 | 中 | `SpatialUIRuntime`：`spatial_ui.component.remove` | 删除通知对应的组件时，通知记录和关键通知的焦点恢复链没有一起清理。之后该通知永远不会过期，`notification.clear` 会返回 5105，同一个 ID 也无法重新创建 | 通知组件改走 `clearNotification` 路径 | `spatial_ui_runtime_state_test.cpp` |
| 7 | 中 | `SpatialUIRuntime::removeComponent` | 删除组件时，它的窗口仍然保留。`state.save` 会把这个孤儿窗口写进状态文件，之后 `state.load` 必然返回 5805，持久化状态无法恢复 | 删除组件时一并移除它的窗口（新增 `SpatialWindowManager::removeWindow`） | `spatial_ui_runtime_state_test.cpp` |
| 8 | 中 | `SpatialWindowManager` | `closeWindow` 只把状态改成 `Closed`，不释放名额。关闭的窗口一直占用 `maximumWindows`，同一个 ID 无法重新打开，还会被持久化，恢复后变成隐藏窗口 | 关闭的窗口不再计入上限；同 ID 的关闭窗口可以重新创建；`findByComponentId` 跳过关闭的窗口；`saveState` 不写入关闭的窗口。关闭的窗口仍可通过 `find()` 查到，与原有单元测试一致 | `spatial_window_manager_test.cpp`、`spatial_ui_runtime_state_test.cpp` |
| 9 | 中 | `SpatialUIRuntime`：`spatial_ui.state.load` | 恢复到一半失败时不回滚，运行时留下半套组件。之后每次重试都会因为“需要空运行时”（5803）被拒绝 | 失败时整体回滚；布局、页签、面板顺序在全部校验通过后才写入 | `spatial_ui_runtime_state_test.cpp`：构造损坏的状态文件，连续加载两次都返回 5805 |
| 10 | 中 | `astra_intent/transport/jsonrpc_server.py` | Unix Socket 服务单线程、逐个处理连接，而且连接没有超时。一个连上后不发数据的客户端会阻塞所有其他请求 | 连接空闲超时 5 秒，超时后关闭该连接并继续服务 | `test_transports.py`：空闲连接不阻塞其他请求（未修复时该用例超时失败） |
| 11 | 低 | `tests/qml/tst_p5_spatial_ui.qml` | `SpatialSystemPanel { spatialModel: spatialModel }` 右侧解析成了面板自己的属性，形成绑定循环，面板从未拿到测试模型。`verify_p5_qml_warnings.py` 只扫描 `apps/astra-shell/qml/`，因此没有发现 | 测试模型的 id 改为 `spatialModelFixture` | QML 套件不再出现 `Binding loop` 告警 |

以上每个问题的回归测试都做过反向验证：只撤销源码修复、保留测试时，对应测试会失败；恢复修复后测试通过。

## 3. 云端验证结果

| 检查项 | 命令 / 方式 | 结果 |
| --- | --- | --- |
| C++ 构建（Debug 与 RelWithDebInfo） | `cmake -G Ninja -DBUILD_TESTING=ON` + `cmake --build` | 通过，无警告 |
| CTest 全量（RelWithDebInfo，断言已启用，串行） | `ctest --timeout 900` | 60/60 通过 |
| CTest 全量（Debug） | `ctest` | 59/59 通过 |
| QML 测试 | `qmltestrunner -input apps/astra-shell/tests/qml`（offscreen，Basic 样式） | 25/25 通过 |
| P2 意图服务 Python 测试 | unit / contract / integration / security / performance | 全部通过 |
| Schema 契约测试 | `tests/contract/test_p1..p5_schemas.py` | 全部通过 |
| 夹具生成器测试 | 3 个 `test_generate` / `test_fixture_generator` | 全部通过 |
| 格式 / Lint | `check_p1_format.py`、`shellcheck scripts/*.sh`、`py_compile`、`compileall` | 全部通过 |
| P2 在线验收 | 在容器的非回环 IP 上启动意图服务，运行 `tests/p2-e2e/test_p2_live_service.py` | 通过 |
| P3 在线验收 | 启动 `astra-spatial-service`，运行 `tests/p3-e2e/test_p3_live_service.py` | `PASSED`，目标丢失后安全暂停，错误码 3305 |
| 文档基线 | `verify_document_baseline.py` | 25 项通过。Mermaid 检查在云端失败，原因是 root 用户下 Chromium 沙箱无法启动（环境问题）；加 `--no-sandbox` 后 7 张图全部渲染成功 |

说明：

- 空间性能测试在 4 核云主机上与其他测试并行（`ctest -j4`）时，会因 CPU 争用超出 20 ms 的 P95 阈值；串行运行时 0.65 秒通过。Mac 上的脚本本来就是串行运行 CTest。
- 云端没有运行、也无法运行：Metal / RHI 渲染、真实窗口截图、`launchctl` 启停、arm64 产物检查、`make *-release-gate` 脚本本身。这些列入第 4 节。

## 4. 云下（本地 Mac）操作需求

以下步骤需要在本地 Mac 上执行，可以交给本地的 Codex App 完成。每一步都列出了验收标准。

### L1 把云端修复同步到本地项目

1. 拉取最新代码：

   ```sh
   cd ~/CodexProjects/project-sync
   git pull origin claude/nice-franklin-taaz1b
   ```

2. 生成相对于 `astra-os/` 的补丁，然后应用到原项目：

   ```sh
   git diff 4bfbca5 HEAD --relative=astra-os -- astra-os > ~/astra-os-cloud-fixes.patch
   cd /Users/apple/CodexProjects/astra-os
   git apply --3way --check ~/astra-os-cloud-fixes.patch
   git apply --3way ~/astra-os-cloud-fixes.patch
   ```

   本地 `astra-os` 是否是独立的 git 仓库，目前未确认。如果不是，去掉 `--3way` 再执行 `git apply`。

- 验收：`--check` 无报错；`git status` 显示第 2 节列出的文件有改动，并新增 `apps/astra-shell/src/clients/LocalSocketResponse.h` 和本报告。
- 如果有冲突：说明本地在上传后又有新改动。把冲突文件和 `git apply` 的输出发回云端处理，不要手工删改。

### L2 重新构建并跑 P1 发布门禁

```sh
make p1-clean && make p1-release-gate
```

- 验收：全部通过。重点看：`astra-shell-intent-client-tests`（响应改为分段返回）、`astra-shell-p2-controller-integration-tests`（新增隐私收紧用例）、`p1-test-metal`、`p1-test-window-interaction`。

### L3 P2、P3 验证（含真实局域网地址）

```sh
make p2-verify
make p3-verify
```

- 验收：通过。P2 新增“空闲连接不阻塞”用例，大约多耗时 0.2 秒。

### L4 P4 发布门禁

```sh
make p4-clean && make p4-release-gate
```

- 验收：通过。重点看：投影服务分段请求用例、`projection.output.frame` 取帧（客户端现在会读完整行）、Metal 证据、视觉基线、性能与稳定性。

### L5 P5 发布门禁（按 AGENTS.md，需要跑两次）

```sh
make p5-clean && make p5-release-gate
```

- 第一次：在当前提交的分支上运行。
- 第二次：合并到 `develop` 后再运行一次。
- 验收：全部通过，包括真实的 P3-P4-P5 服务链验收。
- 注意：`RelWithDebInfo` 的测试程序现在会保留断言。以前被 `NDEBUG` 静默跳过的断言（包括性能阈值）现在会真正生效。如果出现新失败，这是修复后暴露出的真实问题，请把完整输出发回云端，不要回退第 2 节第 1 项。

### L6 文档基线（Mermaid 需要本地浏览器环境）

```sh
make docs-verify
```

- 验收：全部通过，包括 Mermaid 渲染。

### L7 回写报告，保持两端一致

1. 以上门禁会重新生成 `docs/reports/` 下的 `p1-*`、`p4-*`、`p5-*` 报告。云端没有改动这些文件。
2. 在本地原项目提交所有改动：代码、测试、重新生成的报告。
3. 把同样的内容同步回 `project-sync/astra-os/`，推送到 `claude/nice-franklin-taaz1b`。

- 验收：云端拉取后，`astra-os/` 与本地原项目内容一致。

## 5. 待确认事项（未修改，需要决策）

以下是审查中看到的设计层面疑点。它们要么需要产品或架构决策，要么超出本次“只修缺陷”的范围，所以没有修改。标注“推断”的，是没有找到文档依据的判断。

1. `ProjectionPolicyRequest::roomTrusted` 默认为 `true`，与“权限默认拒绝”的工程规则不一致（推断）。P1 的调用方都没有显式设置这个字段。需要确认是有意为之，还是应改为默认 `false`，并由上游显式授予。
2. 恢复状态时，`TASK_CARD` 和 `NOTIFICATION` 类型的组件只会恢复成普通组件，任务内容和通知内容不会恢复。已确认代码是这样；是否符合预期需要确认。
3. `spatial_ui.target.available` 不会重新显示在 `target.lost` 时被隐藏的组件，需要由调用方逐个恢复（推断为有意设计）。
4. `window_count` 仍然把 `CLOSED` 窗口计算在内。本次只修复了名额占用和持久化问题，没有改变对外返回的计数语义。
5. 建议给 `astra-spatial-performance-tests` 设置 CTest 的 `RUN_SERIAL`，避免并行运行时 CPU 争用导致误报。

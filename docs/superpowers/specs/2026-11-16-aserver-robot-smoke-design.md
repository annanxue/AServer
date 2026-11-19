# AServer Windows 核心协议冒烟测试设计

## 目标

在当前 Windows 环境中启动 AServer 的最小本机服务链，通过日志、端口状态和 RobotClient 验证核心网络协议链路。测试尽量绕过外部 Auth、MySQL、BF、GM 和支付服务，但不伪造 AServer 内部协议处理结果。

## 范围

本次测试覆盖：

- `logserver`、`loginserver`、`gameserver`、`robotclient` 的 Windows Release 运行。
- 本机 `127.0.0.1` 网络连接和动态端口分配。
- RobotClient 连接 LoginServer、获取本地测试登录响应、连接 GameServer。
- 基础认证/协议消息和至少 10 秒心跳存活窗口。
- 服务日志中的启动错误、Lua 错误、异常退出、断线重连和外部依赖阻塞。

本次不覆盖：

- 真实 SDK/Auth、支付、GM、BF 业务。
- 真实 MySQL 数据一致性和完整游戏业务回归。
- 将完整 `luacommon` 游戏数据永久纳入 AServer 仓库。

## 运行时架构

新增 Windows 测试运行器，建议路径为 `tests/robot_smoke/run_core_smoke.ps1`。每次运行创建独立临时目录 `%TEMP%\\AServer-smoke-<timestamp>`，不修改仓库中的 `*.lua.svn` 原始配置。

运行器执行以下步骤：

1. 检查 `bin/Release/` 中的 `loginserver.exe`、`gameserver.exe`、`logserver.exe`、`robotclient.exe` 及运行时 DLL。
2. 从同级 `ahxt_server/server/luacommon` 复制公共 Lua 数据，替换 AServer 中 `link ...` 占位文件对应的运行时目录。
3. 从各服务的 `*.lua.svn` 生成临时 `.lua` 配置，设置 `Daemon=0`、`127.0.0.1` 和动态空闲端口。
4. 将日志路径、Lua 脚本路径和资源路径指向临时运行时。
5. 启动本地 HTTP 登录 stub，返回 RobotClient 所需的测试账号字段：`uid`、`sign`、`timestamp`、`p_uid`、`channelId`、`channelLable`。
6. 按 `logserver`、`loginserver`、`gameserver`、`robotclient` 顺序启动，并在每一步等待进程、端口和日志条件。
7. 测试完成后停止整个进程树；成功时清理临时目录，失败时保留目录并输出路径。

如果运行日志证明 Login/Game 启动仍强制依赖 DB 或其他外部服务，只增加显式、默认关闭的 `SmokeMode` 分支，跳过外部连接初始化，不伪造内部认证或消息处理。

## 测试断言

### 服务启动

- 所有必需进程保持存活。
- 所有配置监听端口进入 listening 状态。
- 日志没有启动级 `fatal`、`assert`、Lua traceback、配置加载失败或快速崩溃。

### RobotClient 登录链路

- RobotClient 日志出现 `on_loginsvr_connect`。
- 本地 HTTP stub 收到登录请求。
- RobotClient 成功解析登录响应，无 HTTP 超时或 JSON 解码错误。
- RobotClient 日志出现 `on_gamesvr_connect`。
- GameServer 日志出现客户端连接及认证处理。

### 心跳与稳定性

- RobotClient 和服务进程持续运行至少 10 秒。
- 至少完成一轮基础认证/协议消息收发。
- 心跳窗口内无异常断线、无限重连或进程退出。

结构化结果至少包含以下检查：

```text
PASS  binaries
PASS  logserver_started
PASS  loginserver_started
PASS  gameserver_started
PASS  robot_login_connected
PASS  local_login_stub_response
PASS  robot_game_connected
PASS  heartbeat_window
PASS  no_startup_fatal
```

## 诊断与错误处理

- 不依赖服务退出码判断成功；同时检查进程存活、端口、关键日志和 RobotClient 事件。
- 失败时保存每个服务的 stdout/stderr、应用日志、端口快照、进程快照和 `result.json`。
- 测试脚本退出码：`0` 表示通过，`1` 表示代码/配置/协议失败，`2` 表示明确的外部依赖阻塞。
- 不写入真实账号、密码或远程服务凭据。
- 不自动修改 Windows 防火墙或系统服务。
- 生产配置和正常启动路径默认保持不变；任何 `SmokeMode` 默认关闭。

## 验证顺序

1. 先运行现有 Release 产物，收集第一轮完整日志。
2. 按首个真实失败点修复配置、运行时资源或跨平台代码。
3. 每次修复后重新运行冒烟测试，确认没有把网络连接误判为业务成功。
4. 测试通过后，再补充可重复执行的机器人测试脚本和现有文档中的运行方式。

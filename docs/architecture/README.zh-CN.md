# 架构与合成 CAN

> [English](README.md)

## 组合与归属

[源码清单](../../product/sources.json)选择 Product 源码。[Product 组合](../../product/product.c)绑定目录、路由、设置、策略及 UI。[固件组合](../../product/firmware.c)提供独立的 domain、presentation、diagnostic、UI 存储及语言初始化。

CAN 适配器向 Framework runtime/Core 发布语义值。Product 应用层生成呈现，LVGL 负责显示并提交意图。本机设置工作流放在 `services/`，不在控件回调中解释。共享传输、持久化及更新实现保留在 Framework。

## 权威输入

| 输入 | 生成 / 消费文件 |
|---|---|
| [目录](../../catalog/demo_catalog.json) | `generated/demo_catalog.*`、信号身份、过期门限及描述符 |
| [接收 DBC](../../protocol/can/demo.dbc) | `generated/can/demo.*`、适配器及路由表 |
| [域映射](../../protocol/can/domain-map.yaml) | 生成适配器所需信号映射 |
| [发送 DBC](../../protocol/can/demo_tx.dbc) | 与 `protocol/can/demo_pdo.c` 对照测试的契约 |
| [字体记录](../../assets/fonts.json) | 入库的中文字体子集 |
| [资源记录](../../assets/manifest.json) | 图标、来源许可证和哈希 |

## 合成报文契约

以下均为合成标准 CAN 帧，DLC 8，多字节小端，在 Demo CAN0 路由上使用。接收信号的单位、有符号性和缩放以 DBC 为准，新鲜度以目录为准。SDL2 在进程内注入帧，不设置物理总线波特率。

| ID | 方向 | 内容 |
|---|---|---|
| `0x100` | 接收 | 车速、转角、运行小时 |
| `0x101` | 接收 | SOC、电压、充电状态 |
| `0x102` | 接收 | 举升高度 |
| `0x103` | 接收 | 载荷 |
| `0x104` | 接收 | 座椅、制动、空挡、告警及温度 |
| `0x381` | 周期发送 | 运动数据、新鲜度及序号 |
| `0x481` | 周期发送 | 状态、新鲜度、序号、布局版本及代数 |

发送 DBC 周期为 100 ms。Demo 没有车辆控制命令路由。周期合成状态帧不意味着允许在真实车辆上复用这些 ID。协议修改必须执行 DBC/C 一致性测试，并明确审查状态和新鲜度。

## UI 与资源

UI 使用原生 LVGL。字体和图标属于 Product 资源，保留入库生成元数据。新增界面文字时先补翻译并重新生成字体子集，不使用整屏设计图片替代。实际渲染见[图集](../ui/screenshots.zh-CN.md)。

## 墙上时钟与本地时间

顶栏时钟读取 Framework 的墙上时钟契约，而不是运行时长计数器。契约固定输出 UTC；本 Product 施加一个就地注释的东八区偏移，并用 24 小时制 `HH:MM` 渲染，因此不需要额外的翻译文案或字体子集。来源缺失、读取失败或采样越界时渲染 `--:--`，而不是捏造一个时间。

`sim/main.c` 绑定一个固定且可写的来源，使图集与主机测试逐字节可复现；固件构建则从 Framework 平台端口取得真实板载 RTC。管理员页有一行"时钟"，它打开共用的数字编辑器并通过同一契约写回：保留输入的时与分，并在时钟已可信时沿用当前日期。

## 兼容性

按照截图证据和消费方 gitlink 记录的 Framework 版本构建。本仓库不包含 SDK 工具链或板级配置。通用修复进入 Framework，客户扩展放在独立私有 Product。参见[构建](../build/README.zh-CN.md)。

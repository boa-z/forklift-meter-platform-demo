# 叉车仪表 Demo Product

> [English](README.md)

面向 forklift-meter-platform 的完整公开合成 Product，展示 800×480 原生 LVGL 仪表、中英双语、合成 CAN、本机设置及可追溯验证。它不包含客户协议，也不宣称通过车辆验收。

## 从这里开始

1. 按[构建与运行](docs/build/README.zh-CN.md)通过 Framework 编译并启动 SDL2。
2. 将运行结果与 [SDL2 实际截图](docs/ui/screenshots.zh-CN.md)对照。
3. 新建独立 Product 时参考 [Product 手册](docs/README.zh-CN.md)。

![英文主界面](docs/ui/screenshots/dashboard-en.png)
![中文主界面](docs/ui/screenshots/dashboard-zh-CN.png)

## 仓库边界

本仓库管理 `product/sources.json`、目录、协议、应用、UI、字体、资源、测试和文档。消费它的 Framework 管理通用运行时与构建工具。Framework 通过 `products/demo` 固定引用本仓库；构建选择 `METER_PRODUCT_ROOT=products/demo`，也可传入其他检出的绝对路径。本仓库不另建第二套独立 CMake 构建。

| 目录 | 职责 |
|---|---|
| `product/` | 源码清单、组合、策略、固件及测试 |
| `catalog/`、`generated/` | 合成定义与入库生成结果 |
| `protocol/`、`application/`、`services/` | 报文映射、呈现与设置工作流 |
| `ui/`、`assets/` | 原生 LVGL 渲染、字体、图标和许可证 |
| `sim/`、`fixtures/`、`tests/` | 合成主机输入与 Product 回归测试 |
| `docs/`、`tools/` | Product 手册与实际截图导出/检查 |

## 许可证与贡献

第一方代码沿用 Framework 的 [Apache-2.0 许可证](LICENSE)。第三方字体/图标声明保留在 `assets/LICENSES/` 和 `generated/can/`。请阅读[开发规则](AGENTS.md)和[验证要求](docs/validation/README.zh-CN.md)。先提交 Product 修改，再更新消费它的 Framework gitlink。

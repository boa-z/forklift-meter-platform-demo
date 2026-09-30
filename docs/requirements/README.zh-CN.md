# 参考需求

> [English](README.md)

## 范围

这是原创合成参考仪表。LVGL 实现本身作为参考 UI，不对应客户 PSD 或车辆 DBC。主机显示固定为 800×480。

## 验收矩阵

| ID | 需求 | 检查 / 证据 |
|---|---|---|
| DEMO-01 | 构建时只选择一个 Product，目录及组合独立 | `product/sources.json`、Framework Product 边界测试 |
| DEMO-02 | 主界面、监测、故障、设置四个页面 | `sdl-smoke`、`ui-pagination`、双语截图 |
| DEMO-03 | 中英双语与所需字体子集 | `i18n-ui`、`fonts`、目视检查 |
| DEMO-04 | 合成 CAN 解码及周期编码与 DBC 一致 | `dbc-generation`、`can-replay`、`demo-pdo-dbc` |
| DEMO-05 | 区分 unknown、stale、offline、error 状态 | `demo-scenarios`、`ui-fixtures`、状态截图 |
| DEMO-06 | 设置意图和呈现不在渲染层解释策略 | `demo-settings-app`、`product-presentation`、架构检查 |
| DEMO-07 | 可展示更新进度，但不冒充安装结果 | `update-ui`、明确标识的预览截图 |
| DEMO-08 | Product 文档和截图跟随实现 | `demo-docs-sync`、`demo-docs-evidence` |

## 验收边界

截图审查检查所捕获页面的裁切、导航、字形和未知值显示。自动化检查补充人工审查，单个 PASS 报告不等于设计一致性。完整主机测试覆盖范围大于图集。

主机测试不能证明 CAN 电气行为、真实亮度、EEPROM 寿命、更新可信性、回滚或车辆安全。真实 Product 必须自行记录已确认协议、总线参数、权限、存储策略、硬件身份及上板验收。

参见[架构](../architecture/README.zh-CN.md)和[验证](../validation/README.zh-CN.md)。

# 验证与交付

> [English](README.md)

## 执行检查

按[构建指南](../build/README.zh-CN.md)运行消费方 Framework 的完整 CTest。`product/tests.cmake` 注册本 Product 的测试。固件与 SDL2 配置需要分别保留证据。

| 检查 | 证明范围 |
|---|---|
| `dbc-generation`、`demo-pdo-dbc`、`can-replay` | 生成解码器与合成报文契约 |
| `catalog`、`fonts`、`assets` | 定义、翻译字形覆盖、源码固定版本及资源精确哈希 |
| `demo-settings-app`、`product-presentation` | 设置意图与呈现 |
| `ui-pagination`、`i18n-ui`、`ui-fixtures`、`demo-scenarios`、`sdl-smoke` | 导航、双语、状态与确定性主机路径 |
| `demo-docs-sync`、`demo-docs-evidence` | 双语结构、本地链接及关联源码的截图 |

## 截图证据

[图集](../ui/screenshots.zh-CN.md)由真实可执行程序生成。[清单](../ui/screenshots/manifest.json)记录十六张 800×480 图片、捕获参数、源码摘要、可执行文件 SHA256 和模拟器报告。每个用例要求 PASS、语言/页面/帧数符合请求且对象数量稳定。导出器验证 RGB 无损转换。

入库图片检查仅使用 Python 标准库；只有重新生成截图时才需要 Pillow。`.gitattributes` 固定文本 LF 检出，防止 Windows 换行转换导致生成字体和 SVG 来源哈希失效。不得仅为掩盖意外变更而更新哈希基线。

## 审查与记录

每次交付记录 Framework 提交、Product 提交、依赖 SHA、工具版本、构建选项、测试结果及日志位置。Product 专项记录放在本目录。截图清单与已审查图集共同保存。UI 修改时检查双语字形、单位、裁切、页码和 unknown/stale 表现。

图集覆盖主导航页面、监测分页、五种异常状态和一个更新预览。不覆盖所有密码框、管理员操作和设置交互；专项 UI 测试覆盖额外流程。修改某个流程且需要视觉证据时，扩展捕获用例。

## 上板验收单独进行

本次文档完善仅记录主机渲染 UI。这些图片不证明新固件安装、CAN 传输、物理屏幕或断电测试。上板证据应关联其自身镜像哈希、Product 版本及日志。合成参考行为不等于客户车辆验收。

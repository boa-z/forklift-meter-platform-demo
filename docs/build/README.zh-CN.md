# 构建、运行与截图

> [English](README.md)

## 构建主机 Product

在消费本仓库的 Framework 根目录运行以下命令，本仓库检出在 `products/demo`。先准备主机 Python 3、CMake、Ninja、C/C++ 编译器及 SDL2 开发文件。SDK Python/SCons 保持不变。

```text
git submodule update --init --recursive
python -m pip install -r tools/protocol/requirements.txt -r tools/hil/requirements.txt -r tools/ota/requirements.txt
cmake -S . -B build-demo -G Ninja -DCMAKE_BUILD_TYPE=Debug -DMETER_PRODUCT_ROOT=products/demo -DMETER_ENABLE_UPDATE=ON
cmake --build build-demo --parallel
ctest --test-dir build-demo --output-on-failure
```

通过判据：配置选中 `products/demo`，编译生成 `meter-demo`，CTest 零失败。Windows 可执行文件为 `meter-demo.exe`，Linux 为 `meter-demo`。缺少 Product 清单时初始化 submodule；找不到 SDL2 时设置 `-DSDL2_DIR=PATH_TO_SDL2_CMAKE`，并把运行时 DLL 目录加入 PATH。

## 启动与检查

以下为 Windows 示例，仍从 Framework 根目录执行：

```text
./build-demo/meter-demo.exe --set-language chinese
./build-demo/meter-demo.exe --hidden --frames 180 --scenario stale --set-language chinese --capture stale.bmp
```

Linux 去掉 `.exe`。交互模式打开 800×480 窗口，关闭窗口即可退出。自动截图使用固定帧数。`--page 0..3` 分别选择主界面、监测、故障、设置；`--subpage 1` 进入监测第二页。设置使用分类按钮，不存在第二个分页页。

模拟器输出 JSON 报告，要求 `result=PASS`、`language`/`page` 正确、完成帧数符合请求且对象数量稳定。它生成合成输入，不向车辆 CAN 发报文。

## 重新导出实际图集

先编译当前源码。使用已安装 Pillow 的主机 Python，并把 SDL2 DLL 目录加入 PATH。以下命令只写入 Product 自有图集文件：

```text
python -m pip install Pillow
python products/demo/tools/capture_screenshots.py --framework-root . --binary build-demo/meter-demo.exe
python tools/check_docs_sync.py --root products/demo
python products/demo/tools/check_docs.py
```

导出器使用 dummy 视频驱动运行十六个独立 SDL2 进程，将捕获的 BMP 像素转换为 PNG，不缩放、不重绘，并检查尺寸、页面、语言及模拟器状态。[清单](../ui/screenshots/manifest.json)保存命令、报告、可执行文件哈希、源码摘要和图片哈希。提交前逐张检查。

Base commit ID 表示捕获时检出的祖先提交；源码摘要描述实际被捕获的文件，包括未提交修改。源码摘要不匹配时必须重新构建和导出。清单是证据，不是固件密码学签名。

## 固件与 CAN OTA

在 Framework SDK 集成中通过 `METER_PRODUCT_ROOT` 选择本 Product。SDK 镜像、更新包生成及 CAN 刷入遵循 Framework 的 `docs/build/build.md` 和 `docs/ota/can-update.md`。板级配置与传输属于 Framework/SDK，不在 Product 内复制第二套 OTA 实现。

图集更新画面仅为 `--update-preview download`，不生成更新包、不传输或安装固件。真实 OTA 结果需要板端更新前后身份和传输原始日志。

# Forklift Meter Demo Product

> [中文版](README.zh-CN.md)

A complete public, synthetic Product for forklift-meter-platform. It demonstrates an 800×480 native LVGL instrument, English and Simplified Chinese, synthetic CAN, local settings and documented validation. It contains no customer protocol or vehicle acceptance claim.

## Start here

1. Follow [build and run](docs/build/README.md) to build through Framework and launch SDL2.
2. Compare the running app with the [actual screenshot gallery](docs/ui/screenshots.md).
3. Use the [Product handbook](docs/README.md) when creating an independent Product.

![English dashboard](docs/ui/screenshots/dashboard-en.png)
![Chinese dashboard](docs/ui/screenshots/dashboard-zh-CN.png)

## Repository boundary

This repository owns its `product/sources.json`, catalog, protocol, application, UI, fonts, assets, tests and documentation. The consuming Framework owns reusable runtime and build tooling. Framework pins this repository as `products/demo`; build with `METER_PRODUCT_ROOT=products/demo`, or an absolute path to another checkout. This repository intentionally has no second standalone CMake build.

| Directory | Responsibility |
|---|---|
| `product/` | Source manifest, composition, policies, firmware and tests |
| `catalog/`, `generated/` | Synthetic definitions and checked-in generated output |
| `protocol/`, `application/`, `services/` | Wire mapping, presentation and settings workflow |
| `ui/`, `assets/` | Native LVGL rendering, fonts, icons and licenses |
| `sim/`, `fixtures/`, `tests/` | Synthetic host input and Product regression tests |
| `docs/`, `tools/` | Product handbook and actual screenshot export/checks |

## License and contribution

First-party code retains the Framework's [Apache-2.0 license](LICENSE). Third-party font/icon notices remain under `assets/LICENSES/` and `generated/can/`. Read [development rules](AGENTS.md) and [validation requirements](docs/validation/README.md). Commit Product changes first, then update the consuming Framework gitlink.

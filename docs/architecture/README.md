# Architecture and synthetic CAN

> [中文版](README.zh-CN.md)

## Composition and ownership

[Source manifest](../../product/sources.json) selects Product sources. [Product composition](../../product/product.c) binds its catalog, routes, settings, policies and UI. [Firmware composition](../../product/firmware.c) provides distinct domain, presentation, diagnostic and UI stores plus locale initialization.

CAN adapters publish semantic values to Framework runtime/Core. Product application code creates presentation; LVGL renders it and submits intents. Local settings workflow belongs in `services/`, not in widget callbacks. Shared transport, persistence and update implementation stay in Framework.

## Authoritative inputs

| Input | Generated / consuming files |
|---|---|
| [Catalog](../../catalog/demo_catalog.json) | `generated/demo_catalog.*`, signal identities, stale thresholds and descriptors |
| [Receive DBC](../../protocol/can/demo.dbc) | `generated/can/demo.*`, adapter and route table |
| [Domain map](../../protocol/can/domain-map.yaml) | Signal mapping for the generated adapter |
| [Transmit DBC](../../protocol/can/demo_tx.dbc) | Contract tested against `protocol/can/demo_pdo.c` |
| [Font record](../../assets/fonts.json) | Checked-in Chinese font subsets |
| [Asset record](../../assets/manifest.json) | Icons, source licenses and hashes |

## Synthetic wire contract

All following frames are synthetic standard CAN frames, DLC 8, little-endian fields, on the Demo CAN0 route. RX signal units, signedness and scaling are defined in the DBC; freshness is defined in the catalog. SDL2 injects frames in-process and does not configure a physical bus bitrate.

| ID | Direction | Contents |
|---|---|---|
| `0x100` | Receive | Speed, steering and operating hours |
| `0x101` | Receive | SOC, voltage and charging |
| `0x102` | Receive | Lift height |
| `0x103` | Receive | Load |
| `0x104` | Receive | Seat, brake, neutral, warning and temperatures |
| `0x381` | Periodic transmit | Motion, freshness and sequence |
| `0x481` | Periodic transmit | Status, freshness, sequence, layout version and generation |

Transmit DBC cycle time is 100 ms. The Demo has no vehicle-control command route. Periodic synthetic status does not grant permission to reuse these identifiers on a real vehicle. Protocol changes require DBC/C agreement tests and explicit state/freshness review.

## UI and resources

The UI is native LVGL. Fonts and icons remain Product resources with checked-in generation metadata. Add translated text and regenerate the font subset before changing a screen; do not substitute a full-screen design image. Actual rendered output is in the [gallery](../ui/screenshots.md).

## Compatibility

Build against the Framework revision recorded in screenshot evidence and the consuming gitlink. This repository alone has no SDK toolchain or board configuration. Generic fixes go to Framework; customer extensions belong in a separate private Product. See [build](../build/README.md).

# Reference requirements

> [中文版](README.zh-CN.md)

## Scope

This is an original synthetic reference instrument. The LVGL implementation is the reference UI; there is no customer PSD or vehicle DBC behind it. The host display is fixed at 800×480.

## Acceptance matrix

| ID | Requirement | Check / evidence |
|---|---|---|
| DEMO-01 | Exactly one build-time Product, independent catalog and composition | `product/sources.json`, Framework Product boundary tests |
| DEMO-02 | Dashboard, monitor, faults and settings pages | `sdl-smoke`, `ui-pagination`, bilingual screenshots |
| DEMO-03 | English/Chinese text with the required glyph subset | `i18n-ui`, `fonts`, visual review |
| DEMO-04 | Synthetic CAN decode and periodic encoding agree with DBC | `dbc-generation`, `can-replay`, `demo-pdo-dbc` |
| DEMO-05 | Unknown, stale, offline and error states remain distinguishable | `demo-scenarios`, `ui-fixtures`, state screenshots |
| DEMO-06 | Settings intents and presentation stay outside renderer policy | `demo-settings-app`, `product-presentation`, architecture guard |
| DEMO-07 | Update progress can be presented without claiming installation | `update-ui`, explicitly labelled preview screenshot |
| DEMO-08 | Product docs and screenshots track implementation | `demo-docs-sync`, `demo-docs-evidence` |

## Acceptance boundaries

Screenshot review checks clipping, navigation, glyphs and unknown-value presentation for captured cases. Automated checks complement that review; a PASS report alone does not establish design fidelity. The complete host suite covers more cases than the gallery.

Host tests do not qualify CAN electrical behavior, physical brightness, EEPROM endurance, update trust, rollback or vehicle safety. A real Product must record its own approved protocol, bus settings, permissions, storage policy, hardware identity and board acceptance.

See [architecture](../architecture/README.md) and [validation](../validation/README.md).

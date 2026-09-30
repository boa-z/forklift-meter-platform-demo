# Reference Product development rules

- This repository contains synthetic public data only. Never import customer source, protocols, screenshots or history.
- Build through a compatible forklift-meter-platform checkout using METER_PRODUCT_ROOT. Keep Framework contracts and tools in Framework.
- Product requirements, architecture, protocol, build instructions, UI and validation live in docs/. Update English and Simplified Chinese together.
- Use native LVGL widgets and layout. Keep fonts, SVG/PNG icons and their licenses within this Product.
- Export screenshots from the actual SDL2 executable with tools/capture_screenshots.py. Preserve pixels and provenance; never redraw evidence.
- Run CTest through Framework, including Product docs, assets, fonts and bilingual UI checks. Host results do not establish board acceptance.
- First-party explanatory code comments use Chinese. Preserve upstream notices.
- Commit Product changes before updating the consuming Framework gitlink.

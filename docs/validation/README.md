# Validation and delivery

> [中文版](README.zh-CN.md)

## Run the checks

Follow the [build guide](../build/README.md) and run the complete consuming Framework CTest suite. `product/tests.cmake` registers this Product's tests. Firmware and SDL2 configurations need independent evidence.

| Gate | What it checks |
|---|---|
| `dbc-generation`, `demo-pdo-dbc`, `can-replay` | Generated decoder and synthetic wire contracts |
| `catalog`, `fonts`, `assets` | Definitions, translated glyph coverage, source pins and exact resource hashes |
| `demo-settings-app`, `product-presentation` | Settings intents and projection |
| `ui-pagination`, `i18n-ui`, `ui-fixtures`, `demo-scenarios`, `sdl-smoke` | Navigation, two languages, states and deterministic host paths |
| `demo-docs-sync`, `demo-docs-evidence` | Bilingual structure, local links and source-bound screenshots |

## Screenshot evidence

The [gallery](../ui/screenshots.md) is generated from the real executable. Its [manifest](../ui/screenshots/manifest.json) records sixteen 800×480 images, capture arguments, source digests, executable SHA256 and simulator reports. All cases require PASS, requested language/page/frame count and stable object count. The exporter verifies lossless RGB conversion.

The checked-in image checker uses Python's standard library; Pillow is needed only to regenerate screenshots. `.gitattributes` pins LF text checkout so Windows line-ending conversion cannot invalidate generated font and SVG source hashes. Never refresh hash baselines merely to hide unexpected changes.

## Review and record

For every release, record Framework commit, Product commit, dependency SHAs, tool versions, build options, test result and log location. Store Product-specific records here. Keep the screenshot manifest with the reviewed gallery. For a UI change, inspect glyphs, units, clipping, page labels and unknown/stale representation in both languages.

The gallery covers root navigation pages, monitor pagination, five abnormal states and one update preview. It does not show every password dialog, admin operation or settings interaction; dedicated UI tests cover additional flows. Extend captures when a changed flow needs visual evidence.

## Board acceptance remains separate

This documentation refresh records host-rendered UI only. No new firmware installation, CAN transfer, physical display check or power-loss test is established by these images. Keep board evidence tied to its own image hash, Product revision and logs. Synthetic reference behavior is not customer vehicle acceptance.

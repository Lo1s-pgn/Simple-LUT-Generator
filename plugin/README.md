# Plugin sources — LSP - Simple LUT Generator

- **`core/LSPLutGeneratorPlugin.cpp`** — **`ImageEffect`**: **`render`**, **`changedParam`** (export, support URLs, **Resolve**-safe param leaf names), factory registration, **`setVersion`** in **`describe`**.
- **`core/LSPLutGeneratorDescribe.cpp`** — Clips, **LUT GENERATOR** / nested **Export** (path, **Set path file**, **LUT name**, **Export LUT**, **Export LUT size**), **SUPPORT**.
- **`core/LSPLutGeneratorProcessor.cpp`** — **`ImageProcessor`**: **Generate** fills the **LUT table**; **Analyze** copies **Source** → output.
- **`core/LSPLutGeneratorPattern.cpp`** — Strip sampling math for the **LUT table** grid.
- **`core/LSPLutGeneratorCube.cpp`** — Binning, **`.cube`** writer, downsample / trilinear, **unique numbered** output path for export.
- **`macos/LSPLutGeneratorDialogs.mm`** — **NSOpenPanel** folder chooser (main thread) for **Export path**; no save panel.
- **`core/LSPLutGeneratorConstants.h`** — **`kPluginIdentifier`**, **`kPluginName`**, grouping **`LSP/Color`**, via **`version_gen.h`**.

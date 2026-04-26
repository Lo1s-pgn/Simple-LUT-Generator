# Plugin sources — LSP - Simple LUT Generator

- **`core/LSPLutGeneratorPlugin.cpp`** — **`ImageEffect`** instance: **`render`**, **`changedParam`**, factory registration.
- **`core/LSPLutGeneratorDescribe.cpp`** — Clips and parameters (operation mode, LUT size, export button).
- **`core/LSPLutGeneratorProcessor.cpp`** — **`ImageProcessor`**: Generate LUT Table mode fills balanced-grid **LUT table**; Analyze mode copies source → output (**`getPixelAddress`** per pixel).
- **`core/LSPLutGeneratorPattern.cpp`** — Strip sampling math for the **LUT table** grid layout.
- **`core/LSPLutGeneratorCube.cpp`** — Binning + **`.cube`** writer; shrink with box average when **`n_src % n_dst == 0`**, else trilinear.
- **`macos/LSPLutGeneratorDialogs.mm`** — Save **`.cube`** via **NSSavePanel** on main thread.
- **`core/LSPLutGeneratorConstants.h`** — **`kPluginIdentifier`** via **`version_gen.h`**.

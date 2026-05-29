# LSP - Simple LUT Generator (OFX)

**Simple LUT Generator** is an OFX plug-in for DaVinci Resolve. Use one instance to fill the frame with a 3D LUT table and another after your grade to analyze the graded image and export a matching **`.cube`** LUT.

## What it does

- **Generate LUT Table** — Draws the largest feasible n×n×n LUT table for the current resolution.
- **Analyze & export LUT** — Uses **Source** (graded table), solves at max size, then optionally box-downsamples to **Export LUT size** when that size divides the max.
- **Export LUT** — Writes a **`.cube`** to a chosen folder; base **LUT name** with automatic `_001`, `_002`, … suffixes if files already exist.

![Example node tree setup for LSP - Simple LUT Generator](./doc/node-tree.png)

## Platform

- **macOS** 11.0+ (universal arm64 + x86_64 by default)
- **Windows** 64-bit (DaVinci Resolve)

Build on each platform separately. Each build writes a **versioned, platform-tagged release folder** containing the `.ofx.bundle`:

| Platform | Release folder (example for v1.0.7) | Inside the bundle |
|----------|-------------------------------------|-------------------|
| macOS | `release/LSP_Simple_LUT_Generator_1.0.7_macos/` | `Contents/MacOS/<name>.ofx` |
| Windows | `release/LSP_Simple_LUT_Generator_1.0.7_windows/` | `Contents/Win64/<name>.ofx` |

Pattern: **`LSP_Simple_LUT_Generator_<version>_<platform>/`**

The bundle inside is always **`LSP_Simple_LUT_Generator_<version>.ofx.bundle`**.

For GitHub Releases, zip each platform folder separately (or ship both in one archive).

## Build

**CMake** is the only build system.

### Prerequisites

- **CMake** 3.22+
- **macOS:** Xcode Command Line Tools
- **Windows:** Visual Studio 2022 Build Tools (MSVC), **Ninja** (`winget install Ninja-build.Ninja`)

See [tools/README.md](tools/README.md) for optional build helper scripts.

### macOS

```bash
./tools/macos/lutgen_build.sh
```

Or manually:

```bash
cmake -S . -B build/macos -DCMAKE_BUILD_TYPE=Release
cmake --build build/macos --target lutgen_all
```

Options at configure time:

- `-DLUTGEN_OFX_FAT_ARCHS=OFF` — single-arch build (host CPU only)
- `-DOFX_SDK_PATH=...` — alternate OpenFX SDK checkout

### Windows

```powershell
tools\windows\lutgen_build.bat
```

Or manually (from a **Developer Command Prompt** or after `vcvars64.bat`):

```powershell
cmake -S . -B build/windows -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl
cmake --build build/windows --target lutgen_all
```

## Release builds (GitHub Actions)

To build **macOS and Windows** in the cloud **without publishing automatically**:

1. Commit and **push** the revision you want built (the workflow uses the branch you select on GitHub, not uncommitted local files).
2. Open the repo on [github.com](https://github.com) → **Actions** → **Build OFX release**.
3. Click **Run workflow**, choose the branch (usually `main`), optionally toggle **macOS universal binary**, then **Run workflow**.
4. When the jobs finish, open **that workflow run** (click the run title, e.g. “Build OFX release #2”) and scroll to the bottom → **Artifacts** → download:
   - `LSP_Simple_LUT_Generator_<version>_macos`
   - `LSP_Simple_LUT_Generator_<version>_windows`

   Artifacts are attached to each run, not to the repo home page or Releases tab. If one job failed, only the successful job’s artifact appears (e.g. Windows may be available even when macOS failed).

   From [run #1](https://github.com/Lo1s-pgn/Simple-LUT-Generator/actions/runs/26659697244): the Windows artifact **`LSP_Simple_LUT_Generator_1.0.8_windows`** was uploaded; macOS failed before upload.

Each artifact is the full versioned release folder (contains the `.ofx.bundle`).

This workflow runs **only when you click Run workflow**. It does **not** run on push, tags, or local archive.

Local archive / version bumps that stay on your Mac do not trigger any GitHub build.

## Installation

Copy the bundle from **your platform’s output folder** into the host OFX plug-ins directory, then restart DaVinci Resolve.

If the plug-in does not appear after an upgrade, quit Resolve and delete its OFX plug-in cache file (see paths below), then relaunch.

### macOS

Use the bundle inside **`release/LSP_Simple_LUT_Generator_<version>_macos/`**. Copy it to:

- `/Library/OFX/Plugins/` (all users), or
- `~/Library/OFX/Plugins/` (current user)

**Resolve OFX cache (delete if needed):**  
`~/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml`

### Windows

Use the bundle inside **`release/LSP_Simple_LUT_Generator_<version>_windows/`**. Copy it to:

`C:\Program Files\Common Files\OFX\Plugins\`

(Elevation required when writing under `Program Files`.)

**Resolve OFX cache (delete if needed):**  
`%APPDATA%\Blackmagic Design\DaVinci Resolve\Support\OFXPluginCacheV2.xml`

## Log file

| Platform | Path |
|----------|------|
| macOS | `~/Library/Application Support/LSP/LutGenerator.log` |
| Windows | `%APPDATA%\LSP\LutGenerator.log` |

Use **Open Log** in the plug-in SUPPORT section.

## macOS Gatekeeper (unsigned local builds)

For ad-hoc signing after install:

```bash
sudo xattr -dr com.apple.quarantine /Library/OFX/Plugins/LSP_Simple_LUT_Generator_<version>.ofx.bundle
sudo codesign --force --deep --sign - /Library/OFX/Plugins/LSP_Simple_LUT_Generator_<version>.ofx.bundle
```


## Repository layout

```
CMakeLists.txt          # Root build (macOS + Windows)
cmake/                  # LutGenVersion, LutGenApple, LutGenWindows, LutGenCommon
plugin/core/            # Portable OFX logic (CPU render + .cube export)
plugin/macos/           # AppKit folder dialog
plugin/windows/         # IFileOpenDialog folder dialog
openfx-sdk/             # Vendored minimal OpenFX 1.5.1 + Support layer
tools/                  # Optional build helpers (see tools/README.md)
```

Build intermediates: `build/macos/` or `build/windows/`. Shippable output: `release/LSP_Simple_LUT_Generator_<version>_macos/` or `_windows/`.

## SDK

Vendored minimal OpenFX SDK in `openfx-sdk/`. Override with `-DOFX_SDK_PATH=...` if needed.

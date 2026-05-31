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

| Platform | Release folder | Inside the bundle |
|----------|-------------------------------------|-------------------|
| macOS | `release/LSP_Simple_LUT_Generator_<version>_macos/` | `Contents/MacOS/<name>.ofx` |
| Windows | `release/LSP_Simple_LUT_Generator_<version>_windows/` | `Contents/Win64/<name>.ofx` |

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

## GitHub Actions

This repo includes build workflows on GitHub Actions. **Build OFX release** builds macOS and Windows in one run (two jobs, two artifacts).

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

## macOS Gatekeeper (unsigned builds)

Release builds are **not signed or notarized**. After you copy the bundle into an OFX folder, macOS may block it from loading in Resolve.

### Method 1 — Terminal (recommended)

Use the path where you actually installed the bundle. Example for the system folder:

```bash
BUNDLE="/Library/OFX/Plugins/LSP_Simple_LUT_Generator_<version>.ofx.bundle"

sudo chmod -R 755 "$BUNDLE"
sudo chown -R root:wheel "$BUNDLE"
sudo xattr -dr com.apple.quarantine "$BUNDLE"
sudo codesign --force --deep --sign - "$BUNDLE"
```

For a **user-only** install (`~/Library/OFX/Plugins/...`), use that path in `BUNDLE` and **skip** the `chown root:wheel` line.

When `sudo` asks for your password, type it and press **Enter** (nothing appears on screen — that is normal).

Quit Resolve completely, then reopen it.

### Method 2 — System Settings (no Terminal)

1. Copy a fresh bundle into `/Library/OFX/Plugins/` (or `~/Library/OFX/Plugins/`).
2. Launch Resolve. If macOS shows a security warning, click **Done**.
3. Open **System Settings → Privacy & Security**, scroll down, and click **Allow Anyway** next to the blocked plug-in.
4. In Resolve: **DaVinci Resolve → Preferences → Video Plugins**, find **LSP - Simple LUT Generator**, enable it, save, and quit Resolve.
5. Launch Resolve again. When prompted, click **Open Anyway** and enter your Mac password.

### Notes

- If your Mac account has **no login password**, the **Open Anyway** step may not work reliably — use Method 1 instead.
- If still missing in Resolve, elete the OFX cache (path in [Installation](#installation) above) and relaunch.

## SDK

Vendored minimal OpenFX SDK in `openfx-sdk/`. Override with `-DOFX_SDK_PATH=...` if needed.

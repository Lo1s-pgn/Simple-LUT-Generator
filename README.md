# LSP - Simple LUT Generator (OFX)

**Simple LUT Generator** is an OFX plug-in for DaVinci Resolve. Use one instance to fill the frame with a 3D LUT table and another after your grade to analyze the graded image and export a matching `**.cube`** LUT.

## What it does

- **Generate LUT Table** — Draws the largest feasible n×n×n LUT table for the current resolution.
- **Analyze & export LUT** — Uses **Source** (graded table), solves at max size, then optionally box-downsamples to **Export LUT size** when that size divides the max.
- **Export LUT** — Writes a `**.cube`** to a chosen folder; base **LUT name** with automatic `**_001`**, `**_002**`, … suffixes if files already exist.

Example node tree setup for LSP - Simple LUT Generator

## Platform

- **macOS** only for now.

## Build

From the repository root:

```bash
make
```

This writes `**plugin/version_gen.h**`, `**build/**` intermediates, and `**release/LSP_Simple_LUT_Generator_<version>.ofx.bundle**`. The Makefile also copies `**install_lsp_lut_generator_ofx.command**` and `**purge_resolve_ofx_cache.command**` into `**release/**` next to the bundle (for zipping a GitHub Release).

## Installation

### Option A — Install from your own build (macOS)

```bash
sudo make install
```

This copies the bundle to `**/Library/OFX/Plugins**`, then runs `**make purge**` so Resolve rescans plug-ins (removes `**OFXPluginCacheV2.xml**` and legacy `**OFXPluginCache.xml**` for the relevant user home — under `**sudo**`, `**SUDO_USER**`’s Library is used). To install **without** clearing the cache:

```bash
sudo make install SKIP_RESOLVE_OFX_CACHE_PURGE=1
```

### Option B — macOS installer (release ZIP)

Download the build from the latest **GitHub Release** and unzip the archive. You should see the `**.ofx.bundle`** and `**install_lsp_lut_generator_ofx.command**` side by side (that `**make**` layout). **Double-click** `**install_lsp_lut_generator_ofx.command`**. Terminal opens, installs to `**~/Library/OFX/Plugins/**`, clears **quarantine** on the installed bundle (use only for bundles you trust), and purges Resolve’s OFX cache the same way as `**make install`**. **Quit DaVinci Resolve** before running the script, then restart the app. If macOS blocks the script the first time, **Control-click** (or right-click) the `**.command`** file → **Open** → confirm.

### Option C — Manual copy

1. Download the build from the latest release and copy into:
  - `**/Library/OFX/Plugins/`** (all users), or  
  - `**~/Library/OFX/Plugins/**` (current user only)
2. Restart DaVinci Resolve (and purge the OFX cache if the plug-in does not appear — see above).

**Finder shortcut to the system plug-ins folder:** **Go** → **Go to Folder…** (**⇧⌘G**), then enter:

```text
/Library/OFX/Plugins/
```

## Remove macOS Gatekeeper warning

Unsigned bundles will be blocked by Gatekeeper :

### Method 1 — Terminal (recommended)

Adjust the bundle name to match your installed version, then run:

```bash
sudo chmod -R 755 /Library/OFX/Plugins/LSP_Simple_LUT_Generator_1.0.5.ofx.bundle
sudo chown -R root:wheel /Library/OFX/Plugins/LSP_Simple_LUT_Generator_1.0.5.ofx.bundle
sudo xattr -dr com.apple.quarantine /Library/OFX/Plugins/LSP_Simple_LUT_Generator_1.0.5.ofx.bundle
sudo codesign --force --deep --sign - /Library/OFX/Plugins/LSP_Simple_LUT_Generator_1.0.5.ofx.bundle
```

Then relaunch DaVinci Resolve.

### Method 2 — System Settings

1. Install the bundle to `**/Library/OFX/Plugins/**`.
2. Open Resolve; if macOS warns, dismiss with **Done**.
3. Open **System Settings** → **Privacy & Security** and use **Allow Anyway** for the blocked plug-in if shown.
4. In Resolve: **Preferences** → **Video Plugins** — enable the plug-in, save, quit, and relaunch; confirm **Open Anyway** if prompted.

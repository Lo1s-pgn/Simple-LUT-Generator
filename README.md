# LSP LUT Generator (OFX)

**LSP LUT Generator** is an **OFX** plug-in for **DaVinci Resolve**. Use one instance to fill the frame with a **3D LUT table** and another after your grade to analyze the graded image and export a matching `**.cube`** LUT.

## What it does

- **Generate LUT Table** — Draws the largest feasible n×n×n LUT table for the current resolution.
- **Analyze & export LUT** — Uses **Source** (graded table), solves at max size, then optionally box-downsamples to **Export LUT size** when that size divides the max.
- **Export LUT** — Writes a `**.cube`** file.
- **Rendering:** **CPU** by default; when the host enables **OFX Metal** rendering, **Generate** and **Analyze** (passthrough) run on the **GPU** via Metal compute. **Export LUT** (`.cube` from Analyze) uses **GPU binning, box-downsample, and trilinear resample** on macOS when Metal succeeds; otherwise it falls back to the CPU path. With Metal enabled, clip pixels are **`id<MTLBuffer>`**; the plug-in binds that buffer for export (it does not treat it as a raw CPU pointer).

Example node tree setup for LSP LUT Generator

**In Resolve:** **Effects** → **OpenFX** → **LSP - Color** → **LSP LUT Generator**.

## Platform

- **macOS** only for now.

## Build

From the repository root:

```bash
make
```

This writes `**plugin/version_gen.h**`, `**build/**` intermediates, and `**release/LSP_LutGenerator_<version>.ofx.bundle**`. The `**release/**` folder is **not** part of the GitHub repository (it is gitignored); it is only your local build output. When you publish a **GitHub Release**, zip the contents of `**release/`** (bundle + installer) and upload that archive manually.

The Makefile also copies `**tools/install_lsp_lut_generator_ofx.command**` into `**release/**` so the zip you upload includes a double-click installer next to the bundle.

## Installation

### Option A — Install from your own build (macOS)

```bash
sudo make install
```

This copies the bundle to `**/Library/OFX/Plugins**`, then runs `**make purge**` so Resolve rescans plug-ins (removes `**OFXPluginCacheV2.xml**` and legacy `**OFXPluginCache.xml**` for the relevant user home — under `**sudo**`, `**SUDO_USER**`’s Library is used). To install **without** clearing the cache:

```bash
sudo make install SKIP_RESOLVE_OFX_CACHE_PURGE=1
```

**Tip:** Quit DaVinci Resolve before installing when possible, then restart the app.

**Purge cache only** (same paths as install): `**make purge`**.

### Option B — macOS one-click installer (release ZIP)

Download the build from the latest release and unzip the archive. Then **double-click** `**install_lsp_lut_generator_ofx.command`**. Terminal opens, installs to `**~/Library/OFX/Plugins/**`, clears **quarantine** on the installed copy (trusted builds only), and purges Resolve’s OFX cache like `**make install`**. Quit Resolve before running, then restart it. If macOS blocks the script the first time, **Control-click** (or right-click) the `**.command`** file → **Open** → confirm.

### Option C — Manual copy

1. Download the build from the latest release and copy into:
  - `**/Library/OFX/Plugins/`** (all users), or  
  - `**~/Library/OFX/Plugins/**` (current user only)
2. Restart DaVinci Resolve (and purge the OFX cache if the plug-in does not appear — `**make purge**` from a source tree, or use the installer command above).

**Finder shortcut to the system plug-ins folder:** **Go** → **Go to Folder…** (**⇧⌘G**), then enter:

```text
/Library/OFX/Plugins/
```

## macOS Gatekeeper (unsigned / not notarized builds)

Prebuilt bundles may show Gatekeeper or “downloaded app” warnings. The **installer command** (Option B) clears `**com.apple.quarantine`** on the copy it installs; use that only for bundles you trust.

### Manual Terminal commands

If you installed by hand, adjust the path and bundle name to match your version:

```bash
xattr -dr com.apple.quarantine ~/Library/OFX/Plugins/LSP_LutGenerator_1.0.2.ofx.bundle
```

For a system install under `**/Library/OFX/Plugins/**`, you may use `**sudo**` with the same `**xattr**` command, and optionally match typical ownership:

```bash
sudo chown -R root:wheel /Library/OFX/Plugins/LSP_LutGenerator_1.0.2.ofx.bundle
sudo chmod -R 755 /Library/OFX/Plugins/LSP_LutGenerator_1.0.2.ofx.bundle
```

Some hosts accept an **ad hoc** signature (developer-notarized distribution is still preferable):

```bash
codesign --force --deep --sign - ~/Library/OFX/Plugins/LSP_LutGenerator_1.0.2.ofx.bundle
```

Then relaunch DaVinci Resolve.

### System Settings

1. Install the bundle to `**/Library/OFX/Plugins/**` (or the user folder).
2. Open Resolve; if macOS warns, dismiss with **Done**.
3. Open **System Settings** → **Privacy & Security** and use **Allow Anyway** for the blocked plug-in if shown.
4. In Resolve: **Preferences** → **Video Plugins** — enable the plug-in, save, quit, and relaunch; confirm **Open Anyway** if prompted.


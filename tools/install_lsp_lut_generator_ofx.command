#!/usr/bin/env bash
# Install LSP - Simple LUT Generator OFX bundle from this directory into macOS OFX plug-in folders,
# clear quarantine on the installed copy, and optionally purge DaVinci Resolve OFX caches.
#
# Double-click this file in Finder (same folder as the .ofx.bundle), or run from Terminal:
#   ./install_lsp_lut_generator_ofx.command
#
# User install (default): ~/Library/OFX/Plugins
# System install:         SYSTEM_INSTALL=1 sudo -E ./install_lsp_lut_generator_ofx.command
#
# Optional environment:
#   OFX_BUNDLE=name.ofx.bundle     — force bundle name (default: single *.ofx.bundle beside this script)
#   SYSTEM_INSTALL=1               — install to /Library/OFX/Plugins (requires sudo)
#   SKIP_RESOLVE_OFX_CACHE_PURGE=1 — do not delete Resolve OFX cache XML files
#   INSTALL_NO_PAUSE=1             — skip "Press Enter" at end (e.g. when piping from Terminal)
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

trap 'if [[ "${INSTALL_NO_PAUSE:-}" != 1 ]] && [[ -t 0 ]]; then printf "\nPress Enter to close this window. "; read -r _ || true; fi' EXIT

resolve_home_for_cache() {
  if [[ "${EUID:-$(id -u)}" -eq 0 ]] && [[ -n "${SUDO_USER:-}" ]]; then
    local h
    h="$(dscl . -read "/Users/${SUDO_USER}" NFSHomeDirectory 2>/dev/null | sed 's/^[^:]*: *//')"
    if [[ -n "${h}" ]]; then
      echo "${h}"
      return
    fi
  fi
  echo "${HOME}"
}

purge_resolve_ofx_cache() {
  local base
  base="$(resolve_home_for_cache)"
  local v2="${base}/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml"
  local v1="${base}/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCache.xml"
  rm -f "${v2}" "${v1}" 2>/dev/null || true
  echo "Purged Resolve OFX cache (if present) for: ${base}"
  echo "  ${v2}"
  echo "  ${v1}"
}

pick_bundle() {
  if [[ -n "${OFX_BUNDLE:-}" ]]; then
    if [[ ! -d "${OFX_BUNDLE}" ]]; then
      echo "OFX_BUNDLE set but not found: ${OFX_BUNDLE}" >&2
      exit 1
    fi
    echo "${OFX_BUNDLE}"
    return
  fi
  local matches
  matches="$(find . -maxdepth 1 -name '*.ofx.bundle' -type d 2>/dev/null | sed 's|^\./||' || true)"
  local count
  count="$(echo "${matches}" | sed '/^$/d' | wc -l | tr -d ' ')"
  if [[ "${count}" -eq 0 ]]; then
    echo "No *.ofx.bundle in ${SCRIPT_DIR}. Put this file next to the bundle from the release ZIP." >&2
    exit 1
  fi
  if [[ "${count}" -ne 1 ]]; then
    echo "Multiple *.ofx.bundle in ${SCRIPT_DIR}; set OFX_BUNDLE to one of:" >&2
    echo "${matches}" >&2
    exit 1
  fi
  echo "${matches}"
}

BUNDLE_NAME="$(pick_bundle)"
SOURCE_PATH="${SCRIPT_DIR}/${BUNDLE_NAME}"

if [[ "${SYSTEM_INSTALL:-}" == 1 ]]; then
  DEST_ROOT="/Library/OFX/Plugins"
else
  DEST_ROOT="${HOME}/Library/OFX/Plugins"
fi

DEST_PATH="${DEST_ROOT}/${BUNDLE_NAME}"

run() {
  if [[ "${SYSTEM_INSTALL:-}" == 1 ]]; then
    sudo "$@"
  else
    "$@"
  fi
}

echo "Source:  ${SOURCE_PATH}"
echo "Install: ${DEST_PATH}"

run mkdir -p "${DEST_ROOT}"
run rm -rf "${DEST_PATH}"
run rsync -a "${SOURCE_PATH}/" "${DEST_PATH}/"

if [[ "${SYSTEM_INSTALL:-}" == 1 ]]; then
  run chown -R root:wheel "${DEST_PATH}"
  run chmod -R 755 "${DEST_PATH}"
else
  chmod -R u+rwX,go+rX "${DEST_PATH}" 2>/dev/null || true
fi

run xattr -dr com.apple.quarantine "${DEST_PATH}" 2>/dev/null || true

if [[ "${SKIP_RESOLVE_OFX_CACHE_PURGE:-}" != 1 ]]; then
  purge_resolve_ofx_cache
  echo "Quit DaVinci Resolve if it is open, then restart."
else
  echo "Skipped Resolve OFX cache purge (SKIP_RESOLVE_OFX_CACHE_PURGE=1)."
fi

echo "Done."

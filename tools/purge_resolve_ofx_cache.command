#!/usr/bin/env bash
# Delete DaVinci Resolve OFX catalog caches so the host rescans ~/Library/OFX/Plugins and /Library/OFX/Plugins.
# Double-click in Finder or: ./purge_resolve_ofx_cache.command
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

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

BASE="$(resolve_home_for_cache)"
V2="${BASE}/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCacheV2.xml"
V1="${BASE}/Library/Application Support/Blackmagic Design/DaVinci Resolve/OFXPluginCache.xml"

echo "Home for cache: ${BASE}"
rm -f "${V2}" "${V1}" 2>/dev/null || true
echo "Removed (if present):"
echo "  ${V2}"
echo "  ${V1}"
echo "Quit DaVinci Resolve, then start it again so OFX is rescanned."
if [[ -t 0 ]]; then
  read -r -p "Press Enter to close. " _ || true
fi

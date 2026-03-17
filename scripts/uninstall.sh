#!/usr/bin/env bash
set -euo pipefail

PREFIX="${HOME}/.local"
SHARE_DIR="${PREFIX}/share/copyclickk"
MANIFEST="${SHARE_DIR}/install-manifest.txt"
DESKTOP_FILE="${PREFIX}/share/applications/copyclickk.desktop"
AUTOSTART_FILE="${HOME}/.config/autostart/copyclickk.desktop"
DATA_DIR="${HOME}/.local/share/copyclickk"
CONFIG_DIR="${HOME}/.config/copyclickk"

removed_any=0

if [[ -f "${MANIFEST}" ]]; then
  echo "[copyclickk] Removing installed files from manifest..."
  while IFS= read -r path; do
    [[ -z "${path}" ]] && continue
    if [[ -e "${path}" || -L "${path}" ]]; then
      rm -rf "${path}"
      removed_any=1
    fi
  done < "${MANIFEST}"
fi

for extra in "${DESKTOP_FILE}" "${AUTOSTART_FILE}"; do
  if [[ -e "${extra}" || -L "${extra}" ]]; then
    rm -rf "${extra}"
    removed_any=1
  fi
done

# Remove empty parent dirs created by install.
rmdir --ignore-fail-on-non-empty "${PREFIX}/share/applications" 2>/dev/null || true
rmdir --ignore-fail-on-non-empty "${HOME}/.config/autostart" 2>/dev/null || true
rmdir --ignore-fail-on-non-empty "${SHARE_DIR}" 2>/dev/null || true

if [[ ${removed_any} -eq 1 ]]; then
  echo "[copyclickk] App files removed."
else
  echo "[copyclickk] No installed files found."
fi

echo "Do you want to remove user data and settings too?"
echo "This deletes:"
echo "- ${DATA_DIR}"
echo "- ${CONFIG_DIR}"
echo "[y/N]"
read -r DELETE_DATA
if [[ "${DELETE_DATA}" =~ ^[Yy]$ ]]; then
  rm -rf "${DATA_DIR}" "${CONFIG_DIR}"
  echo "[copyclickk] User data removed."
else
  echo "[copyclickk] User data kept."
fi

echo "[copyclickk] Uninstall complete."

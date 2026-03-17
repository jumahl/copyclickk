#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
PREFIX="${HOME}/.local"
BIN_DIR="${PREFIX}/bin"
SHARE_DIR="${PREFIX}/share/copyclickk"
APP_DIR="${PREFIX}/share/applications"
AUTOSTART_DIR="${HOME}/.config/autostart"
MANIFEST="${SHARE_DIR}/install-manifest.txt"
DESKTOP_FILE="${APP_DIR}/copyclickk.desktop"
AUTOSTART_FILE="${AUTOSTART_DIR}/copyclickk.desktop"

SKIP_DEPS=0
for arg in "$@"; do
  if [[ "${arg}" == "--skip-deps" ]]; then
    SKIP_DEPS=1
  fi
done

install_deps_fedora() {
  sudo dnf install -y cmake gcc-c++ ninja-build sqlite-devel qt6-qtbase-devel kf6-kglobalaccel-devel
}

install_deps_arch() {
  sudo pacman -S --needed --noconfirm cmake gcc ninja sqlite qt6-base kglobalaccel
}

install_deps_debian_like() {
  sudo apt update
  sudo apt install -y cmake g++ ninja-build libsqlite3-dev qt6-base-dev libkf6globalaccel-dev || {
    echo "[copyclickk] Could not install one or more KDE6 packages automatically."
    echo "[copyclickk] Install at least: cmake g++ ninja-build libsqlite3-dev qt6-base-dev"
  }
}

install_deps_opensuse() {
  sudo zypper install -y cmake gcc-c++ ninja sqlite3-devel qt6-base-devel kf6-kglobalaccel-devel || {
    echo "[copyclickk] Could not install one or more KDE6 packages automatically."
    echo "[copyclickk] Search and install KF6 GlobalAccel dev package for your release."
  }
}

maybe_install_dependencies() {
  if [[ ${SKIP_DEPS} -eq 1 ]]; then
    echo "[copyclickk] Skipping dependency installation (--skip-deps)."
    return
  fi

  if [[ ! -f /etc/os-release ]]; then
    echo "[copyclickk] /etc/os-release not found. Skipping auto dependency install."
    return
  fi

  . /etc/os-release
  echo "Install/update build dependencies for ${ID:-unknown}? [Y/n]"
  read -r INSTALL_DEPS
  if [[ "${INSTALL_DEPS:-}" =~ ^[Nn]$ ]]; then
    return
  fi

  case "${ID:-}" in
    fedora)
      install_deps_fedora
      ;;
    arch)
      install_deps_arch
      ;;
    ubuntu|debian)
      install_deps_debian_like
      ;;
    opensuse*|suse|opensuse-tumbleweed|opensuse-leap)
      install_deps_opensuse
      ;;
    *)
      echo "[copyclickk] Unsupported distro ID for auto deps: ${ID:-unknown}"
      echo "[copyclickk] Please install manually: cmake, C++ compiler, sqlite dev, Qt6 dev, optional KF6 GlobalAccel dev."
      ;;
  esac
}

maybe_install_dependencies

mkdir -p "${BIN_DIR}" "${SHARE_DIR}" "${APP_DIR}"

if ! command -v cmake >/dev/null 2>&1; then
  echo "[copyclickk] ERROR: cmake is required."
  echo "Install dependencies manually and re-run install.sh."
  exit 1
fi

GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GENERATOR_ARGS=("-G" "Ninja")
fi

echo "[copyclickk] Configuring and building..."
cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" "${GENERATOR_ARGS[@]}" -DCOPYCLICKK_BUILD_QT_TRAY_APP=ON
cmake --build "${BUILD_DIR}"

if [[ ! -x "${BUILD_DIR}/src/uiqt/copyclickk_tray" ]]; then
  echo "[copyclickk] ERROR: binary not found at ${BUILD_DIR}/src/uiqt/copyclickk_tray"
  exit 1
fi

cp "${BUILD_DIR}/src/uiqt/copyclickk_tray" "${BIN_DIR}/copyclickk_tray"
rm -rf "${SHARE_DIR}/assets"
cp -r "${PROJECT_ROOT}/assets" "${SHARE_DIR}/assets"

ICON_PATH="${SHARE_DIR}/assets/icons/app/app.png"
if [[ ! -f "${ICON_PATH}" ]]; then
  ICON_PATH="${SHARE_DIR}/assets/icons/tray/clipboard.svg"
fi

cat > "${DESKTOP_FILE}" <<EOF
[Desktop Entry]
Type=Application
Name=CopyClickk
Comment=Clipboard manager for KDE Plasma
Exec=${BIN_DIR}/copyclickk_tray
Icon=${ICON_PATH}
Terminal=false
Categories=Utility;
StartupNotify=false
EOF

echo "Create autostart entry? [y/N]"
read -r CREATE_AUTOSTART
if [[ "${CREATE_AUTOSTART}" =~ ^[Yy]$ ]]; then
  mkdir -p "${AUTOSTART_DIR}"
  cp "${DESKTOP_FILE}" "${AUTOSTART_FILE}"
  echo "[copyclickk] Autostart enabled."
else
  rm -f "${AUTOSTART_FILE}"
fi

{
  echo "${BIN_DIR}/copyclickk_tray"
  echo "${DESKTOP_FILE}"
  echo "${AUTOSTART_FILE}"
  find "${SHARE_DIR}/assets" -type f | sort
  echo "${MANIFEST}"
} > "${MANIFEST}"

echo "[copyclickk] Installed successfully."
echo "Run: ${BIN_DIR}/copyclickk_tray"

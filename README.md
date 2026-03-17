# CopyClickk

Clipboard manager for KDE Plasma with tray integration, image previews, persistent history, and configurable shortcuts.

![CopyClickk Logo](assets/icons/app/app.png)

![Platform](https://img.shields.io/badge/Platform-Linux-blue)
![Desktop](https://img.shields.io/badge/Desktop-KDE%20Plasma-1f6feb)
![Toolkit](https://img.shields.io/badge/Qt-6-41cd52)
![Language](https://img.shields.io/badge/C%2B%2B-20-00599C)
![Storage](https://img.shields.io/badge/Storage-SQLite-003B57)

## Why CopyClickk

CopyClickk is a KDE-focused clipboard manager designed to be practical, fast, and easy to run in daily workflows.

## Features

- Tray app with menu actions in English: `Show History`, `Clear Clipboard`, `Settings`, `Quit`
- Clipboard capture for text, HTML, URLs, and images
- Image support: PNG, JPG/JPEG, WEBP, BMP, ICO
- Thumbnail preview in history list
- Persistent history in SQLite (`~/.local/share/copyclickk/history.db`)
- Configurable history limit (default `30`) with auto-eviction of oldest entries
- Configurable open-history shortcut
- Settings persisted in `~/.config/copyclickk/settings.ini`

## Screens and UX

- Tray icon with quick menu actions
- Click tray icon to open clipboard history
- Image entries shown with thumbnail preview
- Long text entries truncated for cleaner list visualization
- Settings dialog for history limit, preview size, and shortcut

## Quick Install (User Scope)

The project includes scripts for easy install/uninstall under your user account (`~/.local`).

Supported desktop environment target: KDE Plasma.
Supported distro families: Fedora, Arch, Debian/Ubuntu, openSUSE (best effort).

1. Make scripts executable:

```bash
chmod +x scripts/install.sh scripts/uninstall.sh
```

2. Install app (dependencies included in the same script):

```bash
./scripts/install.sh
```

Optional: skip dependency installation prompt:

```bash
./scripts/install.sh --skip-deps
```

3. Run:

```bash
~/.local/bin/copyclickk_tray
```

## One-Command Flow

```bash
chmod +x scripts/install.sh scripts/uninstall.sh
./scripts/install.sh
```

## Uninstall

```bash
./scripts/uninstall.sh
```

The uninstall script:

- removes installed app files (`binary`, `desktop entry`, copied assets)
- asks whether to delete user data and settings
- optionally deletes:
- `~/.local/share/copyclickk`
- `~/.config/copyclickk`

## Compatibility

- Desktop target: KDE Plasma
- Distros: Fedora, Arch, Debian/Ubuntu, openSUSE (best effort)
- Wayland and X11: supported with behavior depending on compositor/session restrictions

## Manual Build (Dev)

Fedora dependencies:

```bash
sudo dnf install -y cmake gcc-c++ ninja-build sqlite-devel qt6-qtbase-devel kf6-kglobalaccel-devel
```

Arch Linux dependencies:

```bash
sudo pacman -S --needed cmake gcc ninja sqlite qt6-base kglobalaccel
```

Debian/Ubuntu dependencies (KDE6 packages depend on release):

```bash
sudo apt update
sudo apt install -y cmake g++ ninja-build libsqlite3-dev qt6-base-dev
```

openSUSE dependencies:

```bash
sudo zypper install -y cmake gcc-c++ ninja sqlite3-devel qt6-base-devel
```

Build and test:

```bash
cmake -S . -B build -G Ninja -DCOPYCLICKK_BUILD_QT_TRAY_APP=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Run local binary:

```bash
./build/src/uiqt/copyclickk_tray
```

## Contributing

Contributions are welcome.

1. Read `CONTRIBUTING.md`
2. Create a branch
3. Add tests for behavior changes
4. Open a pull request with clear notes

## Project Structure

- `src/core`: domain models
- `src/storage`: repository implementations (memory + sqlite)
- `src/privacy`: privacy filtering
- `src/ipc`: application service orchestration
- `src/ui`: UI view models
- `src/uiqt`: Qt tray application
- `assets/icons/app`: app icon files (you can customize)
- `assets/icons/tray/clipboard.svg`: tray icon
- `scripts/install.sh`: user-scope installer
- `scripts/uninstall.sh`: interactive uninstaller
- `tests`: test suite

## Roadmap

1. DBus adapter on top of `ClipboardService`
2. Richer KDE integration for packaging and startup behavior
3. QML-based history UI variant
4. More privacy presets and exclusion rules

## Notes

- If `KF6GlobalAccel` is available, shortcut uses KDE global shortcut backend.
- Without it, the app falls back to application-level shortcut handling.

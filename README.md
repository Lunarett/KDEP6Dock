# KDEP6Dock

KDEP6Dock is a small standalone macOS-like dock built with **C++17 + Qt 6 Widgets** for **KDE Plasma 6**.

It intentionally targets an MVP scope: clean behavior, straightforward architecture, and practical KDE usage without trying to replicate Latte Dock complexity.

## What it supports

- Frameless shell-style dock window (hidden from normal app switchers/taskbar as far as platform policy allows) suitable for bottom-screen placement.
- Smooth icon magnification with continuous cursor-position tracking and neighbor influence.
- Continuous per-item animation using a single `progress` value (`0.0..1.0`) that always moves from current state toward a target and reverses smoothly when direction changes.
- Launch pinned apps by clicking icons.
- Reorder pinned apps with internal drag-and-drop.
- Add apps by dropping `.desktop` files onto the dock.
- Remove apps with right-click -> **Remove from Dock**.
- JSON config persistence for settings and pinned apps.

## What it intentionally does not support (yet)

- Task manager behavior (running window tracking, badges, grouped instances).
- Window previews, bouncing, or advanced effects.
- Auto-hide and panel-replacement features.
- Plasma applet/panel integration (this is a standalone app).

---

## Requirements

Target runtime:

- KDE Plasma 6 desktop on Linux (Wayland or X11).

Build/runtime dependencies:

- CMake >= 3.16
- C++ compiler with C++17 support (GCC/Clang)
- Qt 6 development packages (`Core`, `Gui`, `Widgets`)

Example package hints (distribution-specific names vary):

- Ubuntu/Debian-like: `cmake`, `g++`, `qt6-base-dev`
- Fedora-like: `cmake`, `gcc-c++`, `qt6-qtbase-devel`
- Arch-like: `cmake`, `gcc`, `qt6-base`

---

## Build instructions

From the project root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Convenience build script (captures failures into `crash/`):

```bash
./scripts/build.sh
```

Run:

```bash
./build/kdep6dock
```

Optional install:

```bash
cmake --install build --prefix ~/.local
~/.local/bin/kdep6dock
```

---

## Configuration

Default runtime config path:

```text
~/.config/kdep6dock/config.json
```

The dock loads this file on startup and saves updates when pins/settings change.

If the file does not exist, defaults are used; a file is written after state changes (e.g., add/remove/reorder).

You can start with `config/sample-config.json` and copy it:

```bash
mkdir -p ~/.config/kdep6dock
cp config/sample-config.json ~/.config/kdep6dock/config.json
```

### Key config fields

- `baseIconSize`: base icon size in px.
- `maxScale`: maximum hovered scale factor.
- `spacing`: pixel spacing between slots.
- `animationDurationMs`: approximate time for progress to move between 0 and 1.
- `neighborRadius`: number of neighboring items receiving falloff influence.
- `dockPadding`: inner dock padding.
- `dockMarginBottom`: distance from bottom edge.
- `backgroundOpacity`: dock background alpha.
- `overlapMode`: one of `ignore`, `block`, `dodge` (currently placeholder).
- `pinnedApps`: ordered pinned app list (`desktopFile`, `name`, `icon`, `exec`).

---


### Overlap mode behavior

- `ignore` (fully implemented): overlay mode; no desktop work-area reservation.
- `block` (implemented on X11/XWayland): publishes `_NET_WM_STRUT` / `_NET_WM_STRUT_PARTIAL` so maximized windows can avoid the dock.
- `dodge` (placeholder): currently behaves like `ignore` and logs a warning.

Platform notes:

- On **X11/XWayland**, KDEP6Dock applies dock/panel hints (`_NET_WM_WINDOW_TYPE_DOCK`) and can reserve space in `block` mode.
- On **Wayland**, compositor policy limits standalone clients; KDEP6Dock still uses non-taskbar/non-focus Qt flags, but reliable reserved-space behavior is not guaranteed without compositor-specific protocols.

## Testing guide

### 1) Launch

```bash
./build/kdep6dock
```

Expected:

- A rounded, frameless dock appears near the bottom center of the primary screen.

### 2) Verify hover animation

- Move pointer left/right over an icon without leaving it.
- Expected: magnification changes continuously with cursor X position (no stepped enter/leave behavior), and neighboring icons react smoothly.

### 3) Verify reverse-from-current behavior (critical)

- Move pointer over an icon and then away before animation completes.
- Move quickly back and forth across adjacent icons.
- Expected: each icon progresses continuously from its **current** animation value toward new target (no snap, no restart from 0/1).

### 4) Verify drag reorder

- Click-hold an icon, drag left/right, drop.
- Expected: icon order changes.
- Restart dock and verify new order persists.

### 5) Verify adding apps via `.desktop` drop

- Drag one or more `.desktop` files from e.g. `/usr/share/applications/` onto the dock.
- Expected: valid entries are appended and visible.
- Restart dock and verify pinned entries persist.

### 6) Verify remove app

- Right-click icon -> **Remove from Dock**.
- Expected: icon disappears and removal persists after restart.

### 7) Verify config persistence

- Edit `~/.config/kdep6dock/config.json` manually (e.g., `maxScale`, `baseIconSize`).
- Restart dock.
- Expected: visual behavior and geometry reflect edited values.

### 8) Plasma 6 behavior notes

- The app uses shell-like window flags (`Qt::Tool`, `Qt::FramelessWindowHint`, no-focus flags) and X11 dock hints when available.
- On Wayland, strict compositor policies may limit “true dock” behavior compared to desktop shell-integrated components.
- On X11, staying on top may feel closer to traditional docks.

---

## Debugging notes

KDEP6Dock uses `qDebug()` / `qWarning()` logs for:

- Config load/save outcomes.
- Desktop file parsing errors.
- Drag/drop operations.
- App launch failures.

Run from terminal to see logs:

```bash
./build/kdep6dock
```

If build fails using `./scripts/build.sh`, inspect the generated failure report under:

```text
crash/build-failure-<timestamp>.txt
```

Common issues:

- **Invalid JSON config**: dock falls back to defaults and logs warning.
- **Dropped file ignored**: non-`.desktop` drops are ignored.
- **Missing icons**: icon theme lookup may fail if icon name is unknown.
- **Launch fails**: bad or unavailable `exec` command in pinned app entry.

---

## Known limitations

- Dock placement uses primary screen only.
- No auto-hide.
- No task manager/running-window integration.
- Wayland may constrain some always-on-top/dock semantics for standalone apps.

---

## Future improvements

- Optional settings dialog (editing current JSON fields).
- Better icon fallback handling (desktop file icon paths, pixmap caching).
- Multi-monitor selection and per-screen docking.
- Optional remove-by-drag-out behavior.
- Optional launch animations.

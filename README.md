# KDEP6Dock (Plasma-integrated)

KDEP6Dock is now a **KDE Plasma 6 dock component** (Plasmoid-first architecture), not primarily a standalone window app.

## Architecture choice

**Chosen architecture: custom Plasmoid + KDE Task Manager backend reuse (hybrid).**

Why this was chosen:

- Plasma already solves panel placement, visibility policy, monitor handling, and shell integration.
- KDE Task Manager already solves app/task model behavior better than a custom standalone window.
- We can focus custom code on dock visuals + magnification animation.

So this project now prioritizes:

1. **Plasma integration** (panel-hosted component)
2. **KDE task backend reuse** (`org.kde.taskmanager` model)
3. **Custom rendering and magnification animation layer** in QML

---

## What is reused from KDE now

Inside the plasmoid (`plasmoid/package/contents/ui/main.qml`), task data and actions come from:

- `TaskManager.TasksModel` (`org.kde.taskmanager`)
- task activation/close requests through model APIs

This is a **direct backend/model reuse** approach (not reimplemented app/task tracking).

---

## What this project now customizes

- continuous cursor-position magnification
- neighboring icon influence
- smooth animation back to resting state
- floating rounded dock visuals
- spacing/padding/opacity styling

---

## Current status of old standalone app

The old Qt Widgets executable path still exists as **legacy fallback** but is no longer the primary target.

- `BUILD_STANDALONE_DOCK=OFF` by default in CMake.
- Plasma package install is now the primary installation path.

---

## Configuration model

For Plasma mode, settings are now **Plasma-native config entries** (`contents/config/main.xml`) with the same style tunables:

- `baseIconSize`
- `maxScale`
- `neighborRadius`
- `animationDurationMs`
- `spacing`
- `dockPadding`
- `backgroundOpacity`
- `cornerRadius`

`config/sample-config.json` is kept for legacy compatibility/migration reference.

---

## Build / install

From repo root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
cmake --install build --prefix ~/.local
```

This installs the plasmoid package to:

```text
~/.local/share/plasma/plasmoids/org.kdep6dock.magnifyingdock
```

If needed, legacy standalone binary can still be built with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_STANDALONE_DOCK=ON
cmake --build build -j
```

---

## Running in Plasma panel

1. Restart Plasma shell or relogin (after first install).
2. Enter panel edit mode.
3. Add widgets -> search for **KDEP6Dock**.
4. Add it to panel (or a floating panel setup).

Plasma now handles panel geometry/policies. KDEP6Dock handles visual magnification layer.

---

## Testing checklist

1. Add/remove/reorder tasks in panel context and verify behavior comes from Plasma Task Manager backend.
2. Move mouse across icons and verify smooth continuous magnification.
3. Verify neighbors scale smoothly and return to rest when mouse leaves.
4. Launch/activate apps from dock and verify expected Plasma behavior.
5. Verify style tunables update via plasmoid config.

---

## Platform notes

- **Wayland:** preferred for Plasma-native panel behavior (handled by compositor/shell).
- **X11/XWayland:** works, but shell behavior still managed by Plasma rather than standalone-window hacks.

---

## Future work

- Add explicit section layout support:
  - pinned/running apps
  - separator
  - minimized area
  - trash
- Add plasmoid config UI pages for style presets.
- Deepen integration with task manager features while preserving custom dock visuals.

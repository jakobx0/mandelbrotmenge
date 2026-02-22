# Mandelbrotmenge (C++ / CMake)

## 1) Abhaengigkeiten (Arch Linux)

```bash
sudo pacman -S --needed base-devel cmake boost sdl12-compat
```

## 2) Build mit CMake Presets

```bash
cmake --preset debug
cmake --build --preset debug -j
```

Danach liegt `compile_commands.json` in `build/debug/` und wird bei `make build` automatisch in den Projektordner verlinkt.

## 3) Schnellstart ueber Make (gut fuer `:make` in nvim)

```bash
make deps-arch
make build
make run-server WIDTH=1280 HEIGHT=720 PORT=5000
make run-client HOST=127.0.0.1 PORT=5000
```

## 4) LazyVim / nvim

1. In den Projektordner wechseln und mit nvim oeffnen:

```bash
cd /home/system1/Documents/Projects/GitHub_projects/MandelbrotSet/Mandelbrotmenge
nvim .
```

2. Fuer `clangd` einmal `make build` ausfuehren (erst dann ist die Compile-Datenbank da).
3. Optional fuer lokale Projekt-Shortcuts `.nvim.lua` aktivieren:

In deiner LazyVim/nvim Config:

```lua
vim.opt.exrc = true
vim.opt.secure = true
```

Dann hast du lokal:
- `<leader>mb` baut das Projekt
- `<leader>ms` startet den Server
- `<leader>mc` startet den Client

## 5) Manuell starten

```bash
./build/debug/server 1280 720 5000
./build/debug/client 127.0.0.1 5000
```

## 6) Hinweis fuer andere Distros

Wenn du nicht auf Arch bist, installiere die aequivalenten Pakete fuer:
- C++ Toolchain/Build-System (`gcc`, `make`, `cmake`)
- Boost (`thread`, `chrono`)
- SDL 1.2 kompatible Bibliothek

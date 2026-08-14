# FakeFPS Overlay

A lightweight fake FPS/Ping overlay tool with customizable settings and multiple speed modes.

## 🎮 Features

- **3 Speed Modes** — Fast, Normal, and Slow FPS animation speeds
- **Custom FPS Range** — Set your own Min/Max FPS values (up to 9999)
- **Custom Ping Range** — Set your own Min/Max Ping values with realistic spikes
- **Server Location Spoofing** — Choose from 15 real server locations worldwide
- **Private Server Tag** — Toggle `[Private]` with a random version number
- **Custom Start Time** — Set a specific start time or use random (0-2 hours)
- **Built-in Icon** — Clean purple & black gaming icon

## ⌨️ Hotkeys

| Key | Action |
|-----|--------|
| **F5** | Hide / Show overlay |
| **F6** | Return to menu (change settings) |
| **F8** | Exit completely |

## 📦 Downloads

| File | Description |
|------|-------------|
| `fps fast.exe` | Aggressive FPS jumps (15-45 per frame) |
| `fps normal.exe` | Balanced FPS movement (3-8 per frame) |
| `fps slow.exe` | Smooth FPS walking (1-2 per frame) |

## 🛠️ Building from Source

Requires MinGW (g++) on Windows:

```bash
windres resource.rc -O coff -o resource.o
g++ -std=c++17 -o "fps fast.exe" main.cpp resource.o -lgdi32 -luser32 -mwindows -static -static-libgcc -static-libstdc++
```

## 📄 License

MIT License — free to use, modify, and share.

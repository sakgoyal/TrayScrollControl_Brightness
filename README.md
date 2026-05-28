# Brightness Tray Scroll Control

A lightweight C++ application that lets you easily adjust monitor brightness directly from the Windows system tray using your mouse scroll wheel.

It supports both external monitors (via DDC/CI) and built-in/laptop displays (via WMI), making it a comprehensive hardware-level brightness controller.

Based on my [TrayScrollControl](https://github.com/sakgoyal/TrayScrollControl) project.

## Features

- **Intuitive Controls:** Simply hover over the system tray icon and scroll the mouse wheel to adjust brightness.
- **Multi-Monitor Support:** Detects multiple monitors and lets you control their brightness independently by hovering over the specific monitor's pop-up indicator.
- **Hardware-Level Control:** Uses Windows API and DDC support depending on monitor capabilities.
- **Extremely Lightweight:** Written natively in standard C++ using the Win32 API. It has **zero external dependencies** and compiles to a tiny, standalone executable (**~150 KB**).
- Automatically detects changes in monitors and updates the tray accordingly.

## Previews

### Single Monitor
<img width="164" height="66" alt="image" src="https://github.com/user-attachments/assets/62a7c3eb-f8dd-479c-b888-119b7543eb48" />

### Multi Monitor (icon for each monitor)
<img width="353" height="93" alt="image" src="https://github.com/user-attachments/assets/2a6d6bd0-eab1-481f-acb3-2c51d216eaf4" />

## Usage

1. Launch `control.exe`.
2. Find the new brightness icon in your Windows system tray (bottom right).
3. **Hover** over the icon with your cursor.
4. **Scroll up or down** with your mouse wheel to adjust the screen brightness.
5. If using multiple monitors, hover over the specific monitor's indicator that appears, and scroll.

## Building from Source

### Prerequisites
- `clang` for compiling C++23.
- Windows SDK (to provide `rc.exe` for compiling Windows resources).
- Windows Developer Shell (If using clang from Visual Studio)

### Building from Source

```powershell
# Compile the resource file
rc.exe .\src\resources.rc

# Build the executable using Clang
clang++ -O3 -std=c++23 -o control.exe .\src\main.cpp .\src\resources.res
```

This will output `control.exe` which you can then run directly or place in your startup folder.

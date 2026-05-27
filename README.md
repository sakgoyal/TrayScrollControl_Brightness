# Brightness Tray Scroll Control

Minimal c++ and rust implementation for monitor brightness control with Windows API and DDC support.

Based on: <https://github.com/sakgoyal/TrayScrollControl>

## Compiling from Source

```powershell
clang++ -O3 -std=c++23 -o control.exe .\src\main.cpp .\src\resources.res
```

## Running

```powershell
control.exe
```

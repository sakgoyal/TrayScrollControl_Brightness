# Brightness Tray Scroll Control

Minimal c++ and rust implementation for monitor brightness control with Windows API and DDC support.

Based on: <https://github.com/sakgoyal/TrayScrollControl>

## Compiling from Source

```powershell
rc.exe .\src\resources.rc
clang++ -O3 -std=c++23 -o control.exe .\src\main.cpp .\src\resources.res
```

## Running

```powershell
control.exe
```

## Single Monitor
<img width="164" height="66" alt="image" src="https://github.com/user-attachments/assets/62a7c3eb-f8dd-479c-b888-119b7543eb48" />

## Multi Monitor
<img width="353" height="93" alt="image" src="https://github.com/user-attachments/assets/2a6d6bd0-eab1-481f-acb3-2c51d216eaf4" />

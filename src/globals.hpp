#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <vector>
#include <string>

inline const UINT WM_TRAYICON = WM_USER + 1;
inline const LPCSTR szWindowClass = "TRAYICONSCROLL";

inline HWND hWnd_g = NULL;
inline bool bIsListeningInput = false;
inline int g_hoveredMonitorIndex = -1;

enum MonitorType {
	MONITOR_TYPE_DDC,
	MONITOR_TYPE_WMI
};

struct MonitorInfo {
	MonitorType type;
	HANDLE handle;
	DWORD currentBrightness;
	UINT iconId;
	std::string name;
	NOTIFYICONDATA nid;
};

inline std::vector<MonitorInfo> g_monitors;

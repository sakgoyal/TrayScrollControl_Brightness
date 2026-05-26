#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <vector>
#include "globals.hpp"
#include "brightness.hpp"

#include <hidusage.h>
#include <highlevelmonitorconfigurationapi.h>
#include <shellapi.h>
#include <commctrl.h>

#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

inline void UpdateTrayTooltip(int idx) {
	std::string tooltip = "Brightness: " + std::to_string(g_monitors[idx].currentBrightness) + "\n" + g_monitors[idx].name;

	size_t length = tooltip.copy(g_monitors[idx].nid.szTip, 127);
	g_monitors[idx].nid.szTip[length] = '\0';

	Shell_NotifyIcon(NIM_MODIFY, &g_monitors[idx].nid);

	HWND hTooltip = NULL;
	while ((hTooltip = FindWindowExA(NULL, hTooltip, "tooltips_class32", NULL)) != NULL) {
		PostMessage(hTooltip, TTM_UPDATE, 0, 0);
		PostMessage(hTooltip, TTM_POPUP, 0, 0);
	}
}

inline BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC hdc, LPRECT lprcMonitor, LPARAM pData) {
	DWORD count = 0;
	if (GetNumberOfPhysicalMonitorsFromHMONITOR(hMon, &count) && count > 0) {
		std::vector<PHYSICAL_MONITOR> pMons(count);
		if (GetPhysicalMonitorsFromHMONITOR(hMon, count, pMons.data())) {
			for (DWORD i = 0; i < count; i++) {
				DWORD minB = 0, currB = 0, maxB = 0;
				if (GetMonitorBrightness(pMons[i].hPhysicalMonitor, &minB, &currB, &maxB)) {
					MonitorInfo info = {};
					info.type = MONITOR_TYPE_DDC;
					info.handle = pMons[i].hPhysicalMonitor;
					info.currentBrightness = currB;
					info.iconId = g_monitors.size() + 1;

					std::wstring wname(pMons[i].szPhysicalMonitorDescription);
					info.name.assign(wname.begin(), wname.end());

					g_monitors.push_back(info);
				} else {
					MonitorInfo info = {};
					info.type = MONITOR_TYPE_WMI;
					info.handle = NULL;
					info.currentBrightness = GetWmiBrightness();
					info.iconId = g_monitors.size() + 1;

					std::wstring wname(pMons[i].szPhysicalMonitorDescription);
					info.name.assign(wname.begin(), wname.end());
					info.name += " (Internal WMI)";

					g_monitors.push_back(info);
					DestroyPhysicalMonitor(pMons[i].hPhysicalMonitor);
				}
			}
		}
	}
	return TRUE;
}

inline void CheckIfCursorIsInTrayIconBounds(HWND hWnd) {
	POINT ptCursor;
	GetCursorPos(&ptCursor);

	int foundIndex = -1;
	for (size_t i = 0; i < g_monitors.size(); i++) {
		NOTIFYICONIDENTIFIER niid = {};
		niid.cbSize = sizeof(NOTIFYICONIDENTIFIER);
		niid.hWnd = hWnd;
		niid.uID = g_monitors[i].iconId;

		RECT rcIcon;
		if (SUCCEEDED(Shell_NotifyIconGetRect(&niid, &rcIcon)) && PtInRect(&rcIcon, ptCursor)) {
			foundIndex = (int)i;
			break;
		}
	}

	g_hoveredMonitorIndex = foundIndex;

	if (g_hoveredMonitorIndex != -1) {
		if (!bIsListeningInput) {
			RAWINPUTDEVICE rid = {};
			rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
			rid.usUsage = HID_USAGE_GENERIC_MOUSE;
			rid.dwFlags = RIDEV_INPUTSINK;
			rid.hwndTarget = hWnd;
			if (RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
				bIsListeningInput = true;
			}
		}
	} else {
		if (bIsListeningInput) {
			RAWINPUTDEVICE rid = {};
			rid.usUsagePage = HID_USAGE_PAGE_GENERIC;
			rid.usUsage = HID_USAGE_GENERIC_MOUSE;
			rid.dwFlags = RIDEV_REMOVE;
			rid.hwndTarget = NULL;
			RegisterRawInputDevices(&rid, 1, sizeof(rid));
			bIsListeningInput = false;
		}
	}
}

inline void ShowTrayMenu(HWND hWnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, 1, "Exit");
	SetForegroundWindow(hWnd);
	int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);

	if (cmd == 1) {
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	}
}

inline void CleanupMonitors() {
	for (size_t i = 0; i < g_monitors.size(); i++) {
		Shell_NotifyIcon(NIM_DELETE, &g_monitors[i].nid);
		if (g_monitors[i].type == MONITOR_TYPE_DDC) {
			DestroyPhysicalMonitor(g_monitors[i].handle);
		}
	}
	g_monitors.clear();
	g_hoveredMonitorIndex = -1;
}

inline void InitMonitors() {
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

	for (size_t i = 0; i < g_monitors.size(); i++) {
		g_monitors[i].nid.cbSize		   = sizeof(NOTIFYICONDATA);
		g_monitors[i].nid.hWnd			   = hWnd_g;
		g_monitors[i].nid.uID			   = g_monitors[i].iconId;
		g_monitors[i].nid.uFlags		   = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		g_monitors[i].nid.uCallbackMessage = WM_TRAYICON;
		g_monitors[i].nid.hIcon			   = LoadIcon(NULL, IDI_APPLICATION);
		g_monitors[i].nid.uVersion		   = NOTIFYICON_VERSION_4;

		std::string tooltip = "Brightness: " + std::to_string(g_monitors[i].currentBrightness) + "\n" + g_monitors[i].name;
		size_t length = tooltip.copy(g_monitors[i].nid.szTip, sizeof(g_monitors[i].nid.szTip) - 1);
		g_monitors[i].nid.szTip[length] = '\0';

		Shell_NotifyIcon(NIM_ADD, &g_monitors[i].nid);
	}
}


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "globals.hpp"
#include "brightness.hpp"
#include "tray_monitors.hpp"
#include <highlevelmonitorconfigurationapi.h>

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	CheckIfCursorIsInTrayIconBounds(hWnd);

	switch (message) {
	case WM_TIMER:
		if (wParam >= 1000) {
			int index = (int)(wParam - 1000);
			if (index >= 0 && index < (int)g_monitors.size() && g_monitors[index].type == MONITOR_TYPE_WMI) {
				SetWmiBrightnessAsync(g_monitors[index].currentBrightness);
			}
			KillTimer(hWnd, wParam);
		}
		break;

	case WM_INPUT: {
		UINT dwSize	 = sizeof(RAWINPUT);
		RAWINPUT raw = RAWINPUT();
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
			if (raw.header.dwType == RIM_TYPEMOUSE) {
				USHORT flags = raw.data.mouse.usButtonFlags;
				if (flags & RI_MOUSE_WHEEL) {
					if (g_hoveredMonitorIndex != -1) {
						int delta		 = (short)raw.data.mouse.usButtonData / (short)24;
						MonitorInfo *mon = &g_monitors[g_hoveredMonitorIndex];

						if (delta > 0 && mon->currentBrightness <= 100 - delta) {
							mon->currentBrightness += delta;
						} else if (delta < 0 && mon->currentBrightness >= (DWORD)(-delta)) {
							mon->currentBrightness += delta;
						} else if (delta > 0) {
							mon->currentBrightness = 100;
						} else {
							mon->currentBrightness = 0;
						}

						if (mon->type == MONITOR_TYPE_DDC) {
							SetMonitorBrightness(mon->handle, mon->currentBrightness);
						} else if (mon->type == MONITOR_TYPE_WMI) {
							SetTimer(hWnd, 1000 + g_hoveredMonitorIndex, 100, NULL);
						}

						UpdateTrayTooltip(g_hoveredMonitorIndex);
					}
				}
			}
		}
	} break;

	case WM_TRAYICON: {
		if (lParam == WM_RBUTTONUP) {
			ShowTrayMenu(hWnd);
		} else if (lParam == WM_MOUSEMOVE) {
			CheckIfCursorIsInTrayIconBounds(hWnd);
		}
	} break;

	case WM_DISPLAYCHANGE:
		CleanupMonitors();
		InitMonitors();
		break;

	case WM_DESTROY:
		CleanupMonitors();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int main() {
	SetProcessDPIAware();
	const HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.lpfnWndProc   = WndProc;
	wc.hInstance	   = hInstance;
	wc.lpszClassName = szWindowClass;

	if (!RegisterClass(&wc))
		return 1;

	hWnd_g = CreateWindowEx(0, szWindowClass, NULL, 0, 0, 0, 0, 0, NULL, NULL,
							hInstance, NULL);
	if (!hWnd_g)
		return 1;

	InitMonitors();

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

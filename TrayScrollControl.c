#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <hidusage.h>
#include <shellapi.h>
#include <highlevelmonitorconfigurationapi.h>

#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

constexpr static UINT WM_TRAYICON = WM_USER + 1;
static NOTIFYICONDATA nid = {0};
static bool bIsListeningInput = false;
static DWORD currentBrightness = 0;
const static LPCSTR szWindowClass = "TRAYICONSCROLL";

static PHYSICAL_MONITOR arr[4];
static DWORD count = 0;
static HWND hWnd = NULL;

static void CheckIfCursorIsInTrayIconBounds(HWND hWnd) {
	NOTIFYICONIDENTIFIER niid = {
		.cbSize = sizeof(NOTIFYICONIDENTIFIER),
		.hWnd = hWnd,
		.uID = 1,
		.guidItem = { 0, 0, 0, {0} },
	};

	POINT ptCursor;
	RECT rcIcon;
	if (GetCursorPos(&ptCursor) &&
		SUCCEEDED(Shell_NotifyIconGetRect(&niid, &rcIcon)) &&
		PtInRect(&rcIcon, ptCursor)) {
		if (!bIsListeningInput) {
			RAWINPUTDEVICE rid = {
				.usUsagePage = HID_USAGE_PAGE_GENERIC,
				.usUsage = HID_USAGE_GENERIC_MOUSE,
				.dwFlags = RIDEV_INPUTSINK,
				.hwndTarget = hWnd,
			};

			if (RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
				bIsListeningInput = true;
			}
		}
	} else {
		if (bIsListeningInput) {
			RAWINPUTDEVICE rid = {
				.usUsagePage = HID_USAGE_PAGE_GENERIC,
				.usUsage = HID_USAGE_GENERIC_MOUSE,
				.dwFlags = RIDEV_REMOVE,
				.hwndTarget = NULL, // required when removing
			};

			RegisterRawInputDevices(&rid, 1, sizeof(rid));
			bIsListeningInput = false;
		}
	}
}

static void ShowTrayMenu(HWND hWnd) {
	POINT pt;
	GetCursorPos(&pt);

	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, 1, "Exit");

	SetForegroundWindow(hWnd); // Required for menu to disappear correctly

	int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, NULL);
	DestroyMenu(hMenu);

	if (cmd == 1) {
		PostMessage(hWnd, WM_CLOSE, 0, 0);
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	CheckIfCursorIsInTrayIconBounds(hWnd);

	switch (message) {
	case WM_INPUT: {
		UINT dwSize = sizeof(RAWINPUT);
		RAWINPUT raw = {0};
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
			if (raw.header.dwType == RIM_TYPEMOUSE) {
				USHORT flags = raw.data.mouse.usButtonFlags;
				if (flags & RI_MOUSE_WHEEL) {
					int delta = (short)raw.data.mouse.usButtonData / (short)24; // 120 is one notch
					if (delta > 0 && currentBrightness < 95) {
						currentBrightness += (DWORD)delta;
					} else if (delta < 0 && currentBrightness > 5) {
						currentBrightness -= (DWORD)-delta;
					} else {
						break;
					}
					SetMonitorBrightness(arr[0].hPhysicalMonitor, currentBrightness);
				}
			}
		}
	} break;

	case WM_TRAYICON: {
		if (lParam == WM_RBUTTONUP) {
			ShowTrayMenu(hWnd);
		}
	} break;

	case WM_DESTROY:
		Shell_NotifyIcon(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int main() {
	HMONITOR hMon = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
	if (!hMon) return 1;
	const HINSTANCE hInstance = GetModuleHandle(NULL);

	if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMon, &count) || count == 0) exit(1);
		if (count > 4) exit(1);
		if (!GetPhysicalMonitorsFromHMONITOR(hMon, count, arr)) exit(1);

	const WNDCLASS wcex = {
		.lpfnWndProc = WndProc,
		.hInstance = hInstance,
		.lpszClassName = szWindowClass,
	};

	if (!RegisterClass(&wcex)) return 1;

	hWnd = CreateWindowEx(0, szWindowClass, NULL, 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd) return 1;
	const char tooltip[] = "Scroll Tray Icon\0";
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = 1;
	nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYICON;
	nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	strncpy_s(nid.szTip, 128, tooltip, sizeof(tooltip) - 1);
	if (!Shell_NotifyIcon(NIM_ADD, &nid)) return 1;

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

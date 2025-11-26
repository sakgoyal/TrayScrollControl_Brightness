#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <hidusage.h>
#include <iostream>
#include <print>
#include <shellapi.h>
#include <highlevelmonitorconfigurationapi.h>

#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

using std::cerr;
using std::println;

constexpr UINT WM_TRAYICON = WM_USER + 1;
static NOTIFYICONDATAW nid = {};
static bool bIsListeningInput = false;
static DWORD currentBrightness;


struct PhysicalMonitors {
    PHYSICAL_MONITOR arr[10];
    DWORD count = 0;

    explicit PhysicalMonitors(HMONITOR hMon) {
        if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMon, &count) || count == 0)
            throw std::runtime_error("no physical monitors");

        if (count > 10)
            throw std::runtime_error("monitor count exceeds fixed array");

        if (!GetPhysicalMonitorsFromHMONITOR(hMon, count, arr))
            throw std::runtime_error("GetPhysicalMonitorsFromHMONITOR failed");
    }

    ~PhysicalMonitors() { DestroyPhysicalMonitors(count, arr); }

	DWORD getBrightness(DWORD index) {
		DWORD minB=0, brightness=0, maxB=0;
		if (GetMonitorBrightness(arr[index].hPhysicalMonitor, &minB, &brightness, &maxB)) {
			return brightness;
		}
		return 0;
	}

	DWORD setBrightness(DWORD index, DWORD brightness) {
		if (SetMonitorBrightness(arr[index].hPhysicalMonitor, brightness)) {
				return brightness;
			}
		return 0;
	}
};

static PhysicalMonitors* phys = nullptr;

static bool CheckIfCursorIsInTrayIconBounds(HWND hWnd) {
	NOTIFYICONIDENTIFIER niid = {
		.cbSize = sizeof(NOTIFYICONIDENTIFIER),
		.hWnd = nid.hWnd,
		.uID = nid.uID,
		.guidItem = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } },
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
		return true;
	} else {
		if (bIsListeningInput) {
			RAWINPUTDEVICE rid = {
				.usUsagePage = HID_USAGE_PAGE_GENERIC,
				.usUsage = HID_USAGE_GENERIC_MOUSE,
				.dwFlags = RIDEV_REMOVE,
				.hwndTarget = nullptr, // required when removing
			};

			RegisterRawInputDevices(&rid, 1, sizeof(rid));
			bIsListeningInput = false;
		}
		return false;
	}
}

static void ShowTrayMenu(HWND hWnd) {
	POINT pt;
	GetCursorPos(&pt);

	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, 1, L"Exit");

	SetForegroundWindow(hWnd); // Required for menu to disappear correctly

	int cmd = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hWnd, nullptr);
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
		RAWINPUT raw{};
		if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize) {
			if (raw.header.dwType == RIM_TYPEMOUSE) {
				USHORT flags = raw.data.mouse.usButtonFlags;
				if (flags & RI_MOUSE_WHEEL) {
					auto delta = (short)raw.data.mouse.usButtonData / (short)120; // 120 is one notch
					println("Raw wheel: delta={}", delta);
					DWORD newBrightness = 0;
					if (delta > 0 && currentBrightness <= 100 - (DWORD)(delta * 5)) {
						newBrightness = currentBrightness + (DWORD)(delta * 5);
					} else if (delta < 0 && currentBrightness >= (DWORD)(-delta * 5)) {
						newBrightness = currentBrightness - (DWORD)(-delta * 5);
					} else {
						break;
					}
					std::println("Current Brightness: {}, New Brightness: {}", currentBrightness, newBrightness);
					phys->setBrightness(0, newBrightness);
					currentBrightness = newBrightness;

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
		Shell_NotifyIconW(NIM_DELETE, &nid);
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
	return 0;
}

int main() {
	constexpr LPCWSTR szWindowClass = L"TRAYICONSCROLL";
	HINSTANCE hInstance = GetModuleHandle(nullptr);

	HMONITOR hMon = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
	if (!hMon) return 1;
	phys = new PhysicalMonitors(hMon);
	currentBrightness = phys->getBrightness(0);

	WNDCLASSEX wcex = {
		.cbSize = sizeof(WNDCLASSEX),
		.lpfnWndProc = WndProc,
		.hInstance = hInstance,
		.lpszClassName = szWindowClass,
	};

	if (!RegisterClassEx(&wcex)) {
		println(cerr, "Failed to register window class. Error: {}", GetLastError());
		return 1;
	}

	HWND hWnd = CreateWindowEx(0, szWindowClass, nullptr, 0, 0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) {
		println(cerr, "Failed to create window. Error: {}", GetLastError());
		return 2;
	}

	nid = {
		.cbSize = sizeof(NOTIFYICONDATAW),
		.hWnd = hWnd,
		.uID = 1,
		.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP,
		.uCallbackMessage = WM_TRAYICON,
		.hIcon = LoadIcon(nullptr, IDI_APPLICATION),
		.szTip = L"Scroll Tray Icon",
	};

	if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
		println(cerr, "Failed to add tray icon.");
		return 3;
	}

	MSG msg = {};
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return int(msg.wParam);
}

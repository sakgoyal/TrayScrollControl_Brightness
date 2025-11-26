#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <print>
#include <highlevelmonitorconfigurationapi.h>

#pragma comment(lib, "Dxva2.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")



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
		DWORD minB=0, currentB=0, maxB=0;
		if (GetMonitorBrightness(arr[index].hPhysicalMonitor, &minB, &currentB, &maxB)) {
			if (SetMonitorBrightness(arr[index].hPhysicalMonitor, brightness)) {
				return brightness;
			}
		}
		return 0;
	}
};

int main() {
    HMONITOR hMon = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    if (!hMon) return 1;

    PhysicalMonitors phys(hMon);
	std::println("Current Brightness: {}", phys.getBrightness(0));
	phys.setBrightness(0, 50); // Set brightness of first monitor to 50
	std::println("New Brightness: {}", phys.getBrightness(0));
	return 0;
}

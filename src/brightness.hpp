#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <array>
#include <stdio.h>

inline DWORD GetWmiBrightness() {
	FILE *fp = _popen("powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"(Get-WmiObject -Namespace root/WMI -Class WmiMonitorBrightness).CurrentBrightness\"",
					  "r");
	if (!fp)
		return 50;
	std::array<char, 30> buffer;
	if (fgets(buffer.data(), buffer.size(), fp) != nullptr) {
		_pclose(fp);
		return (DWORD)atoi(buffer.data());
	}
	_pclose(fp);
	return 50;
}

inline void SetWmiBrightnessAsync(DWORD brightness) {
	std::string cmd = "powershell.exe -WindowStyle Hidden -NoProfile -ExecutionPolicy "
					  "Bypass -Command \"(Get-WmiObject -Namespace root/WMI -Class "
					  "WmiMonitorBrightnessMethods).WmiSetBrightness(1," + std::to_string(brightness) + ")\"";

	STARTUPINFOA si = {sizeof(si)};
	PROCESS_INFORMATION pi;
	if (CreateProcessA(NULL, &cmd[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL,
					   &si, &pi)) {
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

#include "app.dllapp.h"
using namespace std;



int DllAppMain(CmdLineW& cl) {
	wstring appEntry = L"AppEntry";
	wstring dllFile;
	wstring dllHostType;

	cl.getopt(L"app-entry", appEntry);
	cl.getopt(L"dll-file", dllFile);
	cl.getopt(L"dll-host-type", dllHostType);
	if (appEntry.empty()) appEntry = L"AppEntry";
	if (dllFile.empty() || dllHostType.empty())
		return ERROR_INVALID_PARAMETER;

	if (dllHostType == L"default") {
		HMODULE hDll = LoadLibraryW(dllFile.c_str());
		if (!hDll) return GetLastError();

		using entry_t = int(__stdcall*) (PCWSTR lpstrCmdLine);
		entry_t entry = (entry_t)GetProcAddress(hDll, ws2c(appEntry));
		if (!entry) return GetLastError();

		return (int)entry(GetCommandLineW());
	}

	return ERROR_NOT_SUPPORTED;
}




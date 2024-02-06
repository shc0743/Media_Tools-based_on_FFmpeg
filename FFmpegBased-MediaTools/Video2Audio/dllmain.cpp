// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


#include "../info.h"

MTL_dllinfo info;
MTL_dllinfo __stdcall GetInfo() {
    info.cb = sizeof(info);
    wcscpy_s(info.szAppName, L"Convert video to audio");
    return info;
}



int __stdcall AppEntry(PCWSTR lpstrCmdLine) {

    MessageBoxW(0, L"Hello World!", L"hello", 0);

    return 0;
}



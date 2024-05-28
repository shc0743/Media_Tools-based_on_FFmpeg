// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "../../resource/tool.h"
#include <CommCtrl.h>
#include "../progress_wizard/wizard.user.h"
#pragma comment(lib, "../progress_wizard/MyProgressWizardLib64.lib")

HMODULE hInst;

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		::hInst = hModule; break;
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
	wcscpy_s(info.szAppName, DoesUserUsesChinese() ?
		L"FFmpeg 在线下载 (从 gyan.dev)" : L"FFmpeg Online Downloader (from gyan.dev)");
	return info;
}




#include "../info.h"
using namespace std;


#include <windows.h>  
#include <wininet.h>  
#include <iostream>  

#pragma comment(lib, "wininet.lib")  


#if 0
// old codes
void myexit() { ExitProcess(ERROR_CANCELLED); }
void CALLBACK InternetCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
DWORD download_main() {
	SetCurrentDirectory(_T(".."));
	HINTERNET hInternet = InternetOpen(L"FFDownloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hInternet == NULL) {
		return GetLastError();
	}

	HINTERNET hRequest = InternetOpenUrlW(hInternet,
		L"https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full.7z",
		NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hRequest == NULL) {
		DWORD dwErr = GetLastError();
		InternetCloseHandle(hInternet);
		return dwErr;
	}

	// 查询文件大小  
	DWORD dwFileSize = 0;
	DWORD dwSize = sizeof(dwFileSize);
	if (!HttpQueryInfo(hRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwFileSize, &dwSize, NULL)) {
		;
	}

	// 打开文件用于写入  
	string fname = GenerateUUID() + ".tmp";
	FILE* file = NULL;
	fopen_s(&file, fname.c_str(), "wb");
	if (!file) {
		DWORD dwErr = GetLastError();
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hInternet);
		return dwErr;
	}

	// 设置回调函数以接收下载进度通知  
	InternetSetStatusCallbackW(hRequest, InternetCallback);

	HMPRGOBJ hObj = CreateMprgObject();
	HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
		.szTitle = L"Download FFmpeg",
		.max = dwFileSize ? dwFileSize : size_t(-1),
	});
	OpenMprgWizard(hWiz);
	SetMprgWizAttribute(hWiz, MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::WizAttrCancelHandler,
		(LONG_PTR)(void*)myexit);

   // MessageBoxW(0, L"1", 0, 0);

	// 开始下载文件  
	char* buffer = new char[1 * 1024 * 1024];
	DWORD bytesRead;
	wstring sz;
	__int64 totalBytesRead = 0; // 使用64位整数来存储总读取字节数，以支持大文件  
	while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
		fwrite(buffer, 1, bytesRead, file);
		totalBytesRead += bytesRead;
		// 在这里更新下载进度  
		int percentage = static_cast<int>((totalBytesRead * 100.0) / dwFileSize);
		//std::cout << "Download progress: " << percentage << "%" << std::endl;
		SetMprgWizardValue(hWiz, totalBytesRead);
		sz = L"Downloading... " + to_wstring(totalBytesRead) + L"/" +
			to_wstring(dwFileSize) + L" bytes (" + to_wstring(percentage) + L"%)";
		SetMprgWizardText(hWiz, sz.c_str());
	}

	// 关闭文件和连接  
	fclose(file);
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hInternet);
	DeleteMprgObject(hObj);
	delete[] buffer;

	wstring nfn = (L"FFmpeg-" + to_wstring(time(0))) + L".7z";
	MoveFileW(s2wc(fname), nfn.c_str());
	Process.StartOnly(L"explorer /select," + nfn);

	return 0;
}

// 回调函数，用于接收下载进度通知  
void CALLBACK InternetCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) {
	return;
}

int __stdcall AppEntry(PCWSTR lpstrCmdLine) {
	CmdLineW cl(lpstrCmdLine);
	InitMprgComponent();
	return (int)download_main();
}
#else
// 如果用户取消，则删除临时文件

HANDLE ggfile;
wstring gsFileName;
void myexit() {
	CloseHandle(ggfile);
	DeleteFileW(gsFileName.c_str());
	ExitProcess(ERROR_CANCELLED);
}
constexpr size_t buffer_size = 256 * 1024 * 1024; // 256MB
void CALLBACK InternetCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
DWORD download_main(wstring url, wstring filename, wstring ua, bool slient) {
	// 预先创建进度条，提升用户体验
	HMPRGOBJ hObj = CreateMprgObject();
	HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
		.szTitle = filename.c_str(),
		.max = size_t(-1),
		//.max = dwFileSize ? dwFileSize : size_t(-1),
		});
	if (!slient) OpenMprgWizard(hWiz);
	SetMprgWizAttribute(hWiz, MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES::WizAttrCancelHandler,
		(LONG_PTR)(void*)myexit);

	SetMprgWizardText(hWiz, L"Establishing connection, please wait...");

	HINTERNET hInternet = InternetOpenW(ua.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hInternet == NULL) {
		return GetLastError();
	}

	HINTERNET hRequest = InternetOpenUrlW(hInternet,
		url.c_str(),
		NULL, 0, INTERNET_FLAG_RELOAD, 0);
	if (hRequest == NULL) {
		DWORD dwErr = GetLastError();
		InternetCloseHandle(hInternet);
		return dwErr;
	}

	// 查询文件大小  
	DWORD dwFileSize = 0;
	DWORD dwSize = sizeof(dwFileSize);
	if (!HttpQueryInfo(hRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &dwFileSize, &dwSize, NULL)) {
		std::cerr << "[WARN]  Cannot query file size. Maybe the server doesn't provide Content-Length header.\n";
	}
	if (dwFileSize) {
		auto data = GetModifiableMprgWizardData(hWiz);
		data->max = dwFileSize;
		UpdateMprgWizard(hWiz);

		wstring sz = L"Downloading... 0/" +
			to_wstring(dwFileSize) + L" bytes (0%)";
		SetMprgWizardText(hWiz, sz.c_str());
	}

	// 打开文件用于写入
	gsFileName = filename;
	ggfile = CreateFileW(filename.c_str(), GENERIC_ALL, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!ggfile || ggfile == INVALID_HANDLE_VALUE) {
		DWORD dwErr = GetLastError();
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hInternet);
		return dwErr;
	}

	// 设置回调函数以接收下载进度通知  
	InternetSetStatusCallbackW(hRequest, InternetCallback);

	// MessageBoxW(0, L"1", 0, 0);

	 // 开始下载文件  
	char* buffer = new char[buffer_size];
	DWORD bytesRead = 0, bytesWrite = 0;
	wstring sz;
	wchar_t* ccsz = new wchar_t[8192];
	__int64 totalBytesRead = 0; // 使用64位整数来存储总读取字节数，以支持大文件  
	while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
		WriteFile(ggfile, buffer, bytesRead, &bytesWrite, NULL);

		totalBytesRead += bytesRead;
		// 在这里更新下载进度  
		int percentage = static_cast<int>((totalBytesRead * 100.0) / dwFileSize);
		//std::cout << "Download progress: " << percentage << "%" << std::endl;
		SetMprgWizardValue(hWiz, totalBytesRead, false);
		sz = L"Downloading... " + to_wstring(totalBytesRead) + L"/" +
			to_wstring(dwFileSize) + L" bytes (" + to_wstring(percentage) + L"%)";
		wcscpy_s(ccsz, 8192, sz.c_str());
		SetMprgWizardText(hWiz, ccsz, false);
	}

	// 关闭文件和连接  
	CloseHandle(ggfile);
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hInternet);
	DeleteMprgObject(hObj);
	delete[] buffer;
	delete[] ccsz;

	return 0;
}

// 回调函数，用于接收下载进度通知  
void CALLBACK InternetCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) {
	return;
}


#include <commdlg.h>
int __stdcall AppEntry(PCWSTR lpstrCmdLine) {
	CmdLineW cl(lpstrCmdLine);
	InitMprgComponent();

	// Initialize the OPENFILENAME members.  

	wchar_t szFile[1024] = L"FFmpeg-Release-Full.7z";
	OPENFILENAMEW Ofn{};
	Ofn.lStructSize = sizeof(OPENFILENAME);
	Ofn.hwndOwner = NULL;
	Ofn.lpstrFilter = L"7-Zip Compressed File\0.7z\0";
	Ofn.lpstrFile = szFile;
	Ofn.nMaxFile = 1024;
	Ofn.Flags = OFN_SHOWHELP | OFN_EXPLORER;
	Ofn.lpstrTitle = L"Where do you want to save the FFmpeg?";

	// Display the Filename common dialog box. The  
	// filename specified by the user is passed  
	// to the CreateEnhMetaFile function and used to  
	// store the metafile on disk.  
	
	if (!GetSaveFileNameW(&Ofn)) return GetLastError();

	wstring nfn = szFile;
	if (nfn.empty()) return ERROR_CANCELLED;
	return (int)download_main(L"https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-full.7z", nfn, 
		L"Windows (c) WinINet Platform Downloader", false);
}
#endif




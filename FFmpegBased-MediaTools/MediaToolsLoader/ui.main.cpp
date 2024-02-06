#include "ui.main.h"
#include <uxtheme.h>
#include "../info.h"
using namespace std;

#include "resource.h"


#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Unhandled Exception Filter
static LONG WINAPI MyTopLevelExceptionFliter(
	_In_ PEXCEPTION_POINTERS ExceptionInfo
);

static LRESULT CALLBACK WndProc_MainWnd(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);

typedef struct {
	HWND
		hList1,
		hBtnOpen,
		hBtnQuit;
	UINT bSafeMode;
} WndData_MainWnd,*WndDataP_MainWnd;


static HFONT ghFont;



static void AddTool(HWND hwnd, LPCWSTR lpTool);
static void LaunchAppInstance(HWND hwnd, WndDataP_MainWnd data, int nSel);



#pragma region wizard

DECLARE_HANDLE(HMPRGOBJ); // Handle: MyProgressWizard Object
DECLARE_HANDLE(HMPRGWIZ); // Handle: MyProgressWizard Wizard

enum class MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES {
	NotApplicable = 0,
	WizAttrTopmost = 0x1001,
	WizAttrUnresizable = 0x1002,
	WizAttrCancelHandler = 0x8001,
};

class MPRG_CREATE_PARAMS {
public:
	size_t cb;

	long width, height;
	int type;
	PCWSTR szTitle;
	PCWSTR szText;
	HWND hParentWindow;

	size_t max, value;

};
class MPRG_WIZARD_DATA {
public:
	size_t cb;

	long width, height;
	int type;

	std::map<MPRG_WIZARD_EXTENSIBLE_ATTRIBUTES, LONG_PTR>* attrs;

	size_t max, value;
	PCWSTR szText;

	HWND hwTipText, hwProgBar, hwCancelBtn;
};
using PMPRG_WIZARD_DATA = MPRG_WIZARD_DATA*;

using TMprgCancelHandler = bool(__stdcall*) (HMPRGWIZ hWiz, HMPRGOBJ hObj);


#pragma endregion

#pragma region wizard-func
bool (*InitMprgComponent)();
HMPRGOBJ(*CreateMprgObject)();
HMPRGWIZ(*CreateMprgWizard)(HMPRGOBJ hObject, MPRG_CREATE_PARAMS, DWORD dwTimeout);
PMPRG_WIZARD_DATA(*GetModifiableMprgWizardData)(HMPRGWIZ hWizard);
bool (*SetMprgWizardValue)(HMPRGWIZ hWizard, size_t currentValue);
bool (*SetMprgWizardText)(HMPRGWIZ hWizard, PCWSTR psz);
DWORD(*DeleteMprgObject)(HMPRGOBJ hObject, bool bForceTerminateIfTimeout);
bool (*OpenMprgWizard)(HMPRGWIZ hWizard, int nShow);
HWND(*GetMprgHwnd)(HMPRGWIZ hWizard);
bool (*UpdateMprgWizard)(HMPRGWIZ hWizard);
bool initProgDll() {
	constexpr auto file = L"MTLProgressUiPlugin.dll";
	if (!FreeResFile(IDR_BIN_PROGRESS_PLUGIN, L"BIN", file)) // 先尝试覆盖
		if (!file_exists(file)) return false; // 覆盖失败
	HMODULE h = LoadLibraryW(file);
	if (!h) return false;
#define declare(x) x = reinterpret_cast<decltype(x)>(GetProcAddress(h, #x));
	declare(InitMprgComponent);
	declare(CreateMprgObject);
	declare(CreateMprgWizard);
	declare(GetModifiableMprgWizardData);
	declare(SetMprgWizardValue);
	declare(SetMprgWizardText);
	declare(DeleteMprgObject);
	declare(OpenMprgWizard);
	declare(GetMprgHwnd);
	declare(UpdateMprgWizard);


	InitMprgComponent();
#undef declare
	return true;
}
#pragma endregion




int UiMain(CmdLineW& cl) {
	INITCOMMONCONTROLSEX icce{};
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_ALL_CLASSES;
	{void(0); }
	if (!InitCommonControlsEx(&icce)) {
		return GetLastError();
	}

	// 安全模式
	bool bIsSafeMode = false;
	if (cl.getopt(L"unexpected-exception-restart")) {

		if (IDCANCEL != MessageBoxW(GetForegroundWindow(), DoesUserUsesChinese() ?
			L"Media Tools Loader 遇到异常，是否在安全模式中运行？\n"
			"这将临时禁用所有已添加的工具，以便进行故障排查。" :
			L"Media Tools Loader encountered an exception. Do you want to "
			"run it in safe mode?\nThis will temporarily disable all added"
			"tools to do troubleshoot.", L"Media Tools Loader",
			MB_ICONQUESTION | MB_OKCANCEL))
			bIsSafeMode = true;

	}
	SetUnhandledExceptionFilter(MyTopLevelExceptionFliter);

	// 设置当前目录
	SetCurrentDirectoryW(GetProgramPathW().c_str());
	wstring dataPath = s2ws(GetProgramInfo().name + ".data");
	if (IsFileOrDirectory(dataPath) == 0)
		CreateDirectoryW(dataPath.c_str(), 0);
	if (!SetCurrentDirectoryW(dataPath.c_str())) {
		MessageBoxW(0, LastErrorStrW().c_str(), 0, MB_ICONHAND);
		return GetLastError();
	}
	dataPath = L"apps";
	if (IsFileOrDirectory(dataPath) == 0)
		CreateDirectoryW(dataPath.c_str(), 0);
	if (!SetCurrentDirectoryW(dataPath.c_str())) {
		MessageBoxW(0, LastErrorStrW().c_str(), 0, MB_ICONHAND);
		return GetLastError();
	}
	SetCurrentDirectoryW(L"..");

	// 进度条初始化
	if (!initProgDll()) {
		if (IDYES == MessageBoxTimeoutW(0, L"Error: Cannot load progress bar"
			" component.\nExit?", 0, MB_ICONERROR | MB_YESNO, 0, 5000))
		return GetLastError();
	}
	
	// 窗口类注册 
	HICON hIcon = LoadIconW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDI_ICON_APP));
	HBRUSH hBg = CreateSolidBrush(RGB(0xF0, 0xF0, 0xF0));
	ghFont = CreateFontW(-14, -7, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
		L"Consolas");
	s7::MyRegisterClassW(MTL_MainWndClass, WndProc_MainWnd, WNDCLASSEXW{
		.hIcon = hIcon,
		.hbrBackground = hBg,
		.hIconSm=hIcon,
	});

	// 支持重新启动管理器 (Restart Manager)
	if (cl.getopt(L"no-restart") == 0) {
		wstring rc; cl.getopt(L"restart-count", rc);
		size_t rcc = 0;
		if (!rc.empty()) rcc = atoll(ws2c(rc));
		wstring c = L"\"" + GetProgramDirW() + L"\" --type=ui --restart-by-"
			"restart-manager --restart-count=" + to_wstring(rcc + 1);

		auto result = RegisterApplicationRestart(c.c_str(), 0);
		if (S_OK != result) {
			SetLastError(result);
			// 支持失败
		}
	}

	// 创建窗口  
	WndData_MainWnd wd{};
	wd.bSafeMode = bIsSafeMode;
	HWND hwnd = CreateWindowExW(0, MTL_MainWndClass, 
		DoesUserUsesChinese() ? L"媒体工具加载器" : L"Media Tools Loader",
		WS_OVERLAPPEDWINDOW,
		0, 0, 640, 480,
		NULL, NULL, 0, &wd);

	if (hwnd == NULL) {
		return GetLastError();
	}

	// 显示窗口 
	CenterWindow(hwnd);
	ShowWindow(hwnd, SW_NORMAL);

	// 消息循环  
	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}




// 窗口过程函数  
static LRESULT CALLBACK WndProc_MainWnd(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
	WndDataP_MainWnd data = (WndDataP_MainWnd)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (message) {
	case WM_CREATE:
	{
		LPCREATESTRUCTW pcr = (LPCREATESTRUCTW)lp;
		if (!pcr) break;
		WndDataP_MainWnd dat = (WndDataP_MainWnd)pcr->lpCreateParams;
		if (!dat) break;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)(void*)dat);

#define MYCTLS_VAR_HINST NULL
#define MYCTLS_VAR_HFONT ghFont
#include "ctls.h"
		dat->hList1 = custom(L"", WC_LISTVIEW, 0, 0, 1, 1,
			LVS_SINGLESEL | LVS_REPORT | WS_BORDER);
		dat->hBtnOpen = button(DoesUserUsesChinese() ?
			L"打开 (&O)" : L"&Open", IDOK);
		dat->hBtnQuit = button(DoesUserUsesChinese() ?
			L"退出 (&Q)" : L"&Quit", IDCANCEL);

		if (dat->bSafeMode) {
			WCHAR wcsTitle[256]{}, wcsExtendedBuffer[300]{};
			GetWindowTextW(hwnd, wcsTitle, 256);
			wcscpy_s(wcsExtendedBuffer, DoesUserUsesChinese() ?
				L"[安全模式] " : L"[Safe Mode] ");
			wcscat_s(wcsExtendedBuffer, wcsTitle);
			SetWindowTextW(hwnd, wcsExtendedBuffer);
		}
		PostMessage(hwnd, WM_USER + 0xf0, 0, 0);

	}
		break;

	case WM_USER + 0xf0:
	{
		if (!data) break;
		SetWindowTheme(data->hList1, L"Explorer", NULL);

		LVCOLUMN lvc{};
		wchar_t sz1[] = L"Tool Name", sz2[] = L"Filename";
		if (DoesUserUsesChinese()) {
			wcscpy_s(sz1, L"工具名称");
			wcscpy_s(sz2, L"文件名");
		}
			
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;

		lvc.iSubItem = 0;
		lvc.pszText = sz1;
		lvc.cx = 400;               // Width of column in pixels.
		// Insert the columns into the list view.
		ListView_InsertColumn(data->hList1, 0, &lvc);

		lvc.iSubItem = 0;
		lvc.pszText = sz2;
		lvc.cx = 180;               // Width of column in pixels.
		// Insert the columns into the list view.
		ListView_InsertColumn(data->hList1, 1, &lvc);

		DragAcceptFiles(hwnd, TRUE);

		PostMessage(hwnd, WM_USER + 0xf1, 0, 0);
	}
		break;

	case WM_USER + 0xf1:
	{
		if (!data) break;
		ListView_DeleteAllItems(data->hList1);

		LVITEM lvI{};

		// Initialize LVITEM members that are common to all items.
		lvI.pszText = LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
		lvI.mask = LVIF_TEXT | LVIF_STATE;
		lvI.stateMask = 0;
		lvI.iSubItem = 0;
		lvI.state = 0;

		wchar_t szBuffer[256]{};
		lvI.pszText = szBuffer;

		lvI.iItem = 0;
		wcscpy_s(szBuffer, DoesUserUsesChinese() ?
			L"重新加载工具列表" : L"Reload tools list");
		ListView_InsertItem(data->hList1, &lvI);

		lvI.iItem = 1;
		wcscpy_s(szBuffer, DoesUserUsesChinese() ?
			L"从本地文件添加工具..." : L"Add tools from local file...");
		ListView_InsertItem(data->hList1, &lvI);

		if (data->bSafeMode > 1) {
			if (MessageBoxW(hwnd, DoesUserUsesChinese() ? L"要退出安全模式吗"
				"？" : L"Quit the Safe Mode?", L"Question", MB_OKCANCEL)
				== IDOK) data->bSafeMode = false;
		}
		if (data->bSafeMode) { return ++data->bSafeMode; }
		HMPRGOBJ hObj = CreateMprgObject();
		HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
			.max = size_t(-1),
		}, 30000);
		OpenMprgWizard(hWiz, SW_NORMAL);
		PMPRG_WIZARD_DATA progressData = GetModifiableMprgWizardData(hWiz);
		AssertEx_AutoHandle(progressData);

		wstring fn;

		WIN32_FIND_DATAW findd{};
		HANDLE hFind = FindFirstFileW((L"./apps/*"), &findd);
		if (!hFind || hFind == INVALID_HANDLE_VALUE) {
			MessageBoxW(hwnd, LastErrorStrW().c_str(), 0, MB_ICONHAND);
			DeleteMprgObject(hObj, true);
			break;
		}
		SetCurrentDirectoryW(L"apps");
		do {
			if (wcscmp(findd.cFileName, L".") == 0 ||
				wcscmp(findd.cFileName, L"..") == 0) continue;
			if (findd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			fn = findd.cFileName;
			if (!fn.ends_with(L".MTL.dll")) continue;

#define die (((void(*)())0)());
			MTL_dllinfo dllinfo{};
			[](PCWSTR psz, MTL_dllinfo* pdll) {
				__try {
#pragma warning(push)
#pragma warning(disable: 6387)
					HMODULE hModule = LoadLibraryW(psz);
					if (!hModule) die;
					typedef MTL_dllinfo(__stdcall* F)();
					F f = (F)GetProcAddress(hModule, "GetInfo");
					MTL_dllinfo if2{};
					if2 = f();
					memcpy(pdll, &if2, sizeof(if2));
					FreeLibrary(hModule);
#pragma warning(pop)
					return true;
				}
				__except (EXCEPTION_EXECUTE_HANDLER) {
					return false;
				}
			}((fn).c_str(), &dllinfo);
#undef die
			if (!dllinfo.cb) {
				// error occurred
				continue;
			}

			++lvI.iItem;
			wcscpy_s(szBuffer, dllinfo.szAppName);
			lvI.iSubItem = 0;
			ListView_InsertItem(data->hList1, &lvI);
			wcscpy_s(szBuffer, findd.cFileName);
			lvI.iSubItem = 1;
			ListView_SetItem(data->hList1, &lvI);
			
		} while (FindNextFileW(hFind, &findd));
		FindClose(hFind);

		SetCurrentDirectoryW(L"..");
		DeleteMprgObject(hObj, true);
	}
		break;

	case WM_COMMAND:
	{
		// wParam的低位字包含了控件ID，高位字包含了通知代码  
		int id = LOWORD(wp);
		int code = HIWORD(wp);

		// 根据控件ID和通知代码处理不同的消息  
		switch (id) {
		case IDCANCEL:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		case IDOK:
			if (code == BN_CLICKED) {
				int nSel = ListView_GetSelectionMark(data->hList1);
				if (nSel < 0) {
					MessageBoxW(hwnd, DoesUserUsesChinese() ? L"请选择要打开的项目" :
						L"Please choose an item to open", 0, MB_ICONERROR);
					break;
				}
				if (nSel == 0) {
					PostMessage(hwnd, WM_USER + 0xf1, 0, 0); break;
				}
				if (nSel == 1) {
					WCHAR cd[1024]{}; GetCurrentDirectoryW(1024, cd);

					OPENFILENAME ofn{};       // common dialog box structure
					wchar_t szFile[260]{};       // buffer for file name

					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hwnd;
					ofn.lpstrFile = szFile;
					ofn.lpstrFile[0] = '\0';
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = L"Extensible Media Tool File\0*.MTL.dll\0"
						"Dynamic Link Library\0*.dll\0All Files\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					if (GetOpenFileName(&ofn) == TRUE) {
						/*	hf = CreateFile(ofn.lpstrFile,
								GENERIC_READ,
								0,
								(LPSECURITY_ATTRIBUTES)NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								(HANDLE)NULL);*/
						SetCurrentDirectoryW(cd);
						AddTool(hwnd, ofn.lpstrFile);
						PostMessage(hwnd, WM_USER + 0xf1, 0, 0);
					}
					SetCurrentDirectoryW(cd);
					break;
				}


				LaunchAppInstance(hwnd, data, nSel);
				
			}
			break;

		default:
			// 未知的控件ID，可以调用默认处理或什么都不做  
			break;
		}
		break;
	}
		break;

	case WM_DROPFILES:
	{
		HMPRGOBJ hObj = CreateMprgObject();
		HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
			.szTitle = L"Add tool(s)",
			.max = size_t(-1) }, 30000);
		if (!hWiz) return AssertEx(hWiz);
		OpenMprgWizard(hWiz, SW_NORMAL);

		SetMprgWizardText(hWiz, L"Processing...");

		HDROP hDrop = (HDROP)wp;
		wchar_t szFilePath[2048]{};
		int fileCount = DragQueryFile(hDrop, (UINT)-1, NULL, 0); // 获取拖放文件的数量  

		auto pd = GetModifiableMprgWizardData(hWiz);
		pd->max = fileCount;
		UpdateMprgWizard(hWiz);
		
		wstring sfc = to_wstring(fileCount), wcsTemp;
		EnableWindow(hwnd, FALSE);

		// 遍历所有拖放的文件  
		for (int i = 0; i < fileCount; ++i) {
			DragQueryFile(hDrop, i, szFilePath, 2048); // 获取文件路径  
			wcsTemp = (L"[" + to_wstring(i) + L"/" + sfc
				+ L"] Adding " + szFilePath);
			SetMprgWizardText(hWiz, wcsTemp.c_str());
			AddTool(hwnd, szFilePath);
			SetMprgWizardValue(hWiz, static_cast<size_t>(i) + 1);
		}

		// 释放拖放文件结构占用的内存  
		DragFinish(hDrop);

		PostMessage(hwnd, WM_USER + 0xf1, 0, 0);
		EnableWindow(hwnd, TRUE);
		MessageBoxTimeoutW(GetMprgHwnd(hWiz), DoesUserUsesChinese() ?
			(L"成功添加了 " + sfc + L" 个工具。").c_str() :
			(L"Successfully added " + sfc + L" tools.").c_str(),
			L"Success", MB_ICONINFORMATION, 0, 1000);
		DeleteMprgObject(hObj, true);
	}
		break;

	case WM_SIZE:
	{
		if (!data) break;
		static RECT rc{}; GetClientRect(hwnd, &rc);

		SetWindowPos(data->hList1, 0, 10, 10, rc.right - rc.left - 20,
			rc.bottom - rc.top - 60, SWP_NOACTIVATE);
		SetWindowPos(data->hBtnOpen, 0, rc.right - rc.left - 180,
			rc.bottom - rc.top - 40, 80, 30, SWP_NOACTIVATE);
		SetWindowPos(data->hBtnQuit, 0, rc.right - rc.left - 90,
			rc.bottom - rc.top - 40, 80, 30, SWP_NOACTIVATE);

	}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, message, wp, lp);
	}

    return 0;
}



static void AddTool(HWND hwnd, LPCWSTR lpTool) {
	wstring newFileName = L"apps/" + GenerateUUIDW() + L".MTL.dll";
	CopyFileW(lpTool, newFileName.c_str(), FALSE);
}

void LaunchAppInstance(HWND hwnd, WndDataP_MainWnd data, int nSel) {
	wchar_t szFilename[2048]{};
	ListView_GetItemText(data->hList1, nSel, 1, szFilename, 2048);
	if (!szFilename[0]) {
		MessageBoxW(hwnd, DoesUserUsesChinese() ? L"获取数据时出现错误" :
			L"An error occurred during getting data", 0, MB_ICONERROR);
		return;
	}
	wstring filename = szFilename;
	wstring szCmdLine = L"Loader --type=app --app-type=dll --app-entry= "
		"--dll-host-type=default --dll-file=\"" + filename + L"\" --spawn"
		"-process-id=" + to_wstring(GetCurrentProcessId()) + L" --spawn-"
		"window=" + to_wstring((LONG_PTR)hwnd);
	wcscpy_s(szFilename, szCmdLine.c_str());

	STARTUPINFOW si{}; PROCESS_INFORMATION pi{};
	si.cb = sizeof(si);
	WCHAR lpcd[2048]{};
	GetCurrentDirectoryW(2048, lpcd);
	wcscat_s(lpcd, L"\\apps");

	if (!CreateProcessW(GetProgramDirW().c_str(), szFilename, 0, 0, 0,
		CREATE_SUSPENDED, 0, lpcd, &si, &pi))
	{
		MessageBoxW(hwnd, LastErrorStrW().c_str(), 0, MB_ICONERROR);
		return;
	}

	using tt = struct { HANDLE hProcess, hThread; HWND hwnd; };
	tt* t = (tt*)malloc(sizeof(tt));
	if (!t) {
		TerminateProcess(pi.hProcess, ERROR_OUTOFMEMORY);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		MessageBoxW(hwnd, ErrorCodeToStringW(ERROR_OUTOFMEMORY).c_str(),
			0, MB_ICONERROR); return;
	}
	t->hProcess = pi.hProcess; t->hThread = pi.hThread; t->hwnd = hwnd;
	
	CloseHandleIfOk(CreateThread(0, 0, [](PVOID pd)->DWORD {
		using tt = struct { HANDLE hProcess, hThread; HWND hwnd; };
		tt* t = (tt*)pd;
	
		ShowWindow(t->hwnd, SW_HIDE);

		DWORD dwExitCode{};
		ResumeThread(t->hThread);
		CloseHandle(t->hThread);
		WaitForSingleObject(t->hProcess, INFINITE);
		GetExitCodeProcess(t->hProcess, &dwExitCode);
		CloseHandle(t->hProcess);

		ShowWindow(t->hwnd, SW_NORMAL);
		SetForegroundWindow(t->hwnd);

		if (dwExitCode != 0) {
			MessageBoxW(t->hwnd, ErrorCodeToStringW(dwExitCode).c_str(),
				0, MB_ICONERROR);
		}
		free(t);
		return dwExitCode;
	}, t, 0, 0));
}




static LONG WINAPI MyTopLevelExceptionFliter(
	_In_ PEXCEPTION_POINTERS ExceptionInfo) {
	//MessageBoxW(NULL, L"Unhandled Exception.\nClick [OK] to terminate process.",
	//	NULL, MB_ICONERROR);
	//if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW ||
	//	ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_INVALID ||
	//	ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_INVALID_DISPOSITION
	//) return EXCEPTION_CONTINUE_SEARCH;
	FILE* fp = NULL;
	string fn = GetProgramInfo().name + "." + to_string(GetCurrentProcessId())
		+ ".crash_report";
	fopen_s(&fp, (fn).c_str(), "w+");
	if (fp) {
		auto _ContextRecord_size = sizeof(*(ExceptionInfo->ContextRecord));
		auto _ExcepRecord_size = sizeof(*(ExceptionInfo->ExceptionRecord));
		fprintf_s(fp, "An unhandled exception occurred.\n"
			"\nException information:\n"
			"    Exception Code: 0x%X\n"
			"    Exception Address: %p\n"
			"    Exception Flags: 0x%X\n"
			"    Full dump: [size: %llu]\n"
			"[BEGIN EXCEPTION INFORMATION]\n"
			, ExceptionInfo->ExceptionRecord->ExceptionCode
			, ExceptionInfo->ExceptionRecord->ExceptionAddress
			, ExceptionInfo->ExceptionRecord->ExceptionFlags
			, ((unsigned long long)_ExcepRecord_size)
		);
		fwrite(ExceptionInfo->ExceptionRecord, _ExcepRecord_size, 1, fp);
		fprintf_s(fp, "\n[END EXCEPTION INFORMATION]\n"
			"\nContext Information:\n"
			"    structure size: %llu\n"
			"[BEGIN CONTEXT INFORMATION]\n"
			, ((unsigned long long)_ContextRecord_size));
		fwrite(ExceptionInfo->ContextRecord, _ContextRecord_size, 1, fp);
		fprintf_s(fp, "\n[END CONTEXT INFORMATION]\n\n");
		fclose(fp);
	}
	/* 重新启动 */
	{
		wstring cl = L"\"" + GetProgramDirW() + L"\" --type=ui "
			"--unexpected-exception-restart --report-file=\"" +
			s2ws(fn) + L"\" ";
		Process.StartOnly(cl);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}



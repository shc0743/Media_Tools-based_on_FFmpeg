#include "ui.main.h"
#include <uxtheme.h>
#include "wizard.user.h"
#include "../info.h"
using namespace std;

#include "resource.h"


#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


static LRESULT CALLBACK WndProc_MainWnd(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);

typedef struct {
	HWND
		hList1,
		hBtnOpen,
		hBtnQuit;
} WndData_MainWnd,*WndDataP_MainWnd;


static HFONT ghFont;



static void AddTool(HWND hwnd, LPCWSTR lpTool);
static void LaunchAppInstance(HWND hwnd, WndDataP_MainWnd data, int nSel);



int UiMain(CmdLineW& cl) {
	INITCOMMONCONTROLSEX icce{};
	icce.dwSize = sizeof(icce);
	icce.dwICC = 0xffff;
	{void(0); }
	if (!InitCommonControlsEx(&icce)) {
		return GetLastError();
	}

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

	// ��������  
	WndData_MainWnd wd{};
	HWND hwnd = CreateWindowExW(0, MTL_MainWndClass, 
		DoesUserUsesChinese() ? L"ý�幤�߼�����" : L"Media Tools Loader",
		WS_OVERLAPPEDWINDOW,
		0, 0, 640, 480,
		NULL, NULL, 0, &wd);

	if (hwnd == NULL) {
		return GetLastError();
	}

	// ��ʾ���� 
	ShowWindow(hwnd, SW_NORMAL);
	CenterWindow(hwnd);

	// ��Ϣѭ��  
	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}




// ���ڹ��̺���  
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
		dat->hBtnOpen = button(DoesUserUsesChinese() ? L"�� (&O)" : L"&Open", IDOK);
		dat->hBtnQuit = button(DoesUserUsesChinese() ? L"�˳� (&Q)" : L"&Quit", IDCANCEL);

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
			wcscpy_s(sz1, L"��������");
			wcscpy_s(sz2, L"�ļ���");
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
			L"���¼��ع����б�" : L"Reload tools list");
		ListView_InsertItem(data->hList1, &lvI);

		lvI.iItem = 1;
		wcscpy_s(szBuffer, DoesUserUsesChinese() ?
			L"�ӱ����ļ���ӹ���..." : L"Add tools from local file...");
		ListView_InsertItem(data->hList1, &lvI);

		HMPRGOBJ hObj = CreateMprgObject();
		HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
			.max = size_t(-1),
		});
		OpenMprgWizard(hWiz);
		PMPRG_WIZARD_DATA progressData = GetModifiableMprgWizardData(hWiz);
		AssertEx_AutoHandle(progressData);

		SetCurrentDirectoryW(GetProgramPathW().c_str());
		wstring dataPath = s2ws(GetProgramInfo().name + ".data");
		if (IsFileOrDirectory(dataPath) == 0)
			CreateDirectoryW(dataPath.c_str(), 0);
		if (!SetCurrentDirectoryW(dataPath.c_str())) {
			MessageBoxW(hwnd, LastErrorStrW().c_str(), 0, MB_ICONHAND);
			DeleteMprgObject(hObj);
			break;
		}
		dataPath = L"apps";
		if (IsFileOrDirectory(dataPath) == 0)
			CreateDirectoryW(dataPath.c_str(), 0);
		if (!SetCurrentDirectoryW(dataPath.c_str())) {
			MessageBoxW(hwnd, LastErrorStrW().c_str(), 0, MB_ICONHAND);
			DeleteMprgObject(hObj);
			break;
		}

		wstring fn;

		WIN32_FIND_DATAW findd{};
		HANDLE hFind = FindFirstFileW((L"./*"), &findd);
		if (!hFind || hFind == INVALID_HANDLE_VALUE) {
			MessageBoxW(hwnd, LastErrorStrW().c_str(), 0, MB_ICONHAND);
			DeleteMprgObject(hObj);
			break;
		}
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
			}(fn.c_str(), &dllinfo);
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

		DeleteMprgObject(hObj);
	}
		break;

	case WM_COMMAND:
	{
		// wParam�ĵ�λ�ְ����˿ؼ�ID����λ�ְ�����֪ͨ����  
		int id = LOWORD(wp);
		int code = HIWORD(wp);

		// ���ݿؼ�ID��֪ͨ���봦��ͬ����Ϣ  
		switch (id) {
		case IDCANCEL:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		case IDOK:
			if (code == BN_CLICKED) {
				int nSel = ListView_GetSelectionMark(data->hList1);
				if (nSel < 0) {
					MessageBoxW(hwnd, DoesUserUsesChinese() ? L"��ѡ��Ҫ�򿪵���Ŀ" :
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
			// δ֪�Ŀؼ�ID�����Ե���Ĭ�ϴ����ʲô������  
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
			.max = size_t(-1) });
		if (!hWiz) return AssertEx(hWiz);
		OpenMprgWizard(hWiz);

		SetMprgWizardText(hWiz, L"Processing...");

		HDROP hDrop = (HDROP)wp;
		wchar_t szFilePath[2048]{};
		int fileCount = DragQueryFile(hDrop, (UINT)-1, NULL, 0); // ��ȡ�Ϸ��ļ�������  

		auto pd = GetModifiableMprgWizardData(hWiz);
		pd->max = fileCount;
		UpdateMprgWizard(hWiz);
		
		wstring sfc = to_wstring(fileCount), wcsTemp;
		EnableWindow(hwnd, FALSE);

		// ���������Ϸŵ��ļ�  
		for (int i = 0; i < fileCount; ++i) {
			DragQueryFile(hDrop, i, szFilePath, 2048); // ��ȡ�ļ�·��  
			wcsTemp = (L"[" + to_wstring(i) + L"/" + sfc
				+ L"] Adding " + szFilePath);
			SetMprgWizardText(hWiz, wcsTemp.c_str());
			AddTool(hwnd, szFilePath);
			SetMprgWizardValue(hWiz, i + 1);
		}

		// �ͷ��Ϸ��ļ��ṹռ�õ��ڴ�  
		DragFinish(hDrop);

		PostMessage(hwnd, WM_USER + 0xf1, 0, 0);
		EnableWindow(hwnd, TRUE);
		MessageBoxTimeoutW(GetMprgHwnd(hWiz), DoesUserUsesChinese() ?
			(L"�ɹ������ " + sfc + L" �����ߡ�").c_str() :
			(L"Successfully added " + sfc + L" tools.").c_str(),
			L"Success", MB_ICONINFORMATION, 0, 1000);
		DeleteMprgObject(hObj);
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
	wstring newFileName = GenerateUUIDW() + L".MTL.dll";
	CopyFileW(lpTool, newFileName.c_str(), FALSE);
}

void LaunchAppInstance(HWND hwnd, WndDataP_MainWnd data, int nSel) {
	wchar_t szFilename[2048]{};
	ListView_GetItemText(data->hList1, nSel, 1, szFilename, 2048);
	if (!szFilename[0]) {
		MessageBoxW(hwnd, DoesUserUsesChinese() ? L"��ȡ����ʱ���ִ���" :
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

	if (!CreateProcessW(GetProgramDirW().c_str(), szFilename, 0, 0, 0,
		CREATE_SUSPENDED, 0, 0, &si, &pi))
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

		if (dwExitCode != 0) {
			MessageBoxW(t->hwnd, ErrorCodeToStringW(dwExitCode).c_str(),
				0, MB_ICONERROR);
		}
		free(t);
		return dwExitCode;
	}, t, 0, 0));
}




// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "../../resource/tool.h"
#include "../progress_wizard/wizard.user.h"
#include <CommCtrl.h>

HMODULE hInst;
#pragma comment(lib, "../progress_wizard/MyProgressWizardLib64.lib")

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		InitMprgComponent();
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
		L"更改视频编码器" : L"Change video codec");
	//CreateThread(0, 0, 0, 0, 0, 0);//Testesteest
    return info;
}




#include <uxtheme.h>
#include "../info.h"
using namespace std;



#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "UxTheme.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


static LRESULT CALLBACK WndProc_MainWnd(HWND hwnd, UINT message, WPARAM wp, LPARAM lp);

typedef struct {
	HWND hwndRoot;
	HWND 
		hList1,
		hText1, hEdit1, hCombo1, 
		hCheck1, hCheck2,
		hBtnRemove, hBtnOk, hBtnCancel;
} WndData_MainWnd, * WndDataP_MainWnd;


static HFONT ghFont;
constexpr auto MTL_MainWndClass = L"Window://deploy23.objects.prod."
"b534fabd76fe449bac3916f0e2881305.app/mtl-app/cls/VideoChangeCodec-App";



int UiMain(CmdLineW& cl) {
	INITCOMMONCONTROLSEX icce{};
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_ALL_CLASSES;
	{void(0); }
	if (!InitCommonControlsEx(&icce)) {
		return GetLastError();
	}

	HICON hIcon = NULL;
	HBRUSH hBg = CreateSolidBrush(RGB(0xF0, 0xF0, 0xF0));
	ghFont = CreateFontW(-14, -7, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_DONTCARE,
		L"Consolas");
	s7::MyRegisterClassW(MTL_MainWndClass, WndProc_MainWnd, WNDCLASSEXW{
		.hIcon = hIcon,
		.hbrBackground = hBg,
		.hIconSm = hIcon,
		});

	// 创建窗口  
	WndData_MainWnd wd{};
	HWND hwnd = CreateWindowExW(0, MTL_MainWndClass,
		GetInfo().szAppName,
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





int __stdcall AppEntry(PCWSTR lpstrCmdLine) {
	CmdLineW cl(lpstrCmdLine);
    return UiMain(cl);
}


// 窗口过程函数  
#include <shellapi.h>
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

		dat->hwndRoot = hwnd;

#define MYCTLS_VAR_HINST NULL
#define MYCTLS_VAR_HFONT ghFont
#include "../MediaToolsLoader/ctls.h"

		dat->hList1 = custom(L"", WC_LISTVIEW, 0, 0, 1, 1,
			LVS_REPORT | WS_BORDER);
		dat->hText1 = text(DoesUserUsesChinese() ?
			L"输出文件夹 (&D):" : L"Output &directory:");
		dat->hEdit1 = edit();
		dat->hCombo1 = custom(L"mp3", WC_COMBOBOXW, 0, 0, 1, 1, CBS_DROPDOWN);
		dat->hCheck1 = button(DoesUserUsesChinese() ?
			L"显示输出 (&O)" : L"Sh&ow output", IDYES, 0, 0, 1, 1, BS_AUTOCHECKBOX);
		dat->hCheck2 = button(DoesUserUsesChinese() ?
			L"硬件加速 (&A)" : L"Hardware &accel", IDYES, 0, 0, 1, 1, BS_AUTOCHECKBOX);
		dat->hBtnRemove = button(DoesUserUsesChinese() ?
			L"移除选中 (&R)" : L"&Remove", IDABORT);
		dat->hBtnOk = button(DoesUserUsesChinese() ?
			L"开始 (&S)" : L"&Start", IDOK);
		dat->hBtnCancel = button(DoesUserUsesChinese() ?
			L"退出 (&Q)" : L"&Quit", IDCANCEL);

		SendMessageW(dat->hCheck2, BM_SETCHECK, BST_CHECKED, 0);

		PostMessage(hwnd, WM_USER + 0xf0, 0, 0);


	}
	break;

	case WM_USER + 0xf0:
	{
		if (!data) break;
		SetWindowTheme(data->hList1, L"Explorer", NULL);

		LVCOLUMN lvc{};
		wchar_t sz1[] = L"File Name", sz2[] = L"Progress";
		if (DoesUserUsesChinese()) {
			wcscpy_s(sz1, L"文件名");
			wcscpy_s(sz2, L"进度");
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

		SendMessage(data->hCombo1, CB_ADDSTRING, 0, (LPARAM)L"hevc");
		SendMessage(data->hCombo1, CB_ADDSTRING, 0, (LPARAM)L"h264");
		SendMessage(data->hCombo1, CB_ADDSTRING, 0, (LPARAM)L"av1");
		SendMessage(data->hCombo1, CB_SETCURSEL, 0, 0); // 默认 hevc

		DragAcceptFiles(hwnd, TRUE);

		PostMessage(hwnd, WM_USER + 0xf1, 0, 0);
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

		case IDABORT:
		{
			// 获取选中的项目数量  
			int selectedCount = ListView_GetSelectedCount(data->hList1);
			std::deque<int> itemsToDelete;

			// 获取第一个选中项的索引  
			int nextItem = -1;

			// 循环直到没有更多的选中项  
			while ((nextItem = ListView_GetNextItem(data->hList1,
				nextItem, LVNI_SELECTED)) != -1) {
				// 检查项目是否确实被选中  
				UINT state = ListView_GetItemState(data->hList1,
					nextItem, LVIS_SELECTED);
				if (state & LVIS_SELECTED) {
					// 输出选中项目的序号  
					itemsToDelete.push_front(nextItem);
					++selectedCount;
				}
			}

			// 执行删除
			for (auto& i : itemsToDelete) {
				ListView_DeleteItem(data->hList1, i);
			}
		}
			break;

		case IDOK:
		{
			HWND hw = 0;
			while ((hw = FindWindowExW(hwnd, hw, 0, 0))) {
				EnableWindow(hw, false);
			}

			
			if (!CloseHandleIfOk(CreateThread(0, 0, [](PVOID pdata)->DWORD {
				WndDataP_MainWnd data = (WndDataP_MainWnd)pdata;
				HWND hwnd = data->hwndRoot;

				HMPRGOBJ hObj = CreateMprgObject();
				HMPRGWIZ hWiz = CreateMprgWizard(hObj, MPRG_CREATE_PARAMS{
					.max = (size_t)ListView_GetItemCount(data->hList1),
				});
				OpenMprgWizard(hWiz);

				wchar_t buffer[2048]{}, processing[] = L"Processing", done[] = L"Done";
				wchar_t codec[16]{}, outdir[2048]{};
				SendMessageW(data->hCombo1, WM_GETTEXT, 16, (LPARAM)&codec);
				SendMessageW(data->hEdit1, WM_GETTEXT, 2048, (LPARAM)&outdir);
				if (!codec[0] || !outdir[0]) {
					MessageBoxW(hwnd, (DoesUserUsesChinese() ?
						(codec[0] ? L"未填写输出文件夹!" : L"未填写codec!") :
						(codec[0] ? L"Please input output directory!" :
							L"Please input codec!")), 0, MB_ICONERROR);
					HWND hw = 0;
					while ((hw = FindWindowExW(hwnd, hw, 0, 0))) {
						EnableWindow(hw, true);
					}
					return 87;
				}
				wstring cl, wtmp;
				STARTUPINFOW si{}; PROCESS_INFORMATION pi{};
				si.cb = sizeof(si);
				si.wShowWindow = SW_HIDE;
				si.dwFlags = STARTF_USESHOWWINDOW;
				bool bShowConsole = BST_CHECKED &
					SendMessage(data->hCheck1, BM_GETSTATE, 0, 0);
				if (bShowConsole) {
					AllocConsole();
					HWND hw = GetConsoleWindow();
					HMENU hMenu = GetSystemMenu(hw, false);
					EnableMenuItem(hMenu, SC_CLOSE, MF_GRAYED | MF_DISABLED);
					si.wShowWindow = SW_NORMAL;
				}
				for (int i = 0, l = ListView_GetItemCount(data->hList1); i < l; ++i) {
					ListView_SetItemText(data->hList1, i, 1, processing);
					ListView_GetItemText(data->hList1, i, 0, buffer, 2048);
					
					cl = L"FFmpeg ";
					if (BST_CHECKED & SendMessage(data->hCheck2, BM_GETSTATE, 0, 0))
						cl += L"-hwaccel auto ";
					cl += L"-i \""s + buffer + L"\" -y ";

					cl += L"-vcodec \"";
					cl += codec;
					cl += L"\" ";

					cl += L"\"";
					wtmp = buffer;
					wtmp = wtmp.substr(wtmp.find_last_of(L"\\") + 1);
					size_t extStart = wtmp.find_last_of(L".");
					if (extStart) { // 保留原来的扩展名
						wstring ext = wtmp.substr(extStart);
						wtmp.erase(extStart);
						wtmp += L"-changecodec";
						wtmp += ext;
					}
					else {
						wtmp += L"-changecodec.mp4";
					}
					cl += outdir;
					if (!cl.ends_with(L"\\")) cl += L"\\";
					cl += wtmp;
					cl += L"\"";

					wcscpy_s(buffer, cl.c_str());
					SetMprgWizardText(hWiz, cl.c_str());
					if (i == 0 && l <= 1) 
						SetMprgWizardValue(hWiz, static_cast<size_t>(i) + 1);
					if (bShowConsole) {
						DWORD n = 0;
						WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
							L"\n> ", 2, &n, 0);
						WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
							buffer, (DWORD)wcslen(buffer), &n, 0);
						WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
							L"\n", 1, &n, 0);
					}
					if (!CreateProcessW(NULL, buffer, 0, 0, 0, 
						0, 0, 0, &si, &pi)) {
						if (bShowConsole) {
							GetSystemMenu(GetConsoleWindow(), true);
							FreeConsole();
						}
						if (GetLastError() == ERROR_FILE_NOT_FOUND) {
							MessageBoxW(hwnd, DoesUserUsesChinese() ?
								L"找不到 ffmpeg 文件。请尝试：\n"
								L"- 去 https://ffmpeg.org/ 下载 FFmpeg\n"
								L"- 将ffmpeg.exe放在本程序目录下\n"
								L"- 或将ffmpeg.exe放入系统PATH\n" :
								L"Sorry but we cannot find FFmpeg executable file."
								" Please try to:\n"
								L"- Download FFmpeg on https://ffmpeg.org/\n"
								L"- Put ffmpeg.exe in this program's folder\n"
								L"- or put ffmpeg.exe into PATH\n", 0, MB_ICONHAND);
						} else
						MessageBoxW(hwnd, LastErrorStrW().c_str(), 0, MB_ICONERROR);
						HWND hw = 0;
						while ((hw = FindWindowExW(hwnd, hw, 0, 0))) {
							EnableWindow(hw, true);
						}
						DeleteMprgObject(hObj);
						return -1;
					}
					DWORD code = -1;
					WaitForSingleObject(pi.hProcess, INFINITE);
					GetExitCodeProcess(pi.hProcess, &code);
					CloseHandle(pi.hThread);
					CloseHandle(pi.hProcess);
					if (code != 0) {
						cl = L"Failed - error code " + to_wstring(code);
						wchar_t szBuffer[256]{};
						wcscpy_s(szBuffer, cl.c_str());
						ListView_SetItemText(data->hList1, i, 1, szBuffer);
					}
					else {
						ListView_SetItemText(data->hList1, i, 1, done);
					}
					SetMprgWizardValue(hWiz, static_cast<size_t>(i) + 1);
				}

				DeleteMprgObject(hObj);
				if (bShowConsole) {
					DWORD n = 0;
					WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"\n\nPress "
						"any key to continue...", 30, &n, 0);
					(void)_getch();
					HWND hw = GetConsoleWindow();
					GetSystemMenu(hw, true);
					FreeConsole();

					PostMessage(hw, WM_CLOSE, 0, 0);
				}
				HWND hw = 0;
				while ((hw = FindWindowExW(hwnd, hw, 0, 0))) {
					EnableWindow(hw, true);
				}
				return 0;
			}, data, 0, 0))) {
				MessageBoxW(hwnd, LastErrorStrW().c_str(), 0, MB_ICONERROR);
				HWND hw = 0;
				while ((hw = FindWindowExW(hwnd, hw, 0, 0))) {
					EnableWindow(hw, true);
				}
			}

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
		HDROP hDrop = (HDROP)wp;
		wchar_t szFilePath[2048]{};
		int fileCount = DragQueryFile(hDrop, (UINT)-1, NULL, 0); // 获取拖放文件的数量  

		wchar_t wcsOutFolder[5]{}; // 检测“输出文件夹”是否填写
		GetWindowTextW(data->hEdit1, wcsOutFolder, 5);

		LVITEM lvI{};
		// Initialize LVITEM members that are common to all items.
		lvI.pszText = LPSTR_TEXTCALLBACK; // Sends an LVN_GETDISPINFO message.
		lvI.mask = LVIF_TEXT | LVIF_STATE;
		lvI.stateMask = 0;
		lvI.iSubItem = 0;
		lvI.state = 0;

		// 遍历所有拖放的文件  
		for (int i = 0; i < fileCount; ++i) {
			DragQueryFile(hDrop, i, szFilePath, 2048); // 获取文件路径

			if (wcsOutFolder[0] == 0 && i == 0) {
				wstring wcs = szFilePath;
				wcs.erase(wcs.find_last_of(L"\\"));
				SetWindowTextW(data->hEdit1, wcs.c_str());
			}

			lvI.iItem = i;
			lvI.iSubItem = 0;
			lvI.pszText = szFilePath;
			ListView_InsertItem(data->hList1, &lvI);
			lvI.iSubItem = 1;
			szFilePath[0] = 0;
			ListView_SetItem(data->hList1, &lvI);
		}

		DragFinish(hDrop);
	}
	break;

	case WM_SIZE:
	{
		if (!data) break;
		static RECT rc{}; GetClientRect(hwnd, &rc);

		SetWindowPos(data->hList1, 0, 10, 10, rc.right - rc.left - 20,
			rc.bottom - rc.top - 90, SWP_NOACTIVATE);
		SetWindowPos(data->hText1, 0, 10,
			rc.bottom - rc.top - 70, 120, 25, SWP_NOACTIVATE);
		SetWindowPos(data->hEdit1, 0, 140,
			rc.bottom - rc.top - 70, rc.right - rc.left - 260, 25, SWP_NOACTIVATE);
		SetWindowPos(data->hCombo1, 0, rc.right - rc.left - 110,
			rc.bottom - rc.top - 70, 100, 25, SWP_NOACTIVATE);
		SetWindowPos(data->hCheck1, 0, 10,
			rc.bottom - rc.top - 40, 120, 30, SWP_NOACTIVATE);
		SetWindowPos(data->hCheck2, 0, 140,
			rc.bottom - rc.top - 40, 120, 30, SWP_NOACTIVATE);
		SetWindowPos(data->hBtnRemove, 0, rc.right - rc.left - 320,
			rc.bottom - rc.top - 40, 130, 30, SWP_NOACTIVATE);
		SetWindowPos(data->hBtnOk, 0, rc.right - rc.left - 180,
			rc.bottom - rc.top - 40, 80, 30, SWP_NOACTIVATE);
		SetWindowPos(data->hBtnCancel, 0, rc.right - rc.left - 90,
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



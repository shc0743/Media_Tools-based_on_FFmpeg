// : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

#include "../../resource/tool.h"
#include "ui.main.h"
#include "app.dllapp.h"
using namespace std;




int main()
{
	CmdLineW cl(GetCommandLineW());

	wstring type; cl.getopt(L"type", type);
	
	

	if (type == L"ui") {
		return UiMain(cl);
	}

	if (type == L"app") {
		wstring appType; cl.getopt(L"app-type", appType);
		if (appType == L"dll") {
			return DllAppMain(cl);
		}
		return ERROR_INVALID_PARAMETER;
	}

	if (cl.argc() > 1) {
		const auto& filename = cl[1];
		if (file_exists(filename)) {
			if (filename.ends_with(L".MTL.dll")) {
				return (int)Process.StartAndWait(L"\"" + GetProgramDirW() +
					L"\" --type=app --app-type=dll --dll-host-type="
					"default --dll-file=\"" + filename + L"\" ");
			}
		}
		return ERROR_INVALID_PARAMETER;
	}

	if (type.empty() && cl.argc() < 2) {
		return (int)Process.StartAndWait(L"\"" + GetProgramDirW() + L"\" --type=ui");
	}

	return ERROR_INVALID_PARAMETER;
}













// : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

#include "../../resource/tool.h"
#include "ui.main.h"
using namespace std;




int main()
{
	CmdLineW cl(GetCommandLineW());

	wstring type; cl.getopt(L"type", type);
	


	if (type == L"ui") {
		return UiMain(cl);
	}

	if (type.empty() && cl.argc() < 2) {
		return (int)Process.StartAndWait(L"\"" + GetProgramDirW() + L"\" --type=ui");
	}

	return ERROR_INVALID_PARAMETER;
}













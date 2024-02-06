#pragma once
#include <Windows.h>
#include "../../resource/tool.h"

int UiMain(CmdLineW& cl);

constexpr auto MTL_MainWndClass = L"Window://"
#ifdef _DEBUG
"dev"
#else
"prod"
#endif
".b534fabd76fe449bac3916f0e2881305.app/mtl/cls/MainWindow";




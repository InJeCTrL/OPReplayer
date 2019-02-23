#include<Windows.h>
#include<Shlwapi.h>
#include<stdio.h>
#pragma comment(lib,"Shlwapi.lib")

#define EMSGBuffer_Max		100000//录制回放缓冲区最大大小
#define WM_OPRERROR			(WM_USER + 1)//录制/回放异常

//可交互的控件ID
#define ID_StRec			3301//开始/停止录制
#define ID_StRep			3302//开始/停止回放
#define ID_LoadFile			3303//加载回放文件
#define	ID_SaveFile			3304//保存回放文件
#define ID_HKSet_Rec		3305//设置开始/停止录制的热键
#define ID_HKSet_Rep		3306//设置开始/停止回放的热键
#define ID_RecResize		3307//录制时最小化选项
#define ID_RepResize		3308//回放时最小化选项
#define ID_UstaVal			3309//当前状态值
#define ID_MemstaVal		3310//内存状态值
#define ID_URL				3311//主页显示

//热键ID
#define HotKey_Rec			1066//录制热键
#define HotKey_Rep			1067//回放热键

//当前状态
#define Status_None			2018//不改变状态
#define Status_Idle			2019//当前：空闲
#define Status_Recording	2020//当前：正在录制
#define Status_Replaying	2021//当前：正在回放
#define Status_Empty		2022//内存：空
#define Status_LoadF		2023//内存：加载文件
#define Status_NoLoad		2024//内存：暂存录制

//各控件句柄
HWND hMainWND;			//主窗体
HWND hFont;				//统一字体

HWND hBtn_Rec;			//录制按钮
HWND hBtn_Rep;			//回放按钮
HWND hBtn_Load;			//加载按钮
HWND hBtn_Save;			//保存按钮

HWND hText_TipHKSet;	//"热键设置："
HWND hText_HK1;			//"开始/停止录制："
HWND hText_HK2;			//"开始/停止回放："
HWND hCombo_HK1;		//录制热键设置
HWND hCombo_HK2;		//回放热键设置

HWND hBtn_MinRec;		//录制时最小化
HWND hBtn_MinRep;		//回放时最小化

HWND hText_TipU;		//"当前状态："
HWND hText_USta;		//当前状态显示
HWND hText_TipMem;		//"内存状态："
HWND hText_MemSta;		//内存状态显示

HWND hSepLine;			//分割线
HWND hURL;				//主页链接

//其它全局变量
TCHAR str_HKList[12][4] = { L"F1",L"F2",L"F3",L"F4",L"F5",L"F6",L"F7",L"F8",L"F9",L"F10",L"F11",L"F12" };//备选热键列表
INT HKRec_Flag = 0, HKRep_Flag = 0;//录制、回放热键是否设置

TCHAR FilePath[4096] = { 0 };//被加载的回放文件路径
TCHAR FileName[4096] = { 0 };//被加载的回放文件名

INT USta = 0;//当前状态 0：空闲 1：录制中 2：回放中
INT MemSta = 0;//内存状态 0：空 1：已装载 2：已暂存

EVENTMSG EMG[EMSGBuffer_Max] = { { 0 } };//录制的键鼠动作
INT nEMG = 0;//键鼠动作数组长度（0开始）
INT pEMG = 0;//键鼠动作数组当前位置

HHOOK HHRecord = NULL, HHReplay = NULL;//录制钩子、回放钩子

//取消录制热键
INT UnHKRec(HWND Mwnd)
{
	UnregisterHotKey(Mwnd, HotKey_Rec);
	HKRec_Flag = 0;
	return 0;
}

//取消回放热键
INT UnHKRep(HWND Mwnd)
{
	UnregisterHotKey(Mwnd, HotKey_Rep);
	HKRep_Flag = 0;
	return 0;
}

//保存文件名
INT _GetFileName(void)
{
	memset(FileName, 0, 4096);
	lstrcpy(FileName, FilePath);
	PathStripPath(FileName);//获取单独的文件名
	
	return 0;
}

//清空内存的回放缓冲
INT ClearRecMem(void)
{
	memset(EMG, 0, EMSGBuffer_Max);
	return 0;
}

//设置状态
INT SetStatusTip(INT UStatus, INT MemStatus)
{
	TCHAR tMemSta[1024] = L"已加载回放文件 ";

	//设置当前状态
	switch (UStatus)
	{
	case Status_Idle:
		SendMessage(hText_USta, WM_SETTEXT, 0, L"空闲");
		break;
	case Status_Recording:
		SendMessage(hText_USta, WM_SETTEXT, 0, L"正在录制");
		break;
	case Status_Replaying:
		SendMessage(hText_USta, WM_SETTEXT, 0, L"正在回放");
		break;
	case Status_None:
	default:
		break;
	}
	if (UStatus != Status_None)
		USta = UStatus - Status_Idle;

	//设置内存状态
	switch (MemStatus)
	{
	case Status_Empty:
		SendMessage(hText_MemSta, WM_SETTEXT, 0, L"空");
		break;
	case Status_LoadF:
		SendMessage(hText_MemSta, WM_SETTEXT, 0, lstrcat(tMemSta, FileName));
		break;
	case Status_NoLoad:
		SendMessage(hText_MemSta, WM_SETTEXT, 0, L"已暂存最近一次录制（未保存）");
		break;
	case Status_None:
	default:
		break;
	}
	if (MemStatus != Status_None)
		MemSta = MemStatus - Status_Empty;

	return 0;
}

//设置各控件使能性、显示文本(除状态)
//change: 0:开始录制 1:停止录制 2：开始回放 3：停止回放
INT SetCTL(INT change)
{
	switch (change)
	{
	case 0:
		SendMessage(hBtn_Rec, WM_SETTEXT, 0, L"停止录制");
		EnableWindow(hBtn_Rep, FALSE);
		break;
	case 1:
		SendMessage(hBtn_Rec, WM_SETTEXT, 0, L"开始录制");
		if (MemSta)
			EnableWindow(hBtn_Rep, TRUE);//内存中有录制信息
		break;
	case 2:
		SendMessage(hBtn_Rep, WM_SETTEXT, 0, L"停止回放");
		EnableWindow(hBtn_Rec, FALSE);
		break;
	case 3:
		SendMessage(hBtn_Rep, WM_SETTEXT, 0, L"开始回放");
		EnableWindow(hBtn_Rec, TRUE);
		break;
	default:
		break;
	}
	if (change == 0 || change == 2)
	{
		EnableWindow(hCombo_HK1, FALSE);
		EnableWindow(hCombo_HK2, FALSE);
		EnableWindow(hBtn_MinRec, FALSE);
		EnableWindow(hBtn_MinRep, FALSE);
		EnableWindow(hBtn_Load, FALSE);
		EnableWindow(hBtn_Save, FALSE);
	}
	else
	{
		EnableWindow(hCombo_HK1, TRUE);
		EnableWindow(hCombo_HK2, TRUE);
		EnableWindow(hBtn_MinRec, TRUE);
		EnableWindow(hBtn_MinRep, TRUE);
		EnableWindow(hBtn_Load, TRUE);
		if (MemSta == 2)
			EnableWindow(hBtn_Save, TRUE);//内存中有暂存录制信息可以保存
		else
			EnableWindow(hBtn_Save, FALSE);//内存中无暂存录制信息不能保存
	}

	return 0;
}

//键鼠操作录制处理过程
LRESULT CALLBACK OPReRecordProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	HOOKPROC ret = 0;

	if (nCode < 0)
		ret = (HOOKPROC)CallNextHookEx(HHRecord, nCode, wParam, lParam);
	else if (nCode == HC_ACTION)
	{
		EMG[nEMG++] = *(PEVENTMSG)lParam;
	}
	else
		return ret;
	if (nEMG == EMSGBuffer_Max - 100)
	{
		CatchRecord();//结束录制
	}
	return ret;
}

//键鼠操作回放处理过程
LRESULT CALLBACK OPReReplayProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	HOOKPROC ret = 0;

	if (pEMG > nEMG)
	{
		CatchReplay();
	}
	if (nCode < 0)
		ret = (HOOKPROC)CallNextHookEx(HHReplay, nCode, wParam, lParam);
	else if (nCode == HC_GETNEXT)
	{
		*(PEVENTMSG)lParam = EMG[pEMG++];
	}
	else
		return ret;

	Sleep(10);
	
	return ret;
}

//开始录制
INT StartRecord(void)
{
	HHRecord = SetWindowsHookEx(WH_JOURNALRECORD, (HOOKPROC)&OPReRecordProc, GetModuleHandle(NULL), 0);
	nEMG = 0;

	if (!HHRecord)
	{
		MessageBox(hMainWND, L"JournalRecord挂钩失败！", L"无法录制", NULL);
		SendMessage(hMainWND, WM_OPRERROR, NULL, NULL);//发送录制异常消息
	}

	return 0;
}

//停止录制
INT StopRecord(void)
{
	UnhookWindowsHookEx(HHRecord);
	return 0;
}

//开始回放
INT StartReplay(void)
{
	HHReplay = SetWindowsHookEx(WH_JOURNALPLAYBACK, (HOOKPROC)&OPReReplayProc, GetModuleHandle(NULL), 0);
	pEMG = 0;

	if (!HHReplay)
	{
		MessageBox(hMainWND, L"JournalReplay挂钩失败！", L"无法回放", NULL);
		SendMessage(hMainWND, WM_OPRERROR, NULL, NULL);//发送录制异常消息
	}

	return 0;
}

//停止回放
INT StopReplay(void)
{
	UnhookWindowsHookEx(HHReplay);
	return 0;
}

//捕获到触发录制
INT CatchRecord(void)
{
	//开始录制
	if (!USta)
	{
		//开始录制
		SetStatusTip(Status_Recording, Status_Empty);
		SetCTL(0);
		if (SendMessage(hBtn_MinRec, BM_GETCHECK, 0, 0) == BST_CHECKED)
			ShowWindow(hMainWND, SW_SHOWMINIMIZED);//录制时最小化
		StartRecord();
	}
	//停止录制
	else
	{
		StopRecord();
		SetStatusTip(Status_Idle, Status_NoLoad);
		SetCTL(1);
		if (SendMessage(hBtn_MinRec, BM_GETCHECK, 0, 0) == BST_CHECKED)
			ShowWindow(hMainWND, SW_SHOWDEFAULT);//录制结束正常显示
	}
	return 0;
}

//捕获到触发回放
INT CatchReplay(void)
{
	//开始回放
	if (!USta)
	{
		SetStatusTip(Status_Replaying, Status_None);
		SetCTL(2);
		if (SendMessage(hBtn_MinRep, BM_GETCHECK, 0, 0) == BST_CHECKED)
			ShowWindow(hMainWND, SW_SHOWMINIMIZED);//回放时最小化
		StartReplay();
	}
	//停止回放
	else
	{
		StopReplay();
		SetStatusTip(Status_Idle, Status_None);
		SetCTL(3);
		if (SendMessage(hBtn_MinRep, BM_GETCHECK, 0, 0) == BST_CHECKED)
			ShowWindow(hMainWND, SW_SHOWDEFAULT);//回放结束正常显示
	}

}

LRESULT WINAPI CtlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT t;
	OPENFILENAME OF = { 0 };//打开文件结构
	INT HK_Index = 0;//选定的热键编号
	FILE *FP = NULL;//文件指针

	//设置文件打开结构
	OF.lStructSize = sizeof(OPENFILENAME);
	OF.hwndOwner = hWnd;
	OF.lpstrFilter = L"OPReplayer键鼠操作回放文件(*.OPRe)\0*.OPRe\0\0";
	OF.lpstrFile = FilePath;
	OF.nMaxFile = 4096;
	OF.lpstrTitle = L"选择OPRe键鼠操作回放文件";
	OF.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;

	switch (message)
	{
	case WM_CREATE:
		hFont = CreateFont(-14, -7, 0, 0, 400, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, TEXT("微软雅黑"));
		hBtn_Rec = CreateWindow(L"Button", L"开始录制", WS_CHILD | WS_VISIBLE | WS_BORDER, 15, 10, 310, 30, hWnd, (HMENU)ID_StRec, hWnd, 0);
		hBtn_Rep = CreateWindow(L"Button", L"开始回放", WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_BORDER, 15, 50, 310, 30, hWnd, (HMENU)ID_StRep, hWnd, 0);
		hBtn_Load = CreateWindow(L"Button", L"加载回放文件", WS_CHILD | WS_VISIBLE | WS_BORDER, 15, 90, 310, 30, hWnd, (HMENU)ID_LoadFile, hWnd, 0);
		hBtn_Save = CreateWindow(L"Button", L"保存最近一次录制到文件", WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_BORDER, 15, 130, 310, 30, hWnd, (HMENU)ID_SaveFile, hWnd, 0);
		hText_TipHKSet = CreateWindow(L"Static", L"热键设置：", WS_CHILD | WS_VISIBLE, 15, 170, 300, 100, hWnd, NULL, hWnd, 0);
		hText_HK1 = CreateWindow(L"Static", L"开始/停止录制：Ctrl+Alt+Shift+", WS_CHILD | WS_VISIBLE, 45, 195, 300, 100, hWnd, NULL, hWnd, 0);
		hCombo_HK1 = CreateWindow(L"ComboBox", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 260, 192, 60, 500, hWnd, (HMENU)ID_HKSet_Rec, hWnd, 0);
		hText_HK2 = CreateWindow(L"Static", L"开始/停止回放：Ctrl+Alt+Shift+", WS_CHILD | WS_VISIBLE, 45, 222, 300, 100, hWnd, NULL, hWnd, 0);
		hCombo_HK2 = CreateWindow(L"ComboBox", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 260, 222, 60, 500, hWnd, (HMENU)ID_HKSet_Rep, hWnd, 0);
		hBtn_MinRec = CreateWindow(L"Button", L"录制时最小化", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_AUTOCHECKBOX, 45, 250, 120, 30, hWnd, (HMENU)ID_RecResize, hWnd, 0);
		hBtn_MinRep = CreateWindow(L"Button", L"回放时最小化", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_AUTOCHECKBOX, 180, 250, 120, 30, hWnd, (HMENU)ID_RepResize, hWnd, 0);
		hText_TipU = CreateWindow(L"Static", L"当前状态：", WS_CHILD | WS_VISIBLE | ES_CENTER, 20, 280, 80, 25, hWnd, NULL, hWnd, 0);
		hText_USta = CreateWindow(L"Static", L"空闲", WS_CHILD | WS_VISIBLE | ES_CENTER, 115, 280, 200, 25, hWnd, (HMENU)ID_UstaVal, hWnd, 0);
		hText_TipMem = CreateWindow(L"Static", L"内存状态：", WS_CHILD | WS_VISIBLE | ES_CENTER, 20, 300, 80, 25, hWnd, NULL, hWnd, 0);
		hText_MemSta = CreateWindow(L"Static", L"空", WS_CHILD | WS_VISIBLE | ES_CENTER, 115, 300, 200, 25, hWnd, (HMENU)ID_MemstaVal, hWnd, 0);
		hSepLine = CreateWindow(L"Static", L"", SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 5, 333, 333, 10, hWnd, NULL, hWnd, 0);
		hURL = CreateWindow(L"Edit", L"软件主页：injectrl.coding.me/OPReplayer", ES_READONLY | WM_NOTIFY | ES_CENTER | WS_CHILD | WS_VISIBLE, 13, 340, 320, 20, hWnd, (HMENU)ID_URL, hWnd, 0);
		
		//设置各控件字体
		SendMessage(hBtn_Rec, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hBtn_Rep, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hBtn_Load, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hBtn_Save, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hText_TipHKSet, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hText_HK1, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hCombo_HK1, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hText_HK2, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hCombo_HK2, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hBtn_MinRec, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hBtn_MinRep, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hText_TipU, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hText_USta, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hText_TipMem, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hText_MemSta, WM_SETFONT, (WPARAM)hFont, NULL);
		SendMessage(hURL, WM_SETFONT, (WPARAM)hFont, NULL);

		//向热键设置combo添加选项
		for (t = 0; t < 12; t++)
		{
			SendMessage(hCombo_HK1, CB_ADDSTRING, 0, str_HKList[t]);
			SendMessage(hCombo_HK2, CB_ADDSTRING, 0, str_HKList[t]);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_URL://单击主页地址
			if (HIWORD(wParam) == WM_KEYDOWN)
				ShellExecute(NULL, L"open", L"http://injectrl.coding.me/OPReplayer", NULL, NULL, SW_SHOWNORMAL);//打开主页
			break;
		case ID_StRec://开始/停止录制
			CatchRecord();
			break;
		case ID_StRep://开始/停止回放
			CatchReplay();
			break;
		case ID_LoadFile://加载回放文件
			//找到文件成功，本次加载文件，提取文件路径、文件名
			if (GetOpenFileName(&OF))
			{
				if (MemSta == 2)
				{
					if (MessageBox(hWnd, L"当前内存中已暂存最近一次未保存的键鼠录制。\n是：用即将加载的回放文件覆盖它\n否：取消加载回放文件", L"OPReplayer", MB_YESNO) != IDYES)
						break;
				}
				if (MemSta)
					ClearRecMem();
				_GetFileName();
				FP = _wfopen(FilePath, L"rb");//只读方式打开文件
				//打开文件失败
				if (!FP)
				{
					MessageBox(hWnd, L"无法打开文件！", L"加载回放文件失败", MB_ICONERROR);
					break;
				}
				//打开文件成功，加载回放文件到内存
				t = 0;
				while (t < EMSGBuffer_Max && (fread(EMG + t, sizeof(EVENTMSG), 1, FP)) > 0)
				{
					t++;
				}
				nEMG = t - 1;

				while ((t = fread(EMG, sizeof(EVENTMSG), 1, FP)) > 0);
				fclose(FP);
				SetStatusTip(Status_None, Status_LoadF);//内存状态切换为已加载文件
				SetCTL(1);
			}
			break;
		case ID_SaveFile://保存录制文件
			OF.lpstrTitle = L"保存OPRe键鼠操作回放文件到...";
			if (GetSaveFileName(&OF))
			{
				FP = _wfopen(FilePath, L"wb");//只写方式打开文件
				//打开文件失败
				if (!FP)
				{
					MessageBox(hWnd, L"无法打开文件！", L"保存回放文件失败", MB_ICONERROR);
					break;
				}
				//打开文件成功，保存内存录制记录到文件
				t = 0;
				while (t <= nEMG)
				{
					fwrite(EMG + t, sizeof(EVENTMSG), 1, FP);
					t++;
				}
				fclose(FP);
				_GetFileName();
				SetStatusTip(Status_None, Status_LoadF);//内存状态切换为已加载文件
			}
			break;
		case ID_HKSet_Rec://设置录制热键
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if ((HK_Index = SendMessage(hCombo_HK1, CB_GETCURSEL, 0, 0)) != -1)
				{
					if (HKRec_Flag)
						UnHKRec(hWnd);//取消旧热键
					if (!RegisterHotKey(hWnd, HotKey_Rec, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_F1 + HK_Index))
					{
						MessageBox(hWnd, L"录制热键注册失败，请尝试其他热键！", L"热键设置失败", MB_ICONERROR);
						SendMessage(hCombo_HK1, CB_SETCURSEL, -1, 0);
						break;
					}
					HKRec_Flag = 1;
				}
			}
			break;
		case ID_HKSet_Rep://设置回放热键
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if ((HK_Index = SendMessage(hCombo_HK2, CB_GETCURSEL, 0, 0)) != -1)
				{
					if (HKRep_Flag)
						UnHKRep(hWnd);//取消旧热键
					if (!RegisterHotKey(hWnd, HotKey_Rep, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_F1 + HK_Index))
					{
						MessageBox(hWnd, L"回放热键注册失败，请尝试其他热键！", L"热键设置失败", MB_ICONERROR);
						SendMessage(hCombo_HK2, CB_SETCURSEL, -1, 0);
						break;
					}
					HKRep_Flag = 1;
				}
			}
			break;
		default:
			break;
		}
		break;
	case WM_OPRERROR://录制/回放异常
		SetStatusTip(Status_Idle, Status_None);
		//录制异常
		if (USta == 1)
		{
			SetCTL(1);
			if (SendMessage(hBtn_MinRec, BM_GETCHECK, 0, 0) == BST_CHECKED)
				ShowWindow(hMainWND, SW_SHOWDEFAULT);//录制结束正常显示
		}
		//回放异常
		if (USta == 2)
		{
			SetCTL(3);
			if (SendMessage(hBtn_MinRep, BM_GETCHECK, 0, 0) == BST_CHECKED)
				ShowWindow(hMainWND, SW_SHOWDEFAULT);//回放结束正常显示
		}
		break;
	case WM_DESTROY:
		//取消旧热键
		if (HKRec_Flag)
			UnHKRec(hWnd);
		if (HKRep_Flag)
			UnHKRep(hWnd);
		DeleteObject(hFont);//销毁字体
		PostQuitMessage(0);
		break;
	case WM_HOTKEY:
		if (LOWORD(wParam) == HotKey_Rec && USta != 2)
			CatchRecord();
		else if (LOWORD(wParam) == HotKey_Rep && USta != 1 && MemSta)
			CatchReplay();
		else
			break;
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASSEX WC;//窗体类
	INT ScreenWidth, ScreenHeight;//屏幕宽度、高度

	WC.cbSize = sizeof(WNDCLASSEX);
	WC.style = CS_HREDRAW | CS_VREDRAW;
	WC.lpfnWndProc = CtlProc;
	WC.cbClsExtra = 0;
	WC.cbWndExtra = 0;
	WC.hInstance = hInstance;
	WC.hIcon = 0;
	WC.hCursor = 0;
	WC.hbrBackground = (HBRUSH)GetSysColorBrush(COLOR_BTNFACE);
	WC.lpszMenuName = 0;
	WC.lpszClassName = L"WND";
	WC.hIconSm = 0;

	RegisterClassEx(&WC);
	ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
	hMainWND = CreateWindow(L"WND", L"OPReplayer键鼠操作录制回放器", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, ScreenWidth / 2 - 178, ScreenHeight / 2 - 200, 356, 400, NULL, 0, 0, 0);
	ShowWindow(hMainWND, 1);
	UpdateWindow(hMainWND);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
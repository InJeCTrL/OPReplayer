#include<Windows.h>
#include<Shlwapi.h>
#include<stdio.h>
#pragma comment(lib,"Shlwapi.lib")

#define EMSGBuffer_Max		100000//¼�ƻطŻ���������С
#define WM_OPRERROR			(WM_USER + 1)//¼��/�ط��쳣

//�ɽ����Ŀؼ�ID
#define ID_StRec			3301//��ʼ/ֹͣ¼��
#define ID_StRep			3302//��ʼ/ֹͣ�ط�
#define ID_LoadFile			3303//���ػط��ļ�
#define	ID_SaveFile			3304//����ط��ļ�
#define ID_HKSet_Rec		3305//���ÿ�ʼ/ֹͣ¼�Ƶ��ȼ�
#define ID_HKSet_Rep		3306//���ÿ�ʼ/ֹͣ�طŵ��ȼ�
#define ID_RecResize		3307//¼��ʱ��С��ѡ��
#define ID_RepResize		3308//�ط�ʱ��С��ѡ��
#define ID_UstaVal			3309//��ǰ״ֵ̬
#define ID_MemstaVal		3310//�ڴ�״ֵ̬
#define ID_URL				3311//��ҳ��ʾ

//�ȼ�ID
#define HotKey_Rec			1066//¼���ȼ�
#define HotKey_Rep			1067//�ط��ȼ�

//��ǰ״̬
#define Status_None			2018//���ı�״̬
#define Status_Idle			2019//��ǰ������
#define Status_Recording	2020//��ǰ������¼��
#define Status_Replaying	2021//��ǰ�����ڻط�
#define Status_Empty		2022//�ڴ棺��
#define Status_LoadF		2023//�ڴ棺�����ļ�
#define Status_NoLoad		2024//�ڴ棺�ݴ�¼��

//���ؼ����
HWND hMainWND;			//������
HWND hFont;				//ͳһ����

HWND hBtn_Rec;			//¼�ư�ť
HWND hBtn_Rep;			//�طŰ�ť
HWND hBtn_Load;			//���ذ�ť
HWND hBtn_Save;			//���水ť

HWND hText_TipHKSet;	//"�ȼ����ã�"
HWND hText_HK1;			//"��ʼ/ֹͣ¼�ƣ�"
HWND hText_HK2;			//"��ʼ/ֹͣ�طţ�"
HWND hCombo_HK1;		//¼���ȼ�����
HWND hCombo_HK2;		//�ط��ȼ�����

HWND hBtn_MinRec;		//¼��ʱ��С��
HWND hBtn_MinRep;		//�ط�ʱ��С��

HWND hText_TipU;		//"��ǰ״̬��"
HWND hText_USta;		//��ǰ״̬��ʾ
HWND hText_TipMem;		//"�ڴ�״̬��"
HWND hText_MemSta;		//�ڴ�״̬��ʾ

HWND hSepLine;			//�ָ���
HWND hURL;				//��ҳ����

//����ȫ�ֱ���
TCHAR str_HKList[12][4] = { L"F1",L"F2",L"F3",L"F4",L"F5",L"F6",L"F7",L"F8",L"F9",L"F10",L"F11",L"F12" };//��ѡ�ȼ��б�
INT HKRec_Flag = 0, HKRep_Flag = 0;//¼�ơ��ط��ȼ��Ƿ�����

TCHAR FilePath[4096] = { 0 };//�����صĻط��ļ�·��
TCHAR FileName[4096] = { 0 };//�����صĻط��ļ���

INT USta = 0;//��ǰ״̬ 0������ 1��¼���� 2���ط���
INT MemSta = 0;//�ڴ�״̬ 0���� 1����װ�� 2�����ݴ�

EVENTMSG EMG[EMSGBuffer_Max] = { { 0 } };//¼�Ƶļ�����
INT nEMG = 0;//���������鳤�ȣ�0��ʼ��
INT pEMG = 0;//���������鵱ǰλ��

HHOOK HHRecord = NULL, HHReplay = NULL;//¼�ƹ��ӡ��طŹ���

//ȡ��¼���ȼ�
INT UnHKRec(HWND Mwnd)
{
	UnregisterHotKey(Mwnd, HotKey_Rec);
	HKRec_Flag = 0;
	return 0;
}

//ȡ���ط��ȼ�
INT UnHKRep(HWND Mwnd)
{
	UnregisterHotKey(Mwnd, HotKey_Rep);
	HKRep_Flag = 0;
	return 0;
}

//�����ļ���
INT _GetFileName(void)
{
	memset(FileName, 0, 4096);
	lstrcpy(FileName, FilePath);
	PathStripPath(FileName);//��ȡ�������ļ���
	
	return 0;
}

//����ڴ�ĻطŻ���
INT ClearRecMem(void)
{
	memset(EMG, 0, EMSGBuffer_Max);
	return 0;
}

//����״̬
INT SetStatusTip(INT UStatus, INT MemStatus)
{
	TCHAR tMemSta[1024] = L"�Ѽ��ػط��ļ� ";

	//���õ�ǰ״̬
	switch (UStatus)
	{
	case Status_Idle:
		SendMessage(hText_USta, WM_SETTEXT, 0, L"����");
		break;
	case Status_Recording:
		SendMessage(hText_USta, WM_SETTEXT, 0, L"����¼��");
		break;
	case Status_Replaying:
		SendMessage(hText_USta, WM_SETTEXT, 0, L"���ڻط�");
		break;
	case Status_None:
	default:
		break;
	}
	if (UStatus != Status_None)
		USta = UStatus - Status_Idle;

	//�����ڴ�״̬
	switch (MemStatus)
	{
	case Status_Empty:
		SendMessage(hText_MemSta, WM_SETTEXT, 0, L"��");
		break;
	case Status_LoadF:
		SendMessage(hText_MemSta, WM_SETTEXT, 0, lstrcat(tMemSta, FileName));
		break;
	case Status_NoLoad:
		SendMessage(hText_MemSta, WM_SETTEXT, 0, L"���ݴ����һ��¼�ƣ�δ���棩");
		break;
	case Status_None:
	default:
		break;
	}
	if (MemStatus != Status_None)
		MemSta = MemStatus - Status_Empty;

	return 0;
}

//���ø��ؼ�ʹ���ԡ���ʾ�ı�(��״̬)
//change: 0:��ʼ¼�� 1:ֹͣ¼�� 2����ʼ�ط� 3��ֹͣ�ط�
INT SetCTL(INT change)
{
	switch (change)
	{
	case 0:
		SendMessage(hBtn_Rec, WM_SETTEXT, 0, L"ֹͣ¼��");
		EnableWindow(hBtn_Rep, FALSE);
		break;
	case 1:
		SendMessage(hBtn_Rec, WM_SETTEXT, 0, L"��ʼ¼��");
		if (MemSta)
			EnableWindow(hBtn_Rep, TRUE);//�ڴ�����¼����Ϣ
		break;
	case 2:
		SendMessage(hBtn_Rep, WM_SETTEXT, 0, L"ֹͣ�ط�");
		EnableWindow(hBtn_Rec, FALSE);
		break;
	case 3:
		SendMessage(hBtn_Rep, WM_SETTEXT, 0, L"��ʼ�ط�");
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
			EnableWindow(hBtn_Save, TRUE);//�ڴ������ݴ�¼����Ϣ���Ա���
		else
			EnableWindow(hBtn_Save, FALSE);//�ڴ������ݴ�¼����Ϣ���ܱ���
	}

	return 0;
}

//�������¼�ƴ������
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
		CatchRecord();//����¼��
	}
	return ret;
}

//��������طŴ������
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

//��ʼ¼��
INT StartRecord(void)
{
	HHRecord = SetWindowsHookEx(WH_JOURNALRECORD, (HOOKPROC)&OPReRecordProc, GetModuleHandle(NULL), 0);
	nEMG = 0;

	if (!HHRecord)
	{
		MessageBox(hMainWND, L"JournalRecord�ҹ�ʧ�ܣ�", L"�޷�¼��", NULL);
		SendMessage(hMainWND, WM_OPRERROR, NULL, NULL);//����¼���쳣��Ϣ
	}

	return 0;
}

//ֹͣ¼��
INT StopRecord(void)
{
	UnhookWindowsHookEx(HHRecord);
	return 0;
}

//��ʼ�ط�
INT StartReplay(void)
{
	HHReplay = SetWindowsHookEx(WH_JOURNALPLAYBACK, (HOOKPROC)&OPReReplayProc, GetModuleHandle(NULL), 0);
	pEMG = 0;

	if (!HHReplay)
	{
		MessageBox(hMainWND, L"JournalReplay�ҹ�ʧ�ܣ�", L"�޷��ط�", NULL);
		SendMessage(hMainWND, WM_OPRERROR, NULL, NULL);//����¼���쳣��Ϣ
	}

	return 0;
}

//ֹͣ�ط�
INT StopReplay(void)
{
	UnhookWindowsHookEx(HHReplay);
	return 0;
}

//���񵽴���¼��
INT CatchRecord(void)
{
	//��ʼ¼��
	if (!USta)
	{
		//��ʼ¼��
		SetStatusTip(Status_Recording, Status_Empty);
		SetCTL(0);
		if (SendMessage(hBtn_MinRec, BM_GETCHECK, 0, 0) == BST_CHECKED)
			ShowWindow(hMainWND, SW_SHOWMINIMIZED);//¼��ʱ��С��
		StartRecord();
	}
	//ֹͣ¼��
	else
	{
		StopRecord();
		SetStatusTip(Status_Idle, Status_NoLoad);
		SetCTL(1);
		if (SendMessage(hBtn_MinRec, BM_GETCHECK, 0, 0) == BST_CHECKED)
			ShowWindow(hMainWND, SW_SHOWDEFAULT);//¼�ƽ���������ʾ
	}
	return 0;
}

//���񵽴����ط�
INT CatchReplay(void)
{
	//��ʼ�ط�
	if (!USta)
	{
		SetStatusTip(Status_Replaying, Status_None);
		SetCTL(2);
		if (SendMessage(hBtn_MinRep, BM_GETCHECK, 0, 0) == BST_CHECKED)
			ShowWindow(hMainWND, SW_SHOWMINIMIZED);//�ط�ʱ��С��
		StartReplay();
	}
	//ֹͣ�ط�
	else
	{
		StopReplay();
		SetStatusTip(Status_Idle, Status_None);
		SetCTL(3);
		if (SendMessage(hBtn_MinRep, BM_GETCHECK, 0, 0) == BST_CHECKED)
			ShowWindow(hMainWND, SW_SHOWDEFAULT);//�طŽ���������ʾ
	}

}

LRESULT WINAPI CtlProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT t;
	OPENFILENAME OF = { 0 };//���ļ��ṹ
	INT HK_Index = 0;//ѡ�����ȼ����
	FILE *FP = NULL;//�ļ�ָ��

	//�����ļ��򿪽ṹ
	OF.lStructSize = sizeof(OPENFILENAME);
	OF.hwndOwner = hWnd;
	OF.lpstrFilter = L"OPReplayer��������ط��ļ�(*.OPRe)\0*.OPRe\0\0";
	OF.lpstrFile = FilePath;
	OF.nMaxFile = 4096;
	OF.lpstrTitle = L"ѡ��OPRe��������ط��ļ�";
	OF.Flags = OFN_FILEMUSTEXIST | OFN_EXPLORER;

	switch (message)
	{
	case WM_CREATE:
		hFont = CreateFont(-14, -7, 0, 0, 400, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, TEXT("΢���ź�"));
		hBtn_Rec = CreateWindow(L"Button", L"��ʼ¼��", WS_CHILD | WS_VISIBLE | WS_BORDER, 15, 10, 310, 30, hWnd, (HMENU)ID_StRec, hWnd, 0);
		hBtn_Rep = CreateWindow(L"Button", L"��ʼ�ط�", WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_BORDER, 15, 50, 310, 30, hWnd, (HMENU)ID_StRep, hWnd, 0);
		hBtn_Load = CreateWindow(L"Button", L"���ػط��ļ�", WS_CHILD | WS_VISIBLE | WS_BORDER, 15, 90, 310, 30, hWnd, (HMENU)ID_LoadFile, hWnd, 0);
		hBtn_Save = CreateWindow(L"Button", L"�������һ��¼�Ƶ��ļ�", WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_BORDER, 15, 130, 310, 30, hWnd, (HMENU)ID_SaveFile, hWnd, 0);
		hText_TipHKSet = CreateWindow(L"Static", L"�ȼ����ã�", WS_CHILD | WS_VISIBLE, 15, 170, 300, 100, hWnd, NULL, hWnd, 0);
		hText_HK1 = CreateWindow(L"Static", L"��ʼ/ֹͣ¼�ƣ�Ctrl+Alt+Shift+", WS_CHILD | WS_VISIBLE, 45, 195, 300, 100, hWnd, NULL, hWnd, 0);
		hCombo_HK1 = CreateWindow(L"ComboBox", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 260, 192, 60, 500, hWnd, (HMENU)ID_HKSet_Rec, hWnd, 0);
		hText_HK2 = CreateWindow(L"Static", L"��ʼ/ֹͣ�طţ�Ctrl+Alt+Shift+", WS_CHILD | WS_VISIBLE, 45, 222, 300, 100, hWnd, NULL, hWnd, 0);
		hCombo_HK2 = CreateWindow(L"ComboBox", L"", CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE, 260, 222, 60, 500, hWnd, (HMENU)ID_HKSet_Rep, hWnd, 0);
		hBtn_MinRec = CreateWindow(L"Button", L"¼��ʱ��С��", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_AUTOCHECKBOX, 45, 250, 120, 30, hWnd, (HMENU)ID_RecResize, hWnd, 0);
		hBtn_MinRep = CreateWindow(L"Button", L"�ط�ʱ��С��", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_AUTOCHECKBOX, 180, 250, 120, 30, hWnd, (HMENU)ID_RepResize, hWnd, 0);
		hText_TipU = CreateWindow(L"Static", L"��ǰ״̬��", WS_CHILD | WS_VISIBLE | ES_CENTER, 20, 280, 80, 25, hWnd, NULL, hWnd, 0);
		hText_USta = CreateWindow(L"Static", L"����", WS_CHILD | WS_VISIBLE | ES_CENTER, 115, 280, 200, 25, hWnd, (HMENU)ID_UstaVal, hWnd, 0);
		hText_TipMem = CreateWindow(L"Static", L"�ڴ�״̬��", WS_CHILD | WS_VISIBLE | ES_CENTER, 20, 300, 80, 25, hWnd, NULL, hWnd, 0);
		hText_MemSta = CreateWindow(L"Static", L"��", WS_CHILD | WS_VISIBLE | ES_CENTER, 115, 300, 200, 25, hWnd, (HMENU)ID_MemstaVal, hWnd, 0);
		hSepLine = CreateWindow(L"Static", L"", SS_ETCHEDHORZ | WS_CHILD | WS_VISIBLE, 5, 333, 333, 10, hWnd, NULL, hWnd, 0);
		hURL = CreateWindow(L"Edit", L"�����ҳ��injectrl.coding.me/OPReplayer", ES_READONLY | WM_NOTIFY | ES_CENTER | WS_CHILD | WS_VISIBLE, 13, 340, 320, 20, hWnd, (HMENU)ID_URL, hWnd, 0);
		
		//���ø��ؼ�����
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

		//���ȼ�����combo���ѡ��
		for (t = 0; t < 12; t++)
		{
			SendMessage(hCombo_HK1, CB_ADDSTRING, 0, str_HKList[t]);
			SendMessage(hCombo_HK2, CB_ADDSTRING, 0, str_HKList[t]);
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_URL://������ҳ��ַ
			if (HIWORD(wParam) == WM_KEYDOWN)
				ShellExecute(NULL, L"open", L"http://injectrl.coding.me/OPReplayer", NULL, NULL, SW_SHOWNORMAL);//����ҳ
			break;
		case ID_StRec://��ʼ/ֹͣ¼��
			CatchRecord();
			break;
		case ID_StRep://��ʼ/ֹͣ�ط�
			CatchReplay();
			break;
		case ID_LoadFile://���ػط��ļ�
			//�ҵ��ļ��ɹ������μ����ļ�����ȡ�ļ�·�����ļ���
			if (GetOpenFileName(&OF))
			{
				if (MemSta == 2)
				{
					if (MessageBox(hWnd, L"��ǰ�ڴ������ݴ����һ��δ����ļ���¼�ơ�\n�ǣ��ü������صĻط��ļ�������\n��ȡ�����ػط��ļ�", L"OPReplayer", MB_YESNO) != IDYES)
						break;
				}
				if (MemSta)
					ClearRecMem();
				_GetFileName();
				FP = _wfopen(FilePath, L"rb");//ֻ����ʽ���ļ�
				//���ļ�ʧ��
				if (!FP)
				{
					MessageBox(hWnd, L"�޷����ļ���", L"���ػط��ļ�ʧ��", MB_ICONERROR);
					break;
				}
				//���ļ��ɹ������ػط��ļ����ڴ�
				t = 0;
				while (t < EMSGBuffer_Max && (fread(EMG + t, sizeof(EVENTMSG), 1, FP)) > 0)
				{
					t++;
				}
				nEMG = t - 1;

				while ((t = fread(EMG, sizeof(EVENTMSG), 1, FP)) > 0);
				fclose(FP);
				SetStatusTip(Status_None, Status_LoadF);//�ڴ�״̬�л�Ϊ�Ѽ����ļ�
				SetCTL(1);
			}
			break;
		case ID_SaveFile://����¼���ļ�
			OF.lpstrTitle = L"����OPRe��������ط��ļ���...";
			if (GetSaveFileName(&OF))
			{
				FP = _wfopen(FilePath, L"wb");//ֻд��ʽ���ļ�
				//���ļ�ʧ��
				if (!FP)
				{
					MessageBox(hWnd, L"�޷����ļ���", L"����ط��ļ�ʧ��", MB_ICONERROR);
					break;
				}
				//���ļ��ɹ��������ڴ�¼�Ƽ�¼���ļ�
				t = 0;
				while (t <= nEMG)
				{
					fwrite(EMG + t, sizeof(EVENTMSG), 1, FP);
					t++;
				}
				fclose(FP);
				_GetFileName();
				SetStatusTip(Status_None, Status_LoadF);//�ڴ�״̬�л�Ϊ�Ѽ����ļ�
			}
			break;
		case ID_HKSet_Rec://����¼���ȼ�
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if ((HK_Index = SendMessage(hCombo_HK1, CB_GETCURSEL, 0, 0)) != -1)
				{
					if (HKRec_Flag)
						UnHKRec(hWnd);//ȡ�����ȼ�
					if (!RegisterHotKey(hWnd, HotKey_Rec, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_F1 + HK_Index))
					{
						MessageBox(hWnd, L"¼���ȼ�ע��ʧ�ܣ��볢�������ȼ���", L"�ȼ�����ʧ��", MB_ICONERROR);
						SendMessage(hCombo_HK1, CB_SETCURSEL, -1, 0);
						break;
					}
					HKRec_Flag = 1;
				}
			}
			break;
		case ID_HKSet_Rep://���ûط��ȼ�
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				if ((HK_Index = SendMessage(hCombo_HK2, CB_GETCURSEL, 0, 0)) != -1)
				{
					if (HKRep_Flag)
						UnHKRep(hWnd);//ȡ�����ȼ�
					if (!RegisterHotKey(hWnd, HotKey_Rep, MOD_CONTROL | MOD_ALT | MOD_SHIFT, VK_F1 + HK_Index))
					{
						MessageBox(hWnd, L"�ط��ȼ�ע��ʧ�ܣ��볢�������ȼ���", L"�ȼ�����ʧ��", MB_ICONERROR);
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
	case WM_OPRERROR://¼��/�ط��쳣
		SetStatusTip(Status_Idle, Status_None);
		//¼���쳣
		if (USta == 1)
		{
			SetCTL(1);
			if (SendMessage(hBtn_MinRec, BM_GETCHECK, 0, 0) == BST_CHECKED)
				ShowWindow(hMainWND, SW_SHOWDEFAULT);//¼�ƽ���������ʾ
		}
		//�ط��쳣
		if (USta == 2)
		{
			SetCTL(3);
			if (SendMessage(hBtn_MinRep, BM_GETCHECK, 0, 0) == BST_CHECKED)
				ShowWindow(hMainWND, SW_SHOWDEFAULT);//�طŽ���������ʾ
		}
		break;
	case WM_DESTROY:
		//ȡ�����ȼ�
		if (HKRec_Flag)
			UnHKRec(hWnd);
		if (HKRep_Flag)
			UnHKRep(hWnd);
		DeleteObject(hFont);//��������
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
	WNDCLASSEX WC;//������
	INT ScreenWidth, ScreenHeight;//��Ļ��ȡ��߶�

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
	hMainWND = CreateWindow(L"WND", L"OPReplayer�������¼�ƻط���", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX, ScreenWidth / 2 - 178, ScreenHeight / 2 - 200, 356, 400, NULL, 0, 0, 0);
	ShowWindow(hMainWND, 1);
	UpdateWindow(hMainWND);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
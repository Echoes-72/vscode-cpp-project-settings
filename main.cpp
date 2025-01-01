#include <ctime>
#include <fstream>
#include <iostream>
#include <vector>
#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>

#include "Include/Calculate.hpp"
#include "Include/Evolution.hpp"
#include "Include/resouse.hpp"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Shell32.lib")

using namespace std;

// constexpr int CUBE = 25;
HRGN InfoRgn;
HDC AppDC;
RECT WindowRect = { 30, 30, 730, 530 }, ClientRect, DataArea = { 0, 0, 150, 0 }, PaintArea;
SIZE MapSize = { 1, 50 };
HWND Dialog;

vector<POINT> AliveCell = vector<POINT>(0);
vector<vector<vector<POINT>>> ChannelGraph;
vector<vector<HttpRequest>> Map        = vector<vector<HttpRequest>>(MapSize.cy, vector<HttpRequest>(MapSize.cx));
vector<vector<float>> CommitmentGraph = vector<vector<float>>(MapSize.cy * MapSize.cx, vector<float>(MapSize.cy *MapSize.cx, 0));

LRESULT CALLBACK WindowProcess(HWND Handle, UINT Msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ModelessProc(HWND Handle, UINT Msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ModleProc(HWND Handle, UINT Msg, WPARAM wParam, LPARAM lParam);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX WebServer;
    TCHAR ClassName[] = TEXT("Cellular_Automata");
    TCHAR AppName[]   = TEXT("Cellular Automata");
    HWND Handle;
    MSG Message;

    //*注册窗口类
    WebServer.cbSize        = sizeof(WNDCLASSEX);
    WebServer.style         = CS_OWNDC;
    WebServer.lpfnWndProc   = WindowProcess;
    WebServer.cbClsExtra    = 0;
    WebServer.cbWndExtra    = sizeof(long);
    WebServer.hInstance     = hInstance;
    WebServer.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    WebServer.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WebServer.hbrBackground = CreateSolidBrush(DefaultColor);
    WebServer.lpszMenuName  = MAKEINTRESOURCE(ID_MENU);
    WebServer.lpszClassName = ClassName;
    WebServer.hIconSm       = NULL;
    RegisterClassEx(&WebServer);

    //*创建主窗口
    Handle = CreateWindowEx(
        WS_EX_TOPMOST,
        ClassName,
        AppName,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VSCROLL,
        WindowRect.left,
        WindowRect.top,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    HACCEL Acclerator = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACC));
    HttpRequest::Hdc = AppDC = GetDC(Handle);
    ShowWindow(Handle, nCmdShow);
    UpdateWindow(Handle);
    //*消息循环分发
    while (GetMessage(&Message, NULL, 0, 0))
    {
        if (Dialog == NULL || !IsDialogMessage(Dialog, &Message))
        {
            if (!TranslateAccelerator(Handle, Acclerator, &Message))
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
        }
    }
    return (INT)Message.wParam;
}
//*主窗口窗口过程
LRESULT CALLBACK WindowProcess(HWND Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT Ps;
    TCHAR Time[] = TEXT("Time:"), SelectedCeil[] = TEXT("Ceil:"), Wealth[] = TEXT("Wealth:"), Limit[] = TEXT("Limit:");
    static RECT TimeRect, TimeArea, CellRect, PosRect, WealthRect, ValueRect, InfoRect, BUTGORECT, BUTSTOPRECT, BUTENDRECT, LimitRect,
        LimEdRect, ProRect, StaticRect;
    DRAWTEXTPARAMS DrawTextParams = { sizeof(DRAWTEXTPARAMS), 4, 1, 0, 0 };
    SCROLLINFO /*HScrollInfo,*/ VScrollInfo;
    static BOOL IsLTracking = false, IsRTracking = false, TimeSet = false, Enableing = TRUE, Changed = FALSE, Invalid = FALSE,
                Running = FALSE, Stopped = FALSE;
    static INT count = 0, TimeLimit = 1000, HPos = 0, VPos = 0, ProPos = 0;
    static HWND ButGo, ButStop, ButErase, LimEditor, ProBar, StaticText;
    static HFONT HFont = CreateFont(
                     15,
                     0,
                     0,
                     0,
                     500,
                     false,
                     false,
                     false,
                     DEFAULT_CHARSET,
                     OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS,
                     DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_SWISS,
                     TEXT("宋体")
                 ),
                 HFon = CreateFont(12, 0, 0, 0, 0, 0, 0, 0, GB2312_CHARSET, 0, 0, 0, 0, TEXT("宋体")),
                 Font = CreateFont(
                     13,
                     0,
                     0,
                     0,
                     500,
                     false,
                     false,
                     false,
                     DEFAULT_CHARSET,
                     OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS,
                     DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_SWISS,
                     TEXT("宋体")
                 );
    static HANDLE EvoluThread, CalaulateThread;
    static EvolutionData EvoluData;
    static POINT SelectedPoint = { 0, 0 };

    switch (Msg)
    {
    case WM_CREATE:
    { //*初始化窗口
        AppDC                 = GetDC(Handle);
        DrawTextParams.cbSize = sizeof(DRAWTEXTPARAMS);
        WindowRect.right     += GetSystemMetrics(SM_CXVSCROLL);

        AdjustWindowRectEx(
            &WindowRect,
            GetWindowLongPtr(Handle, GWL_STYLE),
            GetMenu(Handle) != NULL,
            GetWindowLongPtr(Handle, GWL_EXSTYLE)
        );
        SetWindowPos(Handle, HWND_TOPMOST, 0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, SWP_NOMOVE);
        OffsetRect(&WindowRect, -WindowRect.left, -WindowRect.top);
        GetClientRect(Handle, &PaintArea);
        DataArea.bottom = PaintArea.bottom;
        PaintArea.left  = DataArea.right;
        for (int n = 0; n < MapSize.cy; n++)
        {
            for (int m = 0; m < MapSize.cx; m++)
            {
                Map[n][m].Position = { m, n };
            }
        }
        VScrollInfo.cbSize = sizeof(SCROLLINFO);
        SIZE TextSize;
        SelectObject(AppDC, HFont);
        GetTextExtentPoint32(AppDC, Time, _tcslen(Time), &TextSize);
        TimeRect = { 0, 0, TextSize.cx, TextSize.cy };
        TimeArea = { TimeRect.right, TimeRect.top, DataArea.right, TimeRect.bottom - TimeRect.top };
        GetTextExtentPoint32(AppDC, SelectedCeil, _tcslen(SelectedCeil), &TextSize);
        CellRect = { 0, TimeRect.bottom, TextSize.cx, TimeRect.bottom + TextSize.cy };
        SetRect(&PosRect, CellRect.right, CellRect.top, DataArea.right, CellRect.bottom);
        GetTextExtentPoint32(AppDC, Wealth, _tcslen(Wealth), &TextSize);
        WealthRect = { 0, CellRect.bottom, TextSize.cx, CellRect.bottom + TextSize.cy };
        SetRect(&ValueRect, WealthRect.right, WealthRect.top, DataArea.right, WealthRect.bottom);
        InfoRect = { 10, WealthRect.bottom + 10, DataArea.right - 10, WealthRect.bottom + 240 };
        InfoRgn  = CreateRectRgnIndirect(&InfoRect);

        SetRect(&BUTGORECT, InfoRect.left, InfoRect.bottom + 10, InfoRect.left + 60, InfoRect.bottom + 40);
        SetRect(&BUTSTOPRECT, BUTGORECT.right + 10, BUTGORECT.top, BUTGORECT.right + 70, BUTGORECT.bottom);
        SetRect(&BUTENDRECT, BUTGORECT.left, BUTGORECT.bottom + 10, BUTSTOPRECT.right, BUTGORECT.bottom + 40);

        GetTextExtentPoint32(AppDC, Limit, _tcslen(Limit), &TextSize);
        LimitRect  = { TimeRect.left, BUTENDRECT.bottom + 10, TimeRect.left + TextSize.cx, BUTENDRECT.bottom + TextSize.cy + 10 };
        LimEdRect  = { LimitRect.right + 2, LimitRect.top - 1, DataArea.right - 5, LimitRect.bottom + 1 };
        ProRect    = { LimitRect.left + 5, LimEdRect.bottom + 10, LimEdRect.right, LimEdRect.bottom + 40 };
        StaticRect = { ProRect.left, ProRect.bottom + 10, ProRect.right, ProRect.bottom + 50 };

        ButGo = CreateWindowEx(
            0,
            TEXT("BUTTON"),
            TEXT("GO"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            BUTGORECT.left,
            BUTGORECT.top,
            BUTGORECT.right - BUTGORECT.left,
            BUTGORECT.bottom - BUTGORECT.top,
            Handle,
            (HMENU)(intptr_t)ID_BUTGO,
            NULL,
            NULL
        );
        ButStop = CreateWindowEx(
            0,
            TEXT("BUTTON"),
            TEXT("STOP"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            BUTSTOPRECT.left,
            BUTSTOPRECT.top,
            BUTSTOPRECT.right - BUTSTOPRECT.left,
            BUTSTOPRECT.bottom - BUTSTOPRECT.top,
            Handle,
            (HMENU)(intptr_t)ID_BUTSTOP,
            NULL,
            NULL
        );
        ButErase = CreateWindowEx(
            0,
            TEXT("BUTTON"),
            TEXT("SHUTDOWN"),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            BUTENDRECT.left,
            BUTENDRECT.top,
            BUTENDRECT.right - BUTENDRECT.left,
            BUTENDRECT.bottom - BUTENDRECT.top,
            Handle,
            (HMENU)(intptr_t)ID_BUTERASE,
            NULL,
            NULL
        );
        LimEditor = CreateWindowEx(
            0,
            TEXT("EDIT"),
            NULL,
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | WS_TABSTOP,
            LimEdRect.left,
            LimEdRect.top,
            LimEdRect.right - LimEdRect.left,
            LimEdRect.bottom - LimEdRect.top,
            Handle,
            (HMENU)(intptr_t)(ID_EDITOR),
            NULL,
            NULL
        );
        ProBar = CreateWindowEx(
            0,
            TEXT("msctls_progress32"),
            NULL,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            ProRect.left,
            ProRect.top,
            ProRect.right - ProRect.left,
            ProRect.bottom - ProRect.top,
            Handle,
            (HMENU)(intptr_t)(ID_PROBAR),
            NULL,
            NULL
        );
        StaticText = CreateWindowEx(
            0,
            TEXT("STATIC"),
            NULL,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            StaticRect.left,
            StaticRect.top,
            StaticRect.right - StaticRect.left,
            StaticRect.bottom - StaticRect.top,
            Handle,
            (HMENU)(intptr_t)(ID_STATICTEXT),
            NULL,
            NULL
        );

        SendMessage(LimEditor, WM_SETFONT, (WPARAM)HFon, 0);
        SendMessage(ButGo, WM_SETFONT, (WPARAM)HFont, 0);
        SendMessage(ButStop, WM_SETFONT, (WPARAM)HFont, 0);
        SendMessage(ButErase, WM_SETFONT, (WPARAM)HFont, 0);
        SendMessage(StaticText, WM_SETFONT, (WPARAM)Font, 0);

        EvolutionData::FatherWindow = Handle;
        EvoluData.WM_SELF           = WM_DONE;
        EvoluData.ZoomCellRegion    = InfoRgn;
        EvoluData.ValueRect         = ValueRect;
        EvoluData.ZoomCellPos       = &SelectedPoint;
        EvoluData.Changed           = &Changed;
        InitializeCriticalSection(&EvolutionData::CriticalSection);
        EvoluThread = CreateThread(NULL, 0, Evoulution, &EvoluData, CREATE_SUSPENDED, &EvolutionData::EvolutionThreadID);
        return 0;
    }
    case WM_SIZE:
    { //* 调整窗口大小时，重新设置滚动条的范围和位置
        GetClientRect(Handle, &ClientRect);
        VScrollInfo.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        VScrollInfo.nMin  = 0;
        VScrollInfo.nMax  = MapSize.cy - 1;
        VScrollInfo.nPage = (PaintArea.bottom - PaintArea.top) / HttpRequest::Size.cy;
        VScrollInfo.nPos  = VPos;
        SetScrollInfo(Handle, SB_VERT, &VScrollInfo, TRUE);
        return 0;
    }
    case WM_COMMAND:
    { //* 响应窗口控件消息
        switch (LOWORD(wParam))
        {
        case ID_BUTGO:
        { //*GO按钮
            SetFocus(Handle);
            if (!TimeSet)
            {
                SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_WAIT));
                EnableWindow(ButGo, FALSE);
                Enableing          = FALSE;
                EvoluData.AliveNum = AliveCell.size();
                EnterCriticalSection(&EvolutionData::CriticalSection);
                CalaulateThread = CreateThread(NULL, 0, Calculation, &EvoluData, 0, NULL);
                CloseHandle(CalaulateThread);
                SetWindowText(StaticText, TEXT("Calculating Path..."));
                int Len = GetWindowTextLength(LimEditor);
                if (Len == 0)
                {
                    SetDlgItemInt(Handle, ID_EDITOR, TimeLimit, FALSE);
                    SendMessage(ProBar, PBM_SETRANGE32, 0, TimeLimit * 10);
                }
                else
                {
                    BOOL Judge;
                    int Time = GetDlgItemInt(Handle, ID_EDITOR, &Judge, TRUE);
                    if (Judge && Time > 0)
                    {
                        TimeLimit = Time;
                        SendMessage(ProBar, PBM_SETRANGE32, 0, TimeLimit * 10);
                    }
                    else
                    {
                        SendMessage(ProBar, PBM_SETRANGE32, 0, TimeLimit * 10);
                        MessageBox(Handle, TEXT("输入内容非法"), TEXT("错误"), MB_OK | MB_ICONERROR);
                    }
                }
            }
            break;
        }
        case ID_BUTSTOP:
        { //*STOP按钮
            SetFocus(Handle);
            if (TimeSet)
            {
                KillTimer(Handle, ID_TIMER);
                TimeSet = false;
                EnterCriticalSection(&EvolutionData::CriticalSection);
                SetWindowText(StaticText, TEXT("Stopped"));
                SuspendThread(EvoluThread);
                LeaveCriticalSection(&EvolutionData::CriticalSection);
                Running = false;
                Stopped = true;
            }
            DWORD ExitCode;
            GetExitCodeThread(CalaulateThread, &ExitCode);
            if (ExitCode == STILL_ACTIVE)
            {
                TerminateThread(CalaulateThread, -1);
            }
            break;
        }
        case ID_BUTERASE:
        { //*ERASE按钮
            if (Running || Stopped || Changed || Invalid)
            {
                SetFocus(Handle);
                count = 0;
                FillRect(HttpRequest::Hdc, &TimeArea, WHITE_BRUSH);
                FillRect(HttpRequest::Hdc, &WealthRect, WHITE_BRUSH);
                FillRect(HttpRequest::Hdc, &InfoRect, WHITE_BRUSH);
                FillRect(HttpRequest::Hdc, &PosRect, WHITE_BRUSH);
                SetWindowText(StaticText, TEXT("Erased"));
                KillTimer(Handle, ID_TIMER);
                EnterCriticalSection(&EvolutionData::CriticalSection);
                if (!Stopped) SuspendThread(EvoluThread);
                LeaveCriticalSection(&EvolutionData::CriticalSection);
                ProPos = 0;
                for (vector<HttpRequest> &Temp : Map)
                {
                    for (HttpRequest &temp : Temp)
                    {
                        if (temp.State != DEAD)
                        {
                            int value   = rand() % 256;
                            temp.Wealth = value;
                            temp.Color  = RGB(value, 255, 255);
                        }
                    }
                }
                HBRUSH Brush = CreateSolidBrush(Map[SelectedPoint.x][SelectedPoint.y].Color);
                FillRgn((HDC)wParam, InfoRgn, Brush);
                TimeLimit = 1000;
                TimeSet   = false;
                SendMessage(ProBar, PBM_SETPOS, ProPos, 0);
                InvalidateRect(Handle, NULL, TRUE);
                Running = false;
                Stopped = false;
                Changed = false;
                Invalid = false;
            }
            break;
        } //*以下是菜单选项消息处理
        case ID_FILE_NEW:
        {
            if (Running || Stopped || Changed || Invalid)
            {
                SetFocus(Handle);
                count = 0;
                FillRect(HttpRequest::Hdc, &TimeArea, WHITE_BRUSH);
                FillRect(HttpRequest::Hdc, &WealthRect, WHITE_BRUSH);
                FillRect(HttpRequest::Hdc, &InfoRect, WHITE_BRUSH);
                FillRect(HttpRequest::Hdc, &PosRect, WHITE_BRUSH);
                KillTimer(Handle, ID_TIMER);
                EnterCriticalSection(&EvolutionData::CriticalSection);
                if (Running) SuspendThread(EvoluThread);
                LeaveCriticalSection(&EvolutionData::CriticalSection);
                ProPos = 0;
                for (vector<HttpRequest> &Temp : Map)
                {
                    for (HttpRequest &temp : Temp)
                    {
                        if (temp.State != DEAD)
                        {
                            temp.State  = DEAD;
                            temp.Wealth = 0;
                            temp.Color  = DefaultColor;
                        }
                    }
                }
                AliveCell.clear();
                FillRgn(HttpRequest::Hdc, InfoRgn, (HBRUSH)GetStockObject(WHITE_BRUSH));
                TimeLimit = 1000;
                TimeSet   = false;
                SendMessage(ProBar, PBM_SETPOS, ProPos, 0);
                InvalidateRect(Handle, NULL, TRUE);
                Running = false;
                Stopped = false;
                Changed = false;
                Invalid = false;
            }
            break;
        }
        case ID_FILE_SAVE:
        {
            KillTimer(Handle, ID_TIMER);
            TimeSet = false;
            SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_WAIT));
            EnableWindow(ButGo, FALSE);
            EnableWindow(ButStop, FALSE);
            EnableWindow(ButErase, FALSE);
            Enableing = FALSE;
            EnterCriticalSection(&EvolutionData::CriticalSection);
            SuspendThread(EvoluThread);
            ofstream SaveFile("resource/Cell.txt");
            for (vector<HttpRequest> &Temp : Map)
            {
                for (HttpRequest &temp : Temp)
                {
                    if (temp.State != DEAD)
                    {
                        SaveFile << temp << endl;
                    }
                }
            }
            LeaveCriticalSection(&EvolutionData::CriticalSection);
            SaveFile.close();
            EnableWindow(ButGo, TRUE);
            EnableWindow(ButStop, TRUE);
            EnableWindow(ButErase, TRUE);
            Enableing = TRUE;
            SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
            Changed = false;
            break;
        }
        case ID_FILE_READ:
        {
            KillTimer(Handle, ID_TIMER);
            SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_WAIT));
            EnableWindow(ButGo, FALSE);
            EnableWindow(ButStop, FALSE);
            EnableWindow(ButErase, FALSE);
            Enableing = FALSE;
            ifstream ReadFile("resource/Cell.txt");
            EnterCriticalSection(&EvolutionData::CriticalSection);
            if (Running) SuspendThread(EvoluThread);
            for (vector<HttpRequest> &Temp : Map)
            {
                for (HttpRequest &temp : Temp)
                {
                    if (temp.State != DEAD)
                    {
                        temp.State  = DEAD;
                        temp.Wealth = 0;
                        temp.Color  = DefaultColor;
                    }
                }
            }
            AliveCell.clear();
            int x, y;
            while (!ReadFile.eof())
            {
                ReadFile >> x >> y;
                Map[y][x].State = ALIVE;
                ReadFile >> Map[y][x].Wealth >> Map[y][x].Color;
                AliveCell.push_back({ y, x });
            }

            AliveCell.erase(AliveCell.end() - 1);
            EvoluData.AliveNum = AliveCell.size();
            LeaveCriticalSection(&EvolutionData::CriticalSection);
            EnableWindow(ButGo, TRUE);
            EnableWindow(ButStop, TRUE);
            EnableWindow(ButErase, TRUE);
            Enableing = TRUE;
            SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
            InvalidateRect(Handle, NULL, TRUE);
            Changed = FALSE;
            ReadFile.close();
            TimeSet = false;
            Invalid = true;
            break;
        }
        case ID_HELP_ATTRIBUTE:
            if (Dialog == NULL) Dialog = CreateDialog(NULL, MAKEINTRESOURCE(ID_DIALOG_ATTRIBUTE), Handle, ModelessProc);
            break;
        case ID_HELP_ABOUT:
            if (Dialog == NULL) Dialog = CreateDialogParam(NULL, MAKEINTRESOURCE(ID_DIALOG_ABOUT), Handle, ModelessProc, 18181);
            break;
        case ID_HELP_ACCELERATORS:
            if (Dialog == NULL) Dialog = CreateDialog(NULL, MAKEINTRESOURCE(ID_DIALOG_ACCELERATORS), Handle, ModelessProc);
            break;
        case ID_CODE_IDEA:
            if (Dialog == NULL) Dialog = CreateDialog(NULL, MAKEINTRESOURCE(ID_CODE_IDEA), Handle, ModelessProc);
            break;
        case ID_CODE_TOOL:
            if (Dialog == NULL) Dialog = CreateDialog(NULL, MAKEINTRESOURCE(ID_CODE_TOOL), Handle, ModelessProc);
            break;
        case ID_LEFT  : SendMessage(Handle, WM_HSCROLL, SB_LINELEFT, 0); break;
        case ID_RIGHT : SendMessage(Handle, WM_HSCROLL, SB_LINERIGHT, 0); break;
        case ID_UP    : SendMessage(Handle, WM_VSCROLL, SB_LINEUP, 0); break;
        case ID_DOWN  : SendMessage(Handle, WM_VSCROLL, SB_LINEDOWN, 0); break;
        case ID_H_HOME: SendMessage(Handle, WM_HSCROLL, SB_LEFT, 0); break;
        case ID_V_HOME: SendMessage(Handle, WM_VSCROLL, SB_TOP, 0); break;
        case ID_H_END : SendMessage(Handle, WM_HSCROLL, SB_RIGHT, 0); break;
        case ID_V_END : SendMessage(Handle, WM_VSCROLL, SB_BOTTOM, 0); break;
        case ID_RETURN: SendMessage(ButGo, BM_CLICK, 0, 0); break;
        case ID_SPACE : SendMessage(ButStop, BM_CLICK, 0, 0); break;
        }
        return 0;
    }
    case WM_TIMER:
    { //*计时器
        count++;
        ProPos = count;
        SendMessage(ProBar, PBM_SETPOS, ProPos, 0);
        if (count > TimeLimit * 10)
        {
            SuspendThread(EvoluThread);
            KillTimer(Handle, ID_TIMER);
            TimeSet = false;
            Stopped = TRUE;
            SetWindowText(StaticText, TEXT("Time Limit Reached!"));
            return 0;
        }
        EnterCriticalSection(&EvolutionData::CriticalSection);
        TCHAR TimeText[20];
        _stprintf_s(TimeText, TEXT("%0.1f"), count * 0.1);
        _tcscat_s(TimeText, _countof(TimeText), TEXT("s"));
        FillRect(AppDC, &TimeArea, WHITE_BRUSH);
        SelectObject(AppDC, HFont);
        DrawTextEx(AppDC, TimeText, _tcslen(TimeText), &TimeArea, DT_LEFT | DT_SINGLELINE, &DrawTextParams);
        LeaveCriticalSection(&EvolutionData::CriticalSection);
        PostThreadMessage(EvolutionData::EvolutionThreadID, WM_TIMER, 0, 0);
        return 0;
    }
    case WM_ERASEBKGND:
    { //*擦除背景
        EnterCriticalSection(&EvolutionData::CriticalSection);
        FillRect((HDC)wParam, &DataArea, WHITE_BRUSH);
        SelectObject((HDC)wParam, HFont);
        SetBkMode((HDC)wParam, TRANSPARENT);
        DrawTextEx((HDC)wParam, Time, _tcslen(Time), &TimeRect, DT_LEFT | DT_SINGLELINE, &DrawTextParams);
        DrawTextEx((HDC)wParam, SelectedCeil, _tcslen(SelectedCeil), &CellRect, DT_LEFT | DT_SINGLELINE, &DrawTextParams);
        DrawTextEx((HDC)wParam, Wealth, _tcslen(Wealth), &WealthRect, DT_LEFT | DT_SINGLELINE, &DrawTextParams);
        DrawTextEx((HDC)wParam, Limit, _tcslen(Limit), &LimitRect, DT_LEFT | DT_SINGLELINE, &DrawTextParams);
        HBRUSH Brush = CreateSolidBrush(Map[SelectedPoint.x][SelectedPoint.y].Color);
        FillRgn((HDC)wParam, InfoRgn, Changed ? Brush : WHITE_BRUSH);
        FrameRgn((HDC)wParam, InfoRgn, (HBRUSH)GetStockObject(BLACK_BRUSH), 1, 1);
        DeleteObject(Brush);
        LeaveCriticalSection(&EvolutionData::CriticalSection);
        return TRUE;
    }
    case WM_PAINT:
    { //*绘图
        EnterCriticalSection(&EvolutionData::CriticalSection);
        BeginPaint(Handle, &Ps);
        VScrollInfo.fMask = SIF_POS;
        GetScrollInfo(Handle, SB_VERT, &VScrollInfo);
        int LeftLimit   = max((int)(Ps.rcPaint.left - DataArea.right) / HttpRequest::Size.cy, 0) /* + HScrollInfo.nPos */,
            RightLimit  = 1; // 重绘制区域的右边界
        int TopLimit    = Ps.rcPaint.top / HttpRequest::Size.cy + VScrollInfo.nPos,
            BottomLimit = max((int)(Ps.rcPaint.bottom / HttpRequest::Size.cy) + VScrollInfo.nPos, TopLimit + 1);
        RECT Cell;
        for (int n = TopLimit; n < BottomLimit; n++)
        {
            for (int m = LeftLimit; m < RightLimit; m++)
            {
                SetRect(
                    &Cell,
                    DataArea.right /*  + m * CUBE - HScrollInfo.nPos * CUBE */,
                    (n - VScrollInfo.nPos) * HttpRequest::Size.cy,
                    DataArea.right + (m + 1) * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
                    ((n + 1) - VScrollInfo.nPos) * HttpRequest::Size.cy
                );
                HBRUSH Brush = CreateSolidBrush(Map[n][m].Color);
                FillRect(AppDC, &Cell, Brush);
                DeleteObject(Brush);
            }
        }
        EndPaint(Handle, &Ps);
        LeaveCriticalSection(&EvolutionData::CriticalSection);
        return 0;
    }
    case WM_LBUTTONDOWN:
    { //*鼠标左键按下
        if (!Enableing) return 0;
        POINT MouPos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        SetFocus(Handle);
        if (!PtInRect(&PaintArea, MouPos)) return 0;
        VScrollInfo.fMask = SIF_POS;
        GetScrollInfo(Handle, SB_VERT, &VScrollInfo);
        MouPos = { (GET_X_LPARAM(lParam) - DataArea.right) / HttpRequest::Size.cx /*  + HScrollInfo.nPos */,
                   (GET_Y_LPARAM(lParam)) / HttpRequest::Size.cy + VScrollInfo.nPos };
        HBRUSH HBrush;
        RECT Cell;
        SetRect(
            &Cell,
            DataArea.right + MouPos.x * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
            (MouPos.y - VScrollInfo.nPos) * HttpRequest::Size.cy,
            DataArea.right + (MouPos.x + 1) * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
            ((MouPos.y + 1) - VScrollInfo.nPos) * HttpRequest::Size.cy
        );
        SubtractRect(&Cell, &Cell, &DataArea);
        EnterCriticalSection(&EvolutionData::CriticalSection);
        if (!Map[MouPos.y][MouPos.x].State)
        {
            int value                       = rand() % 256;
            Map[MouPos.y][MouPos.x].Color   = RGB(value, 255, 255);
            Map[MouPos.y][MouPos.x].State   = ALIVE;
            Map[MouPos.y][MouPos.x].Wealth += (255 - value);
            IsLTracking                     = TRUE;
            Changed                         = TRUE;
            AliveCell.push_back({ MouPos.y, MouPos.x });
        }
        SelectedPoint         = { MouPos.y, MouPos.x };
        EvoluData.ZoomCellPos = &Map[MouPos.y][MouPos.x].Position;
        HBrush                = CreateSolidBrush(Map[MouPos.y][MouPos.x].Color);
        FillRect(HttpRequest::Hdc, &Cell, HBrush);
        FillRgn(HttpRequest::Hdc, InfoRgn, HBrush);
        FrameRgn(HttpRequest::Hdc, InfoRgn, (HBRUSH)GetStockObject(BLACK_BRUSH), 1, 1);
        TCHAR PosText[20];
        _stprintf_s(PosText, TEXT("(%d,%d)"), MouPos.x, MouPos.y);
        FillRect(HttpRequest::Hdc, &PosRect, WHITE_BRUSH);
        DrawText(HttpRequest::Hdc, PosText, _tcslen(PosText), &PosRect, DT_LEFT | DT_SINGLELINE);
        TCHAR WealthText[20];
        _stprintf_s(WealthText, TEXT("%d"), Map[MouPos.y][MouPos.x].Wealth);
        FillRect(HttpRequest::Hdc, &ValueRect, WHITE_BRUSH);
        DrawTextEx(HttpRequest::Hdc, WealthText, _tcslen(WealthText), &ValueRect, DT_LEFT | DT_SINGLELINE, &DrawTextParams);
        LeaveCriticalSection(&EvolutionData::CriticalSection);
        DeleteObject(HBrush);
        SetCapture(Handle);
        SetFocus(Handle);
        return 0;
    }
    case WM_RBUTTONDOWN:
    { //*鼠标右键按下
        if (!Enableing) return 0;
        POINT MouPos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        SetFocus(Handle);
        if (!PtInRect(&PaintArea, MouPos)) return 0;
        VScrollInfo.fMask = SIF_POS;
        GetScrollInfo(Handle, SB_VERT, &VScrollInfo);
        MouPos = { (GET_X_LPARAM(lParam) - DataArea.right) / HttpRequest::Size.cx /*  + HScrollInfo.nPos */,
                   (GET_Y_LPARAM(lParam)) / HttpRequest::Size.cy + VScrollInfo.nPos };
        EnterCriticalSection(&EvolutionData::CriticalSection);
        if (Map[MouPos.y][MouPos.x].State)
        {
            IsRTracking                    = TRUE;
            Map[MouPos.y][MouPos.x].State  = DEAD;
            Map[MouPos.y][MouPos.x].Color  = DefaultColor;
            Map[MouPos.y][MouPos.x].Wealth = 0;
            RECT Cell;
            SetRect(
                &Cell,
                DataArea.right + MouPos.x * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
                (MouPos.y - VScrollInfo.nPos) * HttpRequest::Size.cy,
                DataArea.right + (MouPos.x + 1) * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
                ((MouPos.y + 1) - VScrollInfo.nPos) * HttpRequest::Size.cy
            );
            SubtractRect(&Cell, &Cell, &DataArea);
            FillRect(HttpRequest::Hdc, &Cell, (HBRUSH)GetClassLongPtr(Handle, GCLP_HBRBACKGROUND));
            Changed = TRUE;
            MouPos  = { MouPos.y, MouPos.x };
            for (auto it = AliveCell.begin(); it != AliveCell.end(); it++)
            {
                if (it->x == MouPos.x && it->y == MouPos.y)
                {
                    AliveCell.erase(it);
                    break;
                }
            }
        }
        SelectedPoint         = { MouPos.y, MouPos.x };
        EvoluData.ZoomCellPos = &Map[MouPos.y][MouPos.x].Position;
        FillRgn(HttpRequest::Hdc, InfoRgn, (HBRUSH)GetClassLongPtr(Handle, GCLP_HBRBACKGROUND));
        FrameRgn(HttpRequest::Hdc, InfoRgn, (HBRUSH)GetStockObject(BLACK_BRUSH), 1, 1);
        TCHAR PosText[20];
        _stprintf_s(PosText, TEXT("(%d,%d)"), MouPos.x, MouPos.y);
        FillRect(HttpRequest::Hdc, &PosRect, WHITE_BRUSH);
        DrawText(HttpRequest::Hdc, PosText, _tcslen(PosText), &PosRect, DT_LEFT | DT_SINGLELINE);
        TCHAR WealthText[20];
        _stprintf_s(WealthText, TEXT("%d"), Map[MouPos.y][MouPos.x].Wealth);
        FillRect(HttpRequest::Hdc, &ValueRect, WHITE_BRUSH);
        DrawTextEx(HttpRequest::Hdc, WealthText, _tcslen(WealthText), &ValueRect, DT_LEFT | DT_SINGLELINE, &DrawTextParams);
        LeaveCriticalSection(&EvolutionData::CriticalSection);
        SetCapture(Handle);
        return 0;
    }
    case WM_RBUTTONUP:
    { //*鼠标右键释放
        if (IsRTracking && Running)
        {
            if (TimeSet)
            {
                KillTimer(Handle, ID_TIMER);
                TimeSet = FALSE;
            }
            EnableWindow(ButGo, FALSE);
            Enableing          = FALSE;
            EvoluData.AliveNum = AliveCell.size();
            EnterCriticalSection(&EvolutionData::CriticalSection);
            SuspendThread(EvoluThread);
            CalaulateThread = CreateThread(NULL, 0, Calculation, &EvoluData, 0, NULL);
            SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_WAIT));
            CloseHandle(CalaulateThread);
            SetWindowText(StaticText, TEXT("正在计算影响路径..."));
        }
        IsRTracking = false;
        ReleaseCapture();
        return 0;
    }
    case WM_LBUTTONUP:
    { //*鼠标左键释放
        if (IsLTracking && Running)
        {
            if (TimeSet)
            {
                KillTimer(Handle, ID_TIMER);
                TimeSet = FALSE;
            }
            EnableWindow(ButGo, FALSE);
            Enableing          = FALSE;
            EvoluData.AliveNum = AliveCell.size();
            EnterCriticalSection(&EvolutionData::CriticalSection);
            SuspendThread(EvoluThread);
            CalaulateThread = CreateThread(NULL, 0, Calculation, &EvoluData, 0, NULL);
            SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_WAIT));
            CloseHandle(CalaulateThread);
            SetWindowText(StaticText, TEXT("Calculating Path..."));
        }
        IsLTracking = false;
        ReleaseCapture();
        return 0;
    }
    case WM_MOUSEMOVE:
    { //*鼠标移动
        POINT MouPos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        if (!PtInRect(&PaintArea, MouPos)) return 0;
        VScrollInfo.fMask = SIF_POS;
        GetScrollInfo(Handle, SB_VERT, &VScrollInfo);
        MouPos = { (GET_X_LPARAM(lParam) - DataArea.right) / HttpRequest::Size.cx /*  + HScrollInfo.nPos */,
                   (GET_Y_LPARAM(lParam)) / HttpRequest::Size.cy + VScrollInfo.nPos };
        EnterCriticalSection(&EvolutionData::CriticalSection);
        if (!Map[MouPos.y][MouPos.x].State && IsLTracking)
        {
            Map[MouPos.y][MouPos.x].State = ALIVE;
            int value                     = rand() % 256;
            Map[MouPos.y][MouPos.x].Color = RGB(value, 255, 255);
            AliveCell.push_back({ MouPos.y, MouPos.x });
            SelectedPoint                   = { MouPos.y, MouPos.x };
            HBRUSH HBrush                   = CreateSolidBrush(Map[MouPos.y][MouPos.x].Color);
            Map[MouPos.y][MouPos.x].Wealth += (255 - value);
            RECT Cell;
            SetRect(
                &Cell,
                DataArea.right + MouPos.x * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
                (MouPos.y - VScrollInfo.nPos) * HttpRequest::Size.cy,
                DataArea.right + (MouPos.x + 1) * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
                ((MouPos.y + 1) - VScrollInfo.nPos) * HttpRequest::Size.cy
            );
            SubtractRect(&Cell, &Cell, &DataArea);
            FillRect(HttpRequest::Hdc, &Cell, HBrush);
            EvoluData.ZoomCellPos = &Map[MouPos.y][MouPos.x].Position;
            FillRgn(HttpRequest::Hdc, InfoRgn, HBrush);
            TCHAR PosText[20];
            _stprintf_s(PosText, TEXT("(%d,%d)"), MouPos.x, MouPos.y);
            FillRect(HttpRequest::Hdc, &PosRect, WHITE_BRUSH);
            DrawText(HttpRequest::Hdc, PosText, _tcslen(PosText), &PosRect, DT_LEFT | DT_SINGLELINE);
            TCHAR WealthText[20];
            _stprintf_s(WealthText, TEXT("%d"), Map[MouPos.y][MouPos.x].Wealth);
            FillRect(HttpRequest::Hdc, &ValueRect, WHITE_BRUSH);
            DrawTextEx(HttpRequest::Hdc, WealthText, _tcslen(WealthText), &ValueRect, DT_LEFT | DT_SINGLELINE, &DrawTextParams);
            DeleteObject(HBrush);
            Changed = true;
        }
        if (Map[MouPos.y][MouPos.x].State && IsRTracking)
        {
            Map[MouPos.y][MouPos.x].State  = DEAD;
            Map[MouPos.y][MouPos.x].Color  = DefaultColor;
            Map[MouPos.y][MouPos.x].Wealth = 0;
            Map[MouPos.y][MouPos.x].State  = DEAD;
            RECT Cell;
            SetRect(
                &Cell,
                DataArea.right + MouPos.x * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
                (MouPos.y - VScrollInfo.nPos) * HttpRequest::Size.cy,
                DataArea.right + (MouPos.x + 1) * HttpRequest::Size.cx /* - HScrollInfo.nPos * CUBE */,
                ((MouPos.y + 1) - VScrollInfo.nPos) * HttpRequest::Size.cy
            );
            SubtractRect(&Cell, &Cell, &DataArea);
            SelectedPoint = { MouPos.y, MouPos.x };
            FillRect(HttpRequest::Hdc, &Cell, (HBRUSH)GetClassLongPtr(Handle, GCLP_HBRBACKGROUND));
            EvoluData.ZoomCellPos = &Map[MouPos.y][MouPos.x].Position;
            FillRgn(HttpRequest::Hdc, InfoRgn, (HBRUSH)GetClassLongPtr(Handle, GCLP_HBRBACKGROUND));
            Changed = true;
            MouPos  = { MouPos.y, MouPos.x };
            for (auto it = AliveCell.begin(); it != AliveCell.end(); it++)
            {
                if (it->x == MouPos.x && it->y == MouPos.y)
                {
                    AliveCell.erase(it);
                    break;
                }
            }
        }
        FrameRgn(HttpRequest::Hdc, InfoRgn, (HBRUSH)GetStockObject(BLACK_BRUSH), 1, 1);
        LeaveCriticalSection(&EvolutionData::CriticalSection);
        if (TimeSet && (IsLTracking || IsRTracking))
        {
            KillTimer(Handle, ID_TIMER);
            TimeSet = FALSE;
        }
        return 0;
    }
    case WM_VSCROLL:
    { //*垂直滚动条
        VScrollInfo.fMask = SIF_ALL;
        GetScrollInfo(Handle, SB_VERT, &VScrollInfo);
        VPos = VScrollInfo.nPos;

        switch (LOWORD(wParam))
        {
        case SB_LINEUP    : VScrollInfo.nPos -= 1; break;
        case SB_LINEDOWN  : VScrollInfo.nPos += 1; break;
        case SB_PAGEUP    : VScrollInfo.nPos -= VScrollInfo.nPage / 2; break;
        case SB_PAGEDOWN  : VScrollInfo.nPos += VScrollInfo.nPage / 2; break;
        case SB_THUMBTRACK: VScrollInfo.nPos = VScrollInfo.nTrackPos; break;
        case SB_TOP       : VScrollInfo.nPos = VScrollInfo.nMin; break;
        case SB_BOTTOM    : VScrollInfo.nPos = VScrollInfo.nMax; break;
        }
        VScrollInfo.fMask = SIF_POS;
        SetScrollInfo(Handle, SB_VERT, &VScrollInfo, TRUE);
        GetScrollInfo(Handle, SB_VERT, &VScrollInfo);
        if (VScrollInfo.nPos != VPos)
        {
            EnterCriticalSection(&EvolutionData::CriticalSection);
            ScrollWindow(Handle, 0, (VPos - VScrollInfo.nPos) * HttpRequest::Size.cy, &PaintArea, &PaintArea);
            VPos = VScrollInfo.nPos;
            UpdateWindow(Handle);
            LeaveCriticalSection(&EvolutionData::CriticalSection);
        }
        return 0;
    }
    case WM_MOUSEWHEEL:
    { //*鼠标滚轮
        VScrollInfo.fMask = SIF_ALL;
        GetScrollInfo(Handle, SB_VERT, &VScrollInfo);
        VPos              = VScrollInfo.nPos;
        VScrollInfo.nPos -= GET_WHEEL_DELTA_WPARAM(wParam) / 5;
        VScrollInfo.fMask = SIF_POS;
        SetScrollInfo(Handle, SB_VERT, &VScrollInfo, TRUE);
        GetScrollInfo(Handle, SB_VERT, &VScrollInfo);
        if (VScrollInfo.nPos != VPos)
        {
            EnterCriticalSection(&EvolutionData::CriticalSection);
            ScrollWindow(Handle, 0, (VPos - VScrollInfo.nPos) * HttpRequest::Size.cy, &PaintArea, &PaintArea);
            VPos = VScrollInfo.nPos;
            UpdateWindow(Handle);
            LeaveCriticalSection(&EvolutionData::CriticalSection);
        }
        return 0;
    }
    case WM_DONE:
    { //*路径搜索完成
        LeaveCriticalSection(&EvolutionData::CriticalSection);
        EnableWindow(ButGo, TRUE);
        SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
        Enableing = TRUE;
        SetTimer(Handle, ID_TIMER, 100, NULL);
        TimeSet = TRUE;
        Running = true;
        Stopped = false;
        ResumeThread(EvoluThread);
        SetWindowText(StaticText, TEXT("Evolving..."));
        return 0;
    }
    case WM_CLOSE:
    { //*关闭窗口
        if (Changed)
        {
            switch (DialogBoxParam(NULL, MAKEINTRESOURCE(ID_DIALOG_SAVEORNOT), Handle, ModleProc, 0))
            {
            case IDOK:
            {
                SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_WAIT));
                EnableWindow(ButGo, FALSE);
                EnableWindow(ButStop, FALSE);
                EnableWindow(ButErase, FALSE);
                Enableing = FALSE;
                ofstream SaveFile("resource/Cell.txt");
                for (vector<HttpRequest> &Temp : Map)
                {
                    for (HttpRequest &temp : Temp)
                    {
                        if (temp.State != DEAD)
                        {
                            SaveFile << temp << endl;
                        }
                    }
                }
                SaveFile.close();
                EnableWindow(ButGo, TRUE);
                EnableWindow(ButStop, TRUE);
                EnableWindow(ButErase, TRUE);
                Enableing = TRUE;
                SetClassLongPtr(Handle, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
                DestroyWindow(Handle);
                break;
            }
            case IDCANCEL:
            {
                DestroyWindow(Handle);
                break;
            }
            }
            return 0;
        }
    }
    case WM_DESTROY:
    { //*销毁窗口
        PostQuitMessage(0);
        DeleteObject((HBRUSH)GetClassLongPtr(Handle, GCLP_HBRBACKGROUND));
        if (TimeSet)
        {
            KillTimer(Handle, ID_TIMER);
            TimeSet = FALSE;
        }
        PostThreadMessage(EvolutionData::EvolutionThreadID, WM_QUIT, 0, 0);
        DeleteObject(HFont);
        DeleteObject(HFon);
        DeleteObject(InfoRgn);
        CloseHandle(EvoluThread);
        DeleteObject(Font);
        ReleaseDC(Handle, AppDC);
        DeleteCriticalSection(&EvolutionData::CriticalSection);
        return 0;
    }
    }

    return DefWindowProc(Handle, Msg, wParam, lParam);
}
//*模态对话框过程
INT_PTR CALLBACK ModleProc(HWND Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        return TRUE;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            EndDialog(Handle, IDOK);
            Dialog = NULL;
            return TRUE;
        }
        case IDCANCEL:
        {
            EndDialog(Handle, IDCANCEL);
            Dialog = NULL;
            return TRUE;
        }
        }
    default: return FALSE;
    }
    case WM_CLOSE:
    {
        EndDialog(Handle, IDCLOSE);
        Dialog = NULL;
        return TRUE;
    }
    }
    return FALSE;
}

//*非模态对话框过程
INT_PTR CALLBACK ModelessProc(HWND Handle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    HWND SysLink;
    HFONT hFont = CreateFont(12, 0, 0, 0, 0, 0, 0, 0, 0x1, 0, 0, 0, 0, TEXT("MS Shell Dlg"));
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        if (lParam == 18181)
        {
            SysLink = CreateWindowEx(
                0,
                TEXT("SysLink"),
                TEXT("获取更多信息点击<a "
                     "href=\"https://baike.baidu.com/item/%E5%85%83%E8%83%9E%E8%87%AA%E5%8A%A8%E6%9C%BA/"
                     "7085754?fromtitle=细胞自动机&fromid=2765689&fr=aladdin\" ID=\"百度\">这里(百度百科)</a>"),
                WS_CHILD | WS_VISIBLE,
                12,
                80,
                300,
                20,
                Handle,
                (HMENU)(intptr_t)18181,
                NULL,
                NULL
            );
            SendMessage(SysLink, WM_SETFONT, (WPARAM)hFont, FALSE);
        }
        return TRUE;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            DestroyWindow(Handle);
            Dialog = NULL;
            break;
        }
        case IDCANCEL:
        {
            DestroyWindow(Handle);
            Dialog = NULL;
            break;
        }
        }
        return TRUE;
    }
    case WM_NOTIFY:
    {
        PNMLINK Link = (PNMLINK)lParam;
        if (Link->hdr.code == NM_CLICK)
        {
            ShellExecuteW(NULL, L"open", Link->item.szUrl, NULL, NULL, SW_SHOW);
        }
        return TRUE;
    }
    case WM_CLOSE:
    {
        DeleteObject(hFont);
        Dialog = NULL;
        break;
    }
    }
    return FALSE;
}
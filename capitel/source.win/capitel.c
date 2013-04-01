#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objidl.h>

#include "..\..\..\units\common.src\bastypes.h"

#include "capitel.h"
#include "..\..\util\source\register.h"
#include "..\..\..\units\common.src\cfg_file.h"
#include "..\..\..\units\common.src\util.h"
#include "..\..\common\source\global.h"
#include "..\..\common\source\version.h"
#include "..\..\answer\source\answer.h"
#include "..\..\..\units\common.src\os.h"
#include "..\..\..\units\common.src\num2nam.h"
#include "..\..\wave\source\alw2wav.h"

typedef struct sCall
{
  int iNum;
  int iState;
  int iDuration;
  int iIsDigital;
  SYSTEMTIME sTime;
  char cCallerName[256];
  char cCallerNameOrg[256];
  char cCalledPort[256];
  char cWavFileName[MAX_PATH];
  char cAlwFileName[MAX_PATH];
  char cIdxFileName[MAX_PATH];
}
tCall;

LANGID xLangID = 0;
HINSTANCE xAppInst = 0;
static tCall sLastCall;

void capitel_init (void);
void capitel_exit (void);
void capitel_record_welcome (char *name);
void capitel_dorescan        (void);

int onlyDigits(char* s);

#define WM_TRAY_CALLBACK    WM_USER+10

#define MAINWNDCLASS  "CapitelClass"
#define MAINWNDEXTRA  (6 * sizeof(long))

#define MainWnd_GetShowTray(hwnd) \
  (GetWindowLong((hwnd),0))

#define MainWnd_SetShowTray(hwnd,uFlg) \
  (SetWindowLong((hwnd),0,(LONG)(uFlg)))

#define MainWnd_GetHideMin(hwnd) \
  (GetWindowLong((hwnd),1*sizeof(long)))

#define MainWnd_SetHideMin(hwnd,uFlg) \
  (SetWindowLong((hwnd),1*sizeof(long),(LONG)(uFlg)))

#define MainWnd_GetTrayImgLst(hwnd) \
  ((HIMAGELIST)GetWindowLong((hwnd),2*sizeof(long)))

#define MainWnd_SetTrayImgLst(hwnd,hImgLst) \
  (SetWindowLong((hwnd),2*sizeof(long),(LONG)(hImgLst)))

#define MainWnd_GetTrayData(hwnd) \
  ((NOTIFYICONDATA*)GetWindowLong((hwnd),3*sizeof(long)))

#define MainWnd_SetTrayData(hwnd,pData) \
  (SetWindowLong((hwnd),3*sizeof(long),(LONG)(pData)))

#define MainWnd_GetTrayStatus(hwnd) \
  (GetWindowLong((hwnd),4*sizeof(long)))

#define MainWnd_SetTrayStatus(hwnd,iStat) \
  (SetWindowLong((hwnd),4*sizeof(long),(LONG)(iStat)))

#define MainWnd_GetToolTip(hwnd) \
  ((HWND)GetWindowLong((hwnd),5*sizeof(long)))

#define MainWnd_SetToolTip(hwnd,hwndTTip) \
  (SetWindowLong((hwnd),5*sizeof(long),(LONG)(hwndTTip)))

int CheckRegistration(char* name, char* code)
{
  if(!checkRegCode(name,code)) return 0;

  config_file_write_string(STD_CFG_FILE, REGISTER_NAME, name);
  config_file_write_string(STD_CFG_FILE, REGISTER_CODE, code);

  return 1;
}

static int iNumBitmap = 13;
static TBBUTTON tbButtons[] =               /* fuer CreateToolbarEx()       */
{
  {0,  IDM_TOGGLEACTIVATION ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {0,  0                    ,0,               TBSTYLE_SEP   },
  {1,  IDM_PLAY             ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {2,  IDM_PLAY_ALL         ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {3,  IDM_SAVE_AS          ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {4,  IDM_DELETE           ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {0,  0                    ,0,               TBSTYLE_SEP   },
  {5,  IDM_PROPERTIES       ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {6,  IDM_PROP_PORTS       ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
  {7,  IDM_PROP_CALLERS     ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
#ifndef RECOTEL
  {8,  IDM_PROP_ACTIONS     ,TBSTATE_ENABLED, TBSTYLE_BUTTON},
#endif
};

HWND hwndMain = 0;
HWND hwndPopup = 0;
HMENU hPopupMenu = 0;

static char cLastWavTmp[MAX_PATH];

int PreviousInstance(void);
int CenterWindow(HWND hwndChild, HWND hwndParent);
HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
int DisplayProperties(HWND hwnd);
LANGID GetDefaultLanguage(char* res_id);

WNDPROC oldListProc;
LRESULT CALLBACK newListProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AboutProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK PropPage1Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PropPage2Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PropPage3Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PropPage4Proc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK EditPortListProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditPortProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK EditCallerListProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditCallerProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK EditActionListProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditActionProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK RegisterDialogProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PopupDialogProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK MsgBoxProc(HWND, UINT, WPARAM, LPARAM);

int MyMsgBox(HWND hwndOwner,
  char* pText, char* pTitle, int fStyle, char* pIcon);

int MyMsgBoxEx(HWND hwndOwner,
  char* pText, char* pTitle, char* pChkText, int fStyle,
  char* pIcon, char* pCfgFile, char* pCfgKey);

void MainListInit(HWND hwndList);
int MainListInsertCall(HWND hwndList, HWND hwndTTip, tCall* pCall, int select);
tCall* MainListGetCall(HWND hwndList, int index);
void MainListUpdateIcon(HWND hwndList, int index);
void MainListSort(HWND hwndList, int iColumn);

int AddToCallers(HWND hwnd, char* pFileName, char* pNumber);

void sigFunc(short num, void *msg);

void shExecute( char * );
void shRecycle( char * );

int InitTrayIcon(HWND hwnd);
int UpdateTrayIcon(HWND hwnd);
int CleanupTrayIcon(HWND hwnd);
int GetTrayIconRect(RECT* pRect);

int StoreWindowAttributes(HWND hwnd);
int RestoreWindowAttributes(HWND hwnd);

int PlayCalls(HWND hwndList, int iPlayAll);
int GetSpecialFolderPath(HWND hwndOwner, int nFolder, char* pPathBuff);

int AddToolTip(HWND hwndList, HWND hwndTTip, int iItemIdx, char* pText);
int DelToolTip(HWND hwndList, HWND hwndTTip, int iItemIdx);
int UpdateToolRect(HWND hwndList, HWND hwndTTip, int iItemIdx);
int UpdateAllToolRect(HWND hwndList, HWND hwndTTip);

int SaveCall(tCall* pCall);
int ReadCall(tCall* pCall, char* pFileName);


/*
 *
 *  WinMain()
 *
 *  Erstellt das Hauptfenster und bearbeitet die
 *  dort eingehenden Messages.
 *
 */

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  MSG msg;
  char* pArg;
  HANDLE hAccel;
  UCHAR cName[MAX_PATH];

//#ifdef RECOTEL
//  if (time(0) > 921273393) return(1);
//#endif

  if(PreviousInstance()) return 0;

  xAppInst = hInstance;
  xLangID = GetDefaultLanguage(MAKEINTRESOURCE(IDD_ABOUT));

  pArg = strtok(lpCmdLine, " ");
  while(pArg)
  {
    if(!stricmp(pArg,"/lang:eng"))
      xLangID = MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US);
    else if(!stricmp(pArg,"/lang:hun"))
      xLangID = MAKELANGID(LANG_HUNGARIAN, SUBLANG_DEFAULT);
    else if(!stricmp(pArg,"/lang:ger"))
      xLangID = MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN);
    else if(!stricmp(pArg,"/lang:fre"))
      xLangID = MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH);
    else if(!stricmp(pArg,"/lang:spa"))
      xLangID = MAKELANGID(LANG_SPANISH,SUBLANG_SPANISH);
    else if(!stricmp(pArg,"/lang:nor"))
      xLangID = MAKELANGID(LANG_NORWEGIAN,SUBLANG_NORWEGIAN_BOKMAL);
    else if(!stricmp(pArg,"/lang:dut"))
      xLangID = MAKELANGID(LANG_DUTCH, SUBLANG_DUTCH);
    else if(!stricmp(pArg,"/lang:fin"))
      xLangID = MAKELANGID(LANG_FINNISH, SUBLANG_DEFAULT);
    else if(!stricmp(pArg,"/lang:ita"))
      xLangID = MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN);
    else if(!stricmp(pArg,"/lang:nyn"))
      xLangID = MAKELANGID(LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK);
    else if(!stricmp(pArg,"/lang:dan"))
      xLangID = MAKELANGID(LANG_DANISH, SUBLANG_DEFAULT);
    pArg = strtok(0, " ");
  }

  if(IS_NT) SetThreadLocale(MAKELCID(xLangID,SORT_DEFAULT));

  GetModuleFileName(0, cName, sizeof(cName));
  if(strrchr(cName,'\\')) strrchr(cName,'\\')[0] = 0;
  SetCurrentDirectory(cName);

  hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(ID_MAIN));
  hPopupMenu = LoadMenu(hInstance, MAKEINTRESOURCE(ID_POPUP));
  hwndMain = CreateMainWindow(hInstance, nCmdShow);

  capitel_init();

  while(GetMessage(&msg,0,0,0))
  {
    if(!TranslateAccelerator(msg.hwnd, hAccel, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

//  capitel_exit();

  return msg.wParam;
}


/*
 *
 *  PreviousInstance()
 *
 *  Ueberprueft, ob es schon eine Instanz von uns gibt. Wenn ja,
 *  wird das zugehoerige Fenster aktiviert.
 *
 */

int PreviousInstance(void)
{
  HWND hwnd;
  char* pHelp, cName[MAX_PATH];

  GetModuleFileName(0, cName, sizeof(cName));

  pHelp = cName;
  while(*pHelp) *pHelp = tolower(*pHelp == '\\' ? '_' : *pHelp), pHelp++;
  CreateMutex(0, 1, cName);
  if(GetLastError() != ERROR_ALREADY_EXISTS) return 0;

  hwnd = FindWindow(MAINWNDCLASS,0);
  if(hwnd)
  {
    if(IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    if(!IsWindowVisible(hwnd)) ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
  }

  return 1;
}
/*
 *
 *  CreateMainWindow()
 *
 *  registriert die Hauptfenster-Klasse und erstellt
 *  das Hauptfenster.
 *
 */

HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow)
{
  HWND hwnd;
  WNDCLASS wc;
  LONG baseunit = GetDialogBaseUnits();
  char cTitle[256];

  int initial_x = ((200 * LOWORD(baseunit)) / 4) + (GetSystemMetrics(SM_CXSIZEFRAME) * 2) + (GetSystemMetrics(SM_CXEDGE) * 2);
  int initial_y = initial_x * 2 / 3;

  InitCommonControls();

  wc.style         = CS_HREDRAW|CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = MAINWNDEXTRA;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(ID_MAIN));
  wc.hCursor       = LoadCursor(0, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszMenuName  = MAKEINTRESOURCE(ID_MAIN);
  wc.lpszClassName = MAINWNDCLASS;

  if(!RegisterClass(&wc)) return 0;

  LoadString(hInstance, STR_WINDOW_TITLE, cTitle, sizeof(cTitle));
  hwnd = CreateWindow(MAINWNDCLASS, cTitle, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, 0);
  if(!hwnd) return 0;

  if(!RestoreWindowAttributes(hwnd))
  {
    MoveWindow(hwnd, 0, 0, initial_x, initial_y, 0);
    CenterWindow(hwnd, GetDesktopWindow());
    ShowWindow(hwnd, nCmdShow);
  }

  UpdateWindow(hwnd);

  return hwnd;
}


/*
 *
 *  WndProc()
 *
 *  Hauptfenster-Prozedur
 *
 */

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hwndTbar = GetDlgItem(hwnd, ID_TOOLBAR);
    HWND hwndList = GetDlgItem(hwnd, ID_LIST);
    HWND hwndStat = GetDlgItem(hwnd, ID_STATUS);
    HWND hwndTTip = MainWnd_GetToolTip(hwnd);

    switch(message)
    {
        case WM_CREATE:
        {
          NMHDR nmHdr;

//          capitel_init();

          hwndTbar = CreateToolbarEx(hwnd,
            WS_CHILD|WS_VISIBLE|TBSTYLE_TOOLTIPS,
            ID_TOOLBAR, iNumBitmap, xAppInst, ID_TOOLBARBMP,
            tbButtons, sizeof(tbButtons)/sizeof(TBBUTTON),
            20, 18, 20, 18, sizeof(TBBUTTON));

          hwndList = CreateWindowEx(WS_EX_CLIENTEDGE,
            WC_LISTVIEW, 0, WS_CHILD|WS_VISIBLE|LVS_REPORT,
            0, 0, 0, 0, hwnd, (HMENU)ID_LIST, xAppInst, 0);

          hwndStat = CreateWindow(STATUSCLASSNAME,
            0, WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
            0, 0, 0, 0, hwnd, (HMENU)ID_STATUS, xAppInst, 0);

          oldListProc = (WNDPROC)SetWindowLong(hwndList,
            GWL_WNDPROC, (ULONG)newListProc);

          hwndTTip = CreateWindow(TOOLTIPS_CLASS, 0,
            TTS_ALWAYSTIP, 0, 0, 0, 0, hwnd, 0, xAppInst, 0);
          MainWnd_SetToolTip(hwnd, hwndTTip);
          SendMessage(hwndTTip, TTM_ACTIVATE, (WPARAM)1, 0);

          if(config_file_read_ulong(STD_CFG_FILE,
            CAPITEL_ACTIVE,CAPITEL_ACTIVE_DEF))
            SendMessage(hwnd, WM_USER, (WPARAM)8, 0);
          else
            SendMessage(hwnd, WM_USER, (WPARAM)9, 0);

#ifndef RECOTEL
          if(!initRegistration() &&
            config_file_read_ulong(STD_CFG_FILE,DEFAULT_ANSW_DELAY,DEFAULT_ANSW_DELAY_DEF) != 999)
            DialogBoxParam(xAppInst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, (DLGPROC)AboutProc, 1);
#endif

          MainListInit(hwndList);
          InitTrayIcon(hwnd);

          nmHdr.hwndFrom = hwndList;
          nmHdr.idFrom = ID_LIST;
          nmHdr.code = LVN_ITEMCHANGED;
          SendMessage(hwnd, WM_NOTIFY, (WPARAM)ID_LIST, (LPARAM)&nmHdr);

          if(!config_file_read_ulong(STD_CFG_FILE,CAPITEL_RUN_CNT,CAPITEL_RUN_CNT_DEF))
            PostMessage(hwnd, WM_COMMAND, (WPARAM)IDM_PROPERTIES, 0);
          config_file_write_ulong(STD_CFG_FILE,CAPITEL_RUN_CNT,config_file_read_ulong(STD_CFG_FILE,CAPITEL_RUN_CNT,CAPITEL_RUN_CNT_DEF)+1);

          break;
        }

//        case WM_SETTINGCHANGE:
//        {
//            NMHDR sNmHdr = { hwndList, ID_LIST, LVN_ITEMCHANGED };
//            SendMessage(hwnd, WM_NOTIFY, (WPARAM)ID_LIST, (LPARAM)&sNmHdr);
//            break;
//        }

        case WM_SYSCOLORCHANGE:

             SendMessage(hwndTbar, WM_SYSCOLORCHANGE, wParam, lParam);
//             SendMessage(hwndTree, WM_SYSCOLORCHANGE, wParam, lParam);
//             SendMessage(hwndGrip, WM_SYSCOLORCHANGE, wParam, lParam);
             SendMessage(hwndList, WM_SYSCOLORCHANGE, wParam, lParam);
             SendMessage(hwndStat, WM_SYSCOLORCHANGE, wParam, lParam);

             break;

        case WM_SIZE:
            if(GetWindowLong(hwnd, GWL_USERDATA))
                SetWindowPos(hwndList, 0, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER);
            else {
                RECT rectStat, rectTbar;
                SendMessage(hwndTbar, WM_SIZE, 0, 0);
                SendMessage(hwndStat, WM_SIZE, 0, 0);
                GetWindowRect(hwndTbar, &rectTbar);
                GetWindowRect(hwndStat, &rectStat);
                SetWindowPos(hwndList, 0, 0, rectTbar.bottom - rectTbar.top, LOWORD(lParam), HIWORD(lParam) - (rectStat.bottom - rectStat.top) - (rectTbar.bottom - rectTbar.top), SWP_NOZORDER);
            }
            break;

        case WM_ACTIVATE:
            if(LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
                SetFocus(hwndList);
            return 0;

        case WM_MENUSELECT:
        {
          UINT help[2] = {0, 0};

          if(HIWORD(wParam) & MF_POPUP && !(HIWORD(wParam) & MF_SYSMENU))
            help[0] = (UINT)lParam, help[1] = STR_MENUHELP_MAIN1;

          MenuHelp(message, wParam, lParam, (HMENU)help[0], xAppInst, hwndStat, help);

          return 0;
        }

        case WM_NOTIFY:

          switch(((NMHDR*)lParam)->code)
          {
            case TTN_NEEDTEXT:

              ((TOOLTIPTEXT*)lParam)->hinst = xAppInst;
              ((TOOLTIPTEXT*)lParam)->lpszText = (char*)((NMHDR*)lParam)->idFrom;
              return 0;

            case LVN_COLUMNCLICK:

              MainListSort(hwndList, ((NM_LISTVIEW*)lParam)->iSubItem);
              return 0;

            case LVN_ITEMCHANGED:
            {
              tCall* pCall;
              int iIdx, iCnt, iSelCnt, iPlayCnt, iSelPlayCnt;

              iCnt = ListView_GetItemCount(hwndList);
              iSelCnt = ListView_GetSelectedCount(hwndList);

              for(iIdx = iPlayCnt = iSelPlayCnt = 0; iIdx < iCnt; iIdx++)
              {
                pCall = MainListGetCall(hwndList, iIdx);
                if(pCall->iDuration == 0)
                  continue;
                iPlayCnt++;
                if(ListView_GetItemState(hwndList,iIdx,LVIS_SELECTED))
                  iSelPlayCnt++;
              }

              SendMessage(hwndTbar, TB_ENABLEBUTTON,
                (WPARAM)IDM_PLAY, MAKELPARAM(iSelPlayCnt,0));
              SendMessage(hwndTbar, TB_ENABLEBUTTON,
                (WPARAM)IDM_PLAY_ALL, MAKELPARAM(iPlayCnt,0));
              SendMessage(hwndTbar, TB_ENABLEBUTTON,
                (WPARAM)IDM_SAVE_AS, MAKELPARAM((iSelCnt == 1 && iSelPlayCnt == 1),0));
              SendMessage(hwndTbar, TB_ENABLEBUTTON,
                (WPARAM)IDM_DELETE, MAKELPARAM(iSelCnt,0));

              UpdateTrayIcon(hwnd);
              return 0;
            }

            case LVN_DELETEITEM:
            {
              int iCnt;
              tCall* pCall;

              pCall = MainListGetCall(hwndList, ((NMLISTVIEW*)lParam)->iItem);
              DelToolTip(hwndList, hwndTTip, ((NMLISTVIEW*)lParam)->iItem);
              free(pCall);

              PostMessage(hwndList, WM_VSCROLL, 0, 0);

              iCnt = ListView_GetItemCount(hwndList) - 1;
              SendMessage(hwndTbar, TB_ENABLEBUTTON,
                (WPARAM)IDM_PLAY_ALL, MAKELPARAM(iCnt,0));

              UpdateTrayIcon(hwnd);
              return 0;
            }

            case NM_DBLCLK:

              case NM_RETURN:
                SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_PLAY, 1), (LPARAM)((NMHDR*)lParam)->hwndFrom);
                return 0;

              case LVN_KEYDOWN:
                if(((LV_KEYDOWN*)lParam)->wVKey == VK_DELETE)
                  SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_DELETE, 1), (LPARAM)((NMHDR*)lParam)->hwndFrom);
                return 0;

          }
          break;

        case WM_SYSCOMMAND:

          if(wParam == SC_MINIMIZE && MainWnd_GetHideMin(hwnd))
          {
            RECT sRcFrm, sRcTo;

            GetWindowRect(hwnd, &sRcFrm);       /* animation berechnen und      */
            if(GetTrayIconRect(&sRcTo))         /* anzeigen ...                 */
              DrawAnimatedRects(hwnd, IDANI_CAPTION, &sRcFrm, &sRcTo);

            ShowWindow(hwnd, SW_HIDE);          /* fenster unsichtbar machen    */
            return 0;
          }
          break;

        case WM_INITMENU:
        {
          tCall* pCall;
          char caller[256] = "";
          int iIdx, iCnt, iSelCnt, iPlayCnt, iSelPlayCnt;

          iCnt = ListView_GetItemCount(hwndList);
          iSelCnt = ListView_GetSelectedCount(hwndList);

          for(iIdx = iPlayCnt = iSelPlayCnt = 0; iIdx < iCnt; iIdx++)
          {
            pCall = MainListGetCall(hwndList,iIdx);
            if(pCall->iDuration == 0)
              continue;
            iPlayCnt++;
            if(ListView_GetItemState(hwndList,iIdx,LVIS_SELECTED))
              iSelPlayCnt++;
          }

          iIdx = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

          if((HMENU)wParam == GetSubMenu(hPopupMenu,0))
            SetMenuDefaultItem((HMENU)wParam, IDM_PLAY, 0);
          else if((HMENU)wParam == GetSubMenu(hPopupMenu,2))
            SetMenuDefaultItem((HMENU)wParam, IDM_SHOW, 0);

          ListView_GetItemText(hwndList, iIdx, 0, caller, sizeof(caller));

          EnableMenuItem((HMENU)wParam, IDM_PLAY,
            iSelPlayCnt ? MF_ENABLED : MF_GRAYED);
          EnableMenuItem((HMENU)wParam, IDM_PLAY_ALL,
            iPlayCnt ? MF_ENABLED : MF_GRAYED);
          EnableMenuItem((HMENU)wParam, IDM_SAVE_AS,
            (iSelCnt == 1 && iSelPlayCnt == 1) ? MF_ENABLED : MF_GRAYED);
          EnableMenuItem((HMENU)wParam, IDM_DELETE,
            iSelCnt ? MF_ENABLED : MF_GRAYED);
          EnableMenuItem((HMENU)wParam, IDM_ADD_TO_CALLERS,
            (iSelCnt == 1 && onlyDigits(caller)) ? MF_ENABLED : MF_GRAYED);

          CheckMenuItem((HMENU)wParam, IDM_TOPMOST,     /* menu-item checken,   */
            (GetWindowLong(hwnd,GWL_EXSTYLE) & WS_EX_TOPMOST) ? /* wenn topmost */
            MF_CHECKED : MF_UNCHECKED);

          CheckMenuItem((HMENU)wParam, IDM_SHOWTRAY,    /* show-tray-status     */
            MainWnd_GetShowTray(hwnd) ? MF_CHECKED : MF_UNCHECKED); /* checken  */

          CheckMenuItem((HMENU)wParam, IDM_MINHIDE,     /* hide-minimized       */
            MainWnd_GetHideMin(hwnd) ? MF_CHECKED : MF_UNCHECKED); /* checken   */

          return 0;
        }

        case WM_TRAY_CALLBACK:                  /* user hat tray-icon geklickt  */

          if((int)wParam != 1) return 1;
          switch(lParam)
          {
            case WM_LBUTTONDBLCLK:

              SetForegroundWindow(hwnd);
              PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_SHOW,0), 0);
              break;

            case WM_RBUTTONUP:
            {
              POINT pPos;
              SetForegroundWindow(hwnd);
              GetCursorPos(&pPos);
              PostMessage(hwnd, WM_CONTEXTMENU, 0, MAKELPARAM(pPos.x,pPos.y));
              break;
            }
          }
          return 0;

        case WM_COMMAND:

            switch(LOWORD(wParam)) {

// dispatcher ist hier / wfehn

                case IDM_TOGGLEACTIVATION:
                {
                    int active;

                    active = !config_file_read_ulong(
                      STD_CFG_FILE, CAPITEL_ACTIVE, CAPITEL_ACTIVE_DEF);

                    config_file_write_ulong(
                      STD_CFG_FILE, CAPITEL_ACTIVE, active);

                    SendMessage(hwnd,
                      WM_USER, active ? (WPARAM)8 : (WPARAM)9, 0);

                    return 0;
                }

                case IDM_HIDEFRAMECONTROLS:

                    if(GetWindowLong(hwnd, GWL_USERDATA))
                    {
                      RECT rect;
                      ShowWindow(hwndTbar, 1);
                      ShowWindow(hwndStat, 1);
                      SetMenu(hwnd, (HMENU)GetWindowLong(hwnd, GWL_USERDATA));
                      SetWindowLong(hwnd, GWL_USERDATA, 0);
                      GetClientRect(hwnd, &rect);
                      SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(rect.right-rect.left,rect.bottom-rect.top));
                      CheckMenuItem(GetMenu(hwnd),
                        IDM_HIDEFRAMECONTROLS, MF_UNCHECKED);
                      CheckMenuItem(hPopupMenu,
                        IDM_HIDEFRAMECONTROLS, MF_UNCHECKED);
                    }
                    else
                    {
                      ShowWindow(hwndTbar, 0);
                      ShowWindow(hwndStat, 0);
                      SetWindowLong(hwnd, GWL_USERDATA, (LONG)GetMenu(hwnd));
                      SetMenu(hwnd, 0);
                      CheckMenuItem(GetMenu(hwnd),
                        IDM_HIDEFRAMECONTROLS, MF_CHECKED);
                      CheckMenuItem(hPopupMenu,
                        IDM_HIDEFRAMECONTROLS, MF_CHECKED);
                    }
                    return 0;

                case IDM_PROPERTIES:
                    DisplayProperties(hwnd);
                    return 0;

                case IDM_PROP_PORTS:
                    DialogBox(xAppInst,
                      MAKEINTRESOURCE(IDD_EDITPORTLIST),
                      hwnd, (DLGPROC)EditPortListProc);
                    return 0;

                case IDM_PROP_CALLERS:
                    DialogBox(xAppInst,
                      MAKEINTRESOURCE(IDD_EDITCALLERLIST),
                      hwnd, (DLGPROC)EditCallerListProc);
                    return 0;

                case IDM_PROP_ACTIONS:
                    DialogBox(xAppInst,
                      MAKEINTRESOURCE(IDD_EDITACTIONLIST),
                      hwnd, (DLGPROC)EditActionListProc);
                    return 0;

                case IDM_EXIT:

                   SendMessage(hwnd, WM_CLOSE, 0, 0);
                   return 0;

                case IDM_PLAY:

                   answer_stop_bell();
                   PlayCalls(hwndList, 0); /* play selected calls */
                   return 0;

                case IDM_PLAY_ALL:

                   answer_stop_bell();
                   PlayCalls(hwndList, 1); /* play all calls */
                   return 0;

                case IDM_DELETE:
                {
                  tCall* pCall;
                  char cBuff[256];
                  int iIdx, iWarn;

                  answer_stop_bell();

                  iIdx = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                  if(iIdx < 0) return 0;    /* nothing to delete -> get out */

                  for(iWarn = 0; iIdx >= 0;
                    iIdx = ListView_GetNextItem(hwndList,iIdx,LVNI_SELECTED))
                  {
                    pCall = MainListGetCall(hwndList, iIdx);
                    if(pCall->iDuration && !pCall->iState)
                      iWarn++;              /* unabgehoerte rufe zaehlen    */
                  }

                  if(iWarn)                 /* immer warnen, wenn nicht     */
                  {                         /* abgehoerte rufe ausgewaehlt  */
                    LoadString(xAppInst,STR_ASK_REALYDELETE2,cBuff,sizeof(cBuff));
                    if(MyMsgBox(hwnd,cBuff,APPSHORT,MB_YESNO,IDI_WARNING)
                      != IDYES) return 0;
                  }
                  else if(config_file_read_ulong(STD_CFG_FILE,
                    CONFIRM_CALL_DELETE,CONFIRM_CALL_DELETE_DEF))
                  {
                    LoadString(xAppInst,STR_ASK_REALYDELETE,cBuff,sizeof(cBuff));
                    if(MyMsgBox(hwnd,cBuff,APPSHORT,MB_YESNO,IDI_QUESTION)
                      != IDYES) return 0;
                  }

                  while((iIdx = ListView_GetNextItem(hwndList,-1,LVNI_SELECTED)) >= 0)
                  {
                    pCall = MainListGetCall(hwndList, iIdx);

                    remove(pCall->cWavFileName);
                    shRecycle(pCall->cAlwFileName);
                    shRecycle(pCall->cIdxFileName);

                    ListView_DeleteItem(hwndList, iIdx);
                  }

                  UpdateTrayIcon(hwnd);

                  return 0;
                }

                case IDM_SAVE_AS:
                {
                  tCall* pCall;
                  int iIdx, iHlp;
                  OPENFILENAME sOpenFileName;
                  char cAlwFile[MAX_PATH], cWavFile[MAX_PATH], cPath[MAX_PATH],
                    cTmp1[256], cTmp2[256], cTitle[256], cFilt[256], * pHlp;

                  iIdx = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                  if(iIdx < 0) return 0;

                  pCall = MainListGetCall(hwndList, iIdx);
                  strcpy(cAlwFile, pCall->cAlwFileName);

                  memset(&sOpenFileName, 0, sizeof(sOpenFileName));

                  LoadString(xAppInst,STR_FDLG_SAVEAS_FILE,cTmp1,sizeof(cTmp1));
                  ListView_GetItemText(hwndList, iIdx, 0, cTmp2, sizeof(cTmp2));
                  sprintf(cWavFile, cTmp1, cTmp2);

                  for(pHlp = cWavFile; *pHlp; pHlp++)
                  {
                    switch(*pHlp)
                    {
                      case '<': case '>': case ':': case '|': case '/':
                      case '\\': case '\"': memmove(pHlp,pHlp+1,strlen(pHlp));
                    }
                  }

                  LoadString(xAppInst,STR_FDLG_SAVEAS,cTitle,sizeof(cTitle));
                  iHlp = LoadString(xAppInst,STR_FDLG_SAVEAS_FILT,cFilt,sizeof(cFilt));
                  cFilt[iHlp] = cFilt[iHlp+1] = 0;

                  GetSpecialFolderPath(hwnd, CSIDL_PERSONAL, cTmp1);
                  config_file_read_string(STD_CFG_FILE,
                    WINDOW_SAVECALLAS_PATH, cPath, cTmp1);

                  sOpenFileName.lStructSize = sizeof(sOpenFileName);
                  sOpenFileName.hwndOwner = hwnd;
                  sOpenFileName.lpstrFilter = cFilt;
                  sOpenFileName.lpstrFile = cWavFile;
                  sOpenFileName.nMaxFile = sizeof(cWavFile);
                  sOpenFileName.lpstrTitle = cTitle;
                  sOpenFileName.lpstrInitialDir = cPath;
                  sOpenFileName.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|
                    OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST;

                  if(GetSaveFileName(&sOpenFileName))
                  {
                    UpdateWindow(hwnd);
                    SetCursor(LoadCursor(0,IDC_WAIT)); /* show wait cursor  */

                    strncpy(cPath, cWavFile,                  /* extract    */
                      sOpenFileName.nFileOffset - 1);         /* target     */
                    cPath[sOpenFileName.nFileOffset - 1] = 0; /* path and   */
                    config_file_write_string(STD_CFG_FILE,    /* save to    */
                      WINDOW_SAVECALLAS_PATH, cPath);         /* cfg-file   */

                    iHlp = !config_file_read_ulong(STD_CFG_FILE,
                      GENERATE_16_BIT_WAVES, GENERATE_16_BIT_WAVES_DEF);
                    alw2wav(cAlwFile, cWavFile, (short)iHlp);

                    SetCursor(LoadCursor(0,IDC_ARROW)); /* remove wait cur  */
                  }

                  return 0;
                }

                case IDM_ADD_TO_CALLERS:
                {
                  int iIdx;
                  char cNumber[256];

                  iIdx = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
                  if(iIdx < 0) return 0;

                  ListView_GetItemText(hwndList, iIdx, 0, cNumber, sizeof(cNumber));
                  if(!onlyDigits(cNumber)) return 0;
                  if(strchr(cNumber,'(')) strchr(cNumber,'(')[-1] = 0;

                  AddToCallers(hwnd, NAMFILE, cNumber);
                  capitel_dorescan();
                  return 0;
                }

                case IDM_RECORD:
                {
                    int iLen;
                    OPENFILENAME filename;
                    char buff[256], cTitle[256], cFilt[256];

                    strcpy(buff, DEFALWFILE);
                    memset(&filename, 0, sizeof(filename));
                    LoadString(xAppInst,STR_FDLG_RECORD,cTitle,sizeof(cTitle));
                    iLen = LoadString(xAppInst,STR_FDLG_RECORD_FILT,cFilt,sizeof(cFilt));
                    cFilt[iLen] = cFilt[iLen+1] = 0;

                    filename.lStructSize = sizeof(filename);
                    filename.hwndOwner = hwnd;
                    filename.hInstance = xAppInst;
                    filename.lpstrFilter = cFilt;
                    filename.lpstrFile = buff;
                    filename.nMaxFile = sizeof(buff);
                    filename.lpstrTitle = cTitle;
                    filename.Flags = OFN_EXPLORER|OFN_HIDEREADONLY|OFN_NOCHANGEDIR|
                                     OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST;
                    if(GetSaveFileName(&filename))
                        capitel_record_welcome(buff);
                    return 0;
                }

                case IDM_TOPMOST:           /* switch zwischen topmost und  */
                                            /* non-topmost                  */
                  SetWindowPos(hwnd,
                    (GetWindowLong(hwnd,GWL_EXSTYLE) & WS_EX_TOPMOST) ?
                    HWND_NOTOPMOST:HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
                  return 0;

                case IDM_SHOWTRAY:          /* tray-icon ein/aus-schalten   */

                  if(!MainWnd_GetShowTray(hwnd))
                    MainWnd_SetShowTray(hwnd, 1);
                  else
                    MainWnd_SetShowTray(hwnd, 0), MainWnd_SetHideMin(hwnd, 0);

                  UpdateTrayIcon(hwnd);
                  return 0;

                case IDM_MINHIDE:           /* hide-when-minimized ein/aus  */

                  if(MainWnd_GetHideMin(hwnd))
                    MainWnd_SetHideMin(hwnd, 0);
                  else
                    MainWnd_SetHideMin(hwnd, 1), MainWnd_SetShowTray(hwnd, 1);

                  UpdateTrayIcon(hwnd);
                  return 0;

                case IDM_SELECTALL:
                {
                  int iCnt, iIdx;

                  iCnt = ListView_GetItemCount(hwndList);
                  for(iIdx = 0; iIdx < iCnt; iIdx++)
                    ListView_SetItemState(hwndList, iIdx, LVIS_SELECTED, LVIS_SELECTED);

                  return 0;
                }

                case IDM_INVERTSEL:
                {
                  int iCnt, iIdx;

                  iCnt = ListView_GetItemCount(hwndList);
                  for(iIdx = 0; iIdx < iCnt; iIdx++)
                  {
                    ListView_SetItemState(hwndList, iIdx,
                      ListView_GetItemState(hwndList,iIdx,LVIS_SELECTED) ?
                      0 : LVIS_SELECTED, LVIS_SELECTED);
                  }
                  return 0;
                }

                case IDM_SHOW:                      /* fenster wiederherstellen     */

                  if(IsIconic(hwnd))                /* fenster ist minimiert?       */
                    ShowWindow(hwnd, SW_RESTORE);   /* -> fenster restaurieren      */
                  else if(IsWindowVisible(hwnd))    /* fenster ist sichtbar?        */
                    ShowWindow(hwnd, SW_SHOW);      /* -> einfach nur anzeigen      */
                  else
                  {
                    RECT sRcFrm;
                    WINDOWPLACEMENT sPlace;

                    sPlace.length = sizeof(sPlace);
                    GetWindowPlacement(hwnd, &sPlace); /* animation berechnen und   */
                    if(GetTrayIconRect(&sRcFrm)   )    /* anzeigen ...              */
                      DrawAnimatedRects(hwnd, IDANI_CAPTION, &sRcFrm, &sPlace.rcNormalPosition);

                    ShowWindow(hwnd, SW_SHOW);      /* fenster sichtbar machen      */
                  }
                  return 0;

                case IDM_HOMEPAGE:

                  shExecute(APP_HOMEPAGE);
                  return 0;

                case IDM_README:
                {
                  char cCmd[256];
                  LoadString(xAppInst,STR_HELP_README,cCmd,sizeof(cCmd));
                  shExecute(cCmd);
                  return 0;
                }

                case IDM_ORDER:
                {
                  char cCmd[256];
                  LoadString(xAppInst,STR_HELP_ORDER,cCmd,sizeof(cCmd));
                  shExecute(cCmd);
                  return 0;
                }

                case IDM_ORDERBMT:
                {
                  char cCmd[256];
                  LoadString(xAppInst,STR_HELP_ORDERBMT,cCmd,sizeof(cCmd));
                  shExecute(cCmd);
                  return 0;
                }

                case IDM_LICENSE:
                {
                  char cCmd[256];
                  LoadString(xAppInst,STR_HELP_LICENSE,cCmd,sizeof(cCmd));
                  shExecute(cCmd);
                  return 0;
                }

                case IDM_WHATSNEW:
                {
                  char cCmd[256];
                  LoadString(xAppInst,STR_HELP_WHATSNEW,cCmd,sizeof(cCmd));
                  shExecute(cCmd);
                  return 0;
                }

                case IDM_ABOUT:
                    DialogBox(xAppInst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, (DLGPROC)AboutProc);
                    return 0;

                case IDM_REGISTER:
                    DialogBox(xAppInst, MAKEINTRESOURCE(IDD_REGISTER), hwnd, (DLGPROC)RegisterDialogProc);
                    return 0;

            }
            break;

        case WM_CONTEXTMENU:

          if((HWND)wParam == hwndList)        /* context-menu im hauptfenster */
          {
            int iIdx;
            POINT pt;
            RECT rcItem;

            if(lParam == 0xFFFFFFFF)          /* tastatur-event?              */
            {
              iIdx = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
              if(iIdx < 0) SetRectEmpty(&rcItem);           /* rechteck holen */
              else ListView_GetItemRect(hwndList, iIdx, &rcItem, 1);
              pt.x = rcItem.left + (rcItem.right - rcItem.left) / 2; /* mitte */
              pt.y = rcItem.top + (rcItem.bottom - rcItem.top) / 2; /* suchen */
              ClientToScreen(hwndList, &pt);    /* in screen-koord. umwandeln */
            }
            else
            {
              pt.x = LOWORD(lParam);
              pt.y = HIWORD(lParam);
            }

            TrackPopupMenu(GetSubMenu(hPopupMenu,0),
              TPM_LEFTBUTTON|TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, 0);
          }
          else if((HWND)wParam == 0)          /* context-menu im tray         */
          {
            TrackPopupMenu(GetSubMenu(hPopupMenu,2),
              TPM_LEFTBUTTON|TPM_RIGHTBUTTON, LOWORD(lParam), HIWORD(lParam), 0, hwnd, 0);
          }
          break;

        case WM_ENDSESSION:

          SendMessage(hwnd, WM_CLOSE, 1, 0);
          return 0;

        case WM_CLOSE:

          if(!wParam && MyMsgBoxEx(hwnd,MAKEINTRESOURCE(STR_ASK_REALYEXIT),
            APPSHORT,MAKEINTRESOURCE(STR_ASK_REALYEXIT_CHK),MB_YESNO,
            IDI_QUESTION,STD_CFG_FILE,CONFIRM_EXIT_PROGRAM) != IDYES)
            return 0;

          CleanupTrayIcon(hwnd);            /* tray-icon entfernen          */
          StoreWindowAttributes(hwnd);      /* fenster-position speichern   */

          break;

        case WM_DESTROY:

          capitel_exit();

          DestroyWindow(MainWnd_GetToolTip(hwnd));
          DestroyWindow(hwndList);
          DestroyWindow(hwndStat);
          PostQuitMessage(0);

          break;

// wfehn

        case WM_USER:
            switch(wParam)
            {
                case 1: // warning

                  MyMsgBox(hwnd, (char*)lParam, APPSHORT, MB_OK, IDI_WARNING);
                  free((char*)lParam);

                  return 0;

                case 2: // critical error

                  MyMsgBox(hwnd, (char*)lParam, APPSHORT, MB_OK, IDI_ERROR);
                  free((char*)lParam);

                  return 0;

                case 3: // fatal error

                  MyMsgBox(hwnd, (char*)lParam, APPSHORT, MB_OK, IDI_ERROR);
                  free((char*)lParam);

                  return 0;

                case 4: // insert item
                {
                  int index = MainListInsertCall(hwndList,
                    hwndTTip, &sLastCall, 1);

                  if(!config_file_read_ulong(STD_CFG_FILE,
                    RESTORE_WINDOW_ON_NEW_CALL,RESTORE_WINDOW_ON_NEW_CALL_DEF))
                    return 0;

                  if(IsIconic(hwnd))
                    ShowWindow(hwnd, SW_RESTORE);
                  else if(!IsWindowVisible(hwnd))
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_SHOW,0), 0);

                  return 0;
                }

                case 5:  // rescan
                {
                   short rc;
                   tCall sCall;
                   FileInfoTyp FileInfo;

                   ListView_DeleteAllItems(hwndList);

                   for(rc = OsFindFirst(&FileInfo,CALL_MASK_IDX);
                     rc == 0; rc = OsFindNext(&FileInfo))
                   {
                     memset(&sCall, 0, sizeof(sCall));
                     if(!ReadCall(&sCall,FileInfo.FileName)) continue;

                     better_string(sCall.cCallerName, NAMFILE, 2,
                       sCall.cCallerName, sCall.cCallerName);

                    MainListInsertCall(hwndList, hwndTTip, &sCall, 0);
                  }

                  OsFindClose(&FileInfo);
                  return 0;
                }

                case 6: // show incoming-call-popup
                {
                  int iPopFlag;

                  if(hwndPopup) return 0;

                  iPopFlag = config_file_read_ulong(STD_CFG_FILE,
                    SHOW_POPUP_WINDOW, SHOW_POPUP_WINDOW_DEF);
                  if(!iPopFlag) return 0;

                  hwndPopup = CreateDialogParam(xAppInst,
                    MAKEINTRESOURCE(IDD_POPUP), hwnd,
                    (DLGPROC)PopupDialogProc, (LPARAM)iPopFlag);

                  return 0;
                }

                case 7: // destroy incoming-call-popup
                {
                  if(!hwndPopup) return 0;

                  DestroyWindow(hwndPopup);
                  hwndPopup = 0;

                  return 0;
                }

                case 8:  // set status-text to 'registered to...'
                {
                  char cFmt[256], cTxt[256], cName[128];

                  if(initRegistration())
                  {
                    config_file_read_string(STD_CFG_FILE,
                      REGISTER_NAME, cName, REGISTER_NAME);

                    LoadString(xAppInst, STR_TBAR_REG, cFmt, sizeof(cFmt));
                    sprintf(cTxt,cFmt, APPNAME, cName);
                  }
                  else
                  {
                    LoadString(xAppInst, STR_TBAR_UNREG, cFmt, sizeof(cFmt));
                    sprintf(cTxt, cFmt, APPNAME);
                  }

                  SendMessage(hwndStat, SB_SETTEXT, 0, (LPARAM)cTxt);
                  CheckMenuItem(GetMenu(hwnd),
                    IDM_TOGGLEACTIVATION, MF_CHECKED);
                  CheckMenuItem(hPopupMenu,
                    IDM_TOGGLEACTIVATION, MF_CHECKED);

                  UpdateTrayIcon(hwnd);

                  return 0;
                }

                case 9:  // set status-text to 'disabled...'
                {
                  char cTxt[256];

                  LoadString(xAppInst, STR_TBAR_DEACTIVATED, cTxt, sizeof(cTxt));
                  SendMessage(hwndStat, SB_SETTEXT, 0, (LPARAM)cTxt);

                  CheckMenuItem(GetMenu(hwnd),
                    IDM_TOGGLEACTIVATION, MF_UNCHECKED);
                  CheckMenuItem(hPopupMenu,
                    IDM_TOGGLEACTIVATION, MF_UNCHECKED);

                  UpdateTrayIcon(hwnd);

                  return 0;
                }
                case 10:  // set status-text to 'converting...'
                {
                  char cTxt[256];

                  LoadString(xAppInst, STR_TBAR_CONVERTING, cTxt, sizeof(cTxt));
                  SendMessage(hwndStat, SB_SETTEXT, 0, (LPARAM)cTxt);

                  return 0;
                }
                case 11:  // set status-text to 'recording welcome text...'
                {
                  char cTxt[256];

                  LoadString(xAppInst, STR_TBAR_RECWELCOME, cTxt, sizeof(cTxt));
                  SendMessage(hwndStat, SB_SETTEXT, 0, (LPARAM)cTxt);

                  return 0;
                }
            }
            break;

    }

    return DefWindowProc(hwnd, message, wParam, lParam);

}


/*
 *
 *  newListProc()
 *
 *  mit dieser Funktion wird das ListView-Control
 *  unterklassiert, damit es keine WM_COMMAND-messages
 *  verschluckt.
 *
 */

LRESULT CALLBACK newListProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  switch(message)
  {
    case WM_HSCROLL:
    case WM_VSCROLL:
      UpdateAllToolRect(hwnd, MainWnd_GetToolTip(GetParent(hwnd)));
      break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MOUSEMOVE:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    {
      MSG msg = { hwnd, message, wParam, lParam };
      SendMessage(MainWnd_GetToolTip(GetParent(hwnd)),
        TTM_RELAYEVENT, 0, (LPARAM)&msg);
      break;
    }

    case WM_COMMAND:
      if(!lParam)
        return SendMessage(GetParent(hwnd), message, wParam, lParam);
      break;

  }
  return CallWindowProc(oldListProc, hwnd, message, wParam, lParam);
}

/*
 *
 *  DisplayProperties()
 *
 *  ™ffnet den Properties-Dialog.
 *
 */

int DisplayProperties(HWND hwnd)
{
  int iResult;
  PROPSHEETPAGE page[4];
  PROPSHEETHEADER header;
  char* ppText[5], cBuff[256];
  int i=0;

  LoadString(xAppInst, STR_POPWIN_NONE, cBuff, sizeof(cBuff));
  ppText[0] = strdup(cBuff);
  LoadString(xAppInst, STR_POPWIN_TOPLEFT, cBuff, sizeof(cBuff));
  ppText[1] = strdup(cBuff);
  LoadString(xAppInst, STR_POPWIN_TOPRIGHT, cBuff, sizeof(cBuff));
  ppText[2] = strdup(cBuff);
  LoadString(xAppInst, STR_POPWIN_BOTTOMLEFT, cBuff, sizeof(cBuff));
  ppText[3] = strdup(cBuff);
  LoadString(xAppInst, STR_POPWIN_BOTTOMRIGHT, cBuff, sizeof(cBuff));
  ppText[4] = strdup(cBuff);

  memset(page, 0, sizeof(page));
  memset(&header, 0, sizeof(header));

  i = 0;

  page[i].dwSize = sizeof(PROPSHEETPAGE);
  page[i].dwFlags = PSP_DLGINDIRECT;
  page[i].hInstance = xAppInst;
  page[i].pResource =  LoadResource(0,
    FindResource(0,MAKEINTRESOURCE(IDD_PROPPAGE1),RT_DIALOG));
  page[i].pfnDlgProc = (DLGPROC)PropPage1Proc;
  page[i++].lParam = (LPARAM)ppText;

  page[i].dwSize = sizeof(PROPSHEETPAGE);
  page[i].dwFlags = PSP_DLGINDIRECT;
  page[i].hInstance = xAppInst;
  page[i].pResource =  LoadResource(0,
    FindResource(0,MAKEINTRESOURCE(IDD_PROPPAGE2),RT_DIALOG));
  page[i++].pfnDlgProc = (DLGPROC)PropPage2Proc;

#ifndef RECOTEL
  page[i].dwSize = sizeof(PROPSHEETPAGE);
  page[i].dwFlags = PSP_DLGINDIRECT;
  page[i].hInstance = xAppInst;
  page[i].pResource =  LoadResource(0,
    FindResource(0,MAKEINTRESOURCE(IDD_PROPPAGE3),RT_DIALOG));
  page[i++].pfnDlgProc = (DLGPROC)PropPage3Proc;
#endif

  page[i].dwSize = sizeof(PROPSHEETPAGE);
  page[i].dwFlags = PSP_DLGINDIRECT;
  page[i].hInstance = xAppInst;
  page[i].pResource =  LoadResource(0,
    FindResource(0,MAKEINTRESOURCE(IDD_PROPPAGE4),RT_DIALOG));
  page[i++].pfnDlgProc = (DLGPROC)PropPage4Proc;

  header.dwSize = sizeof(PROPSHEETHEADER);
  header.dwFlags = PSH_NOAPPLYNOW|PSH_PROPTITLE|PSH_PROPSHEETPAGE;
  header.hwndParent = hwnd;
  header.hInstance = xAppInst;
  header.pszCaption = APPSHORT;
  header.nPages = i;
  header.nStartPage = 0;
  header.ppsp = page;

  iResult = PropertySheet(&header);

  if(iResult) answer_listen();

  free(ppText[0]);
  free(ppText[1]);
  free(ppText[2]);
  free(ppText[3]);
  free(ppText[4]);

  FreeResource((HGLOBAL)page[0].pResource);
  FreeResource((HGLOBAL)page[1].pResource);
  FreeResource((HGLOBAL)page[2].pResource);
  FreeResource((HGLOBAL)page[3].pResource);

  return iResult;
}


/*
 *
 *  AboutProc()
 *
 *  AboutDialog-Prozedur
 *
 */

LRESULT CALLBACK AboutProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      char cFmt[256], cTxt[256];

      LoadString(xAppInst, STR_ABOUT1, cFmt, sizeof(cFmt));
      sprintf(cTxt, cFmt, APPVER);
      SetDlgItemText(hwnd, IDC_TEXT1, cTxt);

      LoadString(xAppInst, STR_ABOUT2, cFmt, sizeof(cFmt));
      sprintf(cTxt, cFmt, APPDATE);
      SetDlgItemText(hwnd, IDC_TEXT2, cTxt);

      LoadString(xAppInst, STR_ABOUT3, cFmt, sizeof(cFmt));
      sprintf(cTxt, cFmt, APP_AUTOR_1, APP_AUTOR_2);
      SetDlgItemText(hwnd, IDC_TEXT3, cTxt);

      if(!lParam)
      {
        CenterWindow(hwnd, GetWindow(hwnd,GW_OWNER));
      }
      else
      {
        LoadString(xAppInst, STR_ABOUT_UNREG1, cTxt, sizeof(cTxt));
        SetDlgItemText(hwnd, IDC_TEXT4, cTxt);

        LoadString(xAppInst, STR_ABOUT_UNREG2, cTxt, sizeof(cTxt));
        SetDlgItemText(hwnd, IDC_TEXT5, cTxt);

        EnableWindow(GetDlgItem(hwnd, IDOK), 0);
        SetWindowLong(hwnd, DWL_USER, 1);
        SetTimer(hwnd, TID_REGTIMER, 10000, 0);

        CenterWindow(hwnd, GetDesktopWindow());
      }
      return 1;
    }

    case WM_TIMER:

      if((int)wParam == TID_REGTIMER)
      {
        EnableWindow(GetDlgItem(hwnd, IDOK), 1);
        SetWindowLong(hwnd, DWL_USER, 0);
        KillTimer(hwnd, TID_REGTIMER);
      }
      return 1;

    case WM_COMMAND:

      if(!GetWindowLong(hwnd, DWL_USER))
        EndDialog(hwnd, 1);

      return 1;
  }

  return 0;
}


/*
 *
 *  PropPage1Proc() ... PropPage4Proc()
 *
 *  Dialog-Prozeduren fr die einzelnen
 *  Seiten des Properties-Dialogs.
 *
 */

LRESULT CALLBACK PropPage1Proc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      char cVal[128], cDef[128];
      char** ppText = (char**)((PROPSHEETPAGE*)lParam)->lParam;

      /* werte aus konfig-datei lesen und
         dialog-controls initialisieren   */

      /* groupbox "rufannahme" */

      ltoa(DEFAULT_ANSW_DELAY_DEF,cDef,10);
      config_file_read_string(STD_CFG_FILE,
        DEFAULT_ANSW_DELAY, cVal, cDef);
      SetDlgItemText(hwnd, IDC_ANSWER_DELAY, cVal);

      ltoa(DEFAULT_REC_TIME_DEF,cDef,10);
      config_file_read_string(STD_CFG_FILE,
        DEFAULT_REC_TIME, cVal, cDef);
      SetDlgItemText(hwnd, IDC_RECORD_TIME, cVal);

      ltoa(MAX_SILENCE_TIME_DEF,cDef,10);
      config_file_read_string(STD_CFG_FILE,
        MAX_SILENCE_TIME, cVal, cDef);
      SetDlgItemText(hwnd, IDC_SILENCE, cVal);

      /* groupbox "anzeige" */

      CheckDlgButton(hwnd, IDC_RESTORE_WINDOW,
        config_file_read_ulong(STD_CFG_FILE,
        RESTORE_WINDOW_ON_NEW_CALL,RESTORE_WINDOW_ON_NEW_CALL_DEF) ?
        BST_CHECKED : BST_UNCHECKED);

      CheckDlgButton(hwnd, IDC_IGNORE_EMPTY,
        config_file_read_ulong(STD_CFG_FILE,
        IGNORE_EMPTY_CALLS,IGNORE_EMPTY_CALLS_DEF) ?
        BST_CHECKED : BST_UNCHECKED);

      CheckDlgButton(hwnd, IDC_EXPAND_CALLER_ID,
        config_file_read_ulong(STD_CFG_FILE,
        EXPAND_CALLER_ID,EXPAND_CALLER_ID_DEF) ?
        BST_CHECKED : BST_UNCHECKED);

      CheckDlgButton(hwnd, IDC_CONFIRM_DELETE,
        config_file_read_ulong(STD_CFG_FILE,
        CONFIRM_CALL_DELETE,CONFIRM_CALL_DELETE_DEF) ?
        BST_CHECKED : BST_UNCHECKED);

      CheckDlgButton(hwnd, IDC_IS_CALLER_ID,
        config_file_read_ulong(STD_CFG_FILE,
        CAPITEL_IS_CALLER_ID,CAPITEL_IS_CALLER_ID_DEF) ?
        BST_CHECKED : BST_UNCHECKED);

      SendDlgItemMessage(hwnd, IDC_POPUPWIN, CB_ADDSTRING, 0, (LPARAM)ppText[0]);
      SendDlgItemMessage(hwnd, IDC_POPUPWIN, CB_ADDSTRING, 0, (LPARAM)ppText[1]);
      SendDlgItemMessage(hwnd, IDC_POPUPWIN, CB_ADDSTRING, 0, (LPARAM)ppText[2]);
      SendDlgItemMessage(hwnd, IDC_POPUPWIN, CB_ADDSTRING, 0, (LPARAM)ppText[3]);
      SendDlgItemMessage(hwnd, IDC_POPUPWIN, CB_ADDSTRING, 0, (LPARAM)ppText[4]);

      SendDlgItemMessage(hwnd, IDC_POPUPWIN, CB_SETCURSEL,
        config_file_read_ulong(STD_CFG_FILE,
        SHOW_POPUP_WINDOW,SHOW_POPUP_WINDOW_DEF), 0);

      return 1;
    }

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_APPLY)
      {
        char buff[256];

        /* groupbox "rufannahme" */

        GetDlgItemText(hwnd, IDC_ANSWER_DELAY, buff, sizeof(buff));
        config_file_write_ulong(STD_CFG_FILE,
          DEFAULT_ANSW_DELAY, atoi(buff));

        GetDlgItemText(hwnd, IDC_RECORD_TIME, buff, sizeof(buff));
        config_file_write_ulong(STD_CFG_FILE,
          DEFAULT_REC_TIME, atoi(buff));

        GetDlgItemText(hwnd, IDC_SILENCE, buff, sizeof(buff));
        config_file_write_ulong(STD_CFG_FILE,
          MAX_SILENCE_TIME, atoi(buff));

        /* groupbox "anzeige" */

        config_file_write_ulong(STD_CFG_FILE, RESTORE_WINDOW_ON_NEW_CALL,
          !!IsDlgButtonChecked(hwnd, IDC_RESTORE_WINDOW));

        config_file_write_ulong(STD_CFG_FILE, IGNORE_EMPTY_CALLS,
          !!IsDlgButtonChecked(hwnd, IDC_IGNORE_EMPTY));

        config_file_write_ulong(STD_CFG_FILE, EXPAND_CALLER_ID,
          !!IsDlgButtonChecked(hwnd, IDC_EXPAND_CALLER_ID));

        config_file_write_ulong(STD_CFG_FILE, CONFIRM_CALL_DELETE,
          !!IsDlgButtonChecked(hwnd, IDC_CONFIRM_DELETE));

        config_file_write_ulong(STD_CFG_FILE, CAPITEL_IS_CALLER_ID,
          !!IsDlgButtonChecked(hwnd, IDC_IS_CALLER_ID));

        config_file_write_ulong(STD_CFG_FILE, SHOW_POPUP_WINDOW,
          SendDlgItemMessage(hwnd,IDC_POPUPWIN,CB_GETCURSEL,0,0));
      }
      return 1;
  }

  return 0;
}


LRESULT CALLBACK PropPage2Proc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      char cFile[MAX_PATH];

      /* werte aus konfig-datei lesen und
         dialog-controls initialisieren   */

      /* groupbox "ansagetext" */

       config_file_read_string(STD_CFG_FILE,
        WELCOME_WAVE, cFile, WELCOME_WAVE_DEF);
       SetDlgItemText(hwnd, IDC_WELCOME_TEXT, cFile);

      /* groupbox "klingel" */

      CheckDlgButton(hwnd, IDC_WAVE_RINGING,
        config_file_read_ulong(STD_CFG_FILE,
        PLAY_RINGRING_WAVE,PLAY_RINGRING_WAVE_DEF) ?
        BST_CHECKED : BST_UNCHECKED);

      config_file_read_string(STD_CFG_FILE,
        RINGRING_WAVE, cFile, RINGRING_WAVE_DEF);
      SetDlgItemText(hwnd, IDC_RINGING_WAVE, cFile);

      return 1;
    }

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_APPLY)
      {
        char buff[256];

        /* groupbox "ansagetext" */

        GetDlgItemText(hwnd, IDC_WELCOME_TEXT, buff, sizeof(buff));
        config_file_write_string(STD_CFG_FILE, WELCOME_WAVE, buff);

        /* groupbox "klingel" */

        config_file_write_ulong(STD_CFG_FILE, PLAY_RINGRING_WAVE, !!IsDlgButtonChecked(hwnd, IDC_WAVE_RINGING));

        GetDlgItemText(hwnd, IDC_RINGING_WAVE, buff, sizeof(buff));
        config_file_write_string(STD_CFG_FILE, RINGRING_WAVE, buff);
      }
      return 1;

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDC_FILEDLG:
        {
          int iLen;
          OPENFILENAME filename;
          char cFile[MAX_PATH], cTitle[256], cFilt[256];

          LoadString(xAppInst, STR_FDLG_WELCOME, cTitle, sizeof(cTitle));
          iLen = LoadString(xAppInst, STR_FDLG_WELCOME_FILT, cFilt, sizeof(cFilt));
          cFilt[iLen] = cFilt[iLen+1] = 0;

          GetDlgItemText(hwnd, IDC_WELCOME_TEXT, cFile, sizeof(cFile));

          memset(&filename, 0, sizeof(filename));
          filename.lStructSize = sizeof(filename);
          filename.hwndOwner = hwnd;
          filename.hInstance = xAppInst;
          filename.lpstrFilter = cFilt;
          filename.lpstrFile = cFile;
          filename.nMaxFile = sizeof(cFile);
          filename.lpstrTitle = cTitle;
          filename.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|
            OFN_NOCHANGEDIR|OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST;

          if(GetOpenFileName(&filename))
          {
            char cDir[MAX_PATH];
            GetModuleFileName(0, cDir, sizeof(cDir));
            if(strrchr(cDir,'\\')) *strrchr(cDir,'\\') = 0;
            if(strncmp(cFile,cDir,strlen(cDir)))
              SetDlgItemText(hwnd, IDC_WELCOME_TEXT, cFile);
            else
              SetDlgItemText(hwnd, IDC_WELCOME_TEXT,cFile+strlen(cDir)+1);
          }
          return 1;
        }

        case IDC_WAVEDLG:
        {
          int iLen;
          OPENFILENAME filename;
          char cFile[MAX_PATH], cTitle[256], cFilt[256];

          LoadString(xAppInst, STR_FDLG_RINGING, cTitle, sizeof(cTitle));
          iLen = LoadString(xAppInst, STR_FDLG_RINGING_FILT, cFilt, sizeof(cFilt));
          cFilt[iLen] = cFilt[iLen+1] = 0;

          GetDlgItemText(hwnd, IDC_RINGING_WAVE, cFile, sizeof(cFile));

          memset(&filename, 0, sizeof(filename));
          filename.lStructSize = sizeof(filename);
          filename.hwndOwner = hwnd;
          filename.hInstance = xAppInst;
          filename.lpstrFilter = cFilt;
          filename.lpstrFile = cFile;
          filename.nMaxFile = sizeof(cFile);
          filename.lpstrTitle = cTitle;
          filename.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|
            OFN_NOCHANGEDIR|OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST;

          if(GetOpenFileName(&filename))
          {
            char cDir[MAX_PATH];
            GetModuleFileName(0, cDir, sizeof(cDir));
            if(strrchr(cDir,'\\')) *strrchr(cDir,'\\') = 0;
            if(strncmp(cFile,cDir,strlen(cDir)))
              SetDlgItemText(hwnd, IDC_RINGING_WAVE, cFile);
            else
              SetDlgItemText(hwnd, IDC_RINGING_WAVE,cFile+strlen(cDir)+1);
          }

          return 1;
        }

        case IDC_EDIT_PORTS:

          DialogBox(xAppInst,
            MAKEINTRESOURCE(IDD_EDITPORTLIST),
            hwnd, (DLGPROC)EditPortListProc);

          return 1;

        case IDC_EDIT_CALLERS:

          DialogBox(xAppInst,
            MAKEINTRESOURCE(IDD_EDITCALLERLIST),
            hwnd, (DLGPROC)EditCallerListProc);

          return 1;

      }
      return 1;

  }

  return 0;
}


LRESULT CALLBACK PropPage3Proc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      char cVal[1024];

      /* werte aus konfig-datei lesen und
         dialog-controls initialisieren   */

      CheckDlgButton(hwnd, IDC_DTMF_SUPPORT,
        config_file_read_ulong(STD_CFG_FILE,
        DETECT_DTMF_TONES,DETECT_DTMF_TONES_DEF) ?
        BST_CHECKED : BST_UNCHECKED);

      config_file_read_string(STD_CFG_FILE,
        CALL_BACK_NUMBER, cVal, CALL_BACK_NUMBER_DEF);
      SetDlgItemText(hwnd, IDC_FORWARD, cVal);

      return 1;
    }

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_APPLY)
      {
        char buff[256];

        config_file_write_ulong(STD_CFG_FILE,DETECT_DTMF_TONES,
          !!IsDlgButtonChecked(hwnd, IDC_DTMF_SUPPORT));

        GetDlgItemText(hwnd, IDC_FORWARD, buff, sizeof(buff));
        config_file_write_string(STD_CFG_FILE, CALL_BACK_NUMBER, buff);

      }
      return 1;

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDC_EDIT_ACTIONS:

          DialogBox(xAppInst,
            MAKEINTRESOURCE(IDD_EDITACTIONLIST),
            hwnd, (DLGPROC)EditActionListProc);

          return 1;
      }
      return 1;

  }
  return 0;
}


LRESULT CALLBACK PropPage4Proc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      char cFile[MAX_PATH];

      /* werte aus konfig-datei lesen und
         dialog-controls initialisieren   */

      CheckDlgButton(hwnd, IDC_ULAW_CODEC,
        config_file_read_ulong(STD_CFG_FILE,
        CAPITEL_CODEC_ULAW,CAPITEL_CODEC_ULAW_DEF) ?
        BST_CHECKED : BST_UNCHECKED);

      config_file_read_string(STD_CFG_FILE,
        NAME_OF_LOG_FILE, cFile, NAME_OF_LOG_FILE_DEF);
      SetDlgItemText(hwnd, IDC_LOGFILE, cFile);

      return 1;
    }

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_APPLY)
      {
        char buff[256];

        config_file_write_ulong(STD_CFG_FILE, CAPITEL_CODEC_ULAW,
          !!IsDlgButtonChecked(hwnd, IDC_ULAW_CODEC));

        GetDlgItemText(hwnd, IDC_LOGFILE, buff, sizeof(buff));
        config_file_write_string(STD_CFG_FILE, NAME_OF_LOG_FILE, buff);
      }
      return 1;
  }
  return 0;
}

/* ------------------------------------------------------------------------
 * die folgenden funktionen dienen zum editieren der port-liste
 * ------------------------------------------------------------------------ */

typedef enum
{
  eUnchanged = -2, eDefault = -1,
  eNormal = 0, eBusy = 1, eRefuse = 2, eUnavailable = 3
}
tReaction;

typedef struct sPort
{
  int fActive;
  char cDsc[256];
  char cMsn[256];
  char cWelcomeWave[1024];
  char cRingingWave[1024];
  char cForwardNumber[1024];
  tReaction eReact;
  char cOldReact[1024];
  char cAnsDly[1024];
  char cMaxRec[1024];
}
tPort;

typedef struct sCaller
{
  int fActive;
  char cDsc[256];
  char cClip[256];
  char cWelcomeWave[1024];
  char cRingingWave[1024];
  char cForwardNumber[1024];
  tReaction eReact;
  char cOldReact[1024];
  char cAnsDly[1024];
  char cMaxRec[1024];
}
tCaller;

typedef enum
{
  eRemoteControl,
  eReboot,
  eDeactivate,
  eQuit,
  eSetCallback,
  eCommandLine
}
tActionType;

typedef struct sAction
{
  int fActive;
  char cDtmf[256];
  tActionType eType;
  char cCmdProg[1024];
  char cCmdArgs[1024];
}
tAction;


void* GetSelectedPtr(HWND hwndList);
void* GetItemPtr(HWND hwndList, int iIdx);


int InsertPort(HWND hwndList, tPort* pPort);
int UpdatePort(HWND hwndList, tPort* pPort);
int DelPort(HWND hwndList, tPort* pPort);

#define GetSelectedPort(hwnd) \
  ((tPort*)GetSelectedPtr((hwnd)))
#define GetItemPort(hwnd,idx) \
  ((tPort*)GetItemPtr((hwnd),(idx)))

int LoadPortList(HWND hwndList, char* pFileName);
int SavePortList(HWND hwndList, char* pFileName);
int FreePortList(HWND hwndList);


int InsertCaller(HWND hwndList, tCaller* pCaller);
int UpdateCaller(HWND hwndList, tCaller* pCaller);
int DelCaller(HWND hwndList, tCaller* pCaller);

#define GetSelectedCaller(hwnd) \
  ((tCaller*)GetSelectedPtr((hwnd)))
#define GetItemCaller(hwnd,idx) \
  ((tCaller*)GetItemPtr((hwnd),(idx)))

int LoadCallerList(HWND hwndList, char* pFileName);
int SaveCallerList(HWND hwndList, char* pFileName);
int FreeCallerList(HWND hwndList);


int InsertAction(HWND hwndList, tAction* pAction);
int UpdateAction(HWND hwndList, tAction* pAction);
int DelAction(HWND hwndList, tAction* pAction);

#define GetSelectedAction(hwnd) \
  ((tAction*)GetSelectedPtr((hwnd)))
#define GetItemAction(hwnd,idx) \
  ((tAction*)GetItemPtr((hwnd),(idx)))

int LoadActionList(HWND hwndList, char* pFileName);
int SaveActionList(HWND hwndList, char* pFileName);
int FreeActionList(HWND hwndList);


void* GetSelectedPtr(HWND hwndList)
{
  int iIdx;

  iIdx = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
  if(iIdx < 0) return 0;

  return GetItemPtr(hwndList, iIdx);
}

void* GetItemPtr(HWND hwndList, int iIdx)
{
  LV_ITEM sItem;

  sItem.mask = LVIF_PARAM;
  sItem.iItem = iIdx;
  sItem.iSubItem = 0;

  if(!ListView_GetItem(hwndList,&sItem)) return 0;
  else return (void*)sItem.lParam;
}

int CALLBACK SortPortsFunc(
  LPARAM lParam1,
  LPARAM lParam2,
  LPARAM lParamSort)
{
  tPort* pPort1, * pPort2;

  pPort1 = (tPort*)lParam1;
  pPort2 = (tPort*)lParam2;

  return strnicmp(pPort1->cDsc, pPort2->cDsc, strlen(pPort1->cDsc));
}

int InsertPort(HWND hwndList, tPort* pPort)
{
  int iIdx;
  LV_ITEM sItem;

  memset(&sItem, 0, sizeof(sItem));
  sItem.mask = LVIF_PARAM;
  sItem.iItem = ListView_GetItemCount(hwndList);
  sItem.iSubItem = 0;
  sItem.lParam = (LPARAM)pPort;

  iIdx = ListView_InsertItem(hwndList, &sItem);
  UpdatePort(hwndList, pPort);

  return 1;
}

int UpdatePort(HWND hwndList, tPort* pPort)
{
  int iIdx;
  RECT rcClient;
  LV_ITEM sItem;
  LV_FINDINFO sFind;

  sFind.flags = LVFI_PARAM;
  sFind.lParam = (LPARAM)pPort;

  iIdx = ListView_FindItem(hwndList, -1, &sFind);
  if(iIdx < 0) return 0;

  sItem.mask = LVIF_IMAGE;
  sItem.iItem = iIdx;
  sItem.iSubItem = 0;
  sItem.iImage = pPort->fActive;
  ListView_SetItem(hwndList, &sItem);

  ListView_SetItemText(hwndList, iIdx, 0, pPort->cDsc);
  ListView_SetItemText(hwndList, iIdx, 1, pPort->cMsn);

  ListView_SortItems(hwndList, SortPortsFunc, 0);

  GetClientRect(hwndList, &rcClient);
  ListView_SetColumnWidth(hwndList, 1,
    rcClient.right - ListView_GetColumnWidth(hwndList,0));

  return 1;
}

int DelPort(HWND hwndList, tPort* pPort)
{
  int iIdx;
  RECT rcClient;
  LV_FINDINFO sFind;

  sFind.flags = LVFI_PARAM;
  sFind.lParam = (LPARAM)pPort;

  iIdx = ListView_FindItem(hwndList, -1, &sFind);
  if(iIdx < 0) return 0;

  ListView_DeleteItem(hwndList, iIdx);

  GetClientRect(hwndList, &rcClient);
  ListView_SetColumnWidth(hwndList, 1,
    rcClient.right - ListView_GetColumnWidth(hwndList,0));

  free(pPort);

  return 1;
}

int LoadPortList(HWND hwndList, char* pFileName)
{
  FILE* pFile;
  tPort* pPort;
  char cLine[4096], * pHlp;

  pFile = fopen(pFileName, "r");
  if(!pFile) return 0;

  while(fgets(cLine,sizeof(cLine),pFile))
  {
    util_delete_cr(cLine);                  /* delete trailing cr/lf        */

    if(!cLine[0] || cLine[0] == '#')        /* skip if comment or           */
      continue;                             /* empty line                   */

    pPort = (tPort*)calloc(1, sizeof(tPort)); /* allocate and zero out mem  */

    pHlp = strtok(cLine, "|");              /* get msn                      */
    if(pHlp) strcpy(pPort->cMsn, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get description              */
    if(pHlp) strcpy(pPort->cDsc, pHlp);
    else strcpy(pPort->cDsc, pPort->cMsn);  /* no dsc? take msn as dsc ...  */

    if(pHlp) pHlp = strtok(0, "|");         /* get welcome-wave             */
    if(pHlp && pHlp[0] != '*') strcpy(pPort->cWelcomeWave, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get reaction                 */
    if(!pHlp) pPort->eReact = eNormal;
    else
    {
      strcpy(pPort->cOldReact, pHlp);
      while(isdigit(*pHlp)) pHlp++;
      if(*pHlp) pPort->eReact = eUnchanged;
      else pPort->eReact = atoi(pPort->cOldReact);
    }

    if(pHlp) pHlp = strtok(0, "|");         /* get max rec time             */
    if(pHlp && pHlp[0] != '*') strcpy(pPort->cMaxRec, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get answer delay             */
    if(pHlp && pHlp[0] != '*') strcpy(pPort->cAnsDly, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get active flag              */
    pPort->fActive = pHlp ? atoi(pHlp) : 1;

    if(pHlp) pHlp = strtok(0, "|");         /* get ring-wave                */
    if(pHlp && pHlp[0] != '*') strcpy(pPort->cRingingWave, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get forward-number           */
    if(pHlp && pHlp[0] != '*') strcpy(pPort->cForwardNumber, pHlp);

    InsertPort(hwndList, pPort);
  }

  fclose(pFile);

  return 1;
}

int SavePortList(HWND hwndList, char* pFileName)
{
  FILE* pFile;
  tPort* pPort;
  int iIdx, iCnt;

  pFile = fopen(pFileName, "w");
  if(!pFile) return 0;

  iCnt = ListView_GetItemCount(hwndList);
  for(iIdx = 0; iIdx < iCnt; iIdx++)
  {
    pPort = GetItemPort(hwndList, iIdx);
    if(!pPort) continue;

    if(pPort->eReact != eUnchanged)
      sprintf(pPort->cOldReact, "%d", (int)pPort->eReact);

    fprintf(pFile, "%s|%s|%s|%s|%s|%s|%s|%s|%s\n",
      strlen(pPort->cMsn) ? pPort->cMsn : "?",
      strlen(pPort->cDsc) ? pPort->cDsc : "?",
      strlen(pPort->cWelcomeWave) ? pPort->cWelcomeWave : "*",
      pPort->cOldReact,
      strlen(pPort->cMaxRec) ? pPort->cMaxRec : "*",
      strlen(pPort->cAnsDly) ? pPort->cAnsDly : "*",
      pPort->fActive ? "1" : "0",
      strlen(pPort->cRingingWave) ? pPort->cRingingWave : "*",
      strlen(pPort->cForwardNumber) ? pPort->cForwardNumber : "*");
  }

  fclose(pFile);

  return 1;
}

int FreePortList(HWND hwndList)
{
  tPort* pPort;
  int iIdx, iCnt;

  iCnt = ListView_GetItemCount(hwndList);
  for(iIdx = 0; iIdx < iCnt; iIdx++)
  {
    pPort = GetItemPort(hwndList, iIdx);
    if(pPort) free(pPort);
  }

  return 1;
}


int CALLBACK SortCallersFunc(
  LPARAM lParam1,
  LPARAM lParam2,
        LPARAM lParamSort)
{
  tCaller* pCaller1, * pCaller2;

  pCaller1 = (tCaller*)lParam1;
  pCaller2 = (tCaller*)lParam2;

  return strnicmp(pCaller1->cDsc, pCaller2->cDsc, strlen(pCaller1->cDsc));
}

int InsertCaller(HWND hwndList, tCaller* pCaller)
{
  int iIdx;
  LV_ITEM sItem;

  memset(&sItem, 0, sizeof(sItem));
  sItem.mask = LVIF_PARAM;
  sItem.iItem = ListView_GetItemCount(hwndList);
  sItem.iSubItem = 0;
  sItem.lParam = (LPARAM)pCaller;

  iIdx = ListView_InsertItem(hwndList, &sItem);
  UpdateCaller(hwndList, pCaller);

  return 1;
}

int UpdateCaller(HWND hwndList, tCaller* pCaller)
{
  int iIdx;
  RECT rcClient;
  LV_ITEM sItem;
  LV_FINDINFO sFind;

  sFind.flags = LVFI_PARAM;
  sFind.lParam = (LPARAM)pCaller;

  iIdx = ListView_FindItem(hwndList, -1, &sFind);
  if(iIdx < 0) return 0;

  sItem.mask = LVIF_IMAGE;
  sItem.iItem = iIdx;
  sItem.iSubItem = 0;
  sItem.iImage = pCaller->fActive;
  ListView_SetItem(hwndList, &sItem);

  ListView_SetItemText(hwndList, iIdx, 0, pCaller->cDsc);
  ListView_SetItemText(hwndList, iIdx, 1, pCaller->cClip);

  ListView_SortItems(hwndList, SortCallersFunc, 0);
  GetClientRect(hwndList, &rcClient);
  ListView_SetColumnWidth(hwndList, 1,
    rcClient.right - ListView_GetColumnWidth(hwndList,0));

  return 1;
}

int DelCaller(HWND hwndList, tCaller* pCaller)
{
  int iIdx;
  RECT rcClient;
  LV_FINDINFO sFind;

  sFind.flags = LVFI_PARAM;
  sFind.lParam = (LPARAM)pCaller;

  iIdx = ListView_FindItem(hwndList, -1, &sFind);
  if(iIdx < 0) return 0;

  ListView_DeleteItem(hwndList, iIdx);

  GetClientRect(hwndList, &rcClient);
  ListView_SetColumnWidth(hwndList, 1,
    rcClient.right - ListView_GetColumnWidth(hwndList,0));

  free(pCaller);

  return 1;
}

int LoadCallerList(HWND hwndList, char* pFileName)
{
  FILE* pFile;
  tCaller* pCaller;
  char cLine[4096], * pHlp;

  pFile = fopen(pFileName, "r");
  if(!pFile) return 0;

  while(fgets(cLine,sizeof(cLine),pFile))
  {
    util_delete_cr(cLine);                  /* delete trailing cr/lf        */

    if(!cLine[0] || cLine[0] == '#')        /* skip if comment or           */
      continue;                             /* empty line                   */

    pCaller = (tCaller*)calloc(1, sizeof(tCaller)); /* alloc & zero out mem */

    pHlp = strtok(cLine, "|");              /* get msn                      */
    if(pHlp) strcpy(pCaller->cClip, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get description              */
    if(pHlp) strcpy(pCaller->cDsc, pHlp);
    else strcpy(pCaller->cDsc, pCaller->cClip); /* no dsc? take num as dsc  */

    if(pHlp) pHlp = strtok(0, "|");         /* get welcome-wave             */
    if(pHlp && pHlp[0] != '*') strcpy(pCaller->cWelcomeWave, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get reaction                 */
    if(!pHlp || !strcmp(pHlp,"*")) pCaller->eReact = eDefault;
    else
    {
      strcpy(pCaller->cOldReact, pHlp);
      while(isdigit(*pHlp)) pHlp++;
      if(*pHlp) pCaller->eReact = eUnchanged;
      else pCaller->eReact = atoi(pCaller->cOldReact);
    }

    if(pHlp) pHlp = strtok(0, "|");         /* get max rec time             */
    if(pHlp && pHlp[0] != '*') strcpy(pCaller->cMaxRec, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get answer delay             */
    if(pHlp && pHlp[0] != '*') strcpy(pCaller->cAnsDly, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get active flag              */
    pCaller->fActive = pHlp ? atoi(pHlp) : 1;

    if(pHlp) pHlp = strtok(0, "|");         /* get ring-wave                */
    if(pHlp && pHlp[0] != '*') strcpy(pCaller->cRingingWave, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get forward-number           */
    if(pHlp && pHlp[0] != '*') strcpy(pCaller->cForwardNumber, pHlp);

    InsertCaller(hwndList, pCaller);
  }

  fclose(pFile);

  return 1;
}

int SaveCallerList(HWND hwndList, char* pFileName)
{
  FILE* pFile;
  int iIdx, iCnt;
  tCaller* pCaller;

  pFile = fopen(pFileName, "w");
  if(!pFile) return 0;

  iCnt = ListView_GetItemCount(hwndList);
  for(iIdx = 0; iIdx < iCnt; iIdx++)
  {
    pCaller = GetItemCaller(hwndList, iIdx);
    if(!pCaller) continue;

    if(pCaller->eReact != eUnchanged)
      sprintf(pCaller->cOldReact, (pCaller->eReact == eDefault) ?
        "*" : "%d", (int)pCaller->eReact);

    fprintf(pFile, "%s|%s|%s|%s|%s|%s|%s|%s|%s\n",
      strlen(pCaller->cClip) ? pCaller->cClip : "?",
      strlen(pCaller->cDsc) ? pCaller->cDsc : "?",
      strlen(pCaller->cWelcomeWave) ? pCaller->cWelcomeWave : "*",
      pCaller->cOldReact,
      strlen(pCaller->cMaxRec) ? pCaller->cMaxRec : "*",
      strlen(pCaller->cAnsDly) ? pCaller->cAnsDly : "*",
      pCaller->fActive ? "1" : "0",
      strlen(pCaller->cRingingWave) ? pCaller->cRingingWave : "*",
      strlen(pCaller->cForwardNumber) ? pCaller->cForwardNumber : "*");
  }

  fclose(pFile);

  return 1;
}

int FreeCallerList(HWND hwndList)
{
  tCaller* pCaller;
  int iIdx, iCnt;

  iCnt = ListView_GetItemCount(hwndList);
  for(iIdx = 0; iIdx < iCnt; iIdx++)
  {
    pCaller = GetItemCaller(hwndList, iIdx);
    if(pCaller) free(pCaller);
  }

  return 1;
}

int AddToCallers(HWND hwnd, char* pFileName, char* pNumber)
{
  FILE* pFile;
  tCaller sCaller;

  memset(&sCaller, 0, sizeof(sCaller));
  strcpy(sCaller.cClip, pNumber);
  sCaller.fActive = 1;

  if(!DialogBoxParam(xAppInst,
    MAKEINTRESOURCE(IDD_EDITCALLER),hwnd,
    (DLGPROC)EditCallerProc,(LPARAM)&sCaller)) return 0;

  pFile = fopen(pFileName, "a");
  if(!pFile) return 0;

  sprintf(sCaller.cOldReact,
    (sCaller.eReact == eDefault) ? "*" : "%d", (int)sCaller.eReact);

  fprintf(pFile, "%s|%s|%s|%s|%s|%s|%s|%s|%s\n",
    strlen(sCaller.cClip) ? sCaller.cClip : "?",
    strlen(sCaller.cDsc) ? sCaller.cDsc : "?",
    strlen(sCaller.cWelcomeWave) ? sCaller.cWelcomeWave : "*",
    sCaller.cOldReact,
    strlen(sCaller.cMaxRec) ? sCaller.cMaxRec : "*",
    strlen(sCaller.cAnsDly) ? sCaller.cAnsDly : "*",
    sCaller.fActive ? "1" : "0",
    strlen(sCaller.cRingingWave) ? sCaller.cRingingWave : "*",
    strlen(sCaller.cForwardNumber) ? sCaller.cForwardNumber : "*");

  fclose(pFile);

  return 1;
}


int CALLBACK SortActionsFunc(
  LPARAM lParam1,
  LPARAM lParam2,
        LPARAM lParamSort)
{
  tAction* pAction1, * pAction2;

  pAction1 = (tAction*)lParam1;
  pAction2 = (tAction*)lParam2;

  return strnicmp(pAction1->cDtmf, pAction2->cDtmf, strlen(pAction1->cDtmf));
}

int InsertAction(HWND hwndList, tAction* pAction)
{
  int iIdx;
  LV_ITEM sItem;

  memset(&sItem, 0, sizeof(sItem));
  sItem.mask = LVIF_PARAM;
  sItem.iItem = ListView_GetItemCount(hwndList);
  sItem.iSubItem = 0;
  sItem.lParam = (LPARAM)pAction;

  iIdx = ListView_InsertItem(hwndList, &sItem);
  UpdateAction(hwndList, pAction);

  return 1;
}

int UpdateAction(HWND hwndList, tAction* pAction)
{
  int iIdx;
  RECT rcClient;
  LV_ITEM sItem;
  char cText[1024];
  LV_FINDINFO sFind;

  sFind.flags = LVFI_PARAM;
  sFind.lParam = (LPARAM)pAction;

  iIdx = ListView_FindItem(hwndList, -1, &sFind);
  if(iIdx < 0) return 0;

  sItem.mask = LVIF_IMAGE;
  sItem.iItem = iIdx;
  sItem.iSubItem = 0;
  sItem.iImage = pAction->fActive;
  ListView_SetItem(hwndList, &sItem);

  ListView_SetItemText(hwndList, iIdx, 0, pAction->cDtmf);
  switch(pAction->eType)
  {
    case eRemoteControl:
      LoadString(xAppInst, STR_LIST_ACTION_1, cText, sizeof(cText)); break;
    case eReboot:
      LoadString(xAppInst, STR_LIST_ACTION_2, cText, sizeof(cText)); break;
    case eDeactivate:
      LoadString(xAppInst, STR_LIST_ACTION_3, cText, sizeof(cText)); break;
    case eQuit:
      LoadString(xAppInst, STR_LIST_ACTION_4, cText, sizeof(cText)); break;
    case eSetCallback:
      LoadString(xAppInst, STR_LIST_ACTION_5, cText, sizeof(cText)); break;
    case eCommandLine:
      strcpy(cText, pAction->cCmdProg); break;
  }
  ListView_SetItemText(hwndList, iIdx, 1, cText);

  ListView_SortItems(hwndList, SortActionsFunc, 0);

  GetClientRect(hwndList, &rcClient);
  ListView_SetColumnWidth(hwndList, 1,
    rcClient.right - ListView_GetColumnWidth(hwndList,0));

  return 1;
}

int DelAction(HWND hwndList, tAction* pAction)
{
  int iIdx;
  RECT rcClient;
  LV_FINDINFO sFind;

  sFind.flags = LVFI_PARAM;
  sFind.lParam = (LPARAM)pAction;

  iIdx = ListView_FindItem(hwndList, -1, &sFind);
  if(iIdx < 0) return 0;

  ListView_DeleteItem(hwndList, iIdx);

  GetClientRect(hwndList, &rcClient);
  ListView_SetColumnWidth(hwndList, 1,
    rcClient.right - ListView_GetColumnWidth(hwndList,0));

  free(pAction);

  return 1;
}

int LoadActionList(HWND hwndList, char* pFileName)
{
  FILE* pFile;
  tAction* pAction;
  char cLine[4096], * pHlp;

  pFile = fopen(pFileName, "r");
  if(!pFile) return 0;

  while(fgets(cLine,sizeof(cLine),pFile))
  {
    util_delete_cr(cLine);                  /* delete trailing cr/lf        */

    if(!cLine[0] || cLine[0] == '#')        /* skip if comment or           */
      continue;                             /* empty line                   */

    pAction = (tAction*)calloc(1, sizeof(tAction)); /* alloc & zero out mem */

    pHlp = strtok(cLine, "|");              /* get dtmf code                */
    if(pHlp) strcpy(pAction->cDtmf, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get action                   */
    if(pHlp && !stricmp(pHlp,DTMF_REMOTECONTROL))
      pAction->eType = eRemoteControl;
    else if(pHlp && !stricmp(pHlp,DTMF_REBOOT))
      pAction->eType = eReboot;
    else if(pHlp && !stricmp(pHlp,DTMF_DEACTIVATE))
      pAction->eType = eDeactivate;
    else if(pHlp && !stricmp(pHlp,DTMF_QUIT))
      pAction->eType = eQuit;
    else if(pHlp && !stricmp(pHlp,DTMF_SETCALLBACK))
      pAction->eType = eSetCallback;
    else
    {
      pAction->eType = eCommandLine;
      if(pHlp) strcpy(pAction->cCmdProg, pHlp);
    }

    if(pHlp) pHlp = strtok(0, "|");         /* get arguments                */
    if(pHlp) strcpy(pAction->cCmdArgs, pHlp);

    if(pHlp) pHlp = strtok(0, "|");         /* get window-title and skip    */
    if(pHlp) pHlp = strtok(0, "|");         /* get empty field and skip     */

    if(pHlp) pHlp = strtok(0, "|");         /* get active flag              */
    pAction->fActive = pHlp ? atoi(pHlp) : 1;

    InsertAction(hwndList, pAction);
  }

  fclose(pFile);

  return 1;
}

int SaveActionList(HWND hwndList, char* pFileName)
{
  FILE* pFile;
  int iIdx, iCnt;
  tAction* pAction;
  char* pCol2, * pCol3;

  pFile = fopen(pFileName, "w");
  if(!pFile) return 0;

  iCnt = ListView_GetItemCount(hwndList);
  for(iIdx = 0; iIdx < iCnt; iIdx++)
  {
    pAction = GetItemAction(hwndList, iIdx);
    if(!pAction) continue;

    switch(pAction->eType)
    {
      case eRemoteControl:
        pCol2 = DTMF_REMOTECONTROL; pCol3 = 0; break;
      case eReboot:
        pCol2 = DTMF_REBOOT; pCol3 = 0; break;
      case eDeactivate:
        pCol2 = DTMF_DEACTIVATE; pCol3 = 0; break;
      case eQuit:
        pCol2 = DTMF_QUIT; pCol3 = 0; break;
      case eSetCallback:
        pCol2 = DTMF_SETCALLBACK; pCol3 = 0; break;
      case eCommandLine:
        pCol2 = pAction->cCmdProg; pCol3 = pAction->cCmdArgs; break;
    }

    fprintf(pFile, "%s|%s|%s| | |%s\n",
      strlen(pAction->cDtmf) ? pAction->cDtmf : "?",
      pCol2 && strlen(pCol2) ? pCol2 : " ",
      pCol3 && strlen(pCol3) ? pCol3 : " ",
      pAction->fActive ? "1" : "0");
  }

  fclose(pFile);

  return 1;
}

int FreeActionList(HWND hwndList)
{
  tAction* pAction;
  int iIdx, iCnt;

  iCnt = ListView_GetItemCount(hwndList);
  for(iIdx = 0; iIdx < iCnt; iIdx++)
  {
    pAction = GetItemAction(hwndList, iIdx);
    if(pAction) free(pAction);
  }

  return 1;
}


/*
 *
 *  EditPortListProc()
 *
 *  EditPortList-Dialog Prozedur
 *
 */

LRESULT CALLBACK EditPortListProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  HWND hwndList = GetDlgItem(hwnd, IDC_PORT_LIST);

  switch (message)
  {
    case WM_INITDIALOG:
    {
      HICON hIcon;
      LV_COLUMN sListCol;
      HIMAGELIST hImgLst;
      char cTitle[256];
      LONG baseunit = GetDialogBaseUnits();

      hImgLst = ImageList_Create(
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), 1, 1, 1);

      hIcon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_PORT0));
      ImageList_AddIcon(hImgLst, hIcon);
      DeleteObject(hIcon);

      hIcon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_PORT1));
      ImageList_AddIcon(hImgLst, hIcon);
      DeleteObject(hIcon);

      ListView_SetImageList(hwndList, hImgLst, LVSIL_SMALL);

      LoadString(xAppInst,STR_PRT_LIST_COL1,cTitle,sizeof(cTitle));
      sListCol.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
      sListCol.fmt = LVCFMT_LEFT;
      sListCol.cx = (55 * LOWORD(baseunit)) / 4;
      sListCol.pszText = cTitle;
      ListView_InsertColumn(hwndList, (sListCol.iSubItem = 0), &sListCol);

      LoadString(xAppInst,STR_PRT_LIST_COL2,cTitle,sizeof(cTitle));
      sListCol.cx = (63 * LOWORD(baseunit)) / 4;
      sListCol.pszText = cTitle;
      ListView_InsertColumn(hwndList, (sListCol.iSubItem = 1), &sListCol);

      LoadPortList(hwndList, PRTFILE);

      EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 0);
      EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 0);
      CenterWindow(hwnd, GetWindow(hwnd,GW_OWNER));

      return 1;
    }

    case WM_CONTEXTMENU:

      TrackPopupMenu(GetSubMenu(hPopupMenu,1), TPM_LEFTBUTTON|TPM_RIGHTBUTTON, LOWORD(lParam), HIWORD(lParam), 0, hwnd, 0);
      return 1;

    case WM_INITMENU:
    {
      tPort* pPort = GetSelectedPort(hwndList);

      EnableMenuItem((HMENU)wParam, IDM_EDIT,
        pPort ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem((HMENU)wParam, IDM_DELETE,
        pPort ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem((HMENU)wParam, IDM_ACTIVE,
        pPort ? MF_ENABLED : MF_GRAYED);

      CheckMenuItem((HMENU)wParam, IDM_ACTIVE,
        (pPort && pPort->fActive) ? MF_CHECKED : MF_UNCHECKED);

      return 1;
    }


    case WM_NOTIFY:

      switch(((NMHDR*)lParam)->code)
      {
        case NM_DBLCLK:

          SendMessage(hwnd, WM_COMMAND,
            MAKEWPARAM(IDM_EDIT, 1), (LPARAM)((NMHDR*)lParam)->hwndFrom);
          return 1;

        case LVN_KEYDOWN:

          if(((LV_KEYDOWN*)lParam)->wVKey == VK_DELETE)
            SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_DELETE, 1),
            (LPARAM)((NMHDR*)lParam)->hwndFrom);
          return 1;

        case LVN_ITEMCHANGED:

          if(GetSelectedPort(hwndList))
          {
            EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 1);
            EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 1);
          }
          else
          {
            EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 0);
            EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 0);
          }
          return 1;
      }
      return 1;

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDOK:

          SavePortList(hwndList, PRTFILE);
          EndDialog(hwnd, 1);
          break;

        case IDCANCEL:

          EndDialog(hwnd, 0);
          break;

        case IDM_NEW:
        {
          tPort* pPort;

          pPort = (tPort*)DialogBox(xAppInst,
            MAKEINTRESOURCE(IDD_EDITPORT), hwnd, (DLGPROC)EditPortProc);

          if(pPort) InsertPort(hwndList, pPort);

          return 1;
        }

        case IDM_EDIT:
        {
          tPort* pPort = GetSelectedPort(hwndList);

          if(!pPort) return 1;

          if(DialogBoxParam(xAppInst,MAKEINTRESOURCE(IDD_EDITPORT),
            hwnd,(DLGPROC)EditPortProc,(LPARAM)pPort))
            UpdatePort(hwndList, pPort);

          return 1;
        }

        case IDM_DELETE:
        {
          tPort* pPort = GetSelectedPort(hwndList);

          if(!pPort) return 1;
          DelPort(hwndList, pPort);

          return 1;
        }

        case IDM_ACTIVE:
        {
          tPort* pPort = GetSelectedPort(hwndList);

          if(!pPort) return 1;
          pPort->fActive = !pPort->fActive;
          UpdatePort(hwndList, pPort);

          return 1;
        }
      }
      return 1;

    case WM_DESTROY:

      FreePortList(hwndList);
      return 1;
  }

  return 0;
}


/*
 *
 *  EditPortProc()
 *
 *  EditPort-Dialog Prozedur
 *
 */

LRESULT CALLBACK EditPortProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  tPort* pPort = (tPort*)GetWindowLong(hwnd, DWL_USER);

  switch (message)
  {
    case WM_INITDIALOG:
    {
      char cTxt[64];

      pPort = (tPort*)lParam;
      SetWindowLong(hwnd, DWL_USER, (LONG)pPort);

      LoadString(xAppInst, STR_REJECTCAUSE_0, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      LoadString(xAppInst, STR_REJECTCAUSE_1, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      LoadString(xAppInst, STR_REJECTCAUSE_2, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      LoadString(xAppInst, STR_REJECTCAUSE_3, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      if(!pPort)
      {
        CheckDlgButton(hwnd, IDC_ENABLED, BST_CHECKED);
        SendDlgItemMessage(hwnd, IDC_REACTION, CB_SETCURSEL, 0, 0);
      }
      else
      {
        CheckDlgButton(hwnd,
          IDC_ENABLED, pPort->fActive ? BST_CHECKED : BST_UNCHECKED);

        SetDlgItemText(hwnd, IDC_DESCRIPTION, pPort->cDsc);
        SetDlgItemText(hwnd, IDC_MSN, pPort->cMsn);

        switch(pPort->eReact)
        {
          case eNormal:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,0,0);
            break;
          case eBusy:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,1,0);
            break;
          case eRefuse:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,2,0);
            break;
          case eUnavailable:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,3,0);
            break;
        }

        SetDlgItemText(hwnd, IDC_DELAY, pPort->cAnsDly);
        SetDlgItemText(hwnd, IDC_MAXTIME, pPort->cMaxRec);

        SetDlgItemText(hwnd, IDC_FILENAME, pPort->cWelcomeWave);
        SetDlgItemText(hwnd, IDC_WAVENAME, pPort->cRingingWave);
        SetDlgItemText(hwnd, IDC_FORWARD, pPort->cForwardNumber);
      }

      CenterWindow(hwnd, GetWindow(hwnd, GW_OWNER));
      return 1;
    }

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDC_FILEDLG:
        {
          int iLen;
          OPENFILENAME filename;
          char cFile[MAX_PATH], cTitle[256], cFilt[256];

          LoadString(xAppInst, STR_FDLG_WELCOME, cTitle, sizeof(cTitle));
          iLen = LoadString(xAppInst, STR_FDLG_WELCOME_FILT, cFilt, sizeof(cFilt));
          cFilt[iLen] = cFilt[iLen+1] = 0;

          GetDlgItemText(hwnd, IDC_FILENAME, cFile, sizeof(cFile));
          memset(&filename, 0, sizeof(filename));

          filename.lStructSize = sizeof(filename);
          filename.hwndOwner = hwnd;
          filename.hInstance = xAppInst;
          filename.lpstrFilter = cFilt;
          filename.lpstrFile = cFile;
          filename.nMaxFile = sizeof(cFile);
          filename.lpstrTitle = cTitle;
          filename.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|
            OFN_NOCHANGEDIR|OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST;

          if(GetOpenFileName(&filename))
          {
            char cDir[MAX_PATH];
            GetModuleFileName(0, cDir, sizeof(cDir));
            if(strrchr(cDir,'\\')) *strrchr(cDir,'\\') = 0;
            if(strncmp(cFile,cDir,strlen(cDir)))
              SetDlgItemText(hwnd, IDC_FILENAME, cFile);
            else
              SetDlgItemText(hwnd, IDC_FILENAME, cFile+strlen(cDir)+1);
          }

          return 1;
        }
        case IDC_WAVEDLG:
        {
          int iLen;
          char cFile[MAX_PATH], cTitle[256], cFilt[256];
          OPENFILENAME filename;

          LoadString(xAppInst, STR_FDLG_RINGING, cTitle, sizeof(cTitle));
          iLen = LoadString(xAppInst, STR_FDLG_RINGING_FILT, cFilt, sizeof(cFilt));
          cFilt[iLen] = cFilt[iLen+1] = 0;

          GetDlgItemText(hwnd, IDC_WAVENAME, cFile, sizeof(cFile));
          memset(&filename, 0, sizeof(filename));

          filename.lStructSize = sizeof(filename);
          filename.hwndOwner = hwnd;
          filename.hInstance = xAppInst;
          filename.lpstrFilter = cFilt;
          filename.lpstrFile = cFile;
          filename.nMaxFile = sizeof(cFile);
          filename.lpstrTitle = cTitle;
          filename.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|
            OFN_NOCHANGEDIR|OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST;

          if(GetOpenFileName(&filename))
          {
            char cDir[MAX_PATH];
            GetModuleFileName(0, cDir, sizeof(cDir));
            if(strrchr(cDir,'\\')) *strrchr(cDir,'\\') = 0;
            if(strncmp(cFile,cDir,strlen(cDir)))
              SetDlgItemText(hwnd, IDC_WAVENAME, cFile);
            else
              SetDlgItemText(hwnd, IDC_WAVENAME, cFile+strlen(cDir)+1);
          }

          return 1;
        }

        case IDOK:

          if(!GetWindowTextLength(GetDlgItem(hwnd,IDC_MSN)))
          {
            MyMsgBox(hwnd,
              MAKEINTRESOURCE(STR_ERR_NONUMBER),
              MAKEINTRESOURCE(STR_WARNING), MB_OK, IDI_WARNING);
            return 1;
          }

          if(!pPort) pPort = (tPort*)calloc(1,sizeof(tPort));

          pPort->fActive = !!IsDlgButtonChecked(hwnd, IDC_ENABLED);

          GetDlgItemText(hwnd, IDC_DESCRIPTION,
            pPort->cDsc, sizeof(pPort->cDsc));
          GetDlgItemText(hwnd, IDC_MSN,
            pPort->cMsn, sizeof(pPort->cMsn));

          switch(SendDlgItemMessage(hwnd,IDC_REACTION,CB_GETCURSEL,0,0))
          {
            case 0: pPort->eReact = eNormal; break;
            case 1: pPort->eReact = eBusy; break;
            case 2: pPort->eReact = eRefuse; break;
            case 3: pPort->eReact = eUnavailable; break;
          }

          GetDlgItemText(hwnd, IDC_DELAY,
            pPort->cAnsDly, sizeof(pPort->cAnsDly));
          GetDlgItemText(hwnd, IDC_MAXTIME,
            pPort->cMaxRec, sizeof(pPort->cMaxRec));

          GetDlgItemText(hwnd, IDC_FILENAME,
            pPort->cWelcomeWave, sizeof(pPort->cWelcomeWave));
          GetDlgItemText(hwnd, IDC_WAVENAME,
            pPort->cRingingWave, sizeof(pPort->cRingingWave));
          GetDlgItemText(hwnd, IDC_FORWARD,
            pPort->cForwardNumber, sizeof(pPort->cForwardNumber));

          EndDialog(hwnd, (int)pPort);
          return 1;

        case IDCANCEL:

          EndDialog(hwnd, 0);
          return 1;
      }
      return 1;

  }

  return 0;
}

/*
 *
 *  EditCallerListProc()
 *
 *  EditCallerList-Dialog Prozedur
 *
 */

LRESULT CALLBACK EditCallerListProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  HWND hwndList = GetDlgItem(hwnd, IDC_CALLER_LIST);

  switch (message)
  {
    case WM_INITDIALOG:
    {
      HICON hIcon;
      char cTitle[256];
      LV_COLUMN sListCol;
      HIMAGELIST hImgLst;
      LONG baseunit = GetDialogBaseUnits();

      hImgLst = ImageList_Create(
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), 1, 1, 1);

      hIcon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_CALLER0));
      ImageList_AddIcon(hImgLst, hIcon);
      DeleteObject(hIcon);

      hIcon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_CALLER1));
      ImageList_AddIcon(hImgLst, hIcon);
      DeleteObject(hIcon);

      ListView_SetImageList(hwndList, hImgLst, LVSIL_SMALL);

      LoadString(xAppInst,STR_CLR_LIST_COL1,cTitle,sizeof(cTitle));
      sListCol.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
      sListCol.fmt = LVCFMT_LEFT;
      sListCol.cx = (65 * LOWORD(baseunit)) / 4;
      sListCol.pszText = cTitle;
      ListView_InsertColumn(hwndList, (sListCol.iSubItem = 0), &sListCol);

      LoadString(xAppInst,STR_CLR_LIST_COL2,cTitle,sizeof(cTitle));
      sListCol.cx = (77 * LOWORD(baseunit)) / 4;
      sListCol.pszText = cTitle;
      ListView_InsertColumn(hwndList, (sListCol.iSubItem = 1), &sListCol);

      LoadCallerList(hwndList, NAMFILE);

      EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 0);
      EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 0);
      CenterWindow(hwnd, GetWindow(hwnd,GW_OWNER));

      return 1;
    }

    case WM_CONTEXTMENU:

      TrackPopupMenu(GetSubMenu(hPopupMenu,1), TPM_LEFTBUTTON|TPM_RIGHTBUTTON, LOWORD(lParam), HIWORD(lParam), 0, hwnd, 0);
      return 1;

    case WM_INITMENU:
    {
      tCaller* pCaller = GetSelectedCaller(hwndList);

      EnableMenuItem((HMENU)wParam, IDM_EDIT,
        pCaller ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem((HMENU)wParam, IDM_DELETE,
        pCaller ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem((HMENU)wParam, IDM_ACTIVE,
        pCaller ? MF_ENABLED : MF_GRAYED);

      CheckMenuItem((HMENU)wParam, IDM_ACTIVE,
        (pCaller && pCaller->fActive) ? MF_CHECKED : MF_UNCHECKED);

      return 1;
    }


    case WM_NOTIFY:

      switch(((NMHDR*)lParam)->code)
      {
        case NM_DBLCLK:

          SendMessage(hwnd, WM_COMMAND,
            MAKEWPARAM(IDM_EDIT, 1), (LPARAM)((NMHDR*)lParam)->hwndFrom);
          return 1;

        case LVN_KEYDOWN:

          if(((LV_KEYDOWN*)lParam)->wVKey == VK_DELETE)
            SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_DELETE, 1),
            (LPARAM)((NMHDR*)lParam)->hwndFrom);
          return 1;

        case LVN_ITEMCHANGED:

          if(GetSelectedCaller(hwndList))
          {
            EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 1);
            EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 1);
          }
          else
          {
            EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 0);
            EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 0);
          }
          return 1;
      }
      return 1;

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDOK:

          SaveCallerList(hwndList, NAMFILE);
          EndDialog(hwnd, 1);
          break;

        case IDCANCEL:

          EndDialog(hwnd, 0);
          break;

        case IDM_NEW:
        {
          tCaller* pCaller;

          pCaller = (tCaller*)DialogBox(xAppInst,
            MAKEINTRESOURCE(IDD_EDITCALLER), hwnd, (DLGPROC)EditCallerProc);

          if(pCaller) InsertCaller(hwndList, pCaller);

          return 1;
        }

        case IDM_EDIT:
        {
          tCaller* pCaller = GetSelectedCaller(hwndList);

          if(!pCaller) return 1;

          if(DialogBoxParam(xAppInst,MAKEINTRESOURCE(IDD_EDITCALLER),
            hwnd,(DLGPROC)EditCallerProc,(LPARAM)pCaller))
            UpdateCaller(hwndList, pCaller);

          return 1;
        }

        case IDM_DELETE:
        {
          tCaller* pCaller = GetSelectedCaller(hwndList);

          if(!pCaller) return 1;
          DelCaller(hwndList, pCaller);

          return 1;
        }

        case IDM_ACTIVE:
        {
          tCaller* pCaller = GetSelectedCaller(hwndList);

          if(!pCaller) return 1;
          pCaller->fActive = !pCaller->fActive;
          UpdateCaller(hwndList, pCaller);

          return 1;
        }
      }
      return 1;

    case WM_DESTROY:

      FreeCallerList(hwndList);
      return 1;
  }

  return 0;
}

/*
 *
 *  EditCallerProc()
 *
 *  EditCallerProc-Dialog Prozedur
 *
 */

LRESULT CALLBACK EditCallerProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  tCaller* pCaller = (tCaller*)GetWindowLong(hwnd, DWL_USER);

  switch (message)
  {
    case WM_INITDIALOG:
    {
      char cTxt[256];

      pCaller = (tCaller*)lParam;
      SetWindowLong(hwnd, DWL_USER, (LONG)pCaller);

      LoadString(xAppInst, STR_REJECTCAUSE_D, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      LoadString(xAppInst, STR_REJECTCAUSE_0, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      LoadString(xAppInst, STR_REJECTCAUSE_1, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      LoadString(xAppInst, STR_REJECTCAUSE_2, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      LoadString(xAppInst, STR_REJECTCAUSE_3, cTxt, sizeof(cTxt));
      SendDlgItemMessage(hwnd, IDC_REACTION, CB_ADDSTRING, 0, (LPARAM)cTxt);

      if(!pCaller)
      {
        CheckDlgButton(hwnd, IDC_ENABLED, BST_CHECKED);
        SendDlgItemMessage(hwnd, IDC_REACTION, CB_SETCURSEL, 0, 0);
      }
      else
      {
        CheckDlgButton(hwnd,
          IDC_ENABLED, pCaller->fActive ? BST_CHECKED : BST_UNCHECKED);

        SetDlgItemText(hwnd, IDC_DESCRIPTION, pCaller->cDsc);
        SetDlgItemText(hwnd, IDC_MSN, pCaller->cClip);

        switch(pCaller->eReact)
        {
          case eDefault:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,0,0);
            break;
          case eNormal:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,1,0);
            break;
          case eBusy:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,2,0);
            break;
          case eRefuse:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,3,0);
            break;
          case eUnavailable:
            SendDlgItemMessage(hwnd,IDC_REACTION,CB_SETCURSEL,4,0);
            break;
        }

        SetDlgItemText(hwnd, IDC_DELAY, pCaller->cAnsDly);
        SetDlgItemText(hwnd, IDC_MAXTIME, pCaller->cMaxRec);

        SetDlgItemText(hwnd, IDC_FILENAME, pCaller->cWelcomeWave);
        SetDlgItemText(hwnd, IDC_WAVENAME, pCaller->cRingingWave);
        SetDlgItemText(hwnd, IDC_FORWARD, pCaller->cForwardNumber);
      }

      CenterWindow(hwnd, GetWindow(hwnd, GW_OWNER));
      return 1;
    }

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDC_FILEDLG:
        {
          int iLen;
          OPENFILENAME filename;
          char cFile[MAX_PATH], cTitle[256], cFilt[256];

          LoadString(xAppInst, STR_FDLG_WELCOME, cTitle, sizeof(cTitle));
          iLen = LoadString(xAppInst, STR_FDLG_WELCOME_FILT, cFilt, sizeof(cFilt));
          cFilt[iLen] = cFilt[iLen+1] = 0;

          GetDlgItemText(hwnd, IDC_FILENAME, cFile, sizeof(cFile));
          memset(&filename, 0, sizeof(filename));

          filename.lStructSize = sizeof(filename);
          filename.hwndOwner = hwnd;
          filename.hInstance = xAppInst;
          filename.lpstrFilter = cFilt;
          filename.lpstrFile = cFile;
          filename.nMaxFile = sizeof(cFile);
          filename.lpstrTitle = cTitle;
          filename.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|
            OFN_NOCHANGEDIR|OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST;

          if(GetOpenFileName(&filename))
          {
            char cDir[MAX_PATH];
            GetModuleFileName(0, cDir, sizeof(cDir));
            if(strrchr(cDir,'\\')) *strrchr(cDir,'\\') = 0;
            if(strncmp(cFile,cDir,strlen(cDir)))
              SetDlgItemText(hwnd, IDC_FILENAME, cFile);
            else
              SetDlgItemText(hwnd, IDC_FILENAME, cFile+strlen(cDir)+1);
          }

          return 1;
        }
        case IDC_WAVEDLG:
        {
          int iLen;
          char cFile[1024], cTitle[256], cFilt[256];
          OPENFILENAME filename;

          LoadString(xAppInst, STR_FDLG_RINGING, cTitle, sizeof(cTitle));
          iLen = LoadString(xAppInst, STR_FDLG_RINGING_FILT, cFilt, sizeof(cFilt));
          cFilt[iLen] = cFilt[iLen+1] = 0;

          GetDlgItemText(hwnd, IDC_WAVENAME, cFile, sizeof(cFile));
          memset(&filename, 0, sizeof(filename));

          filename.lStructSize = sizeof(filename);
          filename.hwndOwner = hwnd;
          filename.hInstance = xAppInst;
          filename.lpstrFilter = cFilt;
          filename.lpstrFile = cFile;
          filename.nMaxFile = sizeof(cFile);
          filename.lpstrTitle = cTitle;
          filename.Flags = OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|
            OFN_NOCHANGEDIR|OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST;

          if(GetOpenFileName(&filename))
          {
            char cDir[MAX_PATH];
            GetModuleFileName(0, cDir, sizeof(cDir));
            if(strrchr(cDir,'\\')) *strrchr(cDir,'\\') = 0;
            if(strncmp(cFile,cDir,strlen(cDir)))
              SetDlgItemText(hwnd, IDC_WAVENAME, cFile);
            else
              SetDlgItemText(hwnd, IDC_WAVENAME, cFile+strlen(cDir)+1);
          }

          return 1;
        }

        case IDOK:

          if(!GetWindowTextLength(GetDlgItem(hwnd,IDC_MSN)))
          {
            MyMsgBox(hwnd,
              MAKEINTRESOURCE(STR_ERR_NONUMBER),
              MAKEINTRESOURCE(STR_WARNING), MB_OK, IDI_WARNING);
            return 1;
          }

          if(!pCaller) pCaller = (tCaller*)calloc(1,sizeof(tCaller));

          pCaller->fActive = !!IsDlgButtonChecked(hwnd, IDC_ENABLED);

          GetDlgItemText(hwnd, IDC_DESCRIPTION,
            pCaller->cDsc, sizeof(pCaller->cDsc));
          GetDlgItemText(hwnd, IDC_MSN,
            pCaller->cClip, sizeof(pCaller->cClip));

          switch(SendDlgItemMessage(hwnd,IDC_REACTION,CB_GETCURSEL,0,0))
          {
            case 0: pCaller->eReact = eDefault; break;
            case 1: pCaller->eReact = eNormal; break;
            case 2: pCaller->eReact = eBusy; break;
            case 3: pCaller->eReact = eRefuse; break;
            case 4: pCaller->eReact = eUnavailable; break;
          }

          GetDlgItemText(hwnd, IDC_DELAY,
            pCaller->cAnsDly, sizeof(pCaller->cAnsDly));
          GetDlgItemText(hwnd, IDC_MAXTIME,
            pCaller->cMaxRec, sizeof(pCaller->cMaxRec));

          GetDlgItemText(hwnd, IDC_FILENAME,
            pCaller->cWelcomeWave, sizeof(pCaller->cWelcomeWave));
          GetDlgItemText(hwnd, IDC_WAVENAME,
            pCaller->cRingingWave, sizeof(pCaller->cRingingWave));
          GetDlgItemText(hwnd, IDC_FORWARD,
            pCaller->cForwardNumber, sizeof(pCaller->cForwardNumber));

          EndDialog(hwnd, (int)pCaller);
          return 1;

        case IDCANCEL:

          EndDialog(hwnd, 0);
          return 1;
      }
      return 1;

  }

  return 0;
}


/*
 *
 *  EditActionListProc()
 *
 *  EditActionList-Dialog Prozedur
 *
 */

LRESULT CALLBACK EditActionListProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  HWND hwndList = GetDlgItem(hwnd, IDC_ACTION_LIST);

  switch (message)
  {
    case WM_INITDIALOG:
    {
      HICON hIcon;
      char cTitle[256];
      LV_COLUMN sListCol;
      HIMAGELIST hImgLst;
      LONG baseunit = GetDialogBaseUnits();

      hImgLst = ImageList_Create(
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), 1, 1, 1);

      hIcon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_ACTION0));
      ImageList_AddIcon(hImgLst, hIcon);
      DeleteObject(hIcon);

      hIcon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_ACTION1));
      ImageList_AddIcon(hImgLst, hIcon);
      DeleteObject(hIcon);

      ListView_SetImageList(hwndList, hImgLst, LVSIL_SMALL);

      LoadString(xAppInst,STR_ACT_LIST_COL1,cTitle,sizeof(cTitle));
      sListCol.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
      sListCol.fmt = LVCFMT_LEFT;
      sListCol.cx = (40 * LOWORD(baseunit)) / 4;
      sListCol.pszText = cTitle;
      ListView_InsertColumn(hwndList, (sListCol.iSubItem = 0), &sListCol);

      LoadString(xAppInst,STR_ACT_LIST_COL2,cTitle,sizeof(cTitle));
      sListCol.cx = (78 * LOWORD(baseunit)) / 4;
      sListCol.pszText = cTitle;
      ListView_InsertColumn(hwndList, (sListCol.iSubItem = 1), &sListCol);

      LoadActionList(hwndList, ACTFILE);

      EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 0);
      EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 0);
      CenterWindow(hwnd, GetWindow(hwnd,GW_OWNER));

      return 1;
    }

    case WM_CONTEXTMENU:

      TrackPopupMenu(GetSubMenu(hPopupMenu,1), TPM_LEFTBUTTON|TPM_RIGHTBUTTON, LOWORD(lParam), HIWORD(lParam), 0, hwnd, 0);
      return 1;

    case WM_INITMENU:
    {
      tAction* pAction = GetSelectedAction(hwndList);

      EnableMenuItem((HMENU)wParam, IDM_EDIT,
        pAction ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem((HMENU)wParam, IDM_DELETE,
        pAction ? MF_ENABLED : MF_GRAYED);
      EnableMenuItem((HMENU)wParam, IDM_ACTIVE,
        pAction ? MF_ENABLED : MF_GRAYED);

      CheckMenuItem((HMENU)wParam, IDM_ACTIVE,
        (pAction && pAction->fActive) ? MF_CHECKED : MF_UNCHECKED);

      return 1;
    }

    case WM_NOTIFY:

      switch(((NMHDR*)lParam)->code)
      {
        case NM_DBLCLK:

          SendMessage(hwnd, WM_COMMAND,
            MAKEWPARAM(IDM_EDIT, 1), (LPARAM)((NMHDR*)lParam)->hwndFrom);
          return 1;

        case LVN_KEYDOWN:

          if(((LV_KEYDOWN*)lParam)->wVKey == VK_DELETE)
            SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_DELETE, 1),
            (LPARAM)((NMHDR*)lParam)->hwndFrom);
          return 1;

        case LVN_ITEMCHANGED:

          if(GetSelectedAction(hwndList))
          {
            EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 1);
            EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 1);
          }
          else
          {
            EnableWindow(GetDlgItem(hwnd,IDM_EDIT), 0);
            EnableWindow(GetDlgItem(hwnd,IDM_DELETE), 0);
          }
          return 1;
      }
      return 1;

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDOK:

          SaveActionList(hwndList, ACTFILE);
          EndDialog(hwnd, 1);
          break;

        case IDCANCEL:

          EndDialog(hwnd, 0);
          break;

        case IDM_NEW:
        {
          tAction* pAction;

          pAction = (tAction*)DialogBox(xAppInst,
            MAKEINTRESOURCE(IDD_EDITACTION), hwnd, (DLGPROC)EditActionProc);

          if(pAction) InsertAction(hwndList, pAction);

          return 1;
        }

        case IDM_EDIT:
        {
          tAction* pAction = GetSelectedAction(hwndList);

          if(!pAction) return 1;

          if(DialogBoxParam(xAppInst,MAKEINTRESOURCE(IDD_EDITACTION),
            hwnd,(DLGPROC)EditActionProc,(LPARAM)pAction))
            UpdateAction(hwndList, pAction);

          return 1;
        }

        case IDM_DELETE:
        {
          tAction* pAction = GetSelectedAction(hwndList);

          if(!pAction) return 1;
          DelAction(hwndList, pAction);

          return 1;
        }

        case IDM_ACTIVE:
        {
          tAction* pAction = GetSelectedAction(hwndList);

          if(!pAction) return 1;
          pAction->fActive = !pAction->fActive;
          UpdateAction(hwndList, pAction);

          return 1;
        }
      }
      return 1;

    case WM_DESTROY:

      FreeActionList(hwndList);
      return 1;
  }

  return 0;
}



/*
 *
 *  EditActionProc()
 *
 *  EditAction-Dialog Prozedur
 *
 */

LRESULT CALLBACK EditActionProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  tAction* pAction = (tAction*)GetWindowLong(hwnd, DWL_USER);

  switch (message)
  {
    case WM_INITDIALOG:

      pAction = (tAction*)lParam;
      SetWindowLong(hwnd, DWL_USER, (LONG)pAction);

      if(!pAction)
      {
        CheckDlgButton(hwnd, IDC_ENABLED, BST_CHECKED);
        SendDlgItemMessage(hwnd, IDC_REMOTECONTROL, BM_CLICK, 0, 0);
      }
      else
      {
        CheckDlgButton(hwnd,
          IDC_ENABLED, pAction->fActive ? BST_CHECKED : BST_UNCHECKED);

        SetDlgItemText(hwnd, IDC_DTMFCODE, pAction->cDtmf);

        switch(pAction->eType)
        {
          case eRemoteControl:
            SendDlgItemMessage(hwnd, IDC_REMOTECONTROL, BM_CLICK, 0, 0);
            break;
          case eReboot:
            SendDlgItemMessage(hwnd, IDC_REBOOT, BM_CLICK, 0, 0);
            break;
          case eDeactivate:
            SendDlgItemMessage(hwnd, IDC_DEACTIVATE, BM_CLICK, 0, 0);
            break;
          case eQuit:
            SendDlgItemMessage(hwnd, IDC_QUIT, BM_CLICK, 0, 0);
            break;
          case eSetCallback:
            SendDlgItemMessage(hwnd, IDC_SET_CALLBACK, BM_CLICK, 0, 0);
            break;
          case eCommandLine:
            SendDlgItemMessage(hwnd, IDC_EXECUTE, BM_CLICK, 0, 0);
            SetDlgItemText(hwnd, IDC_PROGRAM, pAction->cCmdProg);
            SetDlgItemText(hwnd, IDC_ARGUMENTS, pAction->cCmdArgs);
            break;
        }
      }

      CenterWindow(hwnd, GetWindow(hwnd, GW_OWNER));
      return 1;

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDC_REMOTECONTROL:
        case IDC_SET_CALLBACK:
        case IDC_REBOOT:
        case IDC_DEACTIVATE:
        case IDC_QUIT:
        case IDC_EXECUTE:

          EnableWindow(GetDlgItem(hwnd,IDC_TEXT1),
            IsDlgButtonChecked(hwnd, IDC_EXECUTE) == BST_CHECKED);
          EnableWindow(GetDlgItem(hwnd,IDC_TEXT2),
            IsDlgButtonChecked(hwnd, IDC_EXECUTE) == BST_CHECKED);
          EnableWindow(GetDlgItem(hwnd,IDC_PROGRAM),
            IsDlgButtonChecked(hwnd, IDC_EXECUTE) == BST_CHECKED);
          EnableWindow(GetDlgItem(hwnd,IDC_ARGUMENTS),
            IsDlgButtonChecked(hwnd, IDC_EXECUTE) == BST_CHECKED);

          return 1;

        case IDOK:

          if(!GetWindowTextLength(GetDlgItem(hwnd,IDC_DTMFCODE)))
          {
            MyMsgBox(hwnd,
              MAKEINTRESOURCE(STR_ERR_NODTMFCODE),
              MAKEINTRESOURCE(STR_WARNING), MB_OK, IDI_WARNING);
            return 1;
          }

          if(!pAction) pAction = (tAction*)calloc(1,sizeof(tAction));

          pAction->fActive = !!IsDlgButtonChecked(hwnd, IDC_ENABLED);

          GetDlgItemText(hwnd, IDC_DTMFCODE,
            pAction->cDtmf, sizeof(pAction->cDtmf));

          if(IsDlgButtonChecked(hwnd,IDC_REMOTECONTROL) == BST_CHECKED)
            pAction->eType = eRemoteControl;
          else if(IsDlgButtonChecked(hwnd,IDC_REBOOT) == BST_CHECKED)
            pAction->eType = eReboot;
          else if(IsDlgButtonChecked(hwnd,IDC_DEACTIVATE) == BST_CHECKED)
            pAction->eType = eDeactivate;
          else if(IsDlgButtonChecked(hwnd,IDC_QUIT) == BST_CHECKED)
            pAction->eType = eQuit;
          else if(IsDlgButtonChecked(hwnd,IDC_SET_CALLBACK) == BST_CHECKED)
            pAction->eType = eSetCallback;
          else if(IsDlgButtonChecked(hwnd,IDC_EXECUTE) == BST_CHECKED)
          {
            pAction->eType = eCommandLine;
            GetDlgItemText(hwnd, IDC_PROGRAM,
              pAction->cCmdProg, sizeof(pAction->cCmdProg));
            GetDlgItemText(hwnd, IDC_ARGUMENTS,
              pAction->cCmdArgs, sizeof(pAction->cCmdArgs));
          }

          EndDialog(hwnd, (int)pAction);
          return 1;

        case IDCANCEL:

          EndDialog(hwnd, 0);
          return 1;
      }
      return 1;

  }

  return 0;
}


/*
 *
 *  RegisterDialogProc()
 *
 *  RegisterDialog-Prozedur
 *
 */

LRESULT CALLBACK RegisterDialogProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      char cVal[64];

      config_file_read_string(STD_CFG_FILE,
        REGISTER_NAME, cVal, REGISTER_NAME_DEF);
      SetDlgItemText(hwnd, IDC_REGNAME, cVal);

      config_file_read_string(STD_CFG_FILE,
        REGISTER_CODE, cVal, REGISTER_CODE_DEF);
      SetDlgItemText(hwnd, IDC_REGCODE, cVal);

      CenterWindow(hwnd, GetWindow(hwnd, GW_OWNER));

      return 1;
    }

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDOK:
        {
          char name[64], code[64];

          GetDlgItemText(hwnd, IDC_REGNAME, name, sizeof(name));
          GetDlgItemText(hwnd, IDC_REGCODE, code, sizeof(code));

          if(!CheckRegistration(name,code))
          {
            MyMsgBox(hwnd,
              MAKEINTRESOURCE(STR_MSG_REGFAIL), APPSHORT, MB_OK, IDI_ERROR);
          }
          else
          {
            MyMsgBox(hwnd,
              MAKEINTRESOURCE(STR_MSG_REGOK), APPSHORT, MB_OK, IDI_INFORMATION);
          }

          EndDialog(hwnd, 1);
          return 1;
        }

        case IDCANCEL:

          EndDialog(hwnd, 1);
          return 1;
      }
      return 1;
  }

  return 0;
}


/*
 *
 *  PopupDialogProc()
 *
 *  PopupDialog-Prozedur
 *
 */

LRESULT CALLBACK PopupDialogProc(
  HWND hwnd,
  UINT message,
  WPARAM wParam,
  LPARAM lParam)
{
  switch(message)
  {
    case WM_INITDIALOG:
    {
      HWND hwndOwner;
      char cText[256], cFmt[256];

      LoadString(xAppInst, STR_POPTEXT_1, cText, sizeof(cText));
      SetDlgItemText(hwnd, IDC_TEXT1, cText);

      LoadString(xAppInst, STR_POPTEXT_2, cFmt, sizeof(cFmt));
      sprintf(cText, cFmt, sLastCall.cCallerName);
      SetDlgItemText(hwnd, IDC_TEXT2, cText);

      LoadString(xAppInst, STR_POPTEXT_3, cFmt, sizeof(cFmt));
      sprintf(cText, cFmt, sLastCall.cCalledPort);
      SetDlgItemText(hwnd, IDC_TEXT3, cText);

      hwndOwner = GetWindow(hwnd,GW_OWNER);
      if(!IsIconic(hwndOwner) && IsWindowVisible(hwndOwner))
      {
        CenterWindow(hwnd, GetWindow(hwnd,GW_OWNER));
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
      }
      else
      {
        RECT rDsk, rDlg;

        GetWindowRect(hwnd, &rDlg);
        if(!SystemParametersInfo(SPI_GETWORKAREA,sizeof(rDsk),&rDsk,0))
        {
          rDsk.left = rDsk.top = 0;
          rDsk.right = GetSystemMetrics(SM_CXSCREEN);
          rDsk.bottom = GetSystemMetrics(SM_CYSCREEN);
        }

        switch(lParam)
        {
          case 1: // top left
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
            break;

          case 2: // top right
            SetWindowPos(hwnd, HWND_TOPMOST,
              rDsk.right - rDlg.right + rDlg.left, 0, 0, 0, SWP_NOSIZE);
            break;

          case 3: // bottom left
            SetWindowPos(hwnd, HWND_TOPMOST, 0,
              rDsk.bottom - rDlg.bottom + rDlg.top, 0, 0, SWP_NOSIZE);
            break;

          default: // bottom right
            SetWindowPos(hwnd, HWND_TOPMOST,
              rDsk.right - rDlg.right + rDlg.left,
              rDsk.bottom - rDlg.bottom + rDlg.top, 0, 0, SWP_NOSIZE);
            break;
        }
      }

      return 0;
    }
  }
  return 0;
}

/*
 *
 *  MainListInit()
 *
 *  Initialisiert die Liste im HauptFenster.
 *
 */

void MainListInit(HWND hwndList)
{
    HICON icon;
    LV_COLUMN lvc;
    HIMAGELIST imgl;
    char cTitle[256];
    LONG baseunit = GetDialogBaseUnits();

    imgl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 1, 1);

    icon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_CALL0));
    ImageList_AddIcon(imgl, icon);
    DeleteObject(icon);

    icon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_CALL1));
    ImageList_AddIcon(imgl, icon);
    DeleteObject(icon);

    icon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_CALL2));
    ImageList_AddIcon(imgl, icon);
    DeleteObject(icon);

    icon = LoadIcon(xAppInst, MAKEINTRESOURCE(ID_CALL3));
    ImageList_AddIcon(imgl, icon);
    DeleteObject(icon);

    ListView_SetImageList(hwndList, imgl, LVSIL_SMALL);

    LoadString(xAppInst,STR_LIST_COL1,cTitle,sizeof(cTitle));
    lvc.mask = LVCF_FMT|LVCF_WIDTH|LVCF_TEXT|LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = (70 * LOWORD(baseunit)) / 4;
    lvc.pszText = cTitle;
    ListView_InsertColumn(hwndList, (lvc.iSubItem = 0), &lvc);

    LoadString(xAppInst,STR_LIST_COL2,cTitle,sizeof(cTitle));
    lvc.cx = (30 * LOWORD(baseunit)) / 4;
    lvc.pszText = cTitle;
    ListView_InsertColumn(hwndList, (lvc.iSubItem = 1), &lvc);

    LoadString(xAppInst,STR_LIST_COL3,cTitle,sizeof(cTitle));
    lvc.cx = (30 * LOWORD(baseunit)) / 4;
    lvc.pszText = cTitle;
    ListView_InsertColumn(hwndList, (lvc.iSubItem = 2), &lvc);

    LoadString(xAppInst,STR_LIST_COL4,cTitle,sizeof(cTitle));
    lvc.cx = (20 * LOWORD(baseunit)) / 4;
    lvc.pszText = cTitle;
    ListView_InsertColumn(hwndList, (lvc.iSubItem = 3), &lvc);

    LoadString(xAppInst,STR_LIST_COL5,cTitle,sizeof(cTitle));
    lvc.cx = (50 * LOWORD(baseunit)) / 4;
    lvc.pszText = cTitle;
    ListView_InsertColumn(hwndList, (lvc.iSubItem = 4), &lvc);

}


/*
 *
 *  MainListInsertCall()
 *
 *  Fgt der Liste im Hauptfenster ein Item hinzu.
 *
 */

int MainListInsertCall(HWND hwndList, HWND hwndTTip, tCall* pCall, int select)
{
    int index;
    LV_ITEM lvi;
    tCall* pNewCall;
    char cTime[256], cDate[256], cDuration[64];

    pNewCall = (tCall*)malloc(sizeof(tCall));
    memcpy(pNewCall, pCall, sizeof(tCall));

    sprintf(cDuration, "%d", pNewCall->iDuration);
    GetDateFormat(LOCALE_USER_DEFAULT, 0,
      &pNewCall->sTime, 0, cDate, sizeof(cDate));
    GetTimeFormat(LOCALE_USER_DEFAULT, 0,
      &pNewCall->sTime, 0, cTime, sizeof(cTime));


    lvi.mask = LVIF_TEXT|LVIF_PARAM;
    lvi.iItem = ListView_GetItemCount(hwndList);
    lvi.iSubItem = 0;
    lvi.pszText = pNewCall->cCallerName;
    lvi.lParam = (LPARAM)pNewCall;

    index = ListView_InsertItem(hwndList, &lvi);
    ListView_SetItemText(hwndList, index, 1, cDate);
    ListView_SetItemText(hwndList, index, 2, cTime);
    ListView_SetItemText(hwndList, index, 3, cDuration);
    ListView_SetItemText(hwndList, index, 4, pNewCall->cCalledPort);

    if(select)
    {
      ListView_SetItemState(hwndList, index,
        LVIS_FOCUSED|LVIS_SELECTED, LVIS_FOCUSED|LVIS_SELECTED);
      ListView_EnsureVisible(hwndList, index, 0);
    }

    MainListUpdateIcon(hwndList, index);

    AddToolTip(hwndList, hwndTTip, index, pNewCall->cCallerNameOrg);

    return index;
}


/*
 *
 *  MainListGetCall()
 *
 *  Liefert das Handle des Items, das bei
 *  InsertItem gesetzt wurde.
 *
 */

tCall* MainListGetCall(HWND hwndList, int index)
{
    LV_ITEM lvi;

    lvi.mask = LVIF_PARAM;
    lvi.iItem = index;
    lvi.iSubItem = 0;

    if(!ListView_GetItem(hwndList, &lvi)) return 0;

    return (tCall*)lvi.lParam;
}


/*
 *
 *  MainListUpdateIcon()
 *
 *  W„hlt das einem List-Item zugeordnete Icon aus.
 *  icon ist ein 0-basierter index.
 *
 */

void MainListUpdateIcon(HWND hwndList, int index)
{
  LV_ITEM lvi;
  tCall* pCall;

  pCall = MainListGetCall(hwndList, index);
  if(!pCall) return;

  lvi.mask = LVIF_IMAGE;
  lvi.iItem = index;
  lvi.iSubItem = 0;

  if(pCall->iIsDigital)          /* digitaler ruf */
    lvi.iImage = 3;
  else if(pCall->iDuration == 0) /* ruf ohne text */
    lvi.iImage = 0;
  else if(pCall->iState)         /* ruf ist schon abgehoert */
    lvi.iImage = 2;
  else                           /* ruf ist noch nicht abgehoert */
    lvi.iImage = 1;

  ListView_SetItem(hwndList, &lvi);

  return;
}


int CALLBACK MainListSortFunc(
  LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
  int iResult;
  tCall* pCall1, * pCall2;
  FILETIME sFTime1, sFTime2;

  iResult = 0;
  pCall1 = (tCall*)lParam1;
  pCall2 = (tCall*)lParam2;

  switch(lParamSort)
  {
    case 0: // nach caller name sortieren

      iResult = strncmp(pCall1->cCallerName,
        pCall2->cCallerName, strlen(pCall1->cCallerName));
      break;

    case 1:
    case 2: // nach time-stamp sortieren

      SystemTimeToFileTime(&pCall1->sTime, &sFTime1);
      SystemTimeToFileTime(&pCall2->sTime, &sFTime2);
      iResult = CompareFileTime(&sFTime1, &sFTime2);
      break;

    case 3: // nach dauer und typ sortieren

      if(pCall1->iIsDigital && !pCall2->iIsDigital) iResult = 1;
      else if(!pCall1->iIsDigital && pCall2->iIsDigital) iResult = -1;
      else iResult = pCall1->iDuration - pCall2->iDuration;
      break;

    case 4: // nach ziel-port sortieren

      iResult = strncmp(pCall1->cCalledPort,
        pCall2->cCalledPort, strlen(pCall1->cCalledPort));
      break;
  }

  return iResult;
}

/*
 *
 *  MainListSort()
 *
 *  Sortiert die Anrufe in Hauptfenster
 *
 */

void MainListSort(HWND hwndList, int iColumn)
{
  ListView_SortItems(hwndList, MainListSortFunc, iColumn);
}

/*
 *
 *  CenterWindow()
 *
 *  Zentriert ein Fenster relativ zu einem anderen.
 *
 */

int CenterWindow(HWND hwndChild, HWND hwndParent) {

    int xNew, yNew;
    int wChild, hChild, wParent, hParent;
    RECT rChild, rParent, rWorkArea;

    GetWindowRect(hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    GetWindowRect(hwndParent, &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    if(!SystemParametersInfo(SPI_GETWORKAREA, sizeof(rWorkArea), &rWorkArea, 0)) {
        rWorkArea.left = rWorkArea.top = 0;
        rWorkArea.right = GetSystemMetrics(SM_CXSCREEN);
        rWorkArea.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    xNew = rParent.left + ((wParent - wChild) / 2);
    if(xNew < rWorkArea.left) xNew = rWorkArea.left;
    else if((xNew+wChild) > rWorkArea.right) xNew = rWorkArea.right - wChild;

    yNew = rParent.top  + ((hParent - hChild) / 2);
    if(yNew < rWorkArea.top) yNew = rWorkArea.top;
    else if ((yNew+hChild) > rWorkArea.bottom) yNew = rWorkArea.bottom - hChild;

    return SetWindowPos(hwndChild, 0, xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

}

//############################################################################
//############################################################################
//############################################################################
//############################################################################

short next_welcome = 0;

void capitel_connect_ind (TCapiInfo *msg)
{
  sLastCall.iIsDigital= msg->is_digital;
  strcpy(sLastCall.cCallerName, msg->caller_name);
  strcpy(sLastCall.cCallerNameOrg, msg->caller_org_name);
  strcpy(sLastCall.cCalledPort, msg->called_name);

  PostMessage(hwndMain, WM_USER, (WPARAM)6, 0); /* show incoming-call-popup  */
}

void capitel_number_of_call (short num)
{
  memset(&sLastCall, 0, sizeof(sLastCall));

  sLastCall.iNum = num;
  sprintf(sLastCall.cWavFileName, CALL_MAKE_MASK_WAV, sLastCall.iNum);
  sprintf(sLastCall.cAlwFileName, CALL_MAKE_MASK_ALW, sLastCall.iNum);
  sprintf(sLastCall.cIdxFileName, CALL_MAKE_MASK_IDX, sLastCall.iNum);
}

void capitel_have_no_carrier (unsigned long sec)
{
  PostMessage(hwndMain, WM_USER, (WPARAM)7, 0); /* close incoming-call-popup */

  sLastCall.iDuration = sec;      /* dauer speichern              */
  GetLocalTime(&sLastCall.sTime); /* timestamp erzeugen           */

  if(!sLastCall.iDuration &&
    config_file_read_ulong(STD_CFG_FILE,IGNORE_EMPTY_CALLS,IGNORE_EMPTY_CALLS_DEF))
  {
    Sleep(1000);
    remove(sLastCall.cWavFileName);
    remove(sLastCall.cAlwFileName);
  }
  else if(!next_welcome)
  {
    PostMessage(hwndMain, WM_USER, (WPARAM)4, (LPARAM)NULL);
    SaveCall(&sLastCall);
  }

  next_welcome = 0;

  if (config_file_read_ulong(STD_CFG_FILE,CAPITEL_ACTIVE,CAPITEL_ACTIVE_DEF))
    PostMessage(hwndMain, WM_USER, (WPARAM)8, 0);
  else
    PostMessage(hwndMain, WM_USER, (WPARAM)9, 0);
}

void capitel_convert         (unsigned long converting)
{
  if (converting)
    PostMessage(hwndMain, WM_USER, (WPARAM)10, 0);
  else
    PostMessage(hwndMain, WM_USER, (WPARAM)8, 0);
}

void capitel_dorescan        (void)
{
  PostMessage(hwndMain, WM_USER, (WPARAM)5, (LPARAM)NULL);
}

void capitel_remote_control  (void)
{
  PostMessage(hwndMain, WM_USER, (WPARAM)7, 0); /* close incoming-call-popup */
  capitel_dorescan();
}

void capitel_play_waw        (char *msg)
{
  PlaySound(msg, 0, SND_FILENAME|SND_ASYNC);
}

void capitel_deactivate      (void)
{
  PostMessage(hwndMain, WM_USER, (WPARAM)9, 0);
}

void capitel_doexit          (void)
{
  PostMessage(hwndMain, WM_CLOSE, 1, 0);
}

// ##########################################################################

void sigFunc(short num, void *msg) {

  switch(num) {
    case  1 :
    case  2 :
    case  3 : { char* c = strdup((char*)msg); PostMessage(hwndMain, WM_USER, (WPARAM)num, (LPARAM)c); break; }
    case  4 : capitel_connect_ind     (msg);                     break; // incomming call
    case  5 : capitel_number_of_call  (*(short *)msg);           break; // filename of incomming call
    case  6 : capitel_have_no_carrier (*(unsigned long *)msg);   break; // off_hook
    case  7 : capitel_convert         (*(unsigned long *)msg);   break; // converting wav's
    case  8 : capitel_dorescan        ();                        break; // rescan
    case  9 : capitel_remote_control  ();                        break; // remote_control
    case 10 : capitel_play_waw        (msg);                     break; // play wav
    case 11 : capitel_deactivate      ();                        break; // deactivate
    case 12 : capitel_doexit          ();                        break; // exit capitel
    default : sigFunc (1,"Unknown sigFunc in Capitel");          break; // unknown function
  }
  return;
}

short init_error = 0;

void capitel_init(void)
{
  if(!answer_init(sigFunc,1,LANGUAGE_ENG))
  {
    answer_listen();
    answer_wav2alw_convert_all ();
    PostMessage(hwndMain, WM_USER, (WPARAM)5, (LPARAM)NULL);
  } else {
    init_error = 1;
  }
}

void capitel_exit (void)
{
  if(strlen(cLastWavTmp))                   /* delete temp-file from last   */
    remove(cLastWavTmp);                    /* playback                     */

  if (!init_error) {
    answer_exit();
    while (answer_cannot_close()) Sleep(300);
  }
}

void capitel_record_welcome (char *name)
{
  PostMessage(hwndMain, WM_USER, (WPARAM)11, 0);
  answer_record_welcome (name);
  next_welcome = 1;
}


int onlyDigits(char* s)
{
  while(*s && *s != '(') if(!isdigit(*s) && !isspace(*s)) return 0; else s++;
  return 1;
}

void shExecute(char* pCmd)
{
  ShellExecute(GetDesktopWindow(), 0, pCmd, 0, 0, SW_SHOWDEFAULT);
}

void shRecycle( char *prog )
{
  SHFILEOPSTRUCT  op;
  UCHAR           currDir[MAX_PATH], *buff;

  if( GetCurrentDirectory( sizeof( currDir ), currDir ) )
  {
    buff = malloc( strlen(currDir) + strlen(prog) + 2 );

    strcpy( buff, currDir );
    strcat( buff, "\\" );                   /* append backslash             */
    strcat( buff, prog );
    buff[strlen(buff)+1] = '\0';            /* double null-terminated!      */

    if( util_file_size(buff) > 0 )
    {
      memset( &op, 0, sizeof( op ) );

      op.wFunc                 = FO_DELETE;
      op.pFrom                 = (LPCSTR) buff;
      op.fFlags                = FOF_ALLOWUNDO | FOF_NOCONFIRMATION |
                                 FOF_NOERRORUI | FOF_RENAMEONCOLLISION |
                                 FOF_SILENT;

      SHFileOperation( &op );
    }
    else
      remove( buff );

    free( buff );
  }
}

LANGID GetSystemLanguage()
{
  HKEY hKey;
  LANGID iLangID;
  char cLocale[16];
  HINSTANCE hKrnlDll;
  int iResult, iType, iSize;
  LANGID (APIENTRY *GetUserDefaultUILanguage)(void);

  iLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

  if(IS_W2K)
  {
    hKrnlDll = LoadLibrary("kernel32.dll");
    if(!hKrnlDll) return iLangID;

    GetUserDefaultUILanguage = (LANGID(APIENTRY*)(void))
      GetProcAddress(hKrnlDll, "GetUserDefaultUILanguage");
    if(!GetUserDefaultUILanguage)
    {
      FreeLibrary(hKrnlDll);
      return iLangID;
    }

    iLangID = GetUserDefaultUILanguage();
  }
  else if(IS_NT)
  {
    iResult = RegOpenKeyEx(HKEY_USERS,
      ".DEFAULT\\Control Panel\\International", 0, KEY_READ, &hKey);

    if(iResult == ERROR_SUCCESS)
    {
      iSize = sizeof(cLocale);
      iResult = RegQueryValueEx(hKey, "Locale", 0, &iType, cLocale, &iSize);

      if(iResult == ERROR_SUCCESS && iType == REG_SZ)
        iLangID = LANGIDFROMLCID(strtoul(cLocale,0,16));

      RegCloseKey(hKey);
    }
  }
  else
  {
    iResult = RegOpenKeyEx(HKEY_CURRENT_USER,
      "Control Panel\\Desktop\\ResourceLocale", 0, KEY_READ, &hKey);

    if(iResult == ERROR_SUCCESS)
    {
      iSize = sizeof(cLocale);
      iResult = RegQueryValueEx(hKey, 0, 0, &iType, cLocale, &iSize);

      if(iResult == ERROR_SUCCESS && iType == REG_SZ)
        iLangID = LANGIDFROMLCID(strtoul(cLocale,0,16));

      RegCloseKey(hKey);
    }
  }

  return iLangID;
}

BOOL EnumResLangProc(
  HANDLE hModule,
  LPCTSTR lpszType,
  LPCTSTR lpszName,
  WORD wIDLanguage,
  LONG lParam)
{

  (*((LANGID**)lParam))[0] = wIDLanguage;
  (*((LANGID**)lParam))[1] = 0;
  (*((LANGID**)lParam))++;
  return 1;

}
LANGID* GetResourceLanguages(char* res_id)
{
  static LANGID lang[256];
  LANGID* pLang = lang;

  lang[0] = 0;
  EnumResourceLanguages(0, RT_DIALOG, res_id,
    (ENUMRESLANGPROC)EnumResLangProc, (LONG)&pLang);

  return lang;
}

LANGID GetDefaultLanguage(char* res_id)
{
  int i;
  LANGID DefLang;
  LANGID* pResLang;

  DefLang = GetSystemLanguage();
  pResLang = GetResourceLanguages(res_id);

  for(i = 0; pResLang[i]; i++)
    if(pResLang[i] == DefLang)
      return pResLang[i];

  for(i = 0; pResLang[i]; i++)
    if(PRIMARYLANGID(pResLang[i]) == PRIMARYLANGID(DefLang))
      return pResLang[i];

  return MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
}

int InitTrayIcon(HWND hwnd)
{
  HIMAGELIST hImgLst;
  NOTIFYICONDATA* pData;

  hImgLst = ImageList_LoadBitmap(xAppInst,
    MAKEINTRESOURCE((GetSystemMetrics(SM_CXSMICON)>16) ?
    ID_TRAYBMP_LRG : ID_TRAYBMP_SML),
    (GetSystemMetrics(SM_CXSMICON)>16) ? 20 : 16, 0, 0x00808000);

  MainWnd_SetTrayImgLst(hwnd, hImgLst);

  pData = (NOTIFYICONDATA*)calloc(1,sizeof(NOTIFYICONDATA));
  pData->cbSize = sizeof(NOTIFYICONDATA);
  pData->hWnd = hwnd;
  pData->uID = 0;
  pData->uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
  pData->uCallbackMessage = WM_TRAY_CALLBACK;
  pData->hIcon = 0;
  LoadString(xAppInst, STR_WINDOW_TITLE, pData->szTip, sizeof(pData->szTip));

  MainWnd_SetTrayData(hwnd, pData);

  return 0;
}

int UpdateTrayIcon(HWND hwnd)
{
  LVITEM sItem;
  HWND hwndList;
  HIMAGELIST hImgLst;
  NOTIFYICONDATA* pData;
  int iState, iIdx, iMax;

  hwndList = GetDlgItem(hwnd, ID_LIST);

  hImgLst = MainWnd_GetTrayImgLst(hwnd);
  pData = MainWnd_GetTrayData(hwnd);
  if(!pData) return 0;

  if(MainWnd_GetShowTray(hwnd))             /* status anzeigen ->           */
  {
    iMax = ListView_GetItemCount(hwndList); /* anzahl rufe in liste ...     */
    iState = iMax ? 1 : 0;

    for(iIdx = 0; iIdx < iMax; iIdx++)      /* durch die liste gehen und    */
    {                                       /* pruefen, ob nicht-abgehoerte */
      sItem.mask = LVIF_IMAGE;              /* rufe dabei sind              */
      sItem.iItem = iIdx;
      sItem.iSubItem = 0;
      ListView_GetItem(hwndList, &sItem);
      if(sItem.iImage == 1) iState = 2;
    }

    if(!(GetMenuState(GetMenu(hwnd),IDM_TOGGLEACTIVATION,0) & MF_CHECKED))
      iState = 3;

    if(MainWnd_GetTrayStatus(hwnd) != iState) /* status veraendert?         */
    {
      if(pData->hIcon) DeleteObject(pData->hIcon); /* icon ersetzen         */
      pData->hIcon = ImageList_GetIcon(hImgLst, iState, ILD_TRANSPARENT);
      MainWnd_SetTrayStatus(hwnd, iState);         /* status merken         */
      if(pData->uID) Shell_NotifyIcon(NIM_MODIFY, pData);    /* tray-icon   */
      else pData->uID = 1, Shell_NotifyIcon(NIM_ADD, pData); /* setzen      */
    }
    else if(!pData->hIcon)                  /* noch kein icon geladen?      */
    {
      pData->hIcon = ImageList_GetIcon(hImgLst, iState, ILD_TRANSPARENT);
      if(pData->uID) Shell_NotifyIcon(NIM_MODIFY, pData);
      else pData->uID = 1, Shell_NotifyIcon(NIM_ADD, pData);
    }
  }
  else                                      /* status nicht anzeigen ->     */
  {
    MainWnd_SetTrayStatus(hwnd, 0);         /* alles loeschen ...           */
    if(pData->hIcon) DeleteObject(pData->hIcon), pData->hIcon = 0;
    if(pData->uID) Shell_NotifyIcon(NIM_DELETE, pData), pData->uID = 0;
  }

  return 1;
}

int CleanupTrayIcon(HWND hwnd)
{
  NOTIFYICONDATA* pData;

  ImageList_Destroy(MainWnd_GetTrayImgLst(hwnd));
  MainWnd_SetTrayImgLst(hwnd, 0);

  pData = MainWnd_GetTrayData(hwnd);

  if(pData->hIcon) DeleteObject(pData->hIcon);
  if(pData->uID) Shell_NotifyIcon(NIM_DELETE, pData);

  free(pData);
  MainWnd_SetTrayData(hwnd, 0);

  return 1;
}

int GetTrayIconRect(RECT* pRect)
{
  APPBARDATA sTaskBar;

  sTaskBar.cbSize = sizeof(&sTaskBar);      /* position der taskbar holen   */
  if(!SHAppBarMessage(ABM_GETTASKBARPOS,&sTaskBar)) return 0;

  pRect->left = sTaskBar.rc.right - GetSystemMetrics(SM_CYICON);
  pRect->top = sTaskBar.rc.bottom - GetSystemMetrics(SM_CYICON);
  pRect->right = sTaskBar.rc.right;         /* rechte untere ecke der       */
  pRect->bottom = sTaskBar.rc.bottom;       /* taskbar errechnen            */

  return 1;
}

int StoreWindowAttributes(HWND hwnd)
{
  HWND hwndList;
  char cBuff[256];
  WINDOWPLACEMENT sWndPlace;

  hwndList = GetDlgItem(hwnd, ID_LIST);

  sWndPlace.length = sizeof(sWndPlace); /* wichtig: GetWindowPlacement  */
  GetWindowPlacement(hwnd,&sWndPlace);  /* liefert auch dann die letzte */
                                        /* gueltige position, wenn das  */
                                        /* fenster minimiert wurde!     */
  config_file_write_ulong(STD_CFG_FILE, WINDOW_XPOS,
    sWndPlace.rcNormalPosition.left);
  config_file_write_ulong(STD_CFG_FILE, WINDOW_YPOS,
    sWndPlace.rcNormalPosition.top);
  config_file_write_ulong(STD_CFG_FILE, WINDOW_XSIZE,
    sWndPlace.rcNormalPosition.right - sWndPlace.rcNormalPosition.left);
  config_file_write_ulong(STD_CFG_FILE, WINDOW_YSIZE,
    sWndPlace.rcNormalPosition.bottom - sWndPlace.rcNormalPosition.top);

  sprintf(cBuff, "%d,%d,%d,%d,%d,%d",
    IsIconic(hwnd) ? 1 : 0,
    IsZoomed(hwnd) ? 1 : 0,
    !IsWindowVisible(hwnd) ? 1 : 0,
    (GetWindowLong(hwnd,GWL_EXSTYLE) & WS_EX_TOPMOST) ? 1 : 0,
    MainWnd_GetShowTray(hwnd),
    MainWnd_GetHideMin(hwnd));

  config_file_write_string(STD_CFG_FILE, WINDOW_VIEW_FLAGS, cBuff);

  sprintf(cBuff, "%d,%d,%d,%d,%d",
    ListView_GetColumnWidth(hwndList,0),
    ListView_GetColumnWidth(hwndList,1),
    ListView_GetColumnWidth(hwndList,2),
    ListView_GetColumnWidth(hwndList,3),
    ListView_GetColumnWidth(hwndList,4));

  config_file_write_string(STD_CFG_FILE, WINDOW_COL_SIZE, cBuff);

  config_file_write_ulong(STD_CFG_FILE,
    WINDOW_FRAMECTRLS_HIDDEN, !!GetWindowLong(hwnd,GWL_USERDATA));

  return 1;
}

int RestoreWindowAttributes(HWND hwnd)
{
  HWND hwndList;
  char cBuff[128];
  int iCol0, iCol1, iCol2, iCol3, iCol4, xPos, yPos, xSize, ySize;
  int iIsMin = 0, iIsMax = 0, iHide = 0, iTop = 0, iShowTray = 0, iMinHide = 0;

  hwndList = GetDlgItem(hwnd, ID_LIST);

  xPos = config_file_read_ulong(STD_CFG_FILE, WINDOW_XPOS, WINDOW_XPOS_DEF);
  yPos = config_file_read_ulong(STD_CFG_FILE, WINDOW_YPOS, WINDOW_YPOS_DEF);
  xSize = config_file_read_ulong(STD_CFG_FILE, WINDOW_XSIZE, WINDOW_XSIZE_DEF);
  ySize = config_file_read_ulong(STD_CFG_FILE, WINDOW_YSIZE, WINDOW_YSIZE_DEF);

  if(!xSize || !ySize) return 0;

  config_file_read_string(STD_CFG_FILE,
    WINDOW_VIEW_FLAGS, cBuff, WINDOW_VIEW_FLAGS_DEF);

  sscanf(cBuff,"%d,%d,%d,%d,%d,%d",
    &iIsMin, &iIsMax, &iHide, &iTop, &iShowTray, &iMinHide);

  config_file_read_string(STD_CFG_FILE,
    WINDOW_COL_SIZE, cBuff, WINDOW_COL_SIZE_DEF);

  if(sscanf(cBuff,"%d,%d,%d,%d,%d",&iCol0,&iCol1,&iCol2,&iCol3,&iCol4) == 5)
  {
    ListView_SetColumnWidth(hwndList, 0, iCol0);
    ListView_SetColumnWidth(hwndList, 1, iCol1);
    ListView_SetColumnWidth(hwndList, 2, iCol2);
    ListView_SetColumnWidth(hwndList, 3, iCol3);
    ListView_SetColumnWidth(hwndList, 4, iCol4);
  }

  if(config_file_read_ulong(STD_CFG_FILE,
    WINDOW_FRAMECTRLS_HIDDEN,WINDOW_FRAMECTRLS_HIDDEN_DEF))
  {
    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDM_HIDEFRAMECONTROLS,1), 0);
  }

  MoveWindow(hwnd, xPos, yPos, xSize, ySize, 0);

  if(!iHide)
  {
    if(iIsMax) ShowWindow(hwnd, SW_MAXIMIZE);
    else if(iIsMin) ShowWindow(hwnd, SW_MINIMIZE);
    else ShowWindow(hwnd, SW_SHOW);
  }

  if(iTop) SetWindowPos(hwnd, HWND_TOPMOST, 0, 0,
    0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE);

  if(iShowTray)
    MainWnd_SetShowTray(hwnd, iShowTray),
    UpdateTrayIcon(hwnd);

  if(iMinHide)
    MainWnd_SetHideMin(hwnd, iMinHide);

  return 1;
}

typedef struct sMsgBoxParms
{
  char* pText;
  char* pTitle;
  char* pCheckText;
  int* pCheckFlag;
  int fStyle;
  int fExtended;
  HICON hIcon;
}
tMsgBoxParms;

int MyMsgBox(HWND hwndOwner,
  char* pText, char* pTitle, int fStyle, char* pIcon)
{
  int iRet;
  HINSTANCE hInst;
  tMsgBoxParms sParms;
  char cText[2048], cTitle[256];

  hInst = GetModuleHandle(0);

  if(!HIWORD((DWORD)pText))
    LoadString(hInst, LOWORD((DWORD)pText), cText, sizeof(cText)),
    sParms.pText = cText;
  else sParms.pText = pText;

  if(!pTitle)
    GetWindowText(hwndOwner, cTitle, sizeof(cTitle)),
    sParms.pTitle = cTitle;
  else if(!HIWORD((DWORD)pTitle))
    LoadString(hInst, LOWORD((DWORD)pTitle), cTitle, sizeof(cTitle)),
    sParms.pTitle = cTitle;
  else sParms.pTitle = pTitle;

  sParms.fStyle = fStyle;
  sParms.fExtended = 0;
  sParms.hIcon = LoadIcon(0, pIcon);
  if(!sParms.hIcon) sParms.hIcon = LoadIcon(hInst, pIcon);

  iRet = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MSGBOX),
    hwndOwner, (DLGPROC)MsgBoxProc, (LPARAM)&sParms);

  if(sParms.hIcon) DeleteObject(sParms.hIcon);
  return iRet;
}

int MyMsgBoxEx(HWND hwndOwner,
  char* pText, char* pTitle, char* pChkText, int fStyle,
  char* pIcon, char* pCfgFile, char* pCfgKey)
{
  HINSTANCE hInst;
  tMsgBoxParms sParms;
  int iRet, fCheck = 0;
  char cText[2048], cTitle[256], cChkText[256];

  fCheck = config_file_read_ulong(pCfgFile, pCfgKey, 0);
  if(fCheck) return (fStyle == MB_YESNO) ? IDYES : IDOK;

  hInst = GetModuleHandle(0);

  if(!HIWORD((DWORD)pText))
    LoadString(hInst, LOWORD((DWORD)pText), cText, sizeof(cText)),
    sParms.pText = cText;
  else sParms.pText = pText;

  if(!pTitle)
    GetWindowText(hwndOwner, cTitle, sizeof(cTitle)),
    sParms.pTitle = cTitle;
  else if(!HIWORD((DWORD)pTitle))
    LoadString(hInst, LOWORD((DWORD)pTitle), cTitle, sizeof(cTitle)),
    sParms.pTitle = cTitle;
  else sParms.pTitle = pTitle;

  if(!HIWORD((DWORD)pChkText))
    LoadString(hInst, LOWORD((DWORD)pChkText), cChkText, sizeof(cChkText)),
    sParms.pCheckText = cChkText;
  else sParms.pCheckText = pChkText;

  sParms.fStyle = fStyle;
  sParms.fExtended = 1;
  sParms.pCheckFlag = &fCheck;
  sParms.hIcon = LoadIcon(0, pIcon);
  if(!sParms.hIcon) sParms.hIcon = LoadIcon(hInst, pIcon);

  iRet =  DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_MSGBOXEX),
    hwndOwner, (DLGPROC)MsgBoxProc, (LPARAM)&sParms);

  if(sParms.hIcon) DeleteObject(sParms.hIcon);
  if((iRet == IDYES || iRet == IDOK) && fCheck)
    config_file_write_ulong(pCfgFile, pCfgKey, 1);
  return iRet;

}

LRESULT CALLBACK MsgBoxProc(
  HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
      HDC hdc;
      RECT sRc;
      char cBuf[32];
      HWND hwndOwner;
      HGDIOBJ hOldObj;
      int iNeedH, iCurrH;
      tMsgBoxParms* pParms = (tMsgBoxParms*)lParam;

      SetWindowText(hwnd, pParms->pTitle);
      SetDlgItemText(hwnd, IDC_TEXT, pParms->pText);

      GetClientRect(GetDlgItem(hwnd,IDC_TEXT),&sRc);
      iCurrH = sRc.bottom;
      hdc = GetDC(GetDlgItem(hwnd,IDC_TEXT));
      hOldObj = SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
      iNeedH = DrawText(hdc, pParms->pText,
      strlen(pParms->pText), &sRc, DT_LEFT|DT_CALCRECT|DT_WORDBREAK);
      SelectObject(hdc, hOldObj);
      ReleaseDC(GetDlgItem(hwnd,IDC_TEXT), hdc);

      if(iNeedH > iCurrH)
      {
        GetWindowRect(hwnd, &sRc);
        SetWindowPos(hwnd, 0, 0, 0, sRc.right - sRc.left, sRc.bottom -
          sRc.top + iNeedH - iCurrH, SWP_NOMOVE | SWP_NOZORDER);

        GetWindowRect(GetDlgItem(hwnd,IDC_TEXT), &sRc);
        SetWindowPos(GetDlgItem(hwnd,IDC_TEXT), 0, 0, 0,
          sRc.right - sRc.left, iNeedH, SWP_NOMOVE | SWP_NOZORDER);

        GetWindowRect(GetDlgItem(hwnd,IDOK), &sRc);
        ScreenToClient(hwnd, (POINT*)&sRc);
        SetWindowPos(GetDlgItem(hwnd,IDOK), 0, sRc.left, sRc.top +
          iNeedH - iCurrH, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        GetWindowRect(GetDlgItem(hwnd,IDCANCEL), &sRc);
        ScreenToClient(hwnd, (POINT*)&sRc);
        SetWindowPos(GetDlgItem(hwnd,IDCANCEL), 0, sRc.left, sRc.top +
          iNeedH - iCurrH, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        GetWindowRect(GetDlgItem(hwnd,IDYES), &sRc);
        ScreenToClient(hwnd, (POINT*)&sRc);
        SetWindowPos(GetDlgItem(hwnd,IDYES), 0, sRc.left, sRc.top +
          iNeedH - iCurrH, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        GetWindowRect(GetDlgItem(hwnd,IDNO), &sRc);
        ScreenToClient(hwnd, (POINT*)&sRc);
        SetWindowPos(GetDlgItem(hwnd,IDNO), 0, sRc.left, sRc.top +
          iNeedH - iCurrH, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        GetWindowRect(GetDlgItem(hwnd,IDC_CHECK), &sRc);
        ScreenToClient(hwnd, (POINT*)&sRc);
        SetWindowPos(GetDlgItem(hwnd,IDC_CHECK), 0, sRc.left, sRc.top +
          iNeedH - iCurrH, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
      }

      if(pParms->hIcon)
      {
        SendDlgItemMessage(hwnd, IDC_ICO,
          STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)pParms->hIcon);
      }

      if(pParms->fExtended == 1)
      {
        SetDlgItemText(hwnd, IDC_CHECK, pParms->pCheckText);
        CheckDlgButton(hwnd, IDC_CHECK,
          *(pParms->pCheckFlag) ? BST_CHECKED : BST_UNCHECKED);
        SetWindowLong(hwnd, DWL_USER, (LONG)pParms->pCheckFlag);
      }

      switch(pParms->fStyle)
      {
        case MB_OKCANCEL:
          DestroyWindow(GetDlgItem(hwnd,IDYES));
          DestroyWindow(GetDlgItem(hwnd,IDNO));
          SendMessage(hwnd, DM_SETDEFID, (WPARAM)IDOK, 0);
          break;
        case MB_YESNO:
          DestroyWindow(GetDlgItem(hwnd,IDOK));
          DestroyWindow(GetDlgItem(hwnd,IDCANCEL));
          SendMessage(hwnd, DM_SETDEFID, (WPARAM)IDYES, 0);
          break;
        default:
          GetDlgItemText(hwnd, IDOK, cBuf, sizeof(cBuf));
          SetDlgItemText(hwnd, IDCANCEL, cBuf);
          DestroyWindow(GetDlgItem(hwnd,IDOK));
          DestroyWindow(GetDlgItem(hwnd,IDYES));
          DestroyWindow(GetDlgItem(hwnd,IDNO));
          SetWindowLong(GetDlgItem(hwnd,IDCANCEL), GWL_ID, IDOK);
          SendMessage(hwnd, DM_SETDEFID, (WPARAM)IDOK, 0);
          break;
      }

      hwndOwner = GetWindow(hwnd,GW_OWNER);

      if(!IsWindowVisible(hwndOwner) || IsIconic(hwndOwner))
        CenterWindow(hwnd, 0);
      else
        CenterWindow(hwnd, hwndOwner);

      ShowWindow(hwnd, SW_SHOW);

      return 1;
    }

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDOK:
        case IDYES:
        case IDCANCEL:
        case IDNO:
        {
          int* pChkFlag = (int*)GetWindowLong(hwnd, DWL_USER);
          if(pChkFlag) *pChkFlag = IsDlgButtonChecked(hwnd, IDC_CHECK);
          EndDialog(hwnd, LOWORD(wParam));
          return 1;
        }
      }
  }

  return 0;
}

/*
 *
 * PlayCalls()
 *
 * spielt alle oder alle ausgewaelten anrufe ab
 *
 */

int PlayCalls(HWND hwndList, int iPlayAll)
{
  char* pHlp;
  tCall* pCall;
  int iCnt, iIdx, iHlp;
  FILE* pFile1, * pFile2;
  char cTmp[MAX_PATH], cAlwTmp[MAX_PATH], cWavTmp[MAX_PATH];

  GetTempPath(sizeof(cTmp), cTmp);          /* generate unique file in      */
  GetTempFileName(cTmp, ALW, 0, cAlwTmp); /* the system's temp-dir        */
  GetTempFileName(cTmp, WAV, 0, cWavTmp); /* for alw and wav files        */

  pFile1 = fopen(cAlwTmp, "wb");            /* open temporary alw-file      */
  if(!pFile1) return 0;

  SetCursor(LoadCursor(0,IDC_WAIT));        /* show wait cursor             */

  iCnt = ListView_GetItemCount(hwndList);
  for(iIdx = 0; iIdx < iCnt; iIdx++)        /* step through all items       */
  {
    if(!iPlayAll &&                         /* skip if not selected to play */
       !ListView_GetItemState(hwndList,iIdx,LVIS_SELECTED)) continue;

    pCall = MainListGetCall(hwndList,iIdx); /* get info about this call     */
    if(pCall->iIsDigital) continue;         /* digital call? -> skip it     */
    if(pCall->iDuration == 0) continue;     /* no voice msg? -> skip it     */

    if(ftell(pFile1))                       /* this is not first call?      */
      fwrite(beepdata, 1024, 1, pFile1);    /* write beep to temp alw       */

    pFile2 = fopen(pCall->cAlwFileName, "rb"); /* try to open this file     */
    if(!pFile2) continue;                      /* failed? -> skip it        */

    pHlp = (char*)malloc(10240);             /* alloc buff for copy         */
    while(iHlp = fread(pHlp,1,10240,pFile2)) /* copy contents of the file   */
      fwrite(pHlp, 1, iHlp, pFile1);         /* to temp alw                 */
    free(pHlp);                              /* free copy buff              */

    fclose(pFile2);                         /* close alw file of this call  */
  }

  fclose(pFile1);                           /* close temp-alw               */

  iHlp = !config_file_read_ulong(STD_CFG_FILE,
    GENERATE_16_BIT_WAVES, GENERATE_16_BIT_WAVES_DEF);
  alw2wav(cAlwTmp, cWavTmp, (short)iHlp);   /* convert temp-alw to wav      */
  remove(cAlwTmp);                          /* delete temp-alw              */

  SetCursor(LoadCursor(0,IDC_ARROW));       /* remove wait cursor           */

  if(!PlaySound(cWavTmp,0,SND_FILENAME|SND_ASYNC)) /* start playback        */
  {
    remove(cWavTmp);                        /* failed? remove temp-wav      */
    return 0;                               /* and get out here             */
  }

  if(strlen(cLastWavTmp))                   /* delete temp-file             */
    remove(cLastWavTmp);                    /* from last playback           */
  strcpy(cLastWavTmp, cWavTmp);             /* remember temp file name      */

  iCnt = ListView_GetItemCount(hwndList);
  for(iIdx = 0; iIdx < iCnt; iIdx++)        /* again, step through items    */
  {
    if(!iPlayAll &&                         /* skip if not selected to play */
       !ListView_GetItemState(hwndList,iIdx,LVIS_SELECTED)) continue;

    pCall = MainListGetCall(hwndList,iIdx); /* get info about this call     */
    if(pCall->iIsDigital) continue;         /* digital call? -> skip it     */
    if(pCall->iDuration == 0) continue;     /* no voice msg? -> skip it     */

    pCall->iState = 1;
    MainListUpdateIcon(hwndList, iIdx);     /* set icon to 'heared msg'     */
    SaveCall(pCall);                        /* save new state to idx-file   */
  }

  return 1;
}

int GetSpecialFolderPath(HWND hwndOwner,
  int nFolder, char* pPathBuff)
{
  int rc;
  LPMALLOC pIMalloc;
  ITEMIDLIST* pItemIdList;

  pPathBuff[0] = 0;

  if(SHGetMalloc(&pIMalloc) != NOERROR)
    return 0;

  if(SHGetSpecialFolderLocation(hwndOwner, nFolder, &pItemIdList) != NOERROR)
  {
    pIMalloc->lpVtbl->Release(pIMalloc);
    return 0;
  }

  rc = SHGetPathFromIDList(pItemIdList, pPathBuff);
  pIMalloc->lpVtbl->Free(pIMalloc, pItemIdList);
  pIMalloc->lpVtbl->Release(pIMalloc);

  if(!rc) return 0;
  else return strlen(pPathBuff);
}

int AddToolTip(HWND hwndList, HWND hwndTTip, int iItemIdx, char* pText)
{
  tCall* pCall;
  TOOLINFO info;

  pCall = MainListGetCall(hwndList, iItemIdx);

  memset(&info, 0, sizeof(info));
  info.cbSize = sizeof(info);
  info.uFlags = 0;
  info.hwnd = hwndList;
  info.uId = (UINT)pCall->iNum;
  info.lpszText = pText;

  SendMessage(hwndTTip, TTM_ADDTOOL, 0, (LPARAM)&info);
  UpdateToolRect(hwndList, hwndTTip, iItemIdx);

  return 1;
}

int DelToolTip(HWND hwndList, HWND hwndTTip, int iItemIdx)
{
  tCall* pCall;
  TOOLINFO info;

  pCall = MainListGetCall(hwndList, iItemIdx);

  info.cbSize = sizeof(info);
  info.uFlags = 0;
  info.hwnd = hwndList;
  info.uId = (UINT)pCall->iNum;

  SendMessage(hwndTTip, TTM_DELTOOL, 0, (LPARAM)&info);

  return 1;
}

int UpdateToolRect(HWND hwndList, HWND hwndTTip, int iItemIdx)
{
  tCall* pCall;
  TOOLINFO info;

  pCall = MainListGetCall(hwndList, iItemIdx);

  memset(&info, 0, sizeof(info));
  info.cbSize = sizeof(info);
  info.uFlags = 0;
  info.hwnd = hwndList;
  info.uId = (UINT)pCall->iNum;

  ListView_GetItemRect(hwndList, iItemIdx, &info.rect, LVIR_BOUNDS);

  SendMessage(hwndTTip, TTM_NEWTOOLRECT, 0, (LPARAM)&info);

  return 1;
}

int UpdateAllToolRect(HWND hwndList, HWND hwndTTip)
{
  int iCnt = ListView_GetItemCount(hwndList);
  while(iCnt--) UpdateToolRect(hwndList, hwndTTip, iCnt);
  return 1;
}

int SaveCall(tCall* pCall)
{
  FILE* pFile;

  pFile = fopen(pCall->cIdxFileName, "w");
  if(!pFile) return 0;

  fprintf(pFile, "%s\n", pCall->cCallerName);
  fprintf(pFile, "%02hu.%02hu.%04hu\n",
    pCall->sTime.wDay, pCall->sTime.wMonth, pCall->sTime.wYear);
  fprintf(pFile, "%02hu:%02hu:%02hu\n",
    pCall->sTime.wHour, pCall->sTime.wMinute, pCall->sTime.wSecond);
  fprintf(pFile, "%d\n", pCall->iDuration);
  fprintf(pFile, "%s\n", pCall->cCalledPort);
  fprintf(pFile, "%s\n", pCall->cWavFileName);
  fprintf(pFile, "%d\n", pCall->iState);
  fprintf(pFile, "%d\n", pCall->iIsDigital);
  fprintf(pFile, "%s\n", pCall->cCallerNameOrg);

  fclose(pFile);

  return 1;
}

int ReadCall(tCall* pCall, char* pFileName)
{
  int iLine;
  FILE* pFile;
  char* pHlp, cLine[256];

  pFile = fopen(pFileName, "r");
  if(!pFile) return 0;

  for(pHlp = pFileName; *pHlp && !isdigit(*pHlp); pHlp++) ;

  pCall->iNum = atoi(pHlp);
  strcpy(pCall->cIdxFileName, pFileName);
  sprintf(pCall->cAlwFileName, CALL_MAKE_MASK_ALW, pCall->iNum);

  for(iLine = 0; fgets(cLine,sizeof(cLine),pFile); iLine++)
  {
    while(isspace(cLine[strlen(cLine)-1]))
      cLine[strlen(cLine)-1] = 0;

    switch(iLine)
    {
      case 0: strcpy(pCall->cCallerName, cLine); break;
      case 1: sscanf(cLine, "%hu.%hu.%hu",
        &pCall->sTime.wDay, &pCall->sTime.wMonth, &pCall->sTime.wYear); break;
      case 2: sscanf(cLine, "%hu:%hu:%hu",
        &pCall->sTime.wHour, &pCall->sTime.wMinute, &pCall->sTime.wSecond); break;
      case 3: pCall->iDuration = atoi(cLine); break;
      case 4: strcpy(pCall->cCalledPort, cLine); break;
      case 5: strcpy(pCall->cWavFileName, cLine); break;
      case 6: pCall->iState = atoi(cLine); break;
      case 7: pCall->iIsDigital = atoi(cLine); break;
      case 8: strcpy(pCall->cCallerNameOrg, cLine); break;
    }
  }

  fclose(pFile);

  return 1;
}

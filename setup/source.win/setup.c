
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <commctrl.h>
#include <setupapi.h>
#include <objbase.h>
#include <shlobj.h>
#include <objidl.h>
#include "setup.h"

#define REG_UNINSTALL_PATH \
 "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"

#define IS_WINNT  (BOOL)(GetVersion() < 0x80000000)
#define IS_WIN2K  (BOOL)((IS_WINNT) && (LOBYTE(LOWORD(GetVersion()))>4))
#define IS_WIN32S (BOOL)(!(IS_WINNT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95  (BOOL)(!(IS_WINNT) && !(IS_WIN32S))

#define WM_USER_STEP     WM_USER+10
#define WM_USER_PROGRESS WM_USER+11
#define WM_USER_RESULT   WM_USER+12

#define SLBN_CHECKING 0xFFF0
#define SLBN_CHECKED  0xFFF1

#define WATERMARK_TOTAL_SIZE_X 503
#define WATERMARK_TOTAL_SIZE_Y 313
#define WATERMARK_IMAGE_OFFS_X 0
#define WATERMARK_IMAGE_OFFS_Y 0

#define HEADER_TOTAL_SIZE_X 503
#define HEADER_TOTAL_SIZE_Y 58
#define HEADER_IMAGE_OFFS_X 443
#define HEADER_IMAGE_OFFS_Y 5

#define PSH_WIZARD_LITE         0x00400000
#define PSH_NOCONTEXTHELP       0x02000000

typedef struct sWizardData
{
  HINF hInf;
  char cInfName[MAX_PATH];
  int iInfErrorLine;
  HIMAGELIST hImgLst;
  HFONT hWelcomeTitleFont;
  HFONT hHeaderTitleFont;
  HICON hSmInfoIcon;
  HICON hSmAlertIcon;
  char cWelcomeTitle[256];
  char cWelcomeText[1024];
  char cPage2HdrTitle[256];
  char cPage2HdrSubTitle[256];
  char cPage3HdrTitle[256];
  char cPage3HdrSubTitle[256];
  char cPage4HdrTitle[256];
  char cPage4HdrSubTitle[256];
}
tWizardData;

/* ------------------------------------------------------------------------
 * Lokale Funktionstypen
 * ------------------------------------------------------------------------ */

int PreviousInstance(void);
HWND CreateMainWindow(HINSTANCE hInstance, int nCmdShow);
int SetupWizard(HWND hwndOwner);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WizPage1Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WizPage2Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WizPage3Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WizPage4Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WizPage5Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WizPage6Proc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WizPage7Proc(HWND, UINT, WPARAM, LPARAM);

DWORD WINAPI InstallThread(void* p);
DWORD WINAPI RemoveThread(void* p);

void InitPlatformSectionNames(HINF hInf);

int InitProductSelection(HWND hwnd, HINF hInf);
int BuildProductList(HWND hwnd, HINF hInf, char* ProductList);
int BuildInstallSectionList(HINF hInf, char* ProdList, char* SectList);
int BuildRemoveSectionList(HINF hInf, char* ProdList, char* SectList);
int CheckProductsForFlag(HINF hInf, char* ProdList, int Flag);
void GetProductDescription(HINF hInf, int ProdIdx, char* Buff, int szBuff);
int GetProductVersionDiff(HINF hInf, int ProdIdx);

int GetUninstallProductName(HINF hInf, char* ProdList,
  char* Name, int cbName, int* pIconIdx);
int GetLastUserDir(HINF hInf, char* ProdList, char* Path);

int CreateUninstallEntrys(HINF hInf, char* ProdList);
int RemoveUninstallEntrys(HINF hInf, char* ProdList);

int InstallMultipleSections(HINF hInf, char* Sections, HWND hwnd);
UINT WINAPI QueueCallback(PVOID, UINT, UINT, UINT);

int StopServices(HINF hInf, char* Section);
int StartServices(HINF hInf, char* Section);
int InstallServices(HINF hInf, char* Section);
int DoStopService(char* SvcName);
int DoAddService(HINF hInf, char* SvcName, char* SvcSect);
int DoDelService(char* SvcName);
int DoStartService(char* SvcName);
int AddServiceToGroupOrder(HINF hInf, char* SvcSection);

int UpdateShortcuts(HINF hInf, char* Section);
int CreateShortcut(char* Where, char* Title,
  char* Exe, char* Args, char* Ico, int IcoIdx, int Flags);
int DeleteShortcut(char* Where, char* Title);

int DoExecStatements(HINF hInf, char* Section, int Step, HWND hwndOwner);
int ExecuteCommand(char* Cmd, char* Arg, char* WorkDir, int Flags, HWND hwndOwner);

int GetDefaultTargetPath(HINF hInf, char* Path, char* RelPath);
int QueryTargetPath(HWND hwnd, char* Path, char* RelPath);
int QueryApplPath(char* Path);
int QueryStartMenuProgramFolder(char* Path, char* Where);
int QuerySetupExePath(HINF hInf, char* Section, char* Path);

int GetFileCount(char* Path);
int DelayFileDelete(char* File);
int DelayDirDelete(char* Dir);

int CenterWindow(HWND hwndChild, HWND hwndParent);
LANGID GetDefaultLanguage(char* ResourceID);
char* GetMultiString(const char* c, int index);
int InsertMultiString(char* c, char* s, int index);
int RemoveMultiString(char* c, int index);
int CountMultiStrings(const char* c);
int FindMultiString(const char* c, const char* s);
int GetMultiStringLength(const char* c);

int GetComCtlVersion(DWORD* pMajor, DWORD* pMinor);
int ErrorMessageBox(HWND hwndOwner, char* pText, int iErrCode);

HBITMAP MakeWizardBmp(HWND hwnd, HBITMAP hBmpOrg,
  int iSizeX, int iSizeY, int iOffsX, int iOffsY);
  
int InitWizardData(HINSTANCE hInstance, tWizardData* pData);
int FreeWizardData(tWizardData* pData);

DLGTEMPLATE* LoadDlgTemplate(HINSTANCE hInstance, char* pszTemplate);
void FreeDlgTemplate(DLGTEMPLATE* pDlgTemplate);

/* ------------------------------------------------------------------------
 * Lokale Variablen
 * ------------------------------------------------------------------------ */

static int Remove = 0;
static int Reboot = 0;
static int DisableUI = 0;
static int CatchReboot = 0;
static LANGID AppLang = 0;

static char* InstallSect = 0;
static char* RemoveSect = 0;
static char* ProductSect = 0;

static char ProductList[1024] = "";
static char SectionList[1024] = "";
static char UserTargetDir[MAX_PATH] = "";

static int iWizStyle = 0;
static WNDPROC oldSelectLBproc;

/* ------------------------------------------------------------------------
 * $Func WinMain
 * Funktion: WinMain
 * Aufgabe.: Main-Entrypoint, erstellt hauptfenster und bearbeitet msg-loop
 * Resultat: Exit-Code des programms
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int APIENTRY WinMain(HINSTANCE hInstance,   /* instance-handle              */
                     HINSTANCE hPrevInst,   /* obsolet unter win32          */
                     LPSTR     pCmdLine,    /* zeiger auf kommandozeile     */
                     int       nCmdShow     /* show-befehl                  */
                     )
{
  MSG msg;
  char* Arg;
  HWND hwndMain;
  DEVMODE sDispMode;
  char Path[MAX_PATH];
  DWORD vMajor, vMinor;

  if(PreviousInstance()) return 0;
  AppLang = GetDefaultLanguage(MAKEINTRESOURCE(IDD_WIZPAGE1));

  InitCommonControls();
  if(GetComCtlVersion(&vMajor,&vMinor))
  {
    if(vMajor > 4 || (vMajor == 4 && vMinor >= 71)) iWizStyle = 1;
//    if(vMajor > 5 || (vMajor == 5 && vMinor >= 80)) iWizStyle = 2;
  }

  if(EnumDisplaySettings(0,ENUM_CURRENT_SETTINGS,&sDispMode) &&
    sDispMode.dmBitsPerPel == 4 && IS_WIN95) iWizStyle = 0;

  Arg = strtok(pCmdLine," ");
  while(Arg)
  {
    if(!strnicmp(Arg,"/quiet",6)) DisableUI = 1;
    if(!strnicmp(Arg,"/style:",7)) iWizStyle = atoi(Arg+7);

    if(!strnicmp(Arg,"/install",8)) Remove = 0;
    if(!strnicmp(Arg,"/install:",9))
      strcpy(ProductList,Arg+9), ProductList[strlen(ProductList)+1] = 0;
    if(!strnicmp(Arg,"/remove",7)) Remove = 1;
    if(!strnicmp(Arg,"/remove:",8))
      strcpy(ProductList,Arg+8), ProductList[strlen(ProductList)+1] = 0;

    if(!strnicmp(Arg,"/lang:eng",9))
      AppLang = MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US);
    if(!strnicmp(Arg,"/lang:ger",9))
      AppLang = MAKELANGID(LANG_GERMAN,SUBLANG_GERMAN);
    if(!strnicmp(Arg,"/lang:fre",9))
      AppLang = MAKELANGID(LANG_FRENCH,SUBLANG_FRENCH);
    if(!strnicmp(Arg,"/lang:ita",9))
      AppLang = MAKELANGID(LANG_ITALIAN,SUBLANG_ITALIAN);
    if(!strnicmp(Arg,"/lang:spa",9))
      AppLang = MAKELANGID(LANG_SPANISH,SUBLANG_NEUTRAL);
    if(!strnicmp(Arg,"/lang:dut",9))
      AppLang = MAKELANGID(LANG_DUTCH,SUBLANG_DUTCH);

    Arg = strtok(0," ");
  }

  if(IS_WINNT) SetThreadLocale(MAKELCID(AppLang,SORT_DEFAULT));

  GetModuleFileName(0, Path, sizeof(Path));
  if(strrchr(Path,'\\')) strrchr(Path,'\\')[0] = 0;
  SetCurrentDirectory(Path);

  hwndMain = CreateMainWindow(hInstance, nCmdShow);
  if(!hwndMain) return 0;

  while(GetMessage(&msg, 0, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func PreviousInstance
 * Funktion: PreviousInstance
 * Aufgabe.: ueberpruefen, ob es schon eine instanz von uns gibt
 * Resultat: TRUE, wenn andere instanz gefunden, sonst FALSE
 * Hinweis.: aktiviert das fenster einer evtl. gefundenen instanz
 * ------------------------------------------------------------------------ */
int PreviousInstance(void)
{

  HWND hwnd;
  char buff[256];
  char* c = buff;

  GetModuleFileName(0, buff, sizeof(buff));
  while(*c) *c = tolower(*c == '\\' ? '_' : *c), c++;
  CreateMutex(0, 1, buff);
  if(GetLastError() != ERROR_ALREADY_EXISTS) return 0;

  hwnd = FindWindow(APPNAME, 0);
  if(hwnd)
  {
    if(IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    SetForegroundWindow(hwnd);
  }

  return 1;
}

/* ------------------------------------------------------------------------
 * $Func CreateMainWindow
 * Funktion: CreateMainWindow
 * Aufgabe.: registriert unsere fensterklasse und erstellt das hauptfenster
 * Resultat: window-handle des hauptfensters
 * Hinweis.:
 * ------------------------------------------------------------------------ */
HWND CreateMainWindow(HINSTANCE hInstance,  /* unser instance-handle        */
                      int       nCmdShow    /* show-befehl von WinMain()    */
                      )
{
  WNDCLASS wc;

  wc.style         = CS_HREDRAW|CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(ID_ICON));
  wc.hCursor       = 0;
  wc.hbrBackground = 0;
  wc.lpszMenuName  = 0;
  wc.lpszClassName = APPNAME;

  if(!RegisterClass(&wc)) return 0;

  return CreateWindow(APPNAME, "", 0, 0, 0, 100, 100, 0, 0, hInstance, 0);
}

/* ------------------------------------------------------------------------
 * $Func WndProc
 * Funktion: WndProc
 * Aufgabe.: fensterprozedur des hauptfensters
 * Resultat: abhaengig von der zu bearbeitenden message
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK WndProc(HWND   hwnd,       /* fensterhandle                */
                         UINT   message,    /* message                      */
                         WPARAM wParam,     /* erster message-parameter     */
                         LPARAM lParam      /* zweiter message-parameter    */
                         )
{
  switch(message)
  {
    case WM_CREATE:

      SetupWizard(hwnd);
      SendMessage(hwnd, WM_CLOSE, 0, 0);
      break;

    case WM_QUERYENDSESSION:

      if(!CatchReboot) return 1;
      Reboot = 1;
      return 0;

    case WM_DESTROY:

      PostQuitMessage(0);
      break;
  }

  return DefWindowProc(hwnd, message, wParam, lParam);
}

/* ------------------------------------------------------------------------
 * $Func SetupWizard
 * Funktion: SetupWizard
 * Aufgabe.: zeigt den eigentlichen setup-wizard an
 * Resultat: abhaengig von der zu bearbeitenden message
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int SetupWizard(HWND hwnd)                  /* fensterhandle                */
{
  unsigned int iHlp;
  HINSTANCE hInstance;
  HBITMAP hBmp1, hBmp2;
  tWizardData sData = {0};
  PROPSHEETPAGE sPage = {0};
  PROPSHEETHEADER sHdr = {0};
  DLGTEMPLATE* apDlgRes[7] = {0};
  HPROPSHEETPAGE ahPages[7] = {0};
  char cInfName[MAX_PATH], cTxt[256], cFmt[256];

  hInstance = (HANDLE)GetWindowLong(hwnd, GWL_HINSTANCE);
  
  GetModuleFileName(0, cInfName, sizeof(cInfName));
  if(strrchr(cInfName,'\\')) strrchr(cInfName,'\\')[1] = 0;
  strcat(cInfName, INFNAME);

  sData.hInf = SetupOpenInfFile(cInfName, 0, INF_STYLE_WIN4, &iHlp);
  if(sData.hInf == INVALID_HANDLE_VALUE)
  {
    LoadString(hInstance, STR_INFERROR, cFmt, sizeof(cFmt));
    sprintf(cTxt, cFmt, cInfName, iHlp);
    MessageBox(hwnd, cTxt, 0, MB_OK|MB_ICONERROR);
    return 0;
  }
  
  InitPlatformSectionNames(sData.hInf);  
  InitWizardData(hInstance, &sData);  
   
  sHdr.dwSize = iWizStyle ? sizeof(sHdr) : PROPSHEETHEADER_V1_SIZE;
  sHdr.dwFlags = 0;
  sHdr.hwndParent = hwnd;
  sHdr.hInstance = hInstance;
  sHdr.nStartPage = Remove ? 5 : 0;
  sHdr.phpage = ahPages;
  
  sPage.dwSize = iWizStyle ? sizeof(sPage) : PROPSHEETPAGE_V1_SIZE;
  sPage.hInstance = hInstance;
  sPage.lParam = (LPARAM)&sData;
  
  sPage.dwFlags = PSP_DLGINDIRECT|(iWizStyle?PSP_HIDEHEADER:0);
  sPage.pfnDlgProc = WizPage1Proc;
  sPage.pResource = apDlgRes[sHdr.nPages] = LoadDlgTemplate(hInstance,
    MAKEINTRESOURCE(iWizStyle?IDD_WIZ97PAGE1:IDD_WIZPAGE1));  
  sHdr.phpage[sHdr.nPages++] = CreatePropertySheetPage(&sPage);

  sPage.dwFlags = PSP_DLGINDIRECT|(iWizStyle?PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE:0);
  sPage.pfnDlgProc = WizPage2Proc;
  sPage.pszHeaderTitle = sData.cPage2HdrTitle;
  sPage.pszHeaderSubTitle = sData.cPage2HdrSubTitle;
  sPage.pResource = apDlgRes[sHdr.nPages] = LoadDlgTemplate(hInstance,
    MAKEINTRESOURCE(iWizStyle?IDD_WIZ97PAGE2:IDD_WIZPAGE2));  
  sHdr.phpage[sHdr.nPages++] = CreatePropertySheetPage(&sPage);

  sPage.dwFlags = PSP_DLGINDIRECT|(iWizStyle?PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE:0);
  sPage.pfnDlgProc = WizPage3Proc;
  sPage.pszHeaderTitle = sData.cPage3HdrTitle;
  sPage.pszHeaderSubTitle = sData.cPage3HdrSubTitle;
  sPage.pResource = apDlgRes[sHdr.nPages] = LoadDlgTemplate(hInstance,
    MAKEINTRESOURCE(iWizStyle?IDD_WIZ97PAGE3:IDD_WIZPAGE3));  
  sHdr.phpage[sHdr.nPages++] = CreatePropertySheetPage(&sPage);

  sPage.dwFlags = PSP_DLGINDIRECT|(iWizStyle?PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE:0);
  sPage.pfnDlgProc = WizPage4Proc;
  sPage.pszHeaderTitle = sData.cPage4HdrTitle;
  sPage.pszHeaderSubTitle = sData.cPage4HdrSubTitle;
  sPage.pResource = apDlgRes[sHdr.nPages] = LoadDlgTemplate(hInstance,
    MAKEINTRESOURCE(iWizStyle?IDD_WIZ97PAGE4:IDD_WIZPAGE4));
  sHdr.phpage[sHdr.nPages++] = CreatePropertySheetPage(&sPage);

  sPage.dwFlags = PSP_DLGINDIRECT|(iWizStyle?PSP_HIDEHEADER:0);
  sPage.pfnDlgProc = WizPage5Proc;
  sPage.pResource = apDlgRes[sHdr.nPages] = LoadDlgTemplate(hInstance,
    MAKEINTRESOURCE(iWizStyle?IDD_WIZ97PAGE5:IDD_WIZPAGE5));
  sHdr.phpage[sHdr.nPages++] = CreatePropertySheetPage(&sPage);

  sPage.dwFlags = PSP_DLGINDIRECT|(iWizStyle?PSP_HIDEHEADER:0);
  sPage.pfnDlgProc = WizPage6Proc;
  sPage.pResource = apDlgRes[sHdr.nPages] = LoadDlgTemplate(hInstance,
    MAKEINTRESOURCE(iWizStyle?IDD_WIZ97PAGE6:IDD_WIZPAGE6));
  sHdr.phpage[sHdr.nPages++] = CreatePropertySheetPage(&sPage);

  sPage.dwFlags = PSP_DLGINDIRECT|(iWizStyle?PSP_HIDEHEADER:0);
  sPage.pfnDlgProc = WizPage7Proc;
  sPage.pResource = apDlgRes[sHdr.nPages] = LoadDlgTemplate(hInstance,
    MAKEINTRESOURCE(iWizStyle?IDD_WIZ97PAGE7:IDD_WIZPAGE7));
  sHdr.phpage[sHdr.nPages++] = CreatePropertySheetPage(&sPage);

  if(iWizStyle == 0)
  {
    sHdr.dwFlags |= PSH_WIZARD;
  }
  else if(iWizStyle == 1)
  {  
    sHdr.dwFlags |= PSH_WIZARD97|PSH_WATERMARK|PSH_HEADER|
      PSH_USEHBMWATERMARK|PSH_USEHBMHEADER|PSH_STRETCHWATERMARK;
      
    hBmp1 = LoadBitmap(hInstance, MAKEINTRESOURCE(ID_WATERMARK));
    hBmp2 = LoadBitmap(hInstance, MAKEINTRESOURCE(ID_HEADER));
    
    sHdr.hbmWatermark = MakeWizardBmp(hwnd, hBmp1, WATERMARK_TOTAL_SIZE_X,
      WATERMARK_TOTAL_SIZE_Y, WATERMARK_IMAGE_OFFS_X, WATERMARK_IMAGE_OFFS_Y);
    sHdr.hbmHeader = MakeWizardBmp(hwnd, hBmp2, HEADER_TOTAL_SIZE_X,
      HEADER_TOTAL_SIZE_Y, HEADER_IMAGE_OFFS_X, HEADER_IMAGE_OFFS_Y);
  
    DeleteObject(hBmp1);
    DeleteObject(hBmp2);
  }
  else if(iWizStyle == 2)
  {
    sHdr.dwFlags |= PSH_WIZARD97|PSH_WATERMARK|PSH_HEADER;

    sHdr.pszbmWatermark = MAKEINTRESOURCE(ID_WATERMARK);
    sHdr.pszbmHeader = MAKEINTRESOURCE(ID_HEADER);
  }
  
  PropertySheet(&sHdr);
  if(Reboot) SetupPromptReboot(0, hwnd, 0);

  SetupCloseInfFile(sData.hInf);  
  FreeWizardData(&sData);
  while(sHdr.nPages--) FreeDlgTemplate(apDlgRes[sHdr.nPages]);
 
  return 1;
}

/* ------------------------------------------------------------------------
 * $Func WizPage1Proc
 * Funktion: WizPage1Proc
 * Aufgabe.: fensterprozedur der ersten wizard-propertsheet-page
 * Resultat: abhaengig von der zu bearbeitenden message
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK WizPage1Proc(HWND hwnd,    /* fensterhandle                */
                             UINT message,  /* message                      */
                             WPARAM wParam, /* erster message-parameter     */
                             LPARAM lParam  /* zweiter message-parameter    */
                             )
{
  switch(message)
  {
    case WM_INITDIALOG:
    {
      tWizardData* pData;
      
      pData = (tWizardData*)((PROPSHEETPAGE*)lParam)->lParam;
      
      SendDlgItemMessage(hwnd, IDC_TEXT1, WM_SETFONT,
        (WPARAM)pData->hWelcomeTitleFont, 0);
      SetDlgItemText(hwnd, IDC_TEXT1, pData->cWelcomeTitle);
      SetDlgItemText(hwnd, IDC_TEXT2, pData->cWelcomeText);

      CenterWindow(GetParent(hwnd), GetDesktopWindow());

      return 1;
    }

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_SETACTIVE)
      {
        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
        if(DisableUI) SetWindowLong(hwnd, DWL_MSGRESULT, -1);
      }
      return 1;
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func SelectLBIsItemChecked
 * Funktion: SelectLBIsItemChecked
 * Aufgabe.: prueft, ob ein item in der select-listbox gecheckt ist
 * Resultat: TRUE wenn selektiert, sonst FALSE
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int SelectLBIsItemChecked(HWND hwnd,
                          int iItemIdx)
{
  int iItemData;

  iItemData = SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)iItemIdx, 0);
  if(iItemData == LB_ERR) return 0;

  return LOWORD(iItemData);
}

/* ------------------------------------------------------------------------
 * $Func SelectLBIsItemPale
 * Funktion: SelectLBIsItemPale
 * Aufgabe.: prueft, ob ein item 'blass' bzw. disabled ist
 * Resultat: TRUE wenn erfolgreich, sonst FALSE
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int SelectLBIsItemPale(HWND hwnd,
                       int iItemIdx)
{
  int iItemData;

  iItemData = SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)iItemIdx, 0);
  if(iItemData == LB_ERR) return 0;

  return HIBYTE(HIWORD(iItemData));
}

/* ------------------------------------------------------------------------
 * $Func SelectLBCheckItem
 * Funktion: SelectLBCheckItem
 * Aufgabe.: checkt oder ent-checkt ein item in der select-listbox
 * Resultat: TRUE wenn erfolgreich, sonst FALSE
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int SelectLBCheckItem(HWND hwnd,
                      int iItemIdx,
                      int iCheckState)
{
  int iItemData;
  RECT sItemRect;

  iItemData = SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)iItemIdx, 0);
  if(iItemData == LB_ERR) return 0;

  iItemData = MAKELONG(iCheckState, HIWORD(iItemData));
  SendMessage(hwnd, LB_SETITEMDATA, (WPARAM)iItemIdx, (LPARAM)iItemData);

  SendMessage(hwnd, LB_GETITEMRECT, (WPARAM)iItemIdx, (LPARAM)&sItemRect);
  InvalidateRect(hwnd, &sItemRect, 0);

  SendMessage(GetParent(hwnd), WM_COMMAND,
    MAKEWPARAM(GetWindowLong(hwnd,GWL_ID),SLBN_CHECKED),
    MAKELPARAM(iItemIdx,LOWORD(iItemData)));

  return 1;
}

/* ------------------------------------------------------------------------
 * $Func SelectLBSetImage
 * Funktion: SelectLBSetImage
 * Aufgabe.: legt den index des icons fuer einen eintrag fest
 * Resultat: TRUE wenn erfolgreich, sonst FALSE
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int SelectLBSetImage(HWND hwnd,
                     int iItemIdx,
                     int iImageIdx,
                     int iPale)
{
  int iItemData;
  RECT sItemRect;

  iItemData = SendMessage(hwnd, LB_GETITEMDATA, (WPARAM)iItemIdx, 0);
  if(iItemData == LB_ERR) return 0;

  iItemData = MAKELONG(LOWORD(iItemData),MAKEWORD(iImageIdx,iPale));
  SendMessage(hwnd, LB_SETITEMDATA, (WPARAM)iItemIdx, (LPARAM)iItemData);

  SendMessage(hwnd, LB_GETITEMRECT, (WPARAM)iItemIdx, (LPARAM)&sItemRect);
  InvalidateRect(hwnd, &sItemRect, 0);

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func SelectLBproc
 * Funktion: SelectLBproc
 * Aufgabe.: fensterprozedur fuer die select-listbox
 * Resultat: abhaengig von der zu bearbeitenden message
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK SelectLBproc(HWND hwnd,    /* fensterhandle                */
                             UINT message,  /* message                      */
                             WPARAM wParam, /* erster message-parameter     */
                             LPARAM lParam  /* zweiter message-parameter    */
                             )
{
  switch(message)
  {
    case WM_LBUTTONUP:
    {
      POINT pt;
      RECT rect;
      int iItemIdx, iCheckState;

      pt.x = LOWORD(lParam); pt.y = HIWORD(lParam);
      MapWindowPoints(hwnd, 0, &pt, 1);
      iItemIdx = LBItemFromPt(hwnd, pt, 0);
      if(iItemIdx < 0) break;

      SendMessage(hwnd, LB_GETITEMRECT, (WPARAM)iItemIdx, (LPARAM)&rect);
      rect.right = rect.left + rect.bottom - rect.top;
      pt.x = LOWORD(lParam); pt.y = HIWORD(lParam);
      if(!PtInRect(&rect,pt)) break;

      iCheckState = !SelectLBIsItemChecked(hwnd, iItemIdx);

      SetWindowLong(GetParent(hwnd), DWL_MSGRESULT, 0);
      SendMessage(GetParent(hwnd), WM_COMMAND,
        MAKEWPARAM(GetWindowLong(hwnd,GWL_ID),SLBN_CHECKING),
        MAKELPARAM(iItemIdx,iCheckState));

      if(!GetWindowLong(GetParent(hwnd),DWL_MSGRESULT))
        SelectLBCheckItem(hwnd, iItemIdx, iCheckState);

      break;
    }

    case WM_KEYDOWN:
    {
      int iItemIdx, iCheckState;

      if((char)wParam != VK_SPACE) break;

      iItemIdx = SendMessage(hwnd, LB_GETCURSEL, 0, 0);
      if(iItemIdx < 0) break;

      iCheckState = !SelectLBIsItemChecked(hwnd, iItemIdx);

      SetWindowLong(GetParent(hwnd), DWL_MSGRESULT, 0);
      SendMessage(GetParent(hwnd), WM_COMMAND,
        MAKEWPARAM(GetWindowLong(hwnd,GWL_ID),SLBN_CHECKING),
        MAKELPARAM(iItemIdx,iCheckState));

      if(!GetWindowLong(GetParent(hwnd),DWL_MSGRESULT))
        SelectLBCheckItem(hwnd, iItemIdx, iCheckState);

      break;
    }

  }

  return CallWindowProc(oldSelectLBproc, hwnd, message, wParam, lParam);
}

/* ------------------------------------------------------------------------
 * $Func WizPage2Proc
 * Funktion: WizPage2Proc
 * Aufgabe.: fensterprozedur der zweiten wizard-propertsheet-page
 * Resultat: abhaengig von der zu bearbeitenden message
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK WizPage2Proc(HWND hwnd,    /* fensterhandle                */
                             UINT message,  /* message                      */
                             WPARAM wParam, /* erster message-parameter     */
                             LPARAM lParam) /* zweiter message-parameter    */
{
  tWizardData* pData = (tWizardData*)GetWindowLong(hwnd, DWL_USER);

  switch(message)
  {
    case WM_INITDIALOG:

      pData = (tWizardData*)((PROPSHEETPAGE*)lParam)->lParam;
      
      SendDlgItemMessage(hwnd, IDC_HDR_TITLE, WM_SETFONT,
        (WPARAM)pData->hHeaderTitleFont, 0);        
      SetDlgItemText(hwnd, IDC_HDR_TITLE, pData->cPage2HdrTitle);
      SetDlgItemText(hwnd, IDC_HDR_SUBTITLE, pData->cPage2HdrSubTitle);      
      
      if(!strlen(ProductList) && InitProductSelection(hwnd,pData->hInf))
        SetWindowLong(hwnd, DWL_USER, (LONG)pData);
        
      oldSelectLBproc = (WNDPROC)SetWindowLong(
        GetDlgItem(hwnd,IDC_PRODLIST), GWL_WNDPROC, (ULONG)SelectLBproc);

      SendDlgItemMessage(hwnd, IDC_PRODLIST, LB_SETCURSEL, 0, 0);
      SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_PRODLIST,LBN_SELCHANGE), 0);
      
      return 1;

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_KILLACTIVE)
      {
        BuildProductList(hwnd, pData->hInf, ProductList);
      }
      else if(((NMHDR*)lParam)->code == PSN_SETACTIVE)
      {
        if(!pData)
          SetWindowLong(hwnd, DWL_MSGRESULT, -1);
        else
        {
          SendMessage(hwnd, WM_COMMAND,
            MAKEWPARAM(IDC_PRODLIST,SLBN_CHECKED), 0);
          if(DisableUI)
          {
            SetWindowLong(hwnd, DWL_MSGRESULT, -1),
            BuildProductList(hwnd, pData->hInf, ProductList);
          }
        }
      }
      return 1;

    case WM_COMMAND:

      if(LOWORD(wParam) == IDC_PRODLIST && HIWORD(wParam) == SLBN_CHECKING)
      {
        HINSTANCE hInstance;
        char cProd[256], cFmt[256], cText[256], cTitle[256];

        hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);

        SendDlgItemMessage(hwnd, IDC_PRODLIST,
          LB_GETTEXT, (WPARAM)LOWORD(lParam), (LPARAM)cProd);
        LoadString(hInstance, STR_WARNING, cTitle, sizeof(cTitle));
        LoadString(hInstance, STR_VERSIONWARNING, cFmt, sizeof(cFmt));
        sprintf(cText, cFmt, cProd);

        if(HIWORD(lParam) &&
          SelectLBIsItemPale(GetDlgItem(hwnd,IDC_PRODLIST),LOWORD(lParam)) &&
          MessageBox(hwnd,cText,cTitle,MB_YESNO|MB_DEFBUTTON2|MB_ICONWARNING) != IDYES)
          SetWindowLong(hwnd, DWL_MSGRESULT, 1);
      }
      else if(LOWORD(wParam) == IDC_PRODLIST && HIWORD(wParam) == SLBN_CHECKED)
      {
        int Idx, Cnt, SelCnt;

        Cnt = SendDlgItemMessage(hwnd, IDC_PRODLIST, LB_GETCOUNT, 0, 0);
        for(Idx = 0, SelCnt = 0; Idx < Cnt; Idx++)
          if(SelectLBIsItemChecked(GetDlgItem(hwnd,IDC_PRODLIST),Idx))
             SelCnt++;

        PropSheet_SetWizButtons(GetParent(hwnd),
          PSWIZB_BACK|(SelCnt?PSWIZB_NEXT:0));
      }
      else if(LOWORD(wParam) == IDC_PRODLIST && HIWORD(wParam) == LBN_SELCHANGE)
      {
        char Buff[1024];
        int Idx = SendDlgItemMessage(hwnd, IDC_PRODLIST, LB_GETCURSEL, 0, 0);
        if(Idx >= 0)
          GetProductDescription(pData->hInf, Idx, Buff, sizeof(Buff)),
          SetDlgItemText(hwnd, IDC_TEXT1, Buff);
      }

      return 1;

    case WM_MEASUREITEM:

      if((int)wParam == IDC_PRODLIST)
      {
        int cxImg, cyImg;
        HIMAGELIST hImgLst;
        HINSTANCE hInstance;
        MEASUREITEMSTRUCT* pMeas;

        pMeas = (MEASUREITEMSTRUCT*)lParam;
        hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);

        hImgLst = ImageList_LoadBitmap(hInstance,
          MAKEINTRESOURCE(ID_BMPLIST), 16, 0, 0x00808000);
        SetWindowLong(GetDlgItem(hwnd,IDC_PRODLIST),
          GWL_USERDATA, (LONG)hImgLst);

        ImageList_GetIconSize(hImgLst, &cxImg, &cyImg);
        pMeas->itemHeight = cyImg+2;
      }
      return 1;

    case WM_DRAWITEM:

      if((int)wParam == IDC_PRODLIST)
      {
        RECT rcTxt;
        char aTxt[256];
        HIMAGELIST hImgLst;
        COLORREF col1, col2;
        int cxImg, cyImg, fSel, fHlt, fPale, iImgIdx;
        DRAWITEMSTRUCT* pDrw = (DRAWITEMSTRUCT*)lParam;

        fSel = LOWORD(pDrw->itemData);
        fPale = HIBYTE(HIWORD(pDrw->itemData));
        iImgIdx = LOBYTE(HIWORD(pDrw->itemData));
        fHlt = pDrw->itemState & ODS_SELECTED && pDrw->itemState & ODS_FOCUS;

        hImgLst = (HIMAGELIST)
          GetWindowLong(GetDlgItem(hwnd,IDC_PRODLIST), GWL_USERDATA);
        FillRect(pDrw->hDC, &pDrw->rcItem,
          (HBRUSH)(fHlt ? (COLOR_HIGHLIGHT+1) : (COLOR_WINDOW+1)));

        if(pDrw->itemID >= 0)
        {
          ImageList_GetIconSize(hImgLst, &cxImg, &cyImg);

          ImageList_Draw(hImgLst, fSel ? 14 : 15, pDrw->hDC,
            pDrw->rcItem.left+1, pDrw->rcItem.top+1, ILD_TRANSPARENT);

          ImageList_DrawEx(hImgLst, iImgIdx + 1, pDrw->hDC,
            pDrw->rcItem.left + cyImg + 1, pDrw->rcItem.top + 1, 0, 0, 0,
            (fHlt ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_WINDOW)),
            ILD_TRANSPARENT|(fPale ? ILD_BLEND50 : 0));

          SendDlgItemMessage(hwnd, IDC_PRODLIST,
            LB_GETTEXT, (WPARAM)pDrw->itemID, (LPARAM)aTxt);

          rcTxt.left = pDrw->rcItem.left + 2*cyImg + 4;
          rcTxt.top = pDrw->rcItem.top + 1;
          rcTxt.right = pDrw->rcItem.right - 1;
          rcTxt.bottom = pDrw->rcItem.bottom - 1;


          if(fPale)
            col1 = SetTextColor(pDrw->hDC, GetSysColor(COLOR_GRAYTEXT));
          else if(fHlt)
            col1 = SetTextColor(pDrw->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
          if(fHlt)
            col2 = SetBkColor(pDrw->hDC,GetSysColor(COLOR_HIGHLIGHT));

          DrawText(pDrw->hDC, aTxt, -1, &rcTxt, DT_VCENTER|DT_SINGLELINE);

          if(fPale || fHlt)
            SetTextColor(pDrw->hDC,col1);
          if(fHlt)
            SetBkColor(pDrw->hDC,col2);
        }

        if(pDrw->itemState & ODS_FOCUS) DrawFocusRect(pDrw->hDC,&pDrw->rcItem);
      }
      return 1;
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func WizPage3Proc
 * Funktion: WizPage3Proc
 * Aufgabe.: fensterprozedur der dritten wizard-propertsheet-page
 * Resultat: abhaengig von der zu bearbeitenden message
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK WizPage3Proc(HWND hwnd,    /* fensterhandle                */
                             UINT message,  /* message                      */
                             WPARAM wParam, /* erster message-parameter     */
                             LPARAM lParam) /* zweiter message-parameter    */
{
  tWizardData* pData = (tWizardData*)GetWindowLong(hwnd, DWL_USER);

  switch(message)
  {
    case WM_INITDIALOG:
    {
      char cPath[MAX_PATH];

      pData = (tWizardData*)((PROPSHEETPAGE*)lParam)->lParam;

      SendDlgItemMessage(hwnd, IDC_HDR_TITLE, WM_SETFONT,
        (WPARAM)pData->hHeaderTitleFont, 0);
      SetDlgItemText(hwnd, IDC_HDR_TITLE, pData->cPage3HdrTitle);
      SetDlgItemText(hwnd, IDC_HDR_SUBTITLE, pData->cPage3HdrSubTitle);

      SetWindowLong(hwnd, DWL_USER, (LONG)pData);
      
      if(!GetLastUserDir(pData->hInf,0,cPath))
        GetDefaultTargetPath(pData->hInf, cPath, 0);
      SetDlgItemText(hwnd, IDC_TARGETDIR, cPath);
      
      return 1;
    }

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_SETACTIVE)
      {
        SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_TARGETDIR,EN_CHANGE), 0);
        if(DisableUI) SetWindowLong(hwnd, DWL_MSGRESULT, -1);
      }
      return 1;

    case WM_COMMAND:

      if(LOWORD(wParam) == IDC_TARGETDIR && HIWORD(wParam) == EN_CHANGE)
      {
        int iResult;

        GetDlgItemText(hwnd, IDC_TARGETDIR,
          UserTargetDir, sizeof(UserTargetDir));
        iResult = SetupSetDirectoryId(pData->hInf, DIRID_USER, UserTargetDir);

        PropSheet_SetWizButtons(GetParent(hwnd),
          iResult ? (PSWIZB_BACK|PSWIZB_NEXT) : PSWIZB_BACK);
      }
      else if(LOWORD(wParam) == IDC_BROWSE && HIWORD(wParam) == BN_CLICKED)
      {
        char cBuff[256], cBuff2[256];

        GetDefaultTargetPath(pData->hInf, cBuff, cBuff2);

        if(QueryTargetPath(hwnd,cBuff,cBuff2))
          SetDlgItemText(hwnd, IDC_TARGETDIR, cBuff);
      }
      return 1;
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func WizPage4Proc
 * Funktion: WizPage4Proc
 * Aufgabe.: fensterprozedur wizard-seite, die die installation durchfuehrt
 * Resultat: abhaengig von der zu bearbeitenden message
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK WizPage4Proc(HWND hwnd,    /* fensterhandle                */
                             UINT message,  /* message                      */
                             WPARAM wParam, /* erster message-parameter     */
                             LPARAM lParam) /* zweiter message-parameter    */
{
  tWizardData* pData = (tWizardData*)GetWindowLong(hwnd, DWL_USER);

  switch(message)
  {
    case WM_INITDIALOG:

      pData = (tWizardData*)((PROPSHEETPAGE*)lParam)->lParam;
      
      SendDlgItemMessage(hwnd, IDC_HDR_TITLE,
        WM_SETFONT, (WPARAM)pData->hHeaderTitleFont, 0);
      SendDlgItemMessage(hwnd, IDC_STATIC,
        STM_SETICON, (WPARAM)pData->hSmAlertIcon, 0);

      SetDlgItemText(hwnd, IDC_HDR_TITLE, pData->cPage4HdrTitle);
      SetDlgItemText(hwnd, IDC_HDR_SUBTITLE, pData->cPage4HdrSubTitle);      
      
      SetWindowLong(hwnd, DWL_USER, (LONG)pData);      
      SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETSTEP, (WPARAM)1, 0);
      
      return 1;

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_SETACTIVE)
      {
        int iNeedWinCD;

        iNeedWinCD = CheckProductsForFlag(pData->hInf, ProductList, 0x04);

        ShowWindow(GetDlgItem(hwnd,IDC_TEXT1),   /* show 'you need your     */
          iNeedWinCD ? SW_SHOW : SW_HIDE);       /* windows cd' message     */
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC),
          iNeedWinCD ? SW_SHOW : SW_HIDE);

        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_BACK|PSWIZB_NEXT);
        if(DisableUI) PropSheet_PressButton(GetParent(hwnd), PSBTN_NEXT);
      }
      else if(((NMHDR*)lParam)->code == PSN_QUERYCANCEL) /* user canceled?  */
      {
        if(GetWindowLong(hwnd,GWL_USERDATA))     /* install thread running? */
          SetWindowLong(hwnd, DWL_MSGRESULT, 1); /* -> ignore cancel        */
      }
      else if(((NMHDR*)lParam)->code == PSN_WIZNEXT) /* user clicked next?  */
      {
        int iDummy;
        
        PropSheet_SetWizButtons(GetParent(hwnd), 0);           /* disable   */
        EnableWindow(GetDlgItem(GetParent(hwnd),IDCANCEL), 0); /* buttons   */

        SetWindowLong(hwnd, DWL_MSGRESULT, 1); /* stay on this page         */
        SetWindowLong(hwnd, GWL_USERDATA, 1);  /* flag: thread is running   */

        CreateThread(0, 0, InstallThread, (void*)hwnd, 0, &iDummy);
      }
      return 1;

    case WM_USER_STEP:
    {
      int iNewString;
      char cText[256];
      HINSTANCE hInstance;

      hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);

      switch((int)wParam)
      {
        case 2: iNewString = STR_STOPSERVICE; break;
        case 3: iNewString = STR_COPYING; break;
        case 4: iNewString = STR_REGISTRY; break;
        case 5: iNewString = STR_SHORTCUTS; break;
        case 6: iNewString = STR_STARTSERVICE; break;
        default: iNewString = 0; break;
      }

      if(iNewString)
      {
        LoadString(hInstance, iNewString, cText, sizeof(cText));
        SetDlgItemText(hwnd, IDC_TEXT2, cText);
        ShowWindow(GetDlgItem(hwnd,IDC_PROGRESS), SW_SHOW);
      }
      else
      {
        SetDlgItemText(hwnd, IDC_TEXT2, "");
        ShowWindow(GetDlgItem(hwnd,IDC_PROGRESS), SW_HIDE);
      }

      return 1;
    }

    case WM_USER_PROGRESS:

      if(wParam)                            /* advance one step             */
      {
        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_STEPIT, 0, 0);
      }
      else                                  /* init progress step count     */
      {
        SendDlgItemMessage(hwnd,
          IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0,lParam));
      }
      return 1;

    case WM_USER_RESULT:                    /* thread has finished his work */
    {
      char cText[512];
      HINSTANCE hInstance;

      hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
      SetWindowLong(hwnd, GWL_USERDATA, 0); /* set flag: thread not running */

      if(!wParam)                           /* install not succesfull?      */
      {
        LoadString(hInstance, STR_INSTALLFAIL, cText, sizeof(cText));
        ErrorMessageBox(hwnd, cText, (int)lParam);

        SetDlgItemText(hwnd, IDC_TEXT2, "");
        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETPOS, 0, 0);

        PropSheet_SetWizButtons(GetParent(hwnd),PSWIZB_BACK|PSWIZB_NEXT);
        EnableWindow(GetDlgItem(GetParent(hwnd),IDCANCEL), 1);
      }
      else                                  /* install succesfull?          */
      {
        PropSheet_SetCurSel(GetParent(hwnd), 0, 4);
      }
      return 1;
    }
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func WizPage5Proc
 * Funktion: WizPage5Proc
 * Aufgabe.: fensterprozedur installation-complete-page
 * Resultat: abhaengig von der zu bearbeitenden message
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK WizPage5Proc(HWND hwnd,    /* fensterhandle                */
                             UINT message,  /* message                      */
                             WPARAM wParam, /* erster message-parameter     */
                             LPARAM lParam) /* zweiter message-parameter    */
{
  tWizardData* pData = (tWizardData*)GetWindowLong(hwnd, DWL_USER);

  switch(message)
  {
    case WM_INITDIALOG:

      pData = (tWizardData*)((PROPSHEETPAGE*)lParam)->lParam;

      SendDlgItemMessage(hwnd, IDC_TEXT1, WM_SETFONT,
        (WPARAM)pData->hWelcomeTitleFont, 0);
      SendDlgItemMessage(hwnd, IDC_STATIC,
        STM_SETICON, (WPARAM)pData->hSmAlertIcon, 0);

      return 1;

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_SETACTIVE)
      {
        ShowWindow(GetDlgItem(hwnd,IDC_TEXT2), Reboot ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC), Reboot ? SW_SHOW : SW_HIDE);

        EnableWindow(GetDlgItem(GetParent(hwnd),IDCANCEL), 0);
        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);

        if(DisableUI) PropSheet_PressButton(GetParent(hwnd), PSBTN_FINISH);
      }
      return 1;
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func WizPage6Proc
 * Funktion: WizPage6Proc
 * Aufgabe.: fensterprozedur der uninstall-page des wizard-property-sheets
 * Resultat: abhaengig von der zu bearbeitenden message
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK WizPage6Proc(HWND hwnd,    /* fensterhandle                */
                             UINT message,  /* message                      */
                             WPARAM wParam, /* erster message-parameter     */
                             LPARAM lParam) /* zweiter message-parameter    */
{
  tWizardData* pData = (tWizardData*)GetWindowLong(hwnd, DWL_USER);

  switch(message)
  {
    case WM_INITDIALOG:
    {
      HICON hIcon;
      int iIconIdx;
      char cProdName[256];

      pData = (tWizardData*)((PROPSHEETPAGE*)lParam)->lParam;
      SetWindowLong(hwnd, DWL_USER, (LONG)pData);

      GetUninstallProductName(pData->hInf,
        ProductList, cProdName, sizeof(cProdName), &iIconIdx);

      SendDlgItemMessage(hwnd, IDC_TEXT1, WM_SETFONT,
        (WPARAM)pData->hWelcomeTitleFont, 0);
      SendDlgItemMessage(hwnd, IDC_TEXT2, WM_SETFONT,
        (WPARAM)pData->hHeaderTitleFont, 0);
      SetDlgItemText(hwnd, IDC_TEXT1, pData->cWelcomeTitle);
      SetDlgItemText(hwnd, IDC_TEXT2, cProdName);
      if(iIconIdx)
      {
        hIcon = ImageList_GetIcon(pData->hImgLst, iIconIdx+1, ILD_NORMAL);
        SendDlgItemMessage(hwnd, IDC_STATIC, STM_SETICON, (WPARAM)hIcon, 0);
      }

//      SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETSTEP, (WPARAM)1, 0);
      CenterWindow(GetParent(hwnd), GetDesktopWindow());

      return 1;
    }

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_SETACTIVE)
      {
        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_NEXT);
        if(DisableUI) PropSheet_PressButton(GetParent(hwnd), PSBTN_NEXT);
      }
      else if(((NMHDR*)lParam)->code == PSN_QUERYCANCEL) /* user canceled?  */
      {
        if(GetWindowLong(hwnd,GWL_USERDATA))     /* install thread running? */
          SetWindowLong(hwnd, DWL_MSGRESULT, 1); /* -> ignore cancel        */
      }
      else if(((NMHDR*)lParam)->code == PSN_WIZNEXT) /* user clicked next?  */
      {
        int iDummy;

        PropSheet_SetWizButtons(GetParent(hwnd), 0);           /* disable   */
        EnableWindow(GetDlgItem(GetParent(hwnd),IDCANCEL), 0); /* buttons   */

        SetWindowLong(hwnd, DWL_MSGRESULT, 1); /* stay on this page         */
        SetWindowLong(hwnd, GWL_USERDATA, 1);  /* flag: thread is running   */

        CreateThread(0, 0, RemoveThread, (void*)hwnd, 0, &iDummy);
      }
      return 1;

    case WM_USER_STEP:
    {
      int iNewString;
      char cText[256];
      HINSTANCE hInstance;

      hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);

      switch((int)wParam)
      {
        case 2: iNewString = STR_STOPSERVICE; break;
        case 3: iNewString = STR_DELETING; break;
        case 4: iNewString = STR_REGISTRY; break;
        case 5: iNewString = STR_SHORTCUTS; break;
        case 6: iNewString = STR_STARTSERVICE; break;
        default: iNewString = 0; break;
      }

      if(iNewString)
      {
        LoadString(hInstance, iNewString, cText, sizeof(cText));
        SetDlgItemText(hwnd, IDC_TEXT3, cText);
//        ShowWindow(GetDlgItem(hwnd,IDC_PROGRESS), SW_SHOW);
      }
      else
      {
        SetDlgItemText(hwnd, IDC_TEXT3, "");
//        ShowWindow(GetDlgItem(hwnd,IDC_PROGRESS), SW_HIDE);
      }

      return 1;
    }

//    case WM_USER_PROGRESS:
//
//      if(wParam)                            /* advance one step             */
//      {
//        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_STEPIT, 0, 0);
//      }
//      else                                  /* init progress step count     */
//      {
//        SendDlgItemMessage(hwnd,
//          IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0,lParam));
//      }
//      return 1;

    case WM_USER_RESULT:                    /* thread has finished his work */
    {
      char cText[512];
      HINSTANCE hInstance;

      hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);
      SetWindowLong(hwnd, GWL_USERDATA, 0); /* set flag: thread not running */

      if(!wParam)                           /* uninstall not succesfull?    */
      {
        LoadString(hInstance, STR_REMOVEFAIL, cText, sizeof(cText));
        ErrorMessageBox(hwnd, cText, (int)lParam);

        SetDlgItemText(hwnd, IDC_TEXT3, "");
//        SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETPOS, 0, 0);

        PropSheet_SetWizButtons(GetParent(hwnd),PSWIZB_BACK|PSWIZB_NEXT);
        EnableWindow(GetDlgItem(GetParent(hwnd),IDCANCEL), 1);
      }
      else                                  /* uninstall succesfull?        */
      {
        PropSheet_SetCurSel(GetParent(hwnd), 0, 6);
      }
      return 1;
    }
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func WizPage7Proc
 * Funktion: WizPage7Proc
 * Aufgabe.: fensterprozedur uninstall-complete-page
 * Resultat: abhaengig von der zu bearbeitenden message
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK WizPage7Proc(HWND hwnd,    /* fensterhandle                */
                             UINT message,  /* message                      */
                             WPARAM wParam, /* erster message-parameter     */
                             LPARAM lParam) /* zweiter message-parameter    */
{
  tWizardData* pData = (tWizardData*)GetWindowLong(hwnd, DWL_USER);

  switch(message)
  {
    case WM_INITDIALOG:

      pData = (tWizardData*)((PROPSHEETPAGE*)lParam)->lParam;

      SendDlgItemMessage(hwnd, IDC_TEXT1, WM_SETFONT,
        (WPARAM)pData->hWelcomeTitleFont, 0);
      SendDlgItemMessage(hwnd, IDC_STATIC,
        STM_SETICON, (WPARAM)pData->hSmAlertIcon, 0);

      return 1;

    case WM_NOTIFY:

      if(((NMHDR*)lParam)->code == PSN_SETACTIVE)
      {
        ShowWindow(GetDlgItem(hwnd,IDC_TEXT2), Reboot ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC), Reboot ? SW_SHOW : SW_HIDE);

        EnableWindow(GetDlgItem(GetParent(hwnd),IDCANCEL), 0);
        PropSheet_SetWizButtons(GetParent(hwnd), PSWIZB_FINISH);

        if(DisableUI) PropSheet_PressButton(GetParent(hwnd), PSBTN_FINISH);
      }
      return 1;
  }

  return 0;
}


/* ------------------------------------------------------------------------
 * $Func InstallThread
 * Funktion: InstallThread
 * Aufgabe.: fuehrt die installation durch
 * Resultat: wird nicht ausgewertet
 * ------------------------------------------------------------------------ */
DWORD WINAPI InstallThread(void* pArg)      /* thread-parameter             */
{
  HWND hwnd;
  tWizardData* pData;
  int iSuccess, iErrCode = 0;

  if(IS_WINNT) SetThreadLocale(MAKELCID(AppLang,SORT_DEFAULT));

  hwnd = (HWND)pArg;
  pData = (tWizardData*)GetWindowLong(hwnd, DWL_USER);

  CatchReboot = 1;

  BuildInstallSectionList(pData->hInf, ProductList, SectionList);
  CreateUninstallEntrys(pData->hInf, ProductList);
  iSuccess = InstallMultipleSections(pData->hInf, SectionList, hwnd);
  if(!iSuccess) iErrCode = GetLastError();
  else if(CheckProductsForFlag(pData->hInf,ProductList,0x02)) Reboot = 1;

  CatchReboot = 0;

  PostMessage(hwnd, WM_USER_RESULT, (WPARAM)iSuccess, (LPARAM)iErrCode);
  return 0;
}


/* ------------------------------------------------------------------------
 * $Func RemoveThread
 * Funktion: RemoveThread
 * Aufgabe.: fuehrt die de-installation durch
 * Resultat: wird nicht ausgewertet
 * ------------------------------------------------------------------------ */
DWORD WINAPI RemoveThread(void* pArg)       /* thread-parameter             */
{
  HWND hwnd;
  char Path[256];
  tWizardData* pData;
  int iSuccess, iErrCode = 0;

  if(IS_WINNT) SetThreadLocale(MAKELCID(AppLang,SORT_DEFAULT));

  hwnd = (HWND)pArg;
  pData = (tWizardData*)GetWindowLong(hwnd, DWL_USER);

  CatchReboot = 1;

  if(GetLastUserDir(pData->hInf,ProductList,Path))
    SetupSetDirectoryId(pData->hInf,DIRID_USER,Path);
  BuildRemoveSectionList(pData->hInf, ProductList, SectionList);
  RemoveUninstallEntrys(pData->hInf, ProductList);
  iSuccess = InstallMultipleSections(pData->hInf, SectionList, hwnd);
  if(!iSuccess) iErrCode = GetLastError();
  else if(CheckProductsForFlag(pData->hInf,ProductList,0x02)) Reboot = 1;

  CatchReboot = 0;

  PostMessage(hwnd, WM_USER_RESULT, (WPARAM)iSuccess, (LPARAM)iErrCode);
  return 0;
}


/* ------------------------------------------------------------------------
 * $Func InitPlatformSectionNames
 * Funktion: InitPlatformSectionNames
 * Aufgabe.: initialisiert den dialog mit der produkt-auswahl
 * Resultat: keins
 * Hinweis.:
 * ------------------------------------------------------------------------ */
void InitPlatformSectionNames(HINF hInf)    /* handle der inf-datei         */
{
  INFCONTEXT infContext;

  if(IS_WINNT)
  {
    if(IS_WIN2K && SetupFindFirstLine(hInf,"Install.NT5",0,&infContext))
      InstallSect = "Install.NT5";
    else if(SetupFindFirstLine(hInf,"Install.NTx86",0,&infContext))
      InstallSect = "Install.NTx86";
    else if(SetupFindFirstLine(hInf,"Install.NT",0,&infContext))
      InstallSect = "Install.NT";
    else
      InstallSect = "Install";

    if(IS_WIN2K && SetupFindFirstLine(hInf,"Remove.NT5",0,&infContext))
      RemoveSect = "Remove.NT5";
    else if(SetupFindFirstLine(hInf,"Remove.NTx86",0,&infContext))
      RemoveSect = "Remove.NTx86";
    else if(SetupFindFirstLine(hInf,"Remove.NT",0,&infContext))
      RemoveSect = "Remove.NT";
    else
      RemoveSect = "Remove";

    if(IS_WIN2K && SetupFindFirstLine(hInf,"Products.NT5",0,&infContext))
      ProductSect = "Products.NT5";
    else if(SetupFindFirstLine(hInf,"Products.NTx86",0,&infContext))
      ProductSect = "Products.NTx86";
    else if(SetupFindFirstLine(hInf,"Products.NT",0,&infContext))
      ProductSect = "Products.NT";
    else
      ProductSect = "Products";
  }
  else
  {
    if(SetupFindFirstLine(hInf,"Install.Win",0,&infContext))
      InstallSect = "Install.Win";
    else
      InstallSect = "Install";
    if(SetupFindFirstLine(hInf,"Remove.Win",0,&infContext))
      RemoveSect = "Remove.Win";
    else
      RemoveSect = "Remove";
    if(SetupFindFirstLine(hInf,"Products.Win",0,&infContext))
      ProductSect = "Products.Win";
    else
      ProductSect = "Products";
  }
}

/* ------------------------------------------------------------------------
 * $Func InitProductSelection
 * Funktion: InitProductSelection
 * Aufgabe.: initialisiert den dialog mit der produkt-auswahl
 * Resultat: anzahl der produkte, die installiert werden koennen
 * ------------------------------------------------------------------------ */
int InitProductSelection(HWND hwnd,         /* handle des dialogs           */
                         HINF hInf          /* handle der inf-datei         */
                         )
{
  char Buff[256];
  INFCONTEXT infContext;
  int Cnt = 0, Sel, Pic, Idx, Old;

  if(SetupFindFirstLine(hInf,ProductSect,0,&infContext))
    do
    {
      if(!SetupGetStringField(&infContext,2,Buff,sizeof(Buff),0) ||
        !strlen(Buff)) continue;
      if(!SetupGetStringField(&infContext,1,Buff,sizeof(Buff),0))
        continue;

      Idx = (int)SendDlgItemMessage(hwnd,
        IDC_PRODLIST, LB_ADDSTRING, 0, (LPARAM)Buff);

      Old = (GetProductVersionDiff(hInf,Idx) < 0);

      Sel = SetupGetStringField(&infContext,4,Buff,sizeof(Buff),0) ?
        ((atoi(Buff) & 0x01) && !Old) : 0;

      Pic = SetupGetStringField(&infContext,5,Buff,sizeof(Buff),0) ?
        atoi(Buff) : 0;

      SelectLBSetImage(GetDlgItem(hwnd,IDC_PRODLIST), Idx, Pic, Old);
      SelectLBCheckItem(GetDlgItem(hwnd,IDC_PRODLIST), Idx, Sel);

      Cnt++;
    }
    while(SetupFindNextLine(&infContext, &infContext));

  return Cnt;
}

/* ------------------------------------------------------------------------
 * $Func BuildProductList
 * Funktion: BuildProductList
 * Aufgabe.: baut eine liste der selektierten produkte auf
 * Resultat: anzahl der produkte, die selektiert sind
 * ------------------------------------------------------------------------ */
int BuildProductList(HWND hwnd,             /* handle des dialogs           */
                     HINF hInf,             /* handle der inf-datei         */
                     char* ProdList         /* puffer fuer liste            */
                     )
{
  char Buff[256];
  INFCONTEXT infContext;
  int Cnt = 0, Idx = 0;

  ProdList[0] = ProdList[1] = 0;

  if(SetupFindFirstLine(hInf,ProductSect,0,&infContext))
    do
    {
      if(!SetupGetStringField(&infContext,2,Buff,sizeof(Buff),0) ||
        !strlen(Buff)) continue;

      if(!SelectLBIsItemChecked(GetDlgItem(hwnd,IDC_PRODLIST),Idx++))
        continue;

      if(!SetupGetStringField(&infContext,0,ProdList,128,0))
        continue;

      ProdList += strlen(ProdList)+1;
      Cnt++;
    }
    while(SetupFindNextLine(&infContext,&infContext));

  ProdList[0] = 0;

  return Cnt;
}


/* ------------------------------------------------------------------------
 * $Func BuildInstallSectionList
 * Funktion: BuildInstallSectionList
 * Aufgabe.: baut eine liste von sektionen die installiert werden sollen
 * Resultat: anzahl sektionen
 * ------------------------------------------------------------------------ */
int BuildInstallSectionList(HINF hInf,      /* handle der inf-datei         */
                            char* ProdList, /* liste selektierter produkte  */
                            char* SectList  /* puffer fuer section-liste    */
                            )
{
  int Cnt = 1;
  INFCONTEXT infContext;

  strcpy(SectList, InstallSect);
  SectList += strlen(SectList)+1;

  while(*ProdList)
  {
    if(SetupFindFirstLine(hInf,ProductSect,ProdList,&infContext) &&
      SetupGetStringField(&infContext,2,SectList,128,0))
      SectList += strlen(SectList)+1, Cnt++;
    ProdList += strlen(ProdList)+1;
  }
  SectList[0] = 0;

  return Cnt;
}

/* ------------------------------------------------------------------------
 * $Func BuildRemoveSectionList
 * Funktion: BuildRemoveSectionList
 * Aufgabe.: baut eine liste von sektionen die deinstalliert werden sollen
 * Resultat: anzahl sektionen
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int BuildRemoveSectionList(HINF hInf,       /* handle der inf-datei         */
                           char* ProdList,  /* liste selektierter produkte  */
                           char* SectList   /* puffer fuer section-liste    */
                           )
{
  HKEY hKey;
  int Rest = 0, Cnt = 1;
  INFCONTEXT infContext;
  char* pProd, Prod[256], Key[256];

  if(SetupFindFirstLine(hInf,ProductSect,0,&infContext))
    do
    {
      if(!SetupGetStringField(&infContext,0,Prod,sizeof(Prod),0)) continue;
      sprintf(Key, REG_UNINSTALL_PATH "\\%s", Prod);
      if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,Key,0,KEY_ALL_ACCESS,
        &hKey) != ERROR_SUCCESS) continue;
      RegCloseKey(hKey);
      Rest++;
      pProd = ProdList;
      while(*pProd)
      {
        if(!stricmp(pProd,Prod)) Rest--;
        pProd += strlen(pProd)+1;
      }
    }
    while(SetupFindNextLine(&infContext, &infContext));

  while(*ProdList)
  {
    if(SetupFindFirstLine(hInf,ProductSect,ProdList,&infContext) &&
      SetupGetStringField(&infContext,3,SectList,128,0))
      SectList += strlen(SectList)+1, Cnt++;
    ProdList += strlen(ProdList)+1;
  }

  if(!Rest) strcpy(SectList, RemoveSect), SectList += strlen(SectList)+1;

  SectList[0] = 0;

  return Cnt;
}

/* ------------------------------------------------------------------------
 * $Func CheckProductsForFlag
 * Funktion: CheckProductsForFlag
 * Aufgabe.: ueberprueft ob bei einem der produkte ein flag gesetzt ist
 * Resultat: anzahl produkte, bei denen das flag gesetzt ist
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int CheckProductsForFlag(HINF hInf,         /* handle der inf-datei         */
                         char* ProdList,    /* liste selektierter produkte  */
                         int iFlag)         /* zu pruefendes flag           */
{
  int Cnt;
  char Buff[256];
  INFCONTEXT infContext;

  for(Cnt = 0; *ProdList; ProdList += strlen(ProdList)+1)
  {
    if(!SetupFindFirstLine(hInf,ProductSect,ProdList,&infContext))
      continue;
      
    if(!SetupGetStringField(&infContext,4,Buff,sizeof(Buff),0))
      continue;

    if(atoi(Buff) & iFlag) Cnt++;
  }
  
  return Cnt;
}

/* ------------------------------------------------------------------------
 * $Func GetProductDescription
 * Funktion: GetProductDescription
 * Aufgabe.: liefert die beschreibung eines produktes
 * Resultat: keins
 * Hinweis.:
 * ------------------------------------------------------------------------ */
void GetProductDescription(HINF hInf,       /* handle der inf-datei         */
                           int ProdIdx,     /* index des produkts           */
                           char* Buff,      /* ^puffer                      */
                           int szBuff)      /* groesse des puffers          */
{
  char cTmp[128];
  INFCONTEXT infContext;

  Buff[0] = 0;

  if(!SetupFindFirstLine(hInf,ProductSect,0,&infContext))
    return;

  do
  {
    if(!SetupGetStringField(&infContext,2,cTmp,sizeof(cTmp),0) ||
      !strlen(cTmp)) continue;
    SetupGetStringField(&infContext, 6, Buff, szBuff, 0);
    if(!ProdIdx--) return;
  }
  while(SetupFindNextLine(&infContext,&infContext));

  return;
}

/* ------------------------------------------------------------------------
 * $Func GetProductVersionDiff
 * Funktion: GetProductVersionDiff
 * Aufgabe.: liefert die versions-differenz eines installierten produktes
 * Resultat: > 0 wenn wenn die vorhandene version aelter ist als die zu
 *           installierende, < 0 wenn die vorhandene version neuer ist.
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int GetProductVersionDiff(HINF hInf,        /* handle der inf-datei         */
                          int ProdIdx)      /* index des produkts           */
{
  HKEY hKey;
  LONG iType;
  INFCONTEXT infContext;
  int iSize, iResult, iVerOld, iVerNew;
  char * pOld, * pNew, cTmp[128], cVer[128], cKey[MAX_PATH];

  if(!SetupFindFirstLine(hInf,ProductSect,0,&infContext))
    return 0;

  do
  {
    if(!SetupGetStringField(&infContext,2,cTmp,sizeof(cTmp),0) ||
      !strlen(cTmp)) continue;              /* hat keine install-section    */

    if(ProdIdx--)
      continue;                             /* index noch nicht erreicht    */

    if(!SetupGetStringField(&infContext,0,cTmp,sizeof(cTmp),0))
      return 0;                             /* produkt-name nicht vorhanden */
    if(!SetupGetStringField(&infContext,7,cVer,sizeof(cVer),0))
      return 0;                             /* neue version nicht vorhanden */

    sprintf(cKey, REG_UNINSTALL_PATH "\\%s", cTmp);

    iResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, cKey, 0, KEY_READ, &hKey);
    if(iResult != ERROR_SUCCESS) return 0;  /* alte version unbekannt       */

    iSize = sizeof(cTmp);
    iResult = RegQueryValueEx(hKey, "ProductVersion", 0, &iType, cTmp, &iSize);
    RegCloseKey(hKey);
    if(iResult != ERROR_SUCCESS) return 0;  /* alte version unbekannt       */

    for(pOld = cTmp, pNew = cVer; ; )
    {
      iVerOld = strtol(pOld, &pOld, 10);
      iVerNew = strtol(pNew, &pNew, 10);

      if(iVerOld != iVerNew) break;
      if(*pOld++ != '.') break;
      if(*pNew++ != '.') break;
    }

    return iVerNew - iVerOld;               /* versions-differenz berechnen */
  }
  while(SetupFindNextLine(&infContext,&infContext));

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func CreateUninstallEntrys
 * Funktion: CreateUninstallEntrys
 * Aufgabe.: legt uninstall-eintraege in der registry an
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int CreateUninstallEntrys(HINF hInf,        /* handle der inf-datei         */
                          char* ProdList    /* liste selektierter produkte  */
                          )
{
  HKEY hKey;
  LONG Type;
  INFCONTEXT infContext;
  char Key[256], Cmd[MAX_PATH], Name[256], Sect[256], Uninst[256], Ver[256];

  while(*ProdList)
  {
    if(SetupFindFirstLine(hInf,ProductSect,ProdList,&infContext) &&
      SetupGetStringField(&infContext,1,Name,sizeof(Name),0) &&
      SetupGetStringField(&infContext,2,Sect,sizeof(Sect),0) &&
      SetupGetStringField(&infContext,3,Uninst,sizeof(Uninst),0) &&
      strlen(Uninst))
    {
      if(!QuerySetupExePath(hInf,Sect,Cmd) &&
        !QuerySetupExePath(hInf,InstallSect,Cmd)) return 0;

      strcat(Cmd, " /remove:");
      strcat(Cmd, ProdList);
      sprintf(Key, REG_UNINSTALL_PATH "\\%s", ProdList);

      if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,Key,0,0,0,KEY_ALL_ACCESS,0,
        &hKey,&Type) != ERROR_SUCCESS) return 0;

      RegSetValueEx(hKey, "DisplayName", 0, REG_SZ, Name, strlen(Name)+1);
      RegSetValueEx(hKey, "UninstallString", 0, REG_SZ, Cmd, strlen(Cmd)+1);

      if(SetupGetStringField(&infContext,7,Ver,sizeof(Ver),0))
        RegSetValueEx(hKey, "ProductVersion", 0, REG_SZ, Ver, strlen(Ver)+1);

      if(strlen(UserTargetDir))
        RegSetValueEx(hKey, "UserTargetDir",
        0, REG_SZ, UserTargetDir, strlen(UserTargetDir)+1);

      RegCloseKey(hKey);
    }
    ProdList += strlen(ProdList)+1;
  }

  if(SetupFindFirstLine(hInf,InstallSect,"Uninstall",&infContext) &&
    SetupGetStringField(&infContext,1,Sect,sizeof(Sect),0))
  {
    if(!SetupGetStringField(&infContext,2,Name,sizeof(Name),0))
      strcpy(Name, Sect);

    if(!QuerySetupExePath(hInf,InstallSect,Cmd)) return 0;
    strcat(Cmd, " /remove:");
    strcat(Cmd, Sect);
    sprintf(Key, REG_UNINSTALL_PATH "\\%s", Sect);
    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,Key,0,0,0,KEY_ALL_ACCESS,0,
      &hKey,&Type) != ERROR_SUCCESS) return 0;
    RegSetValueEx(hKey, "DisplayName", 0, REG_SZ, Name, strlen(Name)+1);
    RegSetValueEx(hKey, "UninstallString", 0, REG_SZ, Cmd, strlen(Cmd)+1);
    if(strlen(UserTargetDir))
      RegSetValueEx(hKey, "UserTargetDir", 0,
      REG_SZ, UserTargetDir, strlen(UserTargetDir)+1);
    RegCloseKey(hKey);
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func GetUninstallProductName
 * Funktion: GetUninstallProductName
 * Aufgabe.: liefert den namen des produkts, das deinstalliert wird
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int GetUninstallProductName(HINF hInf,      /* handle der inf-datei         */
                            char* ProdList, /* uninstall-produkt            */
                            char* Name,     /* puffer fuer name             */
                            int cbName,     /* groesse des puffers          */
                            int* iIconIdx)  /* index des product-icons      */
{
  INFCONTEXT infContext;
  char cIcon[64], cKey[256];

  if(strlen(ProdList) &&
    SetupFindFirstLine(hInf,ProductSect,ProdList,&infContext) &&
    SetupGetStringField(&infContext,1,Name,cbName,0))
  {
    if(SetupGetStringField(&infContext,5,cIcon,sizeof(cIcon),0))
      *iIconIdx = atoi(cIcon);
    else *iIconIdx = 0;
    return 1;
  }

  if(SetupFindFirstLine(hInf,InstallSect,"Uninstall",&infContext) &&
    SetupGetStringField(&infContext,1,cKey,sizeof(cKey),0) &&
    (!strlen(ProdList) || !stricmp(cKey,ProdList)) &&
    SetupGetStringField(&infContext,2,Name,cbName,0))
  {
    *iIconIdx = 0;
    return 1;
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func GetLastUserDir
 * Funktion: GetLastUserDir
 * Aufgabe.: liefert das verzeichnis, in das zuletzt installiert wurde
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int GetLastUserDir(HINF hInf,               /* handle der inf-datei         */
                   char* ProdList,          /* liste von produkten          */
                   char* Path               /* puffer fuer pfad             */
                   )
{
  HKEY hKey;
  LONG Type;
  int rc, Size;
  INFCONTEXT infCont;
  char Key[256], Prod[256];

  while(ProdList && *ProdList)
  {
    sprintf(Key, REG_UNINSTALL_PATH "\\%s", ProdList);
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,Key,0,KEY_READ,&hKey) == ERROR_SUCCESS)
    {
      Size = MAX_PATH;
      rc = RegQueryValueEx(hKey, "UserTargetDir", 0, &Type, Path, &Size);
      RegCloseKey(hKey);
      if(rc == ERROR_SUCCESS) return 1;
    }
    ProdList += strlen(ProdList)+1;
  }

  if(SetupFindFirstLine(hInf,ProductSect,0,&infCont))
    do
    {
      if(!SetupGetStringField(&infCont,0,Prod,sizeof(Prod),0)) continue;
      sprintf(Key, REG_UNINSTALL_PATH "\\%s", Prod);
      if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,Key,0,KEY_READ,&hKey) == ERROR_SUCCESS)
      {
        Size = MAX_PATH;
        rc = RegQueryValueEx(hKey, "UserTargetDir", 0, &Type, Path, &Size);
        RegCloseKey(hKey);
        if(rc == ERROR_SUCCESS) return 1;
      }
    }
    while(SetupFindNextLine(&infCont,&infCont));

  if(SetupFindFirstLine(hInf,InstallSect,"Uninstall",&infCont) &&
    SetupGetStringField(&infCont,1,Prod,sizeof(Prod),0))
  {
    sprintf(Key, REG_UNINSTALL_PATH "\\%s", Prod);
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,Key,0,KEY_READ,&hKey) == ERROR_SUCCESS)
    {
      Size = MAX_PATH;
      rc = RegQueryValueEx(hKey, "UserTargetDir", 0, &Type, Path, &Size);
      RegCloseKey(hKey);
      if(rc == ERROR_SUCCESS) return 1;
    }
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func RemoveUninstallEntrys
 * Funktion: RemoveUninstallEntrys
 * Aufgabe.: entfernt uninstall-eintraege aus der registry
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int RemoveUninstallEntrys(HINF hInf,        /* handle der inf-datei         */
                          char* ProdList    /* liste selektierter produkte  */
                          )
{
  HKEY hKey;

  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_UNINSTALL_PATH,
    0, KEY_READ, &hKey) != ERROR_SUCCESS) return 0;

  while(*ProdList)
  {
    RegDeleteKey(hKey, ProdList);
    ProdList += strlen(ProdList)+1;
  }
  RegCloseKey(hKey);

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func InstallMultipleSections
 * Funktion: InstallMultipleSections
 * Aufgabe.: bearbeitet install-sections aus einer inf-datei
 * Resultat: TRUE wenn erfolgreich, sonst GetLastError()
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int InstallMultipleSections(HINF hInf,      /* handle der inf-datei         */
                            char* Sections, /* namen der install-sections   */
                            HWND hwnd       /* callback-fenster             */
                            )
{
  char* Section;
  void** pContext;
  HSPFILEQ FileQ;

  FileQ = SetupOpenFileQueue();

  Section = Sections;
  while(*Section)
  {
    if(!SetupInstallFilesFromInfSection(hInf,0,FileQ,Section,".",SP_COPY_NEWER))
    {
      int err = GetLastError();
      SetupCloseFileQueue(FileQ);
      SetLastError(err);
      return 0;
    }
    Section += strlen(Section)+1;
  }

  Section = Sections;
  while(*Section)
  {
    if(!DoExecStatements(hInf,Section,1,hwnd)) return 0;
    Section += strlen(Section)+1;
  }

  PostMessage(hwnd, WM_USER_STEP, (WPARAM)2, 0);
  Section = Sections;
  while(*Section)
  {
    if(!StopServices(hInf,Section)) Reboot = 1;
    Section += strlen(Section)+1;
  }

  PostMessage(hwnd, WM_USER_STEP, (WPARAM)3, 0);
  pContext = (void**)malloc(3*sizeof(void*));
  pContext[0] = SetupInitDefaultQueueCallbackEx(hwnd,
    hwnd, WM_USER_PROGRESS, 0, 0);
  if(!SetupCommitFileQueue(hwnd,FileQ,QueueCallback,pContext))
  {
    int err = GetLastError();
    SetupTermDefaultQueueCallback(pContext[0]);
    free(pContext);
    SetupCloseFileQueue(FileQ);
    SetLastError(err);
    return 0;
  }
  SetupTermDefaultQueueCallback(pContext[0]);
  free(pContext);
  if(SetupPromptReboot(FileQ,hwnd,1) & SPFILEQ_FILE_IN_USE) Reboot = 1;
  SetupCloseFileQueue(FileQ);

  PostMessage(hwnd, WM_USER_STEP, (WPARAM)4, 0);
  Section = Sections;
  while(*Section)
  {
    if(!SetupInstallFromInfSection(hwnd,hInf,Section,SPINST_INIFILES|
      SPINST_REGISTRY|SPINST_INI2REG,0,0,0,0,0,0,0)) return 0;
    Section += strlen(Section)+1;
  }

  Section = Sections;
  while(*Section)
  {
    if(!InstallServices(hInf,Section)) return 0;
    Section += strlen(Section)+1;
  }

  PostMessage(hwnd, WM_USER_STEP, (WPARAM)5, 0);
  Section = Sections;
  while(*Section)
  {
    if(!UpdateShortcuts(hInf,Section)) return 0;
    Section += strlen(Section)+1;
  }

  PostMessage(hwnd, WM_USER_STEP, (WPARAM)6, 0);
  Section = Sections;
  while(*Section)
  {
    if(!StartServices(hInf,Section)) Reboot = 1;
    Section += strlen(Section)+1;
  }

  PostMessage(hwnd, WM_USER_STEP, (WPARAM)-1, 0);
  Section = Sections;
  while(*Section)
  {
    if(!DoExecStatements(hInf,Section,2,hwnd)) return 0;
    Section += strlen(Section)+1;
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func StopServices
 * Funktion: InstallServices
 * Aufgabe.: stopt services aus einer service-install-section
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int StopServices(HINF hInf,                 /* handle der inf-datei         */
                 char* Section              /* name der install-section     */
                 )
{
  INFCONTEXT infCont;
  char SvcSect[256], Cmd[256], Name[256];

  if(!IS_WINNT) return 1;
  sprintf(SvcSect, "%s.Services", Section);

  if(SetupFindFirstLine(hInf,SvcSect,0,&infCont))
  {
    do
    {
      if(!SetupGetStringField(&infCont,0,Cmd,sizeof(Cmd),0)) continue;
      if(!SetupGetStringField(&infCont,1,Name,sizeof(Name),0)) continue;
      if(stricmp(Cmd,"StopService")) continue;
      if(!DoStopService(Name)) return 0;
    }
    while(SetupFindNextLine(&infCont,&infCont));
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func StartServices
 * Funktion: StartServices
 * Aufgabe.: startet services aus einer service-install-section
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int StartServices(HINF hInf,                /* handle der inf-datei         */
                  char* Section             /* name der install-scetion     */
                  )
{
  INFCONTEXT infCont;
  char SvcSect[256], Cmd[256], Name[256];

  if(!IS_WINNT) return 1;
  sprintf(SvcSect, "%s.Services", Section);

  if(SetupFindFirstLine(hInf,SvcSect,0,&infCont))
  {
    do
    {
      if(!SetupGetStringField(&infCont,0,Cmd,sizeof(Cmd),0)) continue;
      if(!SetupGetStringField(&infCont,1,Name,sizeof(Name),0)) continue;
      if(stricmp(Cmd,"StartService")) continue;
      if(!DoStartService(Name)) return 0;
    }
    while(SetupFindNextLine(&infCont,&infCont));
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func InstallServices
 * Funktion: InstallServices
 * Aufgabe.: bearbeitet service-install-section
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int InstallServices(HINF hInf,              /* handle der inf-datei         */
                    char* Section           /* name der install-section     */
                    )
{
  INFCONTEXT infCont;
  char SvcSect[256], Cmd[256], Name[256], InstSect[256];

  if(!IS_WINNT) return 1;
  sprintf(SvcSect, "%s.Services", Section);

  if(SetupFindFirstLine(hInf,SvcSect,0,&infCont))
  {
    do
    {
      if(!SetupGetStringField(&infCont,0,Cmd,sizeof(Cmd),0)) continue;
      if(!SetupGetStringField(&infCont,1,Name,sizeof(Name),0)) continue;
      if(!stricmp(Cmd,"AddService"))
      {
        SetupGetStringField(&infCont,3,InstSect,sizeof(InstSect),0);
        if(!DoAddService(hInf,Name,InstSect)) return 0;
      }
      else if(!stricmp(Cmd,"DelService"))
      {
        if(!DoDelService(Name)) return 0;
      }
    }
    while(SetupFindNextLine(&infCont,&infCont));
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func DoStopService
 * Funktion: DoStopService
 * Aufgabe.: stopt einen service
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int DoStopService(char* SvcName)            /* service-name                 */
{
  int Cnt = 0;
  SERVICE_STATUS svcStatus;
  SC_HANDLE hManager, hService;

  hManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
  if(!hManager) return 0;

  hService = OpenService(hManager, SvcName, SERVICE_ALL_ACCESS);
  if(!hService && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
  {
    CloseServiceHandle(hManager);
    return 0;
  }
  else if(hService)
  {
    QueryServiceStatus(hService, &svcStatus);
    if(svcStatus.dwCurrentState != SERVICE_STOPPED)
      ControlService(hService, SERVICE_CONTROL_STOP, &svcStatus);
    while(svcStatus.dwCurrentState != SERVICE_STOPPED && Cnt++ < 20)
      Sleep(250), QueryServiceStatus(hService, &svcStatus);
    CloseServiceHandle(hService);
    if(svcStatus.dwCurrentState != SERVICE_STOPPED)
    {
      CloseServiceHandle(hManager);
      return 0;
    }
  }

  CloseServiceHandle(hManager);
  return 1;
}


/* ------------------------------------------------------------------------
 * $Func DoAddService
 * Funktion: DoAddService
 * Aufgabe.: fuegt einen service hinzu (wie in einer inf-section angegeben)
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int DoAddService(HINF hInf,                 /* handle der inf-datei         */
                 char* SvcName,             /* name des services            */
                 char* SvcSection           /* install-section des svc.     */
                 )
{
  INFCONTEXT infCont;
  SC_HANDLE hManager, hService;
  char SvcDesc[256], SvcBinary[256],
    LoadOrdGrp[256], Depend[256], StartName[256];
  char* pSvcDesc = 0, *pSvcBinary = 0,
    *pLoadOrdGrp = 0, *pDepend = 0, *pStartName = 0;
  int Idx = 1, SvcType = 0, StartType = 0, ErrorCtrl = 0;

  if(!SetupFindFirstLine(hInf,SvcSection,0,&infCont)) return 0;

  if(SetupFindFirstLine(hInf,SvcSection,"DisplayName",&infCont))
    SetupGetStringField(&infCont,1,SvcDesc,sizeof(SvcDesc),0),
    pSvcDesc = SvcDesc;

  if(SetupFindFirstLine(hInf,SvcSection,"ServiceType",&infCont))
    SetupGetIntField(&infCont,1,&SvcType);

  if(SetupFindFirstLine(hInf,SvcSection,"StartType",&infCont))
    SetupGetIntField(&infCont,1,&StartType);

  if(SetupFindFirstLine(hInf,SvcSection,"ErrorControl",&infCont))
    SetupGetIntField(&infCont,1,&ErrorCtrl);

  if(SetupFindFirstLine(hInf,SvcSection,"ServiceBinary",&infCont))
    SetupGetStringField(&infCont,1,SvcBinary,sizeof(SvcBinary),0),
    pSvcBinary = SvcBinary;

  if(SetupFindFirstLine(hInf,SvcSection,"LoadOrderGroup",&infCont))
    SetupGetStringField(&infCont,1,LoadOrdGrp,sizeof(LoadOrdGrp),0),
    pLoadOrdGrp = LoadOrdGrp;

  if(SetupFindFirstLine(hInf,SvcSection,"Dependencies",&infCont))
  {
    pDepend = Depend;
    while(SetupGetStringField(&infCont,Idx++,pDepend,sizeof(Depend),0))
      pDepend += strlen(pDepend)+1;
    pDepend[0] = 0;
    pDepend = Depend;
  }

  if(SetupFindFirstLine(hInf,SvcSection,"StartName",&infCont))
    SetupGetStringField(&infCont,1,StartName,sizeof(StartName),0),
    pStartName = StartName;

  hManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
  if(!hManager) return 0;

  hService = OpenService(hManager, SvcName, SERVICE_ALL_ACCESS);
  if(hService)
  {
    ChangeServiceConfig(hService, SvcType, StartType, ErrorCtrl,
      pSvcBinary, pLoadOrdGrp, 0, pDepend, pStartName, 0, pSvcDesc);
    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    AddServiceToGroupOrder(hInf,SvcSection);
    return 1;
  }

  hService = CreateService(hManager, SvcName, pSvcDesc,
    SERVICE_ALL_ACCESS, SvcType, StartType, ErrorCtrl, pSvcBinary,
    pLoadOrdGrp, 0, pDepend, pStartName, 0);
  if(hService)
  {
    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    AddServiceToGroupOrder(hInf,SvcSection);
    return 1;
  }

  CloseServiceHandle(hManager);
  return 0;
}

/* ------------------------------------------------------------------------
 * $Func DoDelService
 * Funktion: DoDelService
 * Aufgabe.: loescht einen service
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int DoDelService(char* SvcName)             /* name des services            */
{
  SC_HANDLE hManager, hService;

  hManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
  if(!hManager) return 0;

  hService = OpenService(hManager, SvcName, SERVICE_ALL_ACCESS);
  if(!hService && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
  {
    CloseServiceHandle(hManager);
    return 0;
  }
  else if(hService)
  {
    DeleteService(hService);
    CloseServiceHandle(hService);
  }

  CloseServiceHandle(hManager);

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func DoStartService
 * Funktion: DoStartService
 * Aufgabe.: startet einen service
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int DoStartService(char* SvcName)           /* name des services            */
{
  int Cnt = 0;
  SERVICE_STATUS svcStatus;
  SC_HANDLE hManager, hService;

  hManager = OpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
  if(!hManager) return 0;

  hService = OpenService(hManager, SvcName, SERVICE_ALL_ACCESS);
  if(!hService)
  {
    CloseServiceHandle(hManager);
    return 0;
  }

  if(StartService(hService,0,0))
    while(Cnt++ < 20)
    {
      QueryServiceStatus(hService, &svcStatus);
      if(svcStatus.dwCurrentState == SERVICE_RUNNING) break;
      else Sleep(250);
    }

  CloseServiceHandle(hService);
  CloseServiceHandle(hManager);

  return (svcStatus.dwCurrentState == SERVICE_RUNNING);
}


/* ------------------------------------------------------------------------
 * $Func AddServiceToGroupOrder
 * Funktion: AddServiceToGroupOrder
 * Aufgabe.: fuegt eine
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int AddServiceToGroupOrder(HINF hInf,       /* handle der inf-datei         */
                           char* SvcSection /* service-install-section      */
                           )
{
  int i;
  HKEY hKey;
  LONG Type, Size;
  INFCONTEXT infCont;
  char GrpOrder[1024], Group[256], GrpRel[256], * pGrpRel = 0;

  if(!SetupFindFirstLine(hInf,SvcSection,"ServiceGroupOrder",&infCont))
    return 1;

  if(!SetupGetStringField(&infCont,1,Group,sizeof(Group),0)) return 0;
  if(!SetupGetStringField(&infCont,2,GrpRel,sizeof(GrpRel),0)) GrpRel[0] = 0;

  if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,"System\\CurrentControlSet\\"
    "Control\\ServiceGroupOrder",0,0,0,KEY_ALL_ACCESS,0,&hKey,&Type)
    != ERROR_SUCCESS) return 0;

  Size = sizeof(GrpOrder);
  if(RegQueryValueEx(hKey,"List",0,&Type,GrpOrder,&Size))
    GrpOrder[0] = GrpOrder[1] = 0;

  i = FindMultiString(GrpOrder, Group);
  if(i >= 0) RemoveMultiString(GrpOrder, i);

  pGrpRel = GrpRel;
  if(GrpRel[0] == '+' || GrpRel[0] == '-') pGrpRel++;

  i = FindMultiString(GrpOrder, pGrpRel);
  if(i < 0) i = 0;
  if(GrpRel[0] == '-') i = i ? i+1 : -1;

  InsertMultiString(GrpOrder, Group, i);

  RegSetValueEx(hKey, "List", 0,
    REG_MULTI_SZ, GrpOrder, GetMultiStringLength(GrpOrder));

  RegCloseKey(hKey);

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func UpdateShortcuts
 * Funktion: UpdateShortcuts
 * Aufgabe.: bearbeitet 'MakeIcons' und 'DelIcons' aus einer section
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int UpdateShortcuts(HINF hInf,              /* handle der inf-datei         */
                    char* Section           /* name der install-section     */
                    )
{
  int Sect;
  INFCONTEXT infCont1, infCont2;
  char SubSect[256], Where[16], Title[256], Exe[256],
    Parms[256], Icon[256], IconIdx[256], Flags[256];

  if(SetupFindFirstLine(hInf,Section,"MakeIcons",&infCont1))
  {
    Sect = 1;
    while(SetupGetStringField(&infCont1,Sect++,SubSect,sizeof(SubSect),0))
    {
      if(!SetupFindFirstLine(hInf,SubSect,0,&infCont2)) continue;
      do
      {
        if(!SetupGetStringField(&infCont2,1,Where,sizeof(Where),0)) continue;
        if(!SetupGetStringField(&infCont2,2,Title,sizeof(Title),0)) continue;
        if(!SetupGetStringField(&infCont2,3,Exe,sizeof(Exe),0)) continue;
        if(!SetupGetStringField(&infCont2,4,Parms,sizeof(Parms),0))
          Parms[0] = 0;
        if(!SetupGetStringField(&infCont2,5,Icon,sizeof(Icon),0))
          Icon[0] = 0;
        if(!SetupGetStringField(&infCont2,6,IconIdx,sizeof(IconIdx),0))
          IconIdx[0] = 0;
        if(!SetupGetStringField(&infCont2,7,Flags,sizeof(Flags),0))
          Flags[0] = 0;
        CreateShortcut(Where, Title, Exe, Parms, Icon,
          atoi(IconIdx), atoi(Flags));
      }
      while(SetupFindNextLine(&infCont2,&infCont2));
    }
  }
  if(SetupFindFirstLine(hInf,Section,"DelIcons",&infCont1))
  {
    Sect = 1;
    while(SetupGetStringField(&infCont1,Sect++,SubSect,sizeof(SubSect),0))
    {
      if(!SetupFindFirstLine(hInf,SubSect,0,&infCont2)) continue;
      do
      {
        if(!SetupGetStringField(&infCont2,1,Where,sizeof(Where),0)) continue;
        if(!SetupGetStringField(&infCont2,2,Title,sizeof(Title),0)) continue;
        DeleteShortcut(Where, Title);
      }
      while(SetupFindNextLine(&infCont2,&infCont2));
    }
  }

  return 1;
}

/* ------------------------------------------------------------------------
 * $Func CreateShortcut
 * Funktion: CreateShortcut
 * Aufgabe.: erzeugt einen link im start-menu
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int CreateShortcut(char* Where,             /* wohin mit dem link           */
                   char* Title,             /* titel des links              */
                   char* Exe,               /* pfad und name der exe-datei  */
                   char* Arguments,         /* parameter                    */
                   char* Icon,              /* icon-datei                   */
                   int IconIdx,             /* icon-index                   */
                   int Flags                /* enthaltenden ordner oeffnen  */
                   )
{
  char* c;
  HRESULT hres;
  IShellLink* psl;
  WORD wLink[MAX_PATH];
  char WorkPath[MAX_PATH], Link[MAX_PATH];

  strcpy(WorkPath, Exe);
  if(strrchr(WorkPath,'\\')) strrchr(WorkPath,'\\')[0] = 0;

  CoInitialize(0);

  hres = CoCreateInstance(&CLSID_ShellLink, 0,
    CLSCTX_INPROC_SERVER, &IID_IShellLink, &psl);

  if(SUCCEEDED(hres))
  {
    IPersistFile* ppf;

    psl->lpVtbl->SetPath(psl, Exe);
    psl->lpVtbl->SetWorkingDirectory(psl, WorkPath);
    psl->lpVtbl->SetArguments(psl, Arguments);
    psl->lpVtbl->SetIconLocation(psl, Icon, IconIdx);

    hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, &ppf);
    if(SUCCEEDED(hres))
    {
      QueryStartMenuProgramFolder(Link, Where);
      c = Link + strlen(Link);
      strcat(Link, "\\");
      strcat(Link, Title);
      strcat(Link, ".lnk");
      while(c = strchr(c+1,'\\'))
        *c = 0, CreateDirectory(Link, 0), *c = '\\';

      MultiByteToWideChar(CP_ACP, 0, Link, -1, wLink, MAX_PATH);

      hres = ppf->lpVtbl->Save(ppf, wLink, TRUE);
      ppf->lpVtbl->Release(ppf);

      if(Flags)
      {
        if(strrchr(Link,'\\')) *strrchr(Link,'\\') = 0;
        ShellExecute(0, 0, Link, 0, 0, SW_SHOWNA);
      }
    }
    psl->lpVtbl->Release(psl);
  }

  CoUninitialize();
  return SUCCEEDED(hres);
}


/* ------------------------------------------------------------------------
 * $Func DeleteShortcut
 * Funktion: DeleteShortcut
 * Aufgabe.: erzeugt einen link im start-menu
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int DeleteShortcut(char* Where,             /* wo ist der link              */
                   char* Title)             /* titel des links              */
{
  char Link[MAX_PATH], Path[MAX_PATH];

  if(QueryStartMenuProgramFolder(Path,Where))
  {
    strcpy(Link, Path);
    strcat(Link, "\\");
    strcat(Link, Title);
    strcat(Link, ".lnk");
    DeleteFile(Link);
    while(strrchr(Link,'\\'))
    {
      strrchr(Link,'\\')[0] = 0;
      if(strlen(Link) <= strlen(Path)) break;
      if(!RemoveDirectory(Link)) break;
    }
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func DoExecStatements
 * Funktion: DoExecStatements
 * Aufgabe.: fuehrt exec-statements aus
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int DoExecStatements(HINF hInf,             /* handle der inf-datei         */
                     char* Section,         /* install-section              */
                     int InstallStep,       /* installations-schritt        */
                     HWND hwndOwner)        /* owner fuer kind-prozesse     */
{
  int Sect;
  INFCONTEXT infCont1, infCont2;
  char SubSect[256], Step[16], Cmd[256], Arg[256],
    WorkDir[256], Flags[16], Path[256];

  if(SetupFindFirstLine(hInf,Section,"Exec",&infCont1))
  {
    Sect = 1;
    while(SetupGetStringField(&infCont1,Sect++,SubSect,sizeof(SubSect),0))
    {
      if(SubSect[0] == '@') ExecuteCommand(SubSect+1, 0, 0, 0, hwndOwner);
      else if(!SetupFindFirstLine(hInf,SubSect,0,&infCont2)) continue;
      do
      {
        if(!SetupGetStringField(&infCont2,1,Step,sizeof(Step),0)) continue;
        if(strtol(Step,0,0) != InstallStep) continue;
        if(!SetupGetStringField(&infCont2,2,Cmd,sizeof(Cmd),0)) continue;
        if(!SetupGetStringField(&infCont2,3,Arg,sizeof(Arg),0))
          Arg[0] = 0;
        if(!SetupGetStringField(&infCont2,4,WorkDir,sizeof(WorkDir),0))
          WorkDir[0] = 0;
        if(!SetupGetStringField(&infCont2,5,Flags,sizeof(Flags),0))
          Flags[0] = 0;
        GetCurrentDirectory(sizeof(Path), Path);
        ExecuteCommand(Cmd, Arg, WorkDir, strtol(Flags,0,0), hwndOwner);
        SetCurrentDirectory(Path);
      }
      while(SetupFindNextLine(&infCont2,&infCont2));
    }
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func ExecuteCommand
 * Funktion: ExecuteCommand
 * Aufgabe.: fuehrt einen externen befehl durch
 * Resultat: haengt von der notifcation ab
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int ExecuteCommand(char* Cmd,               /* befehl                       */
                   char* Arg,               /* argumente                    */
                   char* WorkDir,           /* arbeitsverzeichnis           */
                   int Flags,               /* weitere optionen             */
                   HWND hwndOwner)          /* owner fuer kind-prozesse     */
{
  SHELLEXECUTEINFO execInfo;
  char cArg[1024], cHwnd[16], *pHlp;
  
  strcpy(cArg, Arg);
  sprintf(cHwnd, "0x%08x", hwndOwner);
  
  while((pHlp = strchr(cArg,'%')) && !strnicmp(pHlp,"%hwnd%",6))
  {
    memmove(pHlp + strlen(cHwnd), pHlp + 6, strlen(pHlp+6) + 1);
    strncpy(pHlp, cHwnd, strlen(cHwnd));
  }

  execInfo.cbSize = sizeof(execInfo);
  execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  execInfo.hwnd = GetDesktopWindow();
  execInfo.lpVerb = 0;
  execInfo.lpFile = Cmd;
  execInfo.lpParameters = cArg;
  execInfo.lpDirectory = WorkDir;
  execInfo.nShow = SW_SHOWNORMAL;

  if(!ShellExecuteEx(&execInfo)) return 0;
  if(!execInfo.hProcess) return 0;
  
  // Flags & 0x01 == do not wait for process
  // Flags & 0x02 == delete file after executing, not in combination with 0x01
  
  if(Flags & 0x01)
    return 1;

  WaitForSingleObject(execInfo.hProcess, INFINITE);
  
  if(Flags & 0x02)
    DeleteFile(Cmd);

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func QueueCallback
 * Funktion: QueueCallback
 * Aufgabe.: callback-funktion fuer die file-queue
 * Resultat: haengt von der notifcation ab
 * Hinweis.:
 * ------------------------------------------------------------------------ */
UINT WINAPI QueueCallback(VOID* Context,    /* context-struktur             */
                         UINT Notification, /* notification-code            */
                         UINT Param1,       /* zusaetzlicher parameter      */
                         UINT Param2        /* zusaetzlicher parameter      */
                         )
{
  HANDLE hFind;
  DWORD iResult;
  FILEPATHS* pPath;
  WIN32_FIND_DATA FindData;
  int ZombieFiles, FileCnt;
  void** pContext = (void**)Context;
  char* pLastDir, CurrDir[MAX_PATH], * pHlp1, * pHlp2;

  pLastDir = (char*)pContext[1];
  ZombieFiles = (int)pContext[2];

  switch(Notification)
  {
    case SPFILENOTIFY_STARTQUEUE:

      pLastDir = (void*)malloc(MAX_PATH);
      pLastDir[0] = 0;
      break;

    case SPFILENOTIFY_ENDQUEUE:

      free(pLastDir);
      break;

    case SPFILENOTIFY_STARTCOPY:
    
      pPath = (FILEPATHS*)Param1;
      
      pHlp1 = strrchr(pPath->Source,'\\') ?
        strrchr(pPath->Source,'\\') : pPath->Source;
      pHlp2 = strrchr(pPath->Target,'\\') ?
        strrchr(pPath->Target,'\\') : pPath->Target;
      
      if(!stricmp(pHlp1,pHlp2))
      {      
        pHlp1 = strrchr(pHlp1, '.');
        pHlp2 = strrchr(pHlp2, '.');
      
        if(pHlp1 && !stricmp(pHlp1,".cab") && pHlp2 && !stricmp(pHlp2,".cab"))
        {
          SetFileAttributes(pPath->Target, FILE_ATTRIBUTE_NORMAL);
          iResult = CopyFile(pPath->Source, pPath->Target, 0);
          SetFileAttributes(pPath->Target, FILE_ATTRIBUTE_NORMAL);
          return iResult ? FILEOP_SKIP : FILEOP_ABORT;
        }
      }
      
      break;

    case SPFILENOTIFY_ENDDELETE:

      pPath = (FILEPATHS*)Param1;

      strcpy(CurrDir, pPath->Target);
      if(strrchr(CurrDir,'\\')) strrchr(CurrDir,'\\')[0] = 0;
      if(strcmp(CurrDir,pLastDir))
        strcpy(pLastDir,CurrDir), ZombieFiles = 0;

      hFind = FindFirstFile(pPath->Target, &FindData);
      if(hFind != INVALID_HANDLE_VALUE)
      {
        FindClose(hFind);
        Reboot = 1;
        ZombieFiles++;
        DelayFileDelete((char*)pPath->Target);
      }

      FileCnt = GetFileCount(CurrDir) - ZombieFiles;
      if(!FileCnt && !RemoveDirectory(CurrDir)) DelayDirDelete(CurrDir);

      break;
  }

  pContext[1] = (void*)pLastDir;
  pContext[2] = (void*)ZombieFiles;

  return SetupDefaultQueueCallback(pContext[0],Notification,Param1,Param2);
}


/* ------------------------------------------------------------------------
 * $Func GetDefaultTargetPath
 * Funktion: GetDefaultTargetPath
 * Aufgabe.: liefert zielverzeichnis
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int GetDefaultTargetPath(HINF hInf,         /* handle der inf-datei         */
                         char* Path,        /* ^puffer fuer pfad            */
                         char* RelPath      /* ^puffer fuer relativ-pfad    */
                         )
{
  int DirID = 0;
  INFCONTEXT infCont;
  char Buff[MAX_PATH] = "", Buff2[MAX_PATH] = "";

  if(SetupFindFirstLine(hInf,"DestinationDirs","DefaultDestDir",&infCont))
  {
    SetupGetStringField(&infCont,1,Buff,sizeof(Buff),0);
    SetupGetStringField(&infCont,2,Buff2,sizeof(Buff2),0);
    DirID = atoi(Buff);
  }

  if(DirID == 24 && QueryApplPath(Buff))
  {
    strcpy(Path, Buff);
    if(Path[strlen(Path)-1] != '\\') strcat(Path, "\\");
    if(strlen(Buff2) && Buff2[0] == '\\') strcat(Path, Buff2+1);
    else if(strlen(Buff2)) strcat(Path, Buff2);
    if(Path[strlen(Path)-1] == '\\') Path[strlen(Path)-1] = 0;
    if(RelPath) strcpy(RelPath, Buff2);
  }
  else
  {
    SetupGetTargetPath(hInf, 0, 0, Path, MAX_PATH, 0);
    if(RelPath) strcpy(RelPath, Buff2);
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func QueryTargetPath
 * Funktion: QueryTargetPath
 * Aufgabe.: oeffnet dialog, in dem zielverzeichnis gewaehlt werden kann
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int QueryTargetPath(HWND hwnd,              /* handle des owner-dialogs     */
                    char* Path,             /* puffer fuer pfad             */
                    char* RelPath           /* optional: relativ-pfad       */
                    )
{
  int rc;
  LPMALLOC pIMalloc;
  BROWSEINFO BrowseInfo;
  ITEMIDLIST* pItemIdList;
  char buff[MAX_PATH], buff2[MAX_PATH];
  HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hwnd, GWL_HINSTANCE);

  if(SHGetMalloc(&pIMalloc) != NOERROR) return 0;

  LoadString(hInstance, STR_BROWSETITLE, buff, sizeof(buff));

  BrowseInfo.hwndOwner = hwnd;
  BrowseInfo.pidlRoot = 0;
  BrowseInfo.pszDisplayName = buff2;
  BrowseInfo.lpszTitle = buff;
  BrowseInfo.ulFlags = BIF_RETURNONLYFSDIRS;
  BrowseInfo.lpfn = 0;
  BrowseInfo.lParam = 0;
  BrowseInfo.iImage = 0;

  pItemIdList = (ITEMIDLIST*)SHBrowseForFolder(&BrowseInfo);
  if(pItemIdList)
  {
    rc = SHGetPathFromIDList(pItemIdList,Path);
    pIMalloc->lpVtbl->Free(pIMalloc, pItemIdList);
    pIMalloc->lpVtbl->Release(pIMalloc);
    if(!rc) return 0;

    if(!RelPath) return 1;
    if(RelPath[0] == '\\') RelPath++;
    if(!strlen(RelPath)) return 1;

    if(strrchr(Path,'\\') && !strcmp(strrchr(Path,'\\')+1,RelPath))
      return 1;
    if(Path[strlen(Path)-1] != '\\') strcat(Path, "\\");
    strcat(Path, RelPath);

    return 1;
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func QueryApplPath
 * Funktion: QueryApplPath
 * Aufgabe.: liefert den pfad des applikations-verzeichnisses
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int QueryApplPath(char* Path)               /* puffer fuer den pfad         */
{
  int rc;
  HKEY key;
  LONG type;
  LONG buffsize;
  char buff[MAX_PATH];

  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\"
    "CurrentVersion",0,KEY_READ,&key) != ERROR_SUCCESS) return 0;

  buffsize = sizeof(buff);
  rc = RegQueryValueEx(key, "ProgramFilesDir", 0, &type, buff, &buffsize);
  RegCloseKey(key);
  if(rc != ERROR_SUCCESS) return 0;

  if(type == REG_EXPAND_SZ)
    ExpandEnvironmentStrings(buff, Path, MAX_PATH);
  else if(type == REG_SZ)
    strncpy(Path, buff, MAX_PATH-1), Path[MAX_PATH-1] = 0;
  else return 0;

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func QueryStartMenuProgramFolder
 * Funktion: QueryStartMenuProgramFolder
 * Aufgabe.: liefert den pfad des Programm-Ordners im Start-Menue
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int QueryStartMenuProgramFolder(char* Path, /* puffer fuer den pfad         */
                                char* Where /* wo                           */
                                )
{
  int rc;
  char buff[256];
  LONG type, buffsize;
  HKEY hKeyCU = 0, hKeyLM = 0;

  RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows"
    "\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ, &hKeyCU);
  RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows"
    "\\CurrentVersion\\Explorer\\Shell Folders", 0, KEY_READ, &hKeyLM);

  if(!hKeyCU && !hKeyLM) return 0;
  buffsize = sizeof(buff);

  if(!stricmp(Where,"CDSK"))
  {
    if(hKeyLM)
      rc = RegQueryValueEx(hKeyLM, "Common Desktop", 0, &type, buff, &buffsize);
    if(!hKeyLM || (rc != ERROR_SUCCESS && hKeyCU))
      rc = RegQueryValueEx(hKeyCU, "Desktop", 0, &type, buff, &buffsize);
  }
  else if(!stricmp(Where,"CPRG"))
  {
    if(hKeyLM)
      rc = RegQueryValueEx(hKeyLM, "Common Programs", 0, &type, buff, &buffsize);
    if(!hKeyLM || (rc != ERROR_SUCCESS && hKeyCU))
      rc = RegQueryValueEx(hKeyCU, "Programs", 0, &type, buff, &buffsize);
  }
  else if(!stricmp(Where,"CSTM"))
  {
    if(hKeyLM)
      rc = RegQueryValueEx(hKeyLM, "Common Start Menu", 0, &type, buff, &buffsize);
    if(!hKeyLM || (rc != ERROR_SUCCESS && hKeyCU))
      rc = RegQueryValueEx(hKeyCU, "Start Menu", 0, &type, buff, &buffsize);
  }
  else if(!stricmp(Where,"CSTU"))
  {
    if(hKeyLM)
      rc = RegQueryValueEx(hKeyLM, "Common Startup", 0, &type, buff, &buffsize);
    if(!hKeyLM || (rc != ERROR_SUCCESS && hKeyCU))
      rc = RegQueryValueEx(hKeyCU, "Startup", 0, &type, buff, &buffsize);
  }
  else if(!stricmp(Where,"UDSK"))
  {
    if(hKeyCU)
      rc = RegQueryValueEx(hKeyCU, "Desktop", 0, &type, buff, &buffsize);
  }
  else if(!stricmp(Where,"UPRG"))
  {
    if(hKeyCU)
      rc = RegQueryValueEx(hKeyCU, "Programs", 0, &type, buff, &buffsize);
  }
  else if(!stricmp(Where,"USTM"))
  {
    if(hKeyCU)
      rc = RegQueryValueEx(hKeyCU, "Start Menu", 0, &type, buff, &buffsize);
  }
  else if(!stricmp(Where,"USTU"))
  {
    if(hKeyCU)
      rc = RegQueryValueEx(hKeyCU, "Startup", 0, &type, buff, &buffsize);
  }
  else rc = !ERROR_SUCCESS;

  if(hKeyCU) RegCloseKey(hKeyCU);
  if(hKeyLM) RegCloseKey(hKeyLM);

  if(rc != ERROR_SUCCESS) return 0;

  if(type == REG_SZ)
  {
    strncpy(Path, buff, MAX_PATH-1);
    Path[MAX_PATH-1] = 0;
    return 1;
  }
  if(type == REG_EXPAND_SZ)
  {
    ExpandEnvironmentStrings(buff,Path,MAX_PATH);
    return 1;
  }

  return 0;
}


/* ------------------------------------------------------------------------
 * $Func QuerySetupExePath
 * Funktion: QuerySetupExePath
 * Aufgabe.: liefert den pfad zur installierten setup.exe
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int QuerySetupExePath(HINF hInf,            /* handle der inf-datei         */
                      char* Section,        /* install-section              */
                      char* Path            /* puffer fuer pfad             */
                      )
{
  int Idx = 1;
  char CopySect[256], File[256];
  INFCONTEXT infCont1, infCont2;

  if(SetupFindFirstLine(hInf,Section,"CopyFiles",&infCont1))
  {
    while(SetupGetStringField(&infCont1,Idx++,CopySect,sizeof(CopySect),0))
    {
      if(!SetupFindFirstLine(hInf,CopySect,0,&infCont2)) continue;
      do
      {
        if(!SetupGetStringField(&infCont2,1,File,sizeof(File),0)) continue;
        if(stricmp(File,EXENAME)) continue;
        if(SetupGetTargetPath(hInf,&infCont2,0,Path,MAX_PATH,0))
        {
          strcat(Path,"\\");
          strcat(Path,EXENAME);
          return 1;
        }
      }
      while(SetupFindNextLine(&infCont2,&infCont2));
    }
  }

  return 0;
}


/* ------------------------------------------------------------------------
 * $Func GetFileCount
 * Funktion: GetFileCount
 * Aufgabe.: zaehlt dateien in einem verzeichnis
 * Resultat: anzahl dateien
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int GetFileCount(char* Path)                /* verzeichnis-name             */
{
  int Cnt = 0;
  HANDLE hFind;
  char Buff[MAX_PATH];
  WIN32_FIND_DATA Find;

  sprintf(Buff, "%s\\*", Path);

  hFind = FindFirstFile(Buff,&Find);
  if(hFind == INVALID_HANDLE_VALUE) return 0;

  do
  {
    if(Find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
    else Cnt++;
  }
  while(FindNextFile(hFind,&Find));

  FindClose(hFind);

  return Cnt;
}


/* ------------------------------------------------------------------------
 * $Func DelayFileDelete
 * Funktion: DelayFileDelete
 * Aufgabe.: markiert eine datei zum loeschen beim naechsten systemstart
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int DelayFileDelete(char* File)             /* datei-name                   */
{
  int Size;
  char Ini[256];
  char Name[256];
  char Buff[1024];

  if(IS_WINNT) MoveFileEx(File, 0, MOVEFILE_DELAY_UNTIL_REBOOT);
  else
  {
    GetWindowsDirectory(Ini, sizeof(Ini));
    strcat(Ini, "\\wininit.ini");
    GetShortPathName(File, Name, sizeof(Name));
    Size = GetPrivateProfileSection("Rename", Buff, sizeof(Buff), Ini);
    sprintf(Buff + Size, "NUL=%s%c", Name, 0);
    WritePrivateProfileSection("Rename", Buff, Ini);
  }

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func DelayDirDelete
 * Funktion: DelayDirDelete
 * Aufgabe.: markiert ein verzeichnis zum loeschen beim naechsten systemstart
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int DelayDirDelete(char* Dir)               /* verzeichnis-name             */
{
  if(IS_WINNT) MoveFileEx(Dir, 0, MOVEFILE_DELAY_UNTIL_REBOOT);
  else return 0; /* unter 95 nicht moeglich */

  return 1;
}

/* ------------------------------------------------------------------------
 * $Func CenterWindow
 * Funktion: CenterWindow
 * Aufgabe.: zentriert ein fenster relativ zu einem anderen.
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int CenterWindow(HWND hwndChild,            /* zu zentrierendes fenster     */
                 HWND hwndParent            /* relatives fenster            */
                 )
{

  int xNew, yNew;
  int wChild, hChild, wParent, hParent;
  RECT rChild, rParent, rWorkArea;

  if(!GetWindowRect(hwndChild,&rChild)) return 0;
  wChild = rChild.right - rChild.left;
  hChild = rChild.bottom - rChild.top;

  if(!GetWindowRect(hwndParent,&rParent))
    GetWindowRect(GetDesktopWindow(),&rParent);
  wParent = rParent.right - rParent.left;
  hParent = rParent.bottom - rParent.top;

  if(!SystemParametersInfo(SPI_GETWORKAREA,
    sizeof(rWorkArea), &rWorkArea, 0))
  {
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

  return SetWindowPos(hwndChild,0,xNew,yNew,0,0,SWP_NOSIZE|SWP_NOZORDER);

}


/* ------------------------------------------------------------------------
 * $Func EnumResLangProc
 * Funktion: EnumResLangProc
 * Aufgabe.: callback-funktion fuer GetResourceLanguages
 * Resultat: immer TRUE
 * Hinweis.:
 * ------------------------------------------------------------------------ */
BOOL EnumResLangProc(HANDLE hModule,        /* resource-module handle       */
                     LPCTSTR lpszType,      /* zeiger auf resource-typ      */
                     LPCTSTR lpszName,      /* zeiger auf resource-name     */
                     WORD wIDLanguage,      /* microsoft language-ID        */
                     LONG lParam            /* von EnumResourceLanguages    */
                     )
{

  (*((LANGID**)lParam))[0] = wIDLanguage;
  (*((LANGID**)lParam))[1] = 0;
  (*((LANGID**)lParam))++;
  return 1;

}

/* ------------------------------------------------------------------------
 * $Func GetResourceLanguages
 * Funktion: GetResourceLanguages
 * Aufgabe.: liefert alle sprachen, die in den resourcen verfuegbar sind
 * Resultat: vektor von LANGID's, das ende wird durch 0 gekennzeichnet
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LANGID* GetResourceLanguages(char* res_id   /* resource-name oder -id       */
                            )
{

  static LANGID lang[256];
  LANGID* pLang = lang;

  lang[0] = 0;
  EnumResourceLanguages(0, RT_DIALOG, res_id,
                        (ENUMRESLANGPROC)EnumResLangProc, (LONG)&pLang);
  return lang;

}

/* ------------------------------------------------------------------------
 * $Func GetSystemLanguage
 * Funktion: GetSystemLanguage
 * Aufgabe.: liefert die sprache der installierten windows-version
 * Resultat: LANGID: microsoft-language-identifier
 * Hinweis.: die sprache hat nichts mit dem land zu tun, das als 'current
 *           locale' im control-panel eingestellt ist! es ist lediglich die
 *           sprache der windows-installation.
 * ------------------------------------------------------------------------ */
LANGID GetSystemLanguage()
{
  HKEY hKey;
  LANGID iLangID;
  char cLocale[16];
  HINSTANCE hKrnlDll;
  int iResult, iType, iSize;
  LANGID (APIENTRY *GetUserDefaultUILanguage)(void);

  iLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

  if(IS_WIN2K)
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
  else if(IS_WINNT)
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

/* ------------------------------------------------------------------------
 * $Func GetDefaultLanguage
 * Funktion: GetDefaultLanguage
 * Aufgabe.: liefert einen default-wert fuer die sprache des programms
 * Resultat: LANGID: microsoft language-identifier
 * Hinweis.: der default-wert wird abhaengig von der sprache der
 *           installierten windows-version und den verfuegbaren sprachen
 *           in den resourcen gewaehlt
 * ------------------------------------------------------------------------ */
LANGID GetDefaultLanguage(char* res_id)     /* resource-name oder -id       */
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

/* ------------------------------------------------------------------------
 * $Func GetMultiString
 * Funktion: GetMultiString
 * Aufgabe.: holt einen string aus mehreren aufeinanderfolgenden strings
 * Resultat: zeiger auf den gesuchten string
 * Hinweis.:
 * ------------------------------------------------------------------------ */
char* GetMultiString(const char* c,         /* zeiger auf mehrere strings   */
                     int index)             /* indes des gesuchten strings  */
{
  if(!c) return 0;

  while(index--)
  {
    c += strlen(c) + 1;
    if(!*c) return 0;
  }

  return (char*)c;
}

/* ------------------------------------------------------------------------
 * $Func InsertMultiString
 * Funktion: InsertMultiString
 * Aufgabe.: fuegt einen string in einen multi-string ein
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int InsertMultiString(char* c,              /* zeiger auf mehrere strings   */
                      char* s,              /* einzufuegender string        */
                      int index)            /* index zum einfuegen          */
{
  if(index < 0) index = CountMultiStrings(c);

  while(index--)
  {
    c += strlen(c) + 1;
    if(!*c) break;
  }

  memmove(c+strlen(s)+1, c, GetMultiStringLength(c));
  memcpy(c, s, strlen(s)+1);

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func RemoveMultiString
 * Funktion: RemoveMultiString
 * Aufgabe.: loescht einen string aus einen multi-string
 * Resultat: TRUE wenn erfolgreich
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int RemoveMultiString(char* c,              /* zeiger auf mehrere strings   */
                      int index)            /* index zum loeschen           */
{
  char* p;

  c = GetMultiString(c, index);
  if(!c) return 0;

  p = c + strlen(c) + 1;
  memmove(c, p, GetMultiStringLength(p));

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func CountMultiStrings
 * Funktion: CountMultiStrings
 * Aufgabe.: zaehlt mehrere aufeinaderfolgende strings
 * Resultat: anzahl strings
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int CountMultiStrings(const char* c)
{
  int count = 0;

  while(GetMultiString(c,count)) count++;

  return count;
}


/* ------------------------------------------------------------------------
 * $Func FindMultiString
 * Funktion: FindMultiString
 * Aufgabe.: sucht in einem multi-string nach einem bestimmten string
 * Resultat: index wenn gefunden, sonst -1
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int FindMultiString(const char* c,          /* zeiger auf multi-string      */
                    const char* s           /* zu suchender string          */
                    )
{
  const char* p;
  int index = 0;

  while(p = GetMultiString(c,index))
    if(!strcmp(p,s)) return index;
    else index++;

  return -1;
}


/* ------------------------------------------------------------------------
 * $Func GetMultiStringLength
 * Funktion: GetMultiStringLength
 * Aufgabe.: ermittelt die gesammt-laenge eines multi-string
 * Resultat: gesamtlaenge aller strings mit allen null-terminatoren
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int GetMultiStringLength(const char* c)     /* zeiger auf multi-string      */
{
  const char* p = c;

  if(!*p) p++;
  else while(*p) p += strlen(p)+1;

  return (p-c)+1;
}


/* ------------------------------------------------------------------------
 * $Func GetComCtlVersion
 * Funktion: GetComCtlVersion
 * Aufgabe.: version von comctl32.dll abfragen
 * Resultat: keins
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int GetComCtlVersion(DWORD* pMajor, DWORD* pMinor)
{
  HINSTANCE hComCtl;
  DWORD VerInf[5];
  FARPROC DllGetVersion;

  memset(&VerInf, 0, sizeof(VerInf));
  VerInf[0] = sizeof(VerInf);

  hComCtl = LoadLibrary("comctl32.dll");

  if(!hComCtl) return 0;

  DllGetVersion = GetProcAddress(hComCtl, "DllGetVersion");

  if(!DllGetVersion || DllGetVersion(&VerInf))
    *pMajor = 4, *pMinor = 0;
  else
    *pMajor = VerInf[1], *pMinor = VerInf[2];

  FreeLibrary(hComCtl);

  return 1;
}

/* ------------------------------------------------------------------------
 * $Func ErrorMessageBox
 * Funktion: ErrorMessageBox
 * Aufgabe.: zeigt eine message-box mit beschreibung eines api-errors an
 * ------------------------------------------------------------------------ */
int ErrorMessageBox(HWND hwndOwner,         /* owner-window                 */
                    char* pText,            /* fehler-text                  */
                    int iErrCode)           /* von GetLastError()           */
{
  char* pMsg;
  char cText[256];

  if(iErrCode)
  {
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
      FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
      0, iErrCode, AppLang, (char*)&pMsg, 0, 0);
    sprintf(cText, "%s\r\n\r\n%s", pText, pMsg);
    LocalFree(pMsg);
  }
  else strcpy(cText, pText);

  MessageBox(hwndOwner, cText, 0, MB_OK|MB_ICONERROR);

  return 0;
}

/* ------------------------------------------------------------------------
 * @Function: MakeWizardBmp
 * @Info    : Erzeugt Watermark/Header-Bitmap fuer den Wizard97
 * @Result  : handle des neuen Bitmaps
 * ------------------------------------------------------------------------ */
HBITMAP MakeWizardBmp(HWND hwnd,            /* irgendein fenster-handle     */
                     HBITMAP hBmpOrg,       /* original-wizard-bitmap       */
                     int iSizeX,            /* hoehe des neuen bitmaps      */
                     int iSizeY,            /* breite des neuen bitmaps     */
                     int iOffsX,            /* position des wizard-bitmaps  */
                     int iOffsY)            /* position des wizard-bitmaps  */
{
  RECT sRcHlp;
  BITMAP sBmp;
  HBITMAP hBmpNew;
  HGDIOBJ hOldObj1, hOldObj2;
  HDC hDcWnd, hDcTmp1, hDcTmp2;
  
  GetObject(hBmpOrg, sizeof(sBmp), &sBmp);  /* groesse des original-bitmaps */
  
  hDcWnd = GetDC(hwnd);                     /* fenster-dc holen             */
  hDcTmp1 = CreateCompatibleDC(hDcWnd);     /* zwei kompatible temporaer-   */
  hDcTmp2 = CreateCompatibleDC(hDcWnd);     /* dc's erzeugen                */
  hBmpNew = CreateCompatibleBitmap(hDcWnd,  /* neues bitmap in der          */
    iSizeX, iSizeY);                        /* gefordeten groesse erzeugen  */
    
  hOldObj1 = SelectObject(hDcTmp1,hBmpNew); /* quell- und ziel-bitmap in    */
  hOldObj2 = SelectObject(hDcTmp2,hBmpOrg); /* die temp-dc's selektieren    */
  
  sRcHlp.left = sRcHlp.top = 0;             /* rect-struktur mit kompleter  */
  sRcHlp.right = iSizeX;                    /* groesse des neuen bitmaps    */
  sRcHlp.bottom = iSizeY;                   /* initialisieren               */
  
  FillRect(hDcTmp1, &sRcHlp, (HBRUSH)(COLOR_WINDOW+1)); /* rechteck fuellen */
  BitBlt(hDcTmp1, iOffsX, iOffsY, sBmp.bmWidth, /* org-bmp an gewuenschte   */
    sBmp.bmHeight, hDcTmp2, 0, 0, SRCCOPY);     /* position blitten         */
  
  SelectObject(hDcTmp1, hOldObj1);          /* temporaer-dc's saubermachen  */
  SelectObject(hDcTmp2, hOldObj2);
  
  DeleteDC(hDcTmp1);                        /* temporaer-dc's loeschen      */
  DeleteDC(hDcTmp2);
  
  ReleaseDC(hwnd, hDcWnd);                  /* fenster-dc freigeben         */
  
  return hBmpNew;
}

/* ------------------------------------------------------------------------
 * @Function: InitWizardData
 * @Info    : Laedt diverse Resourcen (Icons, Fonts, Texte) fuer den Wizard
 * @Result  : TRUE wenn erfolgreich
 * ------------------------------------------------------------------------ */
int InitWizardData(HINSTANCE hInstance, tWizardData* pData)
{
  HDC hDc;
  LOGFONT sFont;
  int iLogPixelsY;

  hDc = GetDC(0);
  iLogPixelsY = GetDeviceCaps(hDc, LOGPIXELSY);
  ReleaseDC(0, hDc);  
  
  memset(&sFont, 0, sizeof(sFont));
  strcpy(sFont.lfFaceName, "MS Shell Dlg"); 
  sFont.lfWeight = FW_BOLD; 
  
  sFont.lfHeight = 0 - MulDiv(12, iLogPixelsY, 72);
  pData->hWelcomeTitleFont = CreateFontIndirect(&sFont);
  sFont.lfHeight = 0 - MulDiv(8, iLogPixelsY, 72);
  pData->hHeaderTitleFont = CreateFontIndirect(&sFont);
 
  pData->hImgLst = ImageList_LoadBitmap(hInstance,
    MAKEINTRESOURCE(ID_BMPLIST), 16, 0, 0x00808000);

  pData->hSmInfoIcon = ImageList_GetIcon(pData->hImgLst, 70, ILD_NORMAL);
  pData->hSmAlertIcon = ImageList_GetIcon(pData->hImgLst, 69, ILD_NORMAL);

  LoadString(hInstance, STR_INSTALLTITLE,
    pData->cWelcomeTitle, sizeof(pData->cWelcomeTitle));
  LoadString(hInstance, STR_INSTALLTEXT,
    pData->cWelcomeText, sizeof(pData->cWelcomeText));

  LoadString(hInstance, STR_WIZ97_P2TITLE,
    pData->cPage2HdrTitle, sizeof(pData->cPage2HdrTitle));
  LoadString(hInstance, STR_WIZ97_P2SUBTITLE,
    pData->cPage2HdrSubTitle, sizeof(pData->cPage2HdrSubTitle));

  LoadString(hInstance, STR_WIZ97_P3TITLE,
    pData->cPage3HdrTitle, sizeof(pData->cPage3HdrTitle));
  LoadString(hInstance, STR_WIZ97_P3SUBTITLE,
    pData->cPage3HdrSubTitle, sizeof(pData->cPage3HdrSubTitle));

  LoadString(hInstance, STR_WIZ97_P4TITLE,
    pData->cPage4HdrTitle, sizeof(pData->cPage4HdrTitle));
  LoadString(hInstance, STR_WIZ97_P4SUBTITLE,
    pData->cPage4HdrSubTitle, sizeof(pData->cPage4HdrSubTitle));

  return 1;
}

/* ------------------------------------------------------------------------
 * @Function: FreeWizardData
 * @Info    : Gibt von InitWizardData() geladene Resourcen wieder frei
 * @Result  : TRUE wenn erfolgreich
 * ------------------------------------------------------------------------ */
int FreeWizardData(tWizardData* pData)
{
  DestroyIcon(pData->hSmInfoIcon);
  DestroyIcon(pData->hSmAlertIcon);
  
  ImageList_Destroy(pData->hImgLst);
  
  DeleteObject(pData->hWelcomeTitleFont);
  DeleteObject(pData->hHeaderTitleFont);
  
  return 1;
}

/* ------------------------------------------------------------------------
 * @Function: LoadDlgTemplate
 * @Info    : Laedt eine Dialog-Resource und kopiert sie in den Speicher
 * @Result  : Zeiger auf DLGTEMPLATE
 * ------------------------------------------------------------------------ */
DLGTEMPLATE* LoadDlgTemplate(HINSTANCE hInstance, char* pszTemplate)
{
  HRSRC hRes;
  UINT iResLen;
  char* pResData;
  DLGTEMPLATE* pDlgTemplate;

  hRes = FindResource(hInstance, pszTemplate, RT_DIALOG);
  if(!hRes) return 0;
  
  iResLen = SizeofResource(hInstance, hRes);
  pResData = (char*)LockResource(LoadResource(hInstance,hRes));
  
  if(!pResData || !iResLen)
    return 0;
    
  pDlgTemplate = (DLGTEMPLATE*)malloc(iResLen);
  memcpy(pDlgTemplate, pResData, iResLen);
  
  return pDlgTemplate; 
}

/* ------------------------------------------------------------------------
 * @Function: FreeDlgTemplate
 * @Info    : Gibt den von LoadDlgTemplate() allokierten Speicher frei
 * @Result  : keins
 * ------------------------------------------------------------------------ */
void FreeDlgTemplate(DLGTEMPLATE* pDlgTemplate)
{
  free(pDlgTemplate);
}

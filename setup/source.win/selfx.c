
#include <windows.h>
#include <commctrl.h>
#include <objbase.h>
#include <shlobj.h>
#include <objidl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dos.h>
#include <sys/stat.h>
#include "fdi.h"

#include "selfx.h"

/* ------------------------------------------------------------------------
 * Defines
 * ------------------------------------------------------------------------ */
#define IS_WINNT  (BOOL)(GetVersion() < 0x80000000)
#define IS_WIN2K  (BOOL)((IS_WINNT) && (LOBYTE(LOWORD(GetVersion()))>4))
#define IS_WIN32S (BOOL)(!(IS_WINNT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95  (BOOL)(!(IS_WINNT) && !(IS_WIN32S))

/* ------------------------------------------------------------------------
 * Globale Variablen
 * ------------------------------------------------------------------------ */

static HWND hMainWnd;
static HANDLE hAppInst;
static LANGID AppLang = 0;
static int Stop = 0;
static int ThreadID = 0;
static int Options = 0;
static char TargetDir[MAX_PATH];
static char TempCabinet[MAX_PATH];
static char CommandLine[MAX_PATH];

#define OPT_EXEC_CMDLINE  0x0001
#define OPT_CLEANUP_FILES 0x0002
#define OPT_AUTO_TEMPDIR  0x0004



/* ------------------------------------------------------------------------
 * Prototypen
 * ------------------------------------------------------------------------ */

FNALLOC(mem_alloc);
FNFREE(mem_free);
FNOPEN(file_open);
FNREAD(file_read);
FNWRITE(file_write);
FNCLOSE(file_close);
FNSEEK(file_seek);
FNFDINOTIFY(notification_function);

int QueryTargetDir(HWND hwndOwner, char* Path);
int ExecuteCommand(char* Command, char* WorkDir, int Wait);
LANGID GetDefaultLanguage(char* ResourceID);
int ErrorMessageBox(HWND hwndOwner, int ErrCode);

FNALLOC(mem_alloc)
{
  return malloc(cb);
}

FNFREE(mem_free)
{
  free(pv);
}

FNOPEN(file_open)
{
  return _open(pszFile, oflag, pmode);
}

FNREAD(file_read)
{
  return _read(hf, pv, cb);
}

FNWRITE(file_write)
{
  return _write(hf, pv, cb);
}

FNCLOSE(file_close)
{
  return _close(hf);
}

FNSEEK(file_seek)
{
  return _lseek(hf, dist, seektype);
}

FNFDINOTIFY(notification_function)
{
  switch (fdint)
  {
    case fdintCABINET_INFO: // general information about the cabinet
      return 0;

    case fdintPARTIAL_FILE: // first file in cabinet is continuation
      return 0;

    case fdintCOPY_FILE:    // file to be copied
    {
      char* c, Dest[256];

      if(Stop) return -1;
      SendMessage(hMainWnd, WM_USER+1, 2, (LPARAM)pfdin->psz1);
      
      sprintf(Dest, "%s\\%s", TargetDir, pfdin->psz1);
      c = Dest + strlen(TargetDir);
      while(c = strchr(c+1,'\\'))
        *c = 0, CreateDirectory(Dest, 0), *c = '\\';
      return file_open(Dest,
        _O_BINARY|_O_CREAT|_O_WRONLY|_O_SEQUENTIAL,_S_IREAD|_S_IWRITE);
    }

    case fdintCLOSE_FILE_INFO:  // close the file, set relevant info
    {
      HANDLE hFile;
      char Dest[256];
      FILETIME fTime, fLocTime;

      sprintf(Dest, "%s\\%s", TargetDir, pfdin->psz1);
      file_close(pfdin->hf);

      hFile = CreateFile(Dest, GENERIC_READ|GENERIC_WRITE,
          FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

      if(hFile != INVALID_HANDLE_VALUE)
      {
        if(DosDateTimeToFileTime(pfdin->date,pfdin->time,&fTime)
          && LocalFileTimeToFileTime(&fTime,&fLocTime))
          SetFileTime(hFile, &fLocTime, 0, &fLocTime);
        CloseHandle(hFile);
      }

      SetFileAttributes(Dest, 
        pfdin->attribs & (_A_RDONLY|_A_HIDDEN|_A_SYSTEM|_A_ARCH));

      return 1;
    }

    case fdintNEXT_CABINET: // file continued to next cabinet
      return 0;
  }

  return 0;
}


/* ------------------------------------------------------------------------
 * $Func ExtractResources
 * Funktion: ExtractResources
 * Aufgabe.: extrahiert CAB Datei a.d. Resourcen in die Datei TempCabinet
 * Resultat: True or False
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int ExtractResources(void)
{
  FILE *f;
  UINT len = 0;
  HRSRC hRes = 0;
  char *data = 0;

  hRes = FindResource(GetModuleHandle(0), "CABINET", RT_RCDATA);
  if(!hRes) return 0;

  len = SizeofResource(GetModuleHandle(0), hRes);
  data = (char *)LockResource(LoadResource(GetModuleHandle(0), hRes));
  if(!data || !len) return 0;

  f = fopen(TempCabinet,"wb");
  if(!f) return 0;

  if(len != fwrite(data,1,len,f))
  {
    fclose(f);
    return 0;
  }

  fclose(f);

  return 1;
}


/* ------------------------------------------------------------------------
 * $Func Thread
 * Funktion: Thread
 * Aufgabe.: fuehrt das komplette Entpacken der CAB aus
 * Resultat:
 * Hinweis.:
 * ------------------------------------------------------------------------ */
DWORD WINAPI Thread(void* pr)
{

  HFDI      hfdi;
  ERF       erf;
  FDICABINETINFO  fdici;
  int       hf;
  char      *p=0;
  char      cabinet_name[256];
  char      cabinet_path[256];
  char      cabinet_fullpath[256];
  HWND hwnd = (HWND)pr;

  AppLang = GetDefaultLanguage(MAKEINTRESOURCE(IDD_MAIN));
  if(IS_WINNT) SetThreadLocale(MAKELCID(AppLang,SORT_DEFAULT));

  strcpy(cabinet_fullpath, TempCabinet);

  if(!ExtractResources())
  {
    PostMessage(hwnd, WM_USER+1, 0, (LPARAM)1);
    return 0;
  }

  hfdi = FDICreate(mem_alloc, mem_free, file_open,
                   file_read, file_write, file_close,
                   file_seek, cpu80386, &erf);

  if(hfdi == NULL)
  {
    PostMessage(hwnd, WM_USER+1, 0, (LPARAM)1);
    return 0;
  }

  hf = file_open(cabinet_fullpath, _O_BINARY | _O_RDONLY | _O_SEQUENTIAL, 0);
  if(hf == -1)
  {
    FDIDestroy(hfdi);
    PostMessage(hwnd, WM_USER+1, 0, FDIERROR_CABINET_NOT_FOUND);
    return 0;
  }

  if(!FDIIsCabinet(hfdi,hf,&fdici))
  {
    _close(hf);
    FDIDestroy(hfdi);
    PostMessage(hwnd, WM_USER+1, 0, FDIERROR_NOT_A_CABINET);
    return 0;
  }
  else
  {
    _close(hf);
    PostMessage(hwnd, WM_USER+1, 1, (LPARAM)fdici.cFiles);
  }

  if(!(p = strrchr(cabinet_fullpath,'\\')))
  {
    strcpy(cabinet_name, cabinet_fullpath);
    strcpy(cabinet_path, "");
  }
  else
  {
    strcpy(cabinet_name, p+1);
    strncpy(cabinet_path, cabinet_fullpath, (int) (p-cabinet_fullpath)+1);
    cabinet_path[ (int) (p-cabinet_fullpath)+1 ] = 0;
  }

  if(!FDICopy(hfdi,cabinet_name,cabinet_path,0,notification_function,0,0))
  {
    FDIDestroy(hfdi);
    PostMessage(hwnd, WM_USER+1, 0, (LPARAM)1);
    return 0;
  }

  if(FDIDestroy(hfdi) != TRUE)
  {
    PostMessage(hwnd, WM_USER+1, 0, (LPARAM)1);
    return 0;
  }

  PostMessage(hwnd, WM_USER+1, 0, 0);

  return 0;
}


/* ------------------------------------------------------------------------
 * $Func QueryPathProc
 * Funktion: QueryPathProc
 * Aufgabe.: Fensterprozedur des Verzeichnisauswahl-Dialogs
 * Resultat:
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK QueryPathProc(HWND hwnd,  /* fensterhandle                */
                            UINT message,  /* message                      */
                            WPARAM wParam, /* erster message-parameter     */
                            LPARAM lParam  /* zweiter message-parameter    */
                            )
{
  switch(message)
  {
    case WM_INITDIALOG:
    {
      char Temp[MAX_PATH];

      if(GetTempPath(sizeof(Temp),Temp)) SetDlgItemText(hwnd, IDC_PATH, Temp);

      SetWindowPos(hwnd, 0, GetSystemMetrics(SM_CXICON), 
        GetSystemMetrics(SM_CYICON), 0, 0, SWP_NOSIZE|SWP_NOZORDER);
      SendMessage(hwnd, DM_REPOSITION, 0, 0);

      return 1;
    }

    case WM_COMMAND:

      switch(LOWORD(wParam))
      {
        case IDOK:
          GetDlgItemText(hwnd, IDC_PATH, TargetDir, sizeof(TargetDir));
          EndDialog(hwnd, 1);
          return 1;
        case IDCANCEL:
          EndDialog(hwnd, 0);
          return 1;
        case IDC_BROWSE:
        {
          char Path[256];
          if(QueryTargetDir(hwnd,Path)) SetDlgItemText(hwnd, IDC_PATH, Path);
          return 0;
        }
      }
      return 1;
  }

  return 0;
}


/* ------------------------------------------------------------------------
 * $Func MainProc
 * Funktion: MainProc
 * Aufgabe.: Fensterprozedur des Dialogs
 * Resultat:
 * Hinweis.:
 * ------------------------------------------------------------------------ */
LRESULT CALLBACK MainProc(HWND hwnd,       /* fensterhandle                */
                          UINT message,    /* message                      */
                          WPARAM wParam,   /* erster message-parameter     */
                          LPARAM lParam    /* zweiter message-parameter    */
                          )
{
  switch(message)
  {
    case WM_INITDIALOG:

      hMainWnd = hwnd;

      SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETSTEP, (WPARAM)1, 0);

      Animate_Open(GetDlgItem(hwnd,IDC_AVI), MAKEINTRESOURCE(IDR_AVI));
      Animate_Play(GetDlgItem(hwnd,IDC_AVI), 0, -1, -1);

      SetWindowPos(hwnd, 0, GetSystemMetrics(SM_CXICON), 
        GetSystemMetrics(SM_CYICON), 0, 0, SWP_NOSIZE|SWP_NOZORDER);
      SendMessage(hwnd, DM_REPOSITION, 0, 0);

      CreateThread(0, 0, Thread, (void*)hwnd, 0, &ThreadID);

      return 1;

    case WM_USER+1:
    {
      char buff[256];
      if(!wParam && lParam && !Stop)
      {
        LoadString(hAppInst, STR_ERROR, buff, sizeof(buff));
        MessageBox(hwnd, buff, 0, MB_OK|MB_ICONERROR);
      }
      switch(wParam)
      {
        case 1:
          SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, lParam));
          break;
        case 2:
        {
          char Text[256], Mask[256];
          LoadString(hAppInst, STR_PROGRESS_ACTFILE, Mask, sizeof(Mask));
          sprintf(Text, Mask, (char*)lParam);
          SetDlgItemText(hwnd, IDC_TEXT, Text);
          SendDlgItemMessage(hwnd, IDC_PROGRESS, PBM_STEPIT, 0, 0);
          break;
        }
        default:
          EndDialog(hwnd, !lParam);
          break;
      }
      return 1;
    }

    case WM_COMMAND:

      if(LOWORD(wParam) != IDCANCEL) break;

    case WM_CLOSE:

      Stop = 1;
      return 1;
  }

  return 0;
}


/* ------------------------------------------------------------------------
 * $Func EmptyDir
 * Funktion: EmptyDir
 * Aufgabe.: Löscht alle Datein und Unterverzeichnisse
 * Resultat:
 * Hinweis.:
 * ------------------------------------------------------------------------ */
void EmptyDir(char* Path)
{
  char cProgress[256];
  SHFILEOPSTRUCT sFileOp;

  LoadString(hAppInst,STR_DELETING_FILES,cProgress,sizeof(cProgress));

  memset(&sFileOp, 0, sizeof(sFileOp));
  sFileOp.hwnd = GetDesktopWindow();
  sFileOp.wFunc = FO_DELETE;
  sFileOp.pFrom = Path;
  sFileOp.fFlags = FOF_NOCONFIRMATION|FOF_SIMPLEPROGRESS;
  sFileOp.lpszProgressTitle = cProgress;
          
  SHFileOperation(&sFileOp);                /* und ab ins nirwana ...       */

  return;
}

/* ------------------------------------------------------------------------
 * $Func WinMain
 * Funktion: WinMain
 * Aufgabe.:
 * Resultat: True or False
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int APIENTRY WinMain(HINSTANCE hInstance,   /* instance-handle              */
                     HINSTANCE hPrevInst,   /* obsolet unter win32          */
                     LPSTR     pCmdLine,    /* zeiger auf kommandozeile     */
                     int       nCmdShow)    /* show-befehl                  */
{
  char* c, Buff[256];

  InitCommonControls();
  hAppInst = hInstance;
  AppLang = GetDefaultLanguage(MAKEINTRESOURCE(IDD_MAIN));
  if(IS_WINNT) SetThreadLocale(MAKELCID(AppLang,SORT_DEFAULT));
  
  if(LoadString(hAppInst,STR_TARGET_OS,Buff,sizeof(Buff)))
  {
    if((IS_WIN95 && !stricmp(Buff,"WinNT")) || (IS_WINNT && !stricmp(Buff,"Win95")))
    {
      LoadString(hAppInst, STR_WRONG_OS, Buff, sizeof(Buff));
      MessageBox(GetDesktopWindow(), Buff, 0, MB_OK|MB_ICONERROR);
      return 0;
    }    
  }

  if(LoadString(hAppInst,STR_COMMANDLINE,CommandLine,sizeof(CommandLine)))
  {
    Options = OPT_EXEC_CMDLINE | OPT_AUTO_TEMPDIR | OPT_CLEANUP_FILES;
  }

  if(LoadString(hAppInst,STR_OPTIONS,Buff,sizeof(Buff)))
  {
    if(strchr(Buff,'k') || strchr(Buff,'K'))
      Options &= ~OPT_CLEANUP_FILES;
    if(strchr(Buff,'q') || strchr(Buff,'Q'))
      Options &= ~(OPT_AUTO_TEMPDIR|OPT_CLEANUP_FILES);
  }

  if(!GetTempPath(sizeof(Buff),Buff))
    return ErrorMessageBox(0,GetLastError());

  if(Options & OPT_AUTO_TEMPDIR)
  {
    if(!GetTempFileName(Buff,"tmp",0,TempCabinet))
      return ErrorMessageBox(0,GetLastError());
    if(!GetTempFileName(Buff,"tmp",0,TargetDir))
      return ErrorMessageBox(0,GetLastError());
    DeleteFile(TargetDir);
    if(!CreateDirectory(TargetDir,0))
      return ErrorMessageBox(0,GetLastError());
  }
  else
  {
    if(DialogBox(hAppInst,MAKEINTRESOURCE(IDD_TARGET),
      GetDesktopWindow(),(DLGPROC)QueryPathProc) != IDOK) return 0;
    c = TargetDir;
    while(c = strchr(c+1,'\\'))
      *c = 0, CreateDirectory(TargetDir,0), *c = '\\';
    CreateDirectory(TargetDir,0);
    GetTempFileName(Buff, "tmp", 0, TempCabinet);
  }

  if(DialogBox(hAppInst,MAKEINTRESOURCE(IDD_MAIN),GetDesktopWindow(),
    (DLGPROC)MainProc) && Options & OPT_EXEC_CMDLINE)
  {
    GetCurrentDirectory(sizeof(Buff), Buff);
    SetCurrentDirectory(TargetDir);
    ExecuteCommand(CommandLine, TargetDir, Options & OPT_CLEANUP_FILES);
    SetCurrentDirectory(Buff);
  }  

  DeleteFile(TempCabinet);
  if(Options & OPT_CLEANUP_FILES)
  {
    EmptyDir(TargetDir);
    if(Options & OPT_AUTO_TEMPDIR) RemoveDirectory(TargetDir);
  }

  return 0;
}

/* ------------------------------------------------------------------------
 * $Func QueryTargetDir
 * Funktion: QueryTargetDir
 * Aufgabe.: fragt den benutzer nach zielverzeichnis
 * Resultat: 1 wenn OK, sonst 0
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int QueryTargetDir(HWND hwndOwner,          /* owner-window                 */
                   char* Path)              /* puffer fuer pfad             */
{
  int rc = 0;
  LPMALLOC pIMalloc;
  BROWSEINFO BrowseInfo;
  ITEMIDLIST* pItemIdList;
  char buff[MAX_PATH], buff2[MAX_PATH];

  if(SHGetMalloc(&pIMalloc) != NOERROR) return 0;

  LoadString(hAppInst, STR_BROWSETITLE, buff, sizeof(buff));

  BrowseInfo.hwndOwner = hwndOwner;
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
  }
  pIMalloc->lpVtbl->Release(pIMalloc);

  return !!rc;
}


/* ------------------------------------------------------------------------
 * $Func ExecuteCommand
 * Funktion: ExecuteCommand
 * Aufgabe.: fuehrt einen externen befehl durch
 * Resultat: haengt von der notifcation ab
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int ExecuteCommand(char* Command,           /* befehl                       */
                   char* WorkDir,
                   int Wait)
{
  char* pFile, *pParms = 0;
  SHELLEXECUTEINFO execInfo;

  pFile = Command;
  if(strchr(pFile,' ')) pParms = strchr(pFile,' '), *pParms++ = 0;

  execInfo.cbSize = sizeof(execInfo);
  execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  execInfo.hwnd = GetDesktopWindow();
  execInfo.lpVerb = 0;
  execInfo.lpFile = pFile;
  execInfo.lpParameters = pParms;
  execInfo.lpDirectory = WorkDir;
  execInfo.nShow = SW_SHOWNORMAL;

  if(!ShellExecuteEx(&execInfo)) return 0;
  if(!execInfo.hProcess) return 0;

  if(Wait) WaitForSingleObject(execInfo.hProcess, INFINITE);

  return 1;
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
 * $Func ErrorMessageBox
 * Funktion: ErrorMessageBox
 * Aufgabe.: zeigt eine message-box mit beschreibung eines api-errors an
 * Resultat: 0
 * Hinweis.:
 * ------------------------------------------------------------------------ */
int ErrorMessageBox(HWND hwndOwner,         /* owner-window                 */
                    int ErrCode)            /* von GetLastError()           */
{
  char* pMsg;

  if(!hwndOwner) hwndOwner = GetDesktopWindow();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
    FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
    0, ErrCode, AppLang, (char*)&pMsg, 0, 0);
  MessageBox(hwndOwner, pMsg, 0, MB_OK|MB_ICONERROR);
  LocalFree(pMsg);

  return 0;
}

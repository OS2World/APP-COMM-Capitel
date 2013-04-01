#define INCL_DOS
#define INCL_PM

#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "capitel.h"
#include "cntr.h"
#include "..\..\common\source\version.h"
#include "..\..\common\source\global.h"
#include "texte.h"
//#include "..\..\common\source\texte.h"
#include "config.h"
#include "configrc.h"
#include "..\..\answer\source\answer.h"
#include "..\..\..\units\common.src\cfg_file.h"

void centerDialog(HWND hwndFrame, HWND hWndDlg);
MRESULT EXPENTRY page1Proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY page2Proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY page3Proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY page4Proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

MRESULT EXPENTRY editPortProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY addPortProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY editPeopleProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY addPeopleProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY editDtmfProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY addDtmfProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

SHORT EXPENTRY sortPorts( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID  tmp );
SHORT EXPENTRY sortPeople( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID  tmp );
SHORT EXPENTRY sortDtmf( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID  tmp );

void InitNotebook( LONG, LONG );

void InitEAZCntr( HWND );
void FillEAZCntr( HWND );
void AddItemToEAZCntr( HWND, HPOINTER, char *, char *, char *, char *, char *, char *, char *, char * );
void SaveEAZSetup( HWND );
void CheckEAZSetup( EAZSetup * );

void InitPeopleCntr( HWND );
void FillPeopleCntr( HWND );
void AddItemToPeopleCntr( HWND, HPOINTER, char *, char *, char *, char *,
                          char *, char *, char *, char * );
void SavePeopleSetup( HWND );
void CheckPeopleSetup( PeopleSetup * );

void InitDtmfCntr( HWND );
void FillDtmfCntr( HWND );
void AddItemToDtmfCntr( HWND hwnd, HPOINTER bmp, char *code, char *action,
                        char *params, char *title, char *type, char *active );
void SaveDtmfSetup( HWND );
void CheckDtmfSetup( DtmfSetup * );

char *strip_blanks( char * );

void MsgBox( char * );

extern void delete_cr(char *);
extern CntrRec *currRec;
extern HWND hwndCntr;

ULONG ulPage1, ulPage2, ulPage3, ulPage4;

SWP oldSWP;

SIZEL iconSize = { 16L, 16L };

HWND hwndNotebook, hwndPage1, hwndPage2, hwndPage3, hwndPage4;
HWND hwndPeoplePopup, hwndPortPopup, hwndDtmfPopup;

PeopleSetup *currPerson;
EAZSetup *currPort;
DtmfSetup *currDtmf;

extern HPOINTER portsIcoActive;
extern HPOINTER portsIcoInactive;
extern HPOINTER callersIcoActive;
extern HPOINTER callersIcoInactive;
extern HPOINTER actionIcoActive;
extern HPOINTER actionIcoInactive;

char* strip_blanks(char* pBuff)
{
  char* pHelp;

  pHelp = pBuff;
  while(*pHelp && isspace(*pHelp)) pHelp++;
  if(pHelp > pBuff) memmove(pBuff, pHelp, strlen(pHelp) + 1);

  pHelp = pBuff + strlen(pBuff) - 1;
  while(pHelp >= pBuff && isspace(*pHelp)) pHelp--;
  pHelp[1] = 0;

  return pBuff;
}

MRESULT EXPENTRY configProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
  short i, y;
  char str[512];
  LONG lCxChar, lCyChar;
  HPS hpsChar;
  FONTMETRICS fmMetrics;
  ULONG deletePrompt, ignoreEmpty, showDigital, restoreWin;

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      hwndNotebook = WinWindowFromID( hwnd, CFG_NOTEBOOK );

      hwndPage1 = WinLoadDlg( hwnd, hwndNotebook, page1Proc, NULLHANDLE,
                              CFG_NB_PAGE1, NULL );

      hwndPage2 = WinLoadDlg( hwnd, hwndNotebook, page2Proc, NULLHANDLE,
                              CFG_NB_PAGE2, NULL );

      hwndPage3 = WinLoadDlg( hwnd, hwndNotebook, page3Proc, NULLHANDLE,
                              CFG_NB_PAGE3, NULL );

      hwndPage4 = WinLoadDlg( hwnd, hwndNotebook, page4Proc, NULLHANDLE,
                              CFG_NB_PAGE4, NULL );

      hpsChar = WinGetPS( hwnd );
      GpiQueryFontMetrics( hpsChar, sizeof( fmMetrics ), &fmMetrics );
      WinReleasePS( hpsChar );
      lCxChar = fmMetrics.lAveCharWidth;
      lCyChar = fmMetrics.lMaxBaselineExt;

      InitNotebook( lCxChar, lCyChar );

      switch( *((short *) mp2) )
      {
        case IDI_PORTS:
          WinSendMsg( hwndNotebook, BKM_TURNTOPAGE, MPFROMLONG( ulPage2 ),
                      NULL );
          break;
        case IDI_PEOPLE:
          WinSendMsg( hwndNotebook, BKM_TURNTOPAGE, MPFROMLONG( ulPage3 ),
                      NULL );
          break;
        case IDI_DTMF:
          WinSendMsg( hwndNotebook, BKM_TURNTOPAGE, MPFROMLONG( ulPage4 ),
                      NULL );
          break;
        default:
          break;
      }

      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_BUT_CANCEL:
          WinDismissDlg( hwnd, TRUE );
          break;

        case CFG_BUT_SAVE:
          WinQueryDlgItemText( hwndPage1, CFG_NBP1_DELAY, sizeof( str ), str );
          config_file_write_ulong( STD_CFG_FILE, DEFAULT_ANSW_DELAY,
                                   atoi( str ) );

          WinQueryDlgItemText( hwndPage1, CFG_NBP1_MAXRECTIME, sizeof( str ), str );
          config_file_write_ulong( STD_CFG_FILE, DEFAULT_REC_TIME,
                                   atoi( str ) );

          WinQueryDlgItemText( hwndPage1, CFG_NBP1_SILENCE, sizeof( str ), str );
          config_file_write_ulong( STD_CFG_FILE, MAX_SILENCE_TIME,
                                   atoi( str ) );

          config_file_write_ulong( STD_CFG_FILE, SHOW_POPUP_WINDOW,
            ( int )WinSendDlgItemMsg( hwndPage1,
            CFG_NBP1_POPUPWINDOW, LM_QUERYSELECTION, 0, 0 ) );

          WinQueryDlgItemText( hwndPage1, CFG_NBP1_LOGFILE, sizeof( str ), str );
          config_file_write_string( STD_CFG_FILE, NAME_OF_LOG_FILE, str );

          WinQueryDlgItemText( hwndPage1, CFG_NBP1_WELCOME_FILE, sizeof( str ), str );
          config_file_write_string( STD_CFG_FILE, WELCOME_WAVE, str );

          WinQueryDlgItemText( hwndPage1, CFG_NBP1_RINGING_FILE, sizeof( str ), str );
          config_file_write_string( STD_CFG_FILE, RINGRING_WAVE, str );

          WinQueryDlgItemText( hwndPage1, CFG_NBP1_NOTIFY_NUMBER, sizeof( str ), str );
          config_file_write_string( STD_CFG_FILE, CALL_BACK_NUMBER, str );

          i = (short) WinSendMsg( WinWindowFromID( hwndPage1,
                                 CFG_NBP1_RINGING ), BM_QUERYCHECK, NULL, NULL );
          config_file_write_ulong( STD_CFG_FILE, PLAY_RINGRING_WAVE, i );

          i = (short) WinSendMsg( WinWindowFromID( hwndPage1,
                                 CFG_NBP1_44KHZ ), BM_QUERYCHECK, NULL, NULL );
          config_file_write_ulong( STD_CFG_FILE, GENERATE_16_BIT_WAVES, i );

          i = (short) WinSendMsg( WinWindowFromID( hwndPage1,
                                 CFG_NBP1_EXPAND ), BM_QUERYCHECK, NULL, NULL );
          config_file_write_ulong( STD_CFG_FILE, EXPAND_CALLER_ID, i );

          i = (short) WinSendMsg( WinWindowFromID( hwndPage1,
                                 CFG_NBP1_DTMF ), BM_QUERYCHECK, NULL, NULL );
          config_file_write_ulong( STD_CFG_FILE, DETECT_DTMF_TONES, i );

          i = (short) WinSendMsg( WinWindowFromID( hwndPage1,
                                 CFG_NBP1_ULAW_CODEC ), BM_QUERYCHECK, NULL, NULL );
          config_file_write_ulong( STD_CFG_FILE, CAPITEL_CODEC_ULAW, i );

          deletePrompt = (ULONG) WinSendMsg( WinWindowFromID( hwndPage1,
                                 CFG_NBP1_DELPROMPT ), BM_QUERYCHECK, NULL,
                                 NULL );

          ignoreEmpty = (ULONG) WinSendMsg( WinWindowFromID( hwndPage1,
                                CFG_NBP1_IGNOREEMPTY ), BM_QUERYCHECK,
                                NULL, NULL );

          showDigital = (ULONG) WinSendMsg( WinWindowFromID( hwndPage1,
                                CFG_NBP1_SHOWDIGITAL ), BM_QUERYCHECK,
                                NULL, NULL );

          restoreWin = (ULONG) WinSendMsg( WinWindowFromID( hwndPage1,
                               CFG_NBP1_FLASH ), BM_QUERYCHECK,
                               NULL, NULL );

          config_file_write_ulong( STD_CFG_FILE, RESTORE_WINDOW_ON_NEW_CALL,
                                   restoreWin );
          config_file_write_ulong( STD_CFG_FILE, CONFIRM_CALL_DELETE,
                                   deletePrompt );
          config_file_write_ulong( STD_CFG_FILE, IGNORE_EMPTY_CALLS,
                                   ignoreEmpty );
          config_file_write_ulong( STD_CFG_FILE, CAPITEL_IS_CALLER_ID,
                                   showDigital );


          SaveEAZSetup( WinWindowFromID( hwndPage2, CFG_NBP2_CNTR ) );
          SavePeopleSetup( WinWindowFromID( hwndPage3, CFG_NBP3_CNTR ) );
          SaveDtmfSetup( WinWindowFromID( hwndPage4, CFG_NBP4_CNTR ) );

          answer_wav2alw_convert_all();

          WinDismissDlg( hwnd, TRUE );
          break;

        default:
          break;
      }

    default:
      return( WinDefDlgProc( hwnd, msg, mp1, mp2 ) );
  }

  return 0;
}

void InitNotebook( LONG lCxChar, LONG lCyChar )
{
  ULONG ulWidth, version;

  ulWidth = 10 * lCxChar;

  /* Notebook Setup */
  WinSendMsg( hwndNotebook, BKM_SETDIMENSIONS, MPFROM2SHORT( ulWidth,
              lCyChar*2 ), MPFROMSHORT( BKA_MAJORTAB ) );

  WinSendMsg( hwndNotebook, BKM_SETDIMENSIONS, MPFROM2SHORT( 0, 0 ),
              MPFROMSHORT( BKA_MINORTAB ) );

  WinSendMsg( hwndNotebook, BKM_SETNOTEBOOKCOLORS, MPFROMLONG( CLR_BLACK ),
              MPFROMSHORT( BKA_FOREGROUNDMAJORCOLORINDEX ) );

  WinSendMsg( hwndNotebook, BKM_SETNOTEBOOKCOLORS,
              MPFROMLONG( SYSCLR_FIELDBACKGROUND ),
              MPFROMSHORT( BKA_BACKGROUNDMAJORCOLORINDEX ) );

  WinSendMsg( hwndNotebook, BKM_SETNOTEBOOKCOLORS,
              MPFROMLONG( SYSCLR_FIELDBACKGROUND ),
              MPFROMSHORT( BKA_BACKGROUNDPAGECOLORINDEX ) );

  /* Set Font & Size                                                     */
  DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, (PVOID) &version,
                   sizeof(ULONG) );

  if( version == 40 )
    WinSetPresParam( hwndNotebook, PP_FONTNAMESIZE, sizeof( Defv4CntrFont ),
                     Defv4CntrFont );
  else
    WinSetPresParam( hwndNotebook, PP_FONTNAMESIZE, sizeof( DefCntrFont ),
                     DefCntrFont );

  /* Set up Page 1 */
  ulPage1 = LONGFROMMR( WinSendMsg( hwndNotebook, BKM_INSERTPAGE, 0,
                                    MPFROM2SHORT( BKA_MAJOR |
                                    BKA_STATUSTEXTON | BKA_AUTOPAGESIZE,
                                    BKA_LAST ) ) );

  WinSendMsg( hwndNotebook, BKM_SETSTATUSLINETEXT, MPFROMLONG( ulPage1 ),
              MPFROMP( GENERALSET ) );

  WinSendMsg( hwndNotebook, BKM_SETTABTEXT, MPFROMLONG( ulPage1 ),
              MPFROMP( GENERAL ) );

  WinSendMsg( hwndNotebook, BKM_SETPAGEWINDOWHWND, MPFROMLONG( ulPage1 ),
              MPFROMHWND( hwndPage1 ) );

  /* Set up Page 2 */

  ulPage2 = LONGFROMMR( WinSendMsg( hwndNotebook, BKM_INSERTPAGE, 0,
                        MPFROM2SHORT( BKA_MAJOR | BKA_STATUSTEXTON |
                        BKA_AUTOPAGESIZE, BKA_LAST ) ) );

  WinSendMsg( hwndNotebook, BKM_SETSTATUSLINETEXT, MPFROMLONG( ulPage2 ),
              MPFROMP( PORTSSET ) );

  WinSendMsg( hwndNotebook, BKM_SETTABTEXT, MPFROMLONG( ulPage2 ),
              MPFROMP( PORTS ) );

  WinSendMsg( hwndNotebook, BKM_SETPAGEWINDOWHWND, MPFROMLONG( ulPage2 ),
              MPFROMHWND( hwndPage2 ) );

  /* Set up Page 3 */

  ulPage3 = LONGFROMMR( WinSendMsg( hwndNotebook, BKM_INSERTPAGE, 0,
                        MPFROM2SHORT( BKA_MAJOR | BKA_STATUSTEXTON |
                        BKA_AUTOPAGESIZE, BKA_LAST ) ) );

  WinSendMsg( hwndNotebook, BKM_SETSTATUSLINETEXT, MPFROMLONG( ulPage3 ),
              MPFROMP( CALLERSSET ) );

  WinSendMsg( hwndNotebook, BKM_SETTABTEXT, MPFROMLONG( ulPage3 ),
              MPFROMP( CALLERS ) );

  WinSendMsg( hwndNotebook, BKM_SETPAGEWINDOWHWND, MPFROMLONG( ulPage3 ),
              MPFROMHWND( hwndPage3 ) );

  /* Set up Page 4 */

  ulPage4 = LONGFROMMR( WinSendMsg( hwndNotebook, BKM_INSERTPAGE, 0,
                        MPFROM2SHORT( BKA_MAJOR | BKA_STATUSTEXTON |
                        BKA_AUTOPAGESIZE, BKA_LAST ) ) );

  WinSendMsg( hwndNotebook, BKM_SETSTATUSLINETEXT, MPFROMLONG( ulPage4 ),
              MPFROMP( MAINT_REMOTE_ACT ) );

  WinSendMsg( hwndNotebook, BKM_SETTABTEXT, MPFROMLONG( ulPage4 ),
              MPFROMP( REMOTE_ACT ) );

  WinSendMsg( hwndNotebook, BKM_SETPAGEWINDOWHWND, MPFROMLONG( ulPage4 ),
              MPFROMHWND( hwndPage4 ) );

}

MRESULT EXPENTRY page1Proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char str[512];
  int i;
  FILEDLG fileDlg;

  switch( msg )
  {
    case WM_INITDLG:
      if( config_file_read_ulong( STD_CFG_FILE, PLAY_RINGRING_WAVE, PLAY_RINGRING_WAVE_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_RINGING ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      if( config_file_read_ulong( STD_CFG_FILE, CONFIRM_CALL_DELETE, CONFIRM_CALL_DELETE_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_DELPROMPT ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      if( config_file_read_ulong( STD_CFG_FILE, IGNORE_EMPTY_CALLS, IGNORE_EMPTY_CALLS_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_IGNOREEMPTY ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      if( config_file_read_ulong( STD_CFG_FILE, CAPITEL_IS_CALLER_ID, CAPITEL_IS_CALLER_ID_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_SHOWDIGITAL ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      if( config_file_read_ulong( STD_CFG_FILE, RESTORE_WINDOW_ON_NEW_CALL, RESTORE_WINDOW_ON_NEW_CALL_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_FLASH ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      if( config_file_read_ulong( STD_CFG_FILE, GENERATE_16_BIT_WAVES, GENERATE_16_BIT_WAVES_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_44KHZ ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      if( config_file_read_ulong( STD_CFG_FILE, EXPAND_CALLER_ID, EXPAND_CALLER_ID_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_EXPAND ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      if( config_file_read_ulong( STD_CFG_FILE, DETECT_DTMF_TONES, DETECT_DTMF_TONES_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_DTMF ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      if( config_file_read_ulong( STD_CFG_FILE, CAPITEL_CODEC_ULAW, CAPITEL_CODEC_ULAW_DEF ) )
        WinSendMsg( WinWindowFromID( hwnd, CFG_NBP1_ULAW_CODEC ), BM_SETCHECK,
                    MPFROMSHORT( 1 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_NBP1_DELAY, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_NBP1_MAXRECTIME, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_NBP1_LOGFILE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_NBP1_WELCOME_FILE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_NBP1_RINGING_FILE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      sprintf( str, "%d", config_file_read_ulong( STD_CFG_FILE, DEFAULT_ANSW_DELAY, DEFAULT_ANSW_DELAY_DEF ) );
      WinSetDlgItemText( hwnd, CFG_NBP1_DELAY, (PSZ) str );

      sprintf( str, "%d", config_file_read_ulong( STD_CFG_FILE, DEFAULT_REC_TIME, DEFAULT_REC_TIME_DEF ) );
      WinSetDlgItemText( hwnd, CFG_NBP1_MAXRECTIME, (PSZ) str );

      sprintf( str, "%d", config_file_read_ulong( STD_CFG_FILE, MAX_SILENCE_TIME, MAX_SILENCE_TIME_DEF ) );
      WinSetDlgItemText( hwnd, CFG_NBP1_SILENCE, (PSZ) str );

      WinSendDlgItemMsg( hwnd, CFG_NBP1_POPUPWINDOW,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( POPUP_NONE ) );

      WinSendDlgItemMsg( hwnd, CFG_NBP1_POPUPWINDOW,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( POPUP_TOPLEFT ) );

      WinSendDlgItemMsg( hwnd, CFG_NBP1_POPUPWINDOW,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( POPUP_TOPRIGHT ) );

      WinSendDlgItemMsg( hwnd, CFG_NBP1_POPUPWINDOW,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( POPUP_BOTTOMLEFT ) );

      WinSendDlgItemMsg( hwnd, CFG_NBP1_POPUPWINDOW,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( POPUP_BOTTOMRIGHT ) );

      WinSendDlgItemMsg( hwnd, CFG_NBP1_POPUPWINDOW, LM_SELECTITEM,
        MPFROMLONG(config_file_read_ulong(STD_CFG_FILE,
        SHOW_POPUP_WINDOW,SHOW_POPUP_WINDOW_DEF)), MPFROMLONG(1));

      config_file_read_string( STD_CFG_FILE, NAME_OF_LOG_FILE, str, NAME_OF_LOG_FILE_DEF);
      WinSetDlgItemText( hwnd, CFG_NBP1_LOGFILE, (PSZ) str );

      config_file_read_string( STD_CFG_FILE, WELCOME_WAVE, str, WELCOME_WAVE_DEF);
      WinSetDlgItemText( hwnd, CFG_NBP1_WELCOME_FILE, (PSZ) str );

      config_file_read_string( STD_CFG_FILE, RINGRING_WAVE, str, RINGRING_WAVE_DEF);
      WinSetDlgItemText( hwnd, CFG_NBP1_RINGING_FILE, (PSZ) str );

      config_file_read_string( STD_CFG_FILE, CALL_BACK_NUMBER, str, CALL_BACK_NUMBER_DEF);
      WinSetDlgItemText( hwnd, CFG_NBP1_NOTIFY_NUMBER, (PSZ) str );

      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_NBP1_WELCOME_FDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVALWFILE;
          strcpy( fileDlg.szFullFile, "*.WAV;*.ALW" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
          {
            WinSetDlgItemText( hwnd, CFG_NBP1_WELCOME_FILE,
                               fileDlg.szFullFile );
          }

          break;

        case CFG_NBP1_RINGING_FDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVFILE;
          strcpy( fileDlg.szFullFile, "*.WAV" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
          {
            WinSetDlgItemText( hwnd, CFG_NBP1_RINGING_FILE,
                               fileDlg.szFullFile );
          }

          break;
        default:
          break;
      }
      break;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

MRESULT EXPENTRY page2Proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HWND hwndContainer;
  PCNREDITDATA editData;
  POINTL ptlMouse;

  switch( msg )
  {
    case WM_INITDLG:
      hwndContainer = WinWindowFromID( hwnd, CFG_NBP2_CNTR );

      hwndPortPopup = WinLoadMenu( hwndNotebook, NULLHANDLE,
                                   CFG_PORTS_POPUP );

      InitEAZCntr( hwndContainer );
      FillEAZCntr( hwndContainer );

      break;

    case WM_WINDOWPOSCHANGED:
      WinSetWindowPos( WinWindowFromID( hwnd, CFG_NBP2_CNTR ), 0, 0,
                       0, ( (PSWP) mp1 )->cx, ( (PSWP) mp1 )->cy,
                       SWP_MOVE | SWP_SIZE );
      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_PORTS_QEDIT:
          currPort = (EAZSetup *) WinSendMsg( WinWindowFromID( hwnd,
                                  CFG_NBP2_CNTR ),
                                  CM_QUERYRECORDEMPHASIS,
                                  MPFROMSHORT( CMA_FIRST ),
                                  MPFROMSHORT( CRA_SELECTED ) );

          /* automagically jump to real edit-function */

        case CFG_PORTS_EDIT:
          if( currPort == NULL )
            break;

          WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) editPortProc,
                     NULLHANDLE, CFG_PORTS_EDITDLG, NULL );

          break;

        case CFG_PORTS_ADD:
          WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) addPortProc,
                     NULLHANDLE, CFG_PORTS_EDITDLG, NULL );
          break;

        case CFG_PORTS_QDEL:
          currPort = (EAZSetup *) WinSendMsg( WinWindowFromID( hwnd,
                                  CFG_NBP2_CNTR ),
                                  CM_QUERYRECORDEMPHASIS,
                                  MPFROMSHORT( CMA_FIRST ),
                                  MPFROMSHORT( CRA_SELECTED ) );

          /* automagically jump to real delete-function */

        case CFG_PORTS_DEL:
          if( currPort == NULL )
            break;

          if( WinMessageBox( HWND_DESKTOP, hwnd,
                             DELENTRY,
                             CTELMSG, 0, MB_YESNO | MB_ICONQUESTION |
                             MB_DEFBUTTON2 | MB_APPLMODAL | MB_MOVEABLE )
              == MBID_YES )
           {
             WinSendMsg( WinWindowFromID( hwnd, CFG_NBP2_CNTR ),
                         CM_REMOVERECORD, MPFROMP( &currPort ),
                         MPFROM2SHORT( 1, CMA_FREE | CMA_INVALIDATE ) );
           }

          break;


        default:
          break;
      }

      return 0;

    case WM_CONTROL:
      switch( SHORT2FROMMP( mp1 ) )
      {
        case CN_ENTER:
          if( (currPort = (EAZSetup *) ((PNOTIFYRECORDENTER) mp2)->pRecord)
              == NULL )
            break;

          WinPostMsg( hwnd, WM_COMMAND, MPFROMSHORT( CFG_PORTS_EDIT ), NULL );
          break;

        case CN_REALLOCPSZ:
          editData = (PCNREDITDATA) PVOIDFROMMP( mp2 );

          if( editData->pFieldInfo != NULL )
            return (MPARAM) TRUE;

          return WinDefDlgProc( hwnd, msg, mp1, mp2 );

        case CN_ENDEDIT:
          currPort = (EAZSetup *)
                       (((PCNREDITDATA) PVOIDFROMMP( mp2 ))->pRecord);

          CheckEAZSetup( currPort );
          break;

        case CN_CONTEXTMENU:
          currPort = (EAZSetup *) PVOIDFROMMP( mp2 );

          WinQueryPointerPos( HWND_DESKTOP, &ptlMouse );
          WinMapWindowPoints( HWND_DESKTOP, hwndNotebook, &ptlMouse, 1 );

          WinPopupMenu( hwndNotebook, hwnd, hwndPortPopup, ptlMouse.x,
                        ptlMouse.y, 0, PU_HCONSTRAIN | PU_VCONSTRAIN |
                        PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 |
                        PU_NONE );

          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }
      break;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

void InitEAZCntr( HWND hwnd )
{
  CNRINFO cntrInfo;
  FIELDINFOINSERT finfoInsert;
  PFIELDINFO pfiDetails, pfiCurrent, pfiSplit;
  int fields;
  ULONG version;

  /* Set Font & Size                                                     */
  DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, (PVOID) &version,
                   sizeof(ULONG) );

  if( version == 40 )
    WinSetPresParam( hwnd, PP_FONTNAMESIZE, sizeof( Defv4CntrFont ),
                     Defv4CntrFont );
  else
    WinSetPresParam( hwnd, PP_FONTNAMESIZE, sizeof( DefCntrFont ),
                     DefCntrFont );

  /* Remove all Container Items                                          */
  WinSendMsg( hwnd, CM_REMOVERECORD, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_FREE ) );

  /* Remove all Info about Details-View                                  */
  WinSendMsg( hwnd, CM_REMOVEDETAILFIELDINFO, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_FREE ) );

  /* Get Storage for Columns                                             */
  fields = 9;

  pfiDetails= (PFIELDINFO) PVOIDFROMMR( WinSendMsg(hwnd,
                                                   CM_ALLOCDETAILFIELDINFO,
                                                   MPFROMSHORT( fields ),
                                                   0 ) );

  if( pfiDetails == NULL )
    return;

  pfiCurrent             = pfiDetails;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_BITMAPORICON | CFA_FIREADONLY |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = "";
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.icon );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = PORT;
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.pdesc );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = EAZ_MSN;
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.pport );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_CENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = WELCOME_WAV_ALW;
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.palwfile );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_CENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = RINGING_WAV;
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.pringingwave );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER | CFA_FIREADONLY;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = REACTION;
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.preject );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_CENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = ANSWERDELAY;
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.panswerdelay );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_CENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = MAXRECTIME;
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.pmaxrectime);
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_CENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = NOTIFY_NUMBER;
  pfiCurrent->offStruct  = FIELDOFFSET( EAZSetup, itemInfo.pnotifynumber );
  pfiCurrent->pUserData  = NULL;


  /* Set up Container Info                                               */
  cntrInfo.flWindowAttr   = CV_DETAIL | CA_DETAILSVIEWTITLES;
  cntrInfo.pSortRecord    = (PVOID) sortPorts;
  cntrInfo.cb             = sizeof( CNRINFO );
  cntrInfo.cyLineSpacing  = 1;
  cntrInfo.slBitmapOrIcon = iconSize;

  WinSendMsg( hwnd, CM_SETCNRINFO, MPFROMP( &cntrInfo ),
              MPFROMLONG( CMA_FLWINDOWATTR | CMA_LINESPACING |
              CMA_PSORTRECORD | CMA_SLBITMAPORICON ) );

  finfoInsert.cb = sizeof( FIELDINFOINSERT );
  finfoInsert.pFieldInfoOrder = (PFIELDINFO) CMA_FIRST;
  finfoInsert.fInvalidateFieldInfo = TRUE;
  finfoInsert.cFieldInfoInsert = fields;

  WinSendMsg( hwnd, CM_INSERTDETAILFIELDINFO, MPFROMP( pfiDetails ),
              MPFROMP( &finfoInsert ) );

  return;
}

SHORT EXPENTRY sortPorts( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID tmp )
{
  return( strcmpi( ((EAZSetup *)p1)->itemInfo.port,
                   ((EAZSetup *)p2)->itemInfo.port ) );
}

void AddItemToEAZCntr( HWND hwnd, HPOINTER bmp, char *port, char *desc, char *alwfile,
                       char *reject_cause, char *maxrectime, char *delay,
                       char *ringing, char *notifynum )
{
  EAZSetup *rec;
  RECORDINSERT riInsert;

  rec = (EAZSetup *) PVOIDFROMMR( WinSendMsg( hwnd, CM_ALLOCRECORD,
                            MPFROMLONG(sizeof(EAZSetup)-sizeof(MINIRECORDCORE)),
                            MPFROMSHORT( 1 ) ) );

  strcpy( rec->itemInfo.port, strip_blanks( port ) );
  strcpy( rec->itemInfo.desc, strip_blanks( desc ) );
  strcpy( rec->itemInfo.alwfile, strip_blanks( alwfile ) );
  strcpy( rec->itemInfo.reject_cause, strip_blanks( reject_cause ) );
  strcpy( rec->itemInfo.maxrectime, strip_blanks( maxrectime ) );
  strcpy( rec->itemInfo.answerdelay, strip_blanks( delay ) );
  strcpy( rec->itemInfo.ringingwave, strip_blanks( ringing ) );
  strcpy( rec->itemInfo.notifynumber, strip_blanks( notifynum ) );

  rec->itemInfo.pport         = rec->itemInfo.port;
  rec->itemInfo.pdesc         = rec->itemInfo.desc;
  rec->itemInfo.palwfile      = rec->itemInfo.alwfile;
  rec->itemInfo.preject       = rec->itemInfo.reject_cause;
  rec->itemInfo.pmaxrectime   = rec->itemInfo.maxrectime;
  rec->itemInfo.panswerdelay  = rec->itemInfo.answerdelay;
  rec->itemInfo.pringingwave  = rec->itemInfo.ringingwave;
  rec->itemInfo.pnotifynumber = rec->itemInfo.notifynumber;

  rec->itemInfo.icon         = bmp;

  rec->core.hptrIcon = (HPOINTER) bmp;
  rec->core.pszIcon  = rec->itemInfo.desc;

  CheckEAZSetup( rec );

  riInsert.cb                = sizeof( RECORDINSERT );
  riInsert.pRecordOrder      = ( PRECORDCORE ) CMA_END;
  riInsert.pRecordParent     = NULL;
  riInsert.fInvalidateRecord = TRUE;
  riInsert.zOrder            = CMA_TOP;
  riInsert.cRecordsInsert    = 1;

  WinSendMsg( hwnd,
              CM_INSERTRECORD,
              MPFROMP( rec ),
              MPFROMP( &riInsert ) );

}

void FillEAZCntr( HWND hwnd )
{
  FILE *fh;
  char str[512], port[512], desc[512], alwfile[512];
  char reject_cause[512], maxrectime[512], answerdelay[512];
  char ringing[512], notifynum[512];
  char *stok;
  HPOINTER bmp;

  if( (fh = fopen( PRTFILE, "r" )) != NULL )
  {
    while( fgets( str, 512, fh ) )
    {
      delete_cr( str );

      if( (strlen( str ) > 0) && (str[0] != '#') )
      {
        stok = strtok( str, "|" );
        if( stok ) strcpy( port, stok );
        else strcpy( port, "?" );

        stok = strtok( NULL, "|" );
        if( stok ) strcpy( desc, stok );
        else strcpy( desc, "?" );

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( alwfile, stok );
        else *alwfile = 0;

        stok = strtok( NULL, "|" );
        if( stok )
        {
          switch( atoi( stok ) )
          {
            case 1:
              strcpy( reject_cause, REJECT_1 );
              break;
            case 2:
              strcpy( reject_cause, REJECT_2 );
              break;
            case 3:
              strcpy( reject_cause, REJECT_3 );
              break;
            default:
              strcpy( reject_cause, REJECT_0 );
              break;
          }
        }
        else strcpy( reject_cause, REJECT_0 );

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( maxrectime, stok );
        else *maxrectime = 0;

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( answerdelay, stok );
        else *answerdelay = 0;

        stok = strtok( NULL, "|" );
        if( !stok || atoi(stok) ) bmp = portsIcoActive;
        else bmp = portsIcoInactive;

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( ringing, stok );
        else *ringing = 0;

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( notifynum, stok );
        else *notifynum = 0;

        AddItemToEAZCntr( hwnd, bmp, port, desc, alwfile, reject_cause,
                          maxrectime, answerdelay, ringing, notifynum );
      }
    }

    fclose( fh );
  }

  return;
}

void SaveEAZSetup( HWND hwnd )
{
  FILE *fh;
  EAZSetup *pCore;
  short reject, isActive;

  if( (fh = fopen( PRTFILE, "w" )) != NULL )
  {
    fprintf( fh, CREATEMAINT,
             PRTFILE, APPNAME );

    pCore = (EAZSetup *) WinSendMsg( hwnd, CM_QUERYRECORD, NULL,
                         MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER ) );

    while( pCore )
    {
      if( strcmpi(pCore->itemInfo.reject_cause, REJECT_3) == 0 )
        reject = 3;
      else if( strcmpi(pCore->itemInfo.reject_cause, REJECT_2) == 0 )
        reject = 2;
      else if( strcmpi(pCore->itemInfo.reject_cause, REJECT_1) == 0 )
        reject = 1;
      else
        reject = 0;

      if( pCore->itemInfo.icon != portsIcoActive )
        isActive = 0;
      else
        isActive = 1;

      strip_blanks( pCore->itemInfo.port );
      strip_blanks( pCore->itemInfo.desc );
      strip_blanks( pCore->itemInfo.alwfile );
      strip_blanks( pCore->itemInfo.maxrectime );
      strip_blanks( pCore->itemInfo.answerdelay ),
      strip_blanks( pCore->itemInfo.ringingwave );
      strip_blanks( pCore->itemInfo.notifynumber );

      fprintf( fh, "%s|%s|%s|%d|%s|%s|%d|%s|%s\n",
        strlen( pCore->itemInfo.port ) ? pCore->itemInfo.port : "?",
        strlen( pCore->itemInfo.desc ) ? pCore->itemInfo.desc : "?",
        strlen( pCore->itemInfo.alwfile ) ? pCore->itemInfo.alwfile : "*",
        reject,
        strlen( pCore->itemInfo.maxrectime ) ? pCore->itemInfo.maxrectime : "*",
        strlen( pCore->itemInfo.answerdelay ) ? pCore->itemInfo.answerdelay : "*",
        isActive,
        strlen ( pCore->itemInfo.ringingwave ) ? pCore->itemInfo.ringingwave : "*",
        strlen ( pCore->itemInfo.notifynumber ) ? pCore->itemInfo.notifynumber : "*");

      pCore = (EAZSetup *) WinSendMsg( hwnd, CM_QUERYRECORD,
                           MPFROMP( pCore ),
                           MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER ) );
    }

    fclose( fh );
  }
}

MRESULT EXPENTRY editPortProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char descr[512], alwfile[512], maxrectime[512], answerdelay[512];
  char reject[512], number[512], ringing[512], notifynum[512], isActive;
  FILEDLG fileDlg;

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      if( currPort->itemInfo.icon == portsIcoActive )
        WinSendMsg( WinWindowFromID( hwnd, CFG_PORTS_EDITDLG_ACTIVE ),
                    BM_SETCHECK, MPFROMSHORT( 1 ), NULL );

      sprintf( descr, EDITINGPORT, currPort->itemInfo.port );
      WinSetWindowText( WinWindowFromID( hwnd, FID_TITLEBAR ), (PSZ) descr );

      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_DESCR,
                         currPort->itemInfo.desc );

      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_NUMBER,
                         currPort->itemInfo.port );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_FILE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_FILE,
                         currPort->itemInfo.alwfile );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_RINGING, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_RINGING,
                         currPort->itemInfo.ringingwave );

      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_MAXRECTIM,
                         currPort->itemInfo.maxrectime );

      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_DELAY,
                         currPort->itemInfo.answerdelay );

      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         currPort->itemInfo.reject_cause );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_0 ) );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_1 ) );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_2 ) );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_3 ) );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_NOTIFY, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_NOTIFY,
                         currPort->itemInfo.notifynumber );


      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_PORTS_EDITDLG_FILEDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVALWFILE;
          strcpy( fileDlg.szFullFile, "*.WAV;*.ALW" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_FILE,
                               fileDlg.szFullFile );

          break;
        case CFG_PORTS_EDITDLG_RINGING_FDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVFILE;
          strcpy( fileDlg.szFullFile, "*.WAV" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_RINGING,
                               fileDlg.szFullFile );

          break;

        case CFG_PORTS_EDITDLG_OK:
          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_DESCR,
                               sizeof( descr ), (PSZ) descr );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_NUMBER,
                               sizeof( number ), (PSZ) number );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_FILE,
                               sizeof( alwfile ), (PSZ) alwfile );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_RINGING,
                               sizeof( ringing ), (PSZ) ringing );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_MAXRECTIM,
                               sizeof( maxrectime ), (PSZ) maxrectime );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_DELAY,
                               sizeof( answerdelay ), (PSZ) answerdelay );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_REJECT,
                               sizeof( reject ), (PSZ) reject );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_NOTIFY,
                               sizeof( notifynum ), (PSZ) notifynum );

          strcpy( currPort->itemInfo.desc, descr );
          strcpy( currPort->itemInfo.port, number );
          strcpy( currPort->itemInfo.alwfile, alwfile );
          strcpy( currPort->itemInfo.maxrectime, maxrectime );
          strcpy( currPort->itemInfo.answerdelay, answerdelay );
          strcpy( currPort->itemInfo.reject_cause, reject );
          strcpy( currPort->itemInfo.ringingwave, ringing );
          strcpy( currPort->itemInfo.notifynumber, notifynum );

          isActive = (char) WinSendMsg( WinWindowFromID( hwnd,
                                    CFG_PORTS_EDITDLG_ACTIVE), BM_QUERYCHECK,
                                    NULL, NULL );

          if( isActive )
            currPort->itemInfo.icon = portsIcoActive;
          else
            currPort->itemInfo.icon = portsIcoInactive;

          CheckEAZSetup( currPort );

          WinSendMsg( WinWindowFromID( hwndPage2, CFG_NBP2_CNTR ),
                      CM_INVALIDATERECORD, &currPort,
                      MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );

          WinDismissDlg( hwnd, TRUE );
          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

      return 0;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

MRESULT EXPENTRY addPortProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char descr[512], alwfile[512], maxrectime[512], answerdelay[512];
  char reject[512], number[512], isActive, ringing[512], notifynum[512];
  FILEDLG fileDlg;
  HPOINTER Icon;

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      WinSendMsg( WinWindowFromID( hwnd, CFG_PORTS_EDITDLG_ACTIVE ),
                  BM_SETCHECK, MPFROMSHORT( 1 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_FILE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_RINGING, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_REJECT, REJECT_0 );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_0 ) );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_1 ) );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_2 ) );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_3 ) );

      WinSendDlgItemMsg( hwnd, CFG_PORTS_EDITDLG_NOTIFY, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_PORTS_EDITDLG_FILEDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVALWFILE;
          strcpy( fileDlg.szFullFile, "*.WAV;*.ALW" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_FILE,
                               fileDlg.szFullFile );

          break;

        case CFG_PORTS_EDITDLG_RINGING_FDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVFILE;
          strcpy( fileDlg.szFullFile, "*.WAV" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PORTS_EDITDLG_RINGING,
                               fileDlg.szFullFile );
          break;

        case CFG_PORTS_EDITDLG_OK:
          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_DESCR,
                               sizeof( descr ), (PSZ) descr );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_NUMBER,
                               sizeof( number ), (PSZ) number );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_FILE,
                               sizeof( alwfile ), (PSZ) alwfile );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_RINGING,
                               sizeof( ringing ), (PSZ) ringing );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_MAXRECTIM,
                               sizeof( maxrectime ), (PSZ) maxrectime );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_DELAY,
                               sizeof( answerdelay ), (PSZ) answerdelay );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_REJECT,
                               sizeof( reject ), (PSZ) reject );

          WinQueryDlgItemText( hwnd, CFG_PORTS_EDITDLG_NOTIFY,
                               sizeof( notifynum ), (PSZ) notifynum );

          isActive = (char) WinSendMsg( WinWindowFromID( hwnd,
                                    CFG_PORTS_EDITDLG_ACTIVE), BM_QUERYCHECK,
                                    NULL, NULL );

          if( isActive )
            Icon = portsIcoActive;
          else
            Icon = portsIcoInactive;

          AddItemToEAZCntr( WinWindowFromID( hwndPage2, CFG_NBP2_CNTR ), Icon,
                            number, descr, alwfile, reject, maxrectime,
                            answerdelay, ringing, notifynum );

          WinDismissDlg( hwnd, TRUE );
          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

      return 0;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

MRESULT EXPENTRY page3Proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HWND hwndContainer;
  PCNREDITDATA editData;
  POINTL ptlMouse;

  switch( msg )
  {
    case WM_INITDLG:
      hwndContainer = WinWindowFromID( hwnd, CFG_NBP3_CNTR );

      hwndPeoplePopup = WinLoadMenu( hwndNotebook, NULLHANDLE,
                                     CFG_PEOPLE_POPUP );

      InitPeopleCntr( hwndContainer );
      FillPeopleCntr( hwndContainer );

      break;

    case WM_WINDOWPOSCHANGED:
      WinSetWindowPos( WinWindowFromID( hwnd, CFG_NBP3_CNTR ), 0, 0,
                       0, ( (PSWP) mp1 )->cx, ( (PSWP) mp1 )->cy,
                       SWP_MOVE | SWP_SIZE );
      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_PEOPLE_ADD:
          WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) addPeopleProc,
                     NULLHANDLE, CFG_PEOPLE_EDITDLG, NULL );
          break;

        case CFG_PEOPLE_QEDIT:
          currPerson = (PeopleSetup *) WinSendMsg( WinWindowFromID( hwnd,
                                       CFG_NBP3_CNTR ),
                                       CM_QUERYRECORDEMPHASIS,
                                       MPFROMSHORT( CMA_FIRST ),
                                       MPFROMSHORT( CRA_SELECTED ) );

          /* automagically jump to real edit-function */

        case CFG_PEOPLE_EDIT:
          if( currPerson == NULL )
            break;

          WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) editPeopleProc,
                     NULLHANDLE, CFG_PEOPLE_EDITDLG, NULL );

          break;

        case CFG_PEOPLE_QDEL:
          currPerson = (PeopleSetup *) WinSendMsg( WinWindowFromID( hwnd,
                                       CFG_NBP3_CNTR ),
                                       CM_QUERYRECORDEMPHASIS,
                                       MPFROMSHORT( CMA_FIRST ),
                                       MPFROMSHORT( CRA_SELECTED ) );

          /* automagically jump to real delete-function */

        case CFG_PEOPLE_DEL:
          if( currPerson == NULL )
            break;

          if( WinMessageBox( HWND_DESKTOP, hwnd,
                             DELENTRY,
                             CTELMSG, 0, MB_YESNO | MB_ICONQUESTION |
                             MB_DEFBUTTON2 | MB_APPLMODAL | MB_MOVEABLE )
              == MBID_YES )
           {
             WinSendMsg( WinWindowFromID( hwnd, CFG_NBP3_CNTR ),
                         CM_REMOVERECORD, MPFROMP( &currPerson ),
                         MPFROM2SHORT( 1, CMA_FREE | CMA_INVALIDATE ) );
           }

          break;

        default:
          break;
      }

      return 0;

    case WM_CONTROL:
      switch( SHORT2FROMMP( mp1 ) )
      {
        case CN_ENTER:
          if( (currPerson = (PeopleSetup *) ((PNOTIFYRECORDENTER) mp2)->pRecord)
              == NULL )
            break;

          WinPostMsg( hwnd, WM_COMMAND, MPFROMSHORT( CFG_PEOPLE_EDIT ), NULL );
          break;

        case CN_REALLOCPSZ:
          editData = (PCNREDITDATA) PVOIDFROMMP( mp2 );

          if( editData->pFieldInfo != NULL )
            return (MPARAM) TRUE;

          return WinDefDlgProc( hwnd, msg, mp1, mp2 );

        case CN_ENDEDIT:
          currPerson = (PeopleSetup *)
                         (((PCNREDITDATA) PVOIDFROMMP( mp2 ))->pRecord);

          CheckPeopleSetup( currPerson );

          break;

        case CN_CONTEXTMENU:
          currPerson = (PeopleSetup *) PVOIDFROMMP( mp2 );

          WinQueryPointerPos( HWND_DESKTOP, &ptlMouse );
          WinMapWindowPoints( HWND_DESKTOP, hwndNotebook, &ptlMouse, 1 );

          WinPopupMenu( hwndNotebook, hwnd, hwndPeoplePopup, ptlMouse.x,
                        ptlMouse.y, 0, PU_HCONSTRAIN | PU_VCONSTRAIN |
                        PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 |
                        PU_NONE );

          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }
      break;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

void InitPeopleCntr( HWND hwnd )
{
  CNRINFO cntrInfo;
  FIELDINFOINSERT finfoInsert;
  PFIELDINFO pfiDetails, pfiCurrent, pfiSplit;
  int fields;
  ULONG version;

  /* Set Font & Size                                                     */
  DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, (PVOID) &version,
                   sizeof(ULONG) );

  if( version == 40 )
    WinSetPresParam( hwnd, PP_FONTNAMESIZE, sizeof( Defv4CntrFont ),
                     Defv4CntrFont );
  else
    WinSetPresParam( hwnd, PP_FONTNAMESIZE, sizeof( DefCntrFont ),
                     DefCntrFont );

  /* Remove all Container Items                                          */
  WinSendMsg( hwnd, CM_REMOVERECORD, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_FREE ) );

  /* Remove all Info about Details-View                                  */
  WinSendMsg( hwnd, CM_REMOVEDETAILFIELDINFO, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_FREE ) );

  /* Get Storage for Colums                                              */
  fields = 9;

  pfiDetails= (PFIELDINFO) PVOIDFROMMR( WinSendMsg(hwnd,
                                                   CM_ALLOCDETAILFIELDINFO,
                                                   MPFROMSHORT( fields ),
                                                   0 ) );

  if( pfiDetails == NULL )
    return;

  /* Set up 1st Column                                                   */
  pfiCurrent             = pfiDetails;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_BITMAPORICON | CFA_FIREADONLY |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = "";
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.icon );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = NAMEMSG;
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.pname );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = NUMBERMSG;
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.pnumber );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = WELCOME_WAV_ALW;
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.palwfile );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = RINGING_WAV;
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.pringingwave );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER | CFA_FIREADONLY;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = REACTION;
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.preject );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_CENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = ANSWERDELAY;
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.panswerdelay);
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_CENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = MAXRECTIME;
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.pmaxrectime);
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_CENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = NOTIFY_NUMBER;
  pfiCurrent->offStruct  = FIELDOFFSET( PeopleSetup, itemInfo.pnotifynumber);
  pfiCurrent->pUserData  = NULL;

  /* Set up Container Info                                               */
  cntrInfo.flWindowAttr   = CV_DETAIL | CA_DETAILSVIEWTITLES;
  cntrInfo.pSortRecord    = (PVOID) sortPeople;
  cntrInfo.cb             = sizeof( CNRINFO );
  cntrInfo.cyLineSpacing  = 1;
  cntrInfo.slBitmapOrIcon = iconSize;

  WinSendMsg( hwnd, CM_SETCNRINFO, MPFROMP( &cntrInfo ),
              MPFROMLONG( CMA_FLWINDOWATTR | CMA_LINESPACING |
              CMA_PSORTRECORD | CMA_SLBITMAPORICON ) );

  finfoInsert.cb = sizeof( FIELDINFOINSERT );
  finfoInsert.pFieldInfoOrder = (PFIELDINFO) CMA_FIRST;
  finfoInsert.fInvalidateFieldInfo = TRUE;
  finfoInsert.cFieldInfoInsert = fields;

  WinSendMsg( hwnd, CM_INSERTDETAILFIELDINFO, MPFROMP( pfiDetails ),
              MPFROMP( &finfoInsert ) );

  return;
}

SHORT EXPENTRY sortPeople( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID tmp )
{
  return( strcmpi( ((PeopleSetup *)p1)->itemInfo.name,
                   ((PeopleSetup *)p2)->itemInfo.name ) );
}

void AddItemToPeopleCntr( HWND hwnd, HPOINTER bmp, char *number, char *name,
                          char *alwfile, char *reject_cause, char *maxrectime,
                          char *delay, char *ringing, char *notifynum )
{
  PeopleSetup *rec;
  RECORDINSERT riInsert;

  rec = (PeopleSetup *) PVOIDFROMMR( WinSendMsg( hwnd, CM_ALLOCRECORD,
                        MPFROMLONG(sizeof(PeopleSetup)-sizeof(MINIRECORDCORE)),
                        MPFROMSHORT( 1 ) ) );

  strcpy( rec->itemInfo.name, strip_blanks( name ) );
  strcpy( rec->itemInfo.number, strip_blanks( number ) );
  strcpy( rec->itemInfo.alwfile, strip_blanks( alwfile ) );
  strcpy( rec->itemInfo.reject_cause, strip_blanks( reject_cause ) );
  strcpy( rec->itemInfo.maxrectime, strip_blanks( maxrectime ) );
  strcpy( rec->itemInfo.answerdelay, strip_blanks( delay ) );
  strcpy( rec->itemInfo.ringingwave, strip_blanks( ringing ) );
  strcpy( rec->itemInfo.notifynumber, strip_blanks( notifynum ) );

  rec->itemInfo.pname         = rec->itemInfo.name;
  rec->itemInfo.pnumber       = rec->itemInfo.number;
  rec->itemInfo.palwfile      = rec->itemInfo.alwfile;
  rec->itemInfo.preject       = rec->itemInfo.reject_cause;
  rec->itemInfo.pmaxrectime   = rec->itemInfo.maxrectime;
  rec->itemInfo.panswerdelay  = rec->itemInfo.answerdelay;
  rec->itemInfo.pringingwave  = rec->itemInfo.ringingwave;
  rec->itemInfo.pnotifynumber = rec->itemInfo.notifynumber;

  rec->itemInfo.icon         = bmp;

  rec->core.hptrIcon = (HPOINTER) bmp;
  rec->core.pszIcon  = rec->itemInfo.name;

  CheckPeopleSetup( rec );

  riInsert.cb                = sizeof( RECORDINSERT );
  riInsert.pRecordOrder      = (PRECORDCORE) CMA_END;
  riInsert.pRecordParent     = NULL;
  riInsert.fInvalidateRecord = TRUE;
  riInsert.zOrder            = CMA_TOP;
  riInsert.cRecordsInsert    = 1;

  WinSendMsg( hwnd,
              CM_INSERTRECORD,
              MPFROMP( rec ),
              MPFROMP( &riInsert ) );
}

void FillPeopleCntr( HWND hwnd )
{
  FILE *fh;
  char str[512], number[512], name[512], alwfile[512];
  char reject_cause[512], maxrectime[512], answerdelay[512], ringing[512];
  char notifynum[512];
  char *stok;
  HPOINTER bmp;

  if( (fh = fopen( NAMFILE, "r" )) != NULL )
  {
    while( fgets( str, 512, fh ) )
    {
      delete_cr( str );

      if( (strlen( str ) > 0) && (str[0] != '#') )
      {
        stok = strtok( str, "|" );
        if( stok ) strcpy( name, stok );
        else strcpy( name, "?" );

        stok = strtok( NULL, "|" );
        if( stok ) strcpy( number, stok );
        else strcpy( number, "?" );

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( alwfile, stok );
        else *alwfile = 0;

        stok = strtok( NULL, "|" );
        if( stok != NULL )
        {
          switch( *stok )
          {
            case '0':
              strcpy( reject_cause, REJECT_0 );
              break;
            case '1':
              strcpy( reject_cause, REJECT_1 );
              break;
            case '2':
              strcpy( reject_cause, REJECT_2 );
              break;
            case '3':
              strcpy( reject_cause, REJECT_3 );
              break;
            default:
              strcpy( reject_cause, REJECT_D );
              break;
          }
        }
        else strcpy( reject_cause, REJECT_D );

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( maxrectime, stok );
        else *maxrectime = 0;

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( answerdelay, stok );
        else *answerdelay = 0;

        stok = strtok( NULL, "|" );
        if( !stok || atoi( stok ) ) bmp = callersIcoActive;
        else bmp = callersIcoInactive;

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( ringing, stok );
        else *ringing = 0;

        stok = strtok( NULL, "|" );
        if( stok && *stok != '*' ) strcpy( notifynum, stok );
        else *notifynum = 0;

        AddItemToPeopleCntr( hwnd, bmp, name, number, alwfile, reject_cause,
                             maxrectime, answerdelay, ringing, notifynum );
      }
    }

    fclose( fh );

  }

  return;
}

void SavePeopleSetup( HWND hwnd )
{
  FILE *fh;
  PeopleSetup *pCore;
  char reject;
  short isActive;

  if( (fh = fopen( NAMFILE, "w" )) != NULL )
  {
    fprintf( fh, CREATEMAINT,
             NAMFILE, APPNAME );

    pCore = (PeopleSetup *) WinSendMsg( hwnd, CM_QUERYRECORD, NULL,
                            MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER ) );

    while( pCore )
    {
      if( strcmpi(pCore->itemInfo.reject_cause, REJECT_3) == 0 )
        reject = '3';
      else if( strcmpi(pCore->itemInfo.reject_cause, REJECT_2) == 0 )
        reject = '2';
      else if( strcmpi(pCore->itemInfo.reject_cause, REJECT_1) == 0 )
        reject = '1';
      else if( strcmpi(pCore->itemInfo.reject_cause, REJECT_0) == 0 )
        reject = '0';
      else
        reject = '*';

      if( pCore->itemInfo.icon != callersIcoActive )
        isActive = 0;
      else
        isActive = 1;

      strip_blanks( pCore->itemInfo.name );
      strip_blanks( pCore->itemInfo.alwfile );
      strip_blanks( pCore->itemInfo.maxrectime );
      strip_blanks( pCore->itemInfo.answerdelay );
      strip_blanks( pCore->itemInfo.ringingwave );
      strip_blanks( pCore->itemInfo.notifynumber );

      fprintf( fh, "%s|%s|%s|%c|%s|%s|%d|%s|%s\n",
        strlen( pCore->itemInfo.number ) ? pCore->itemInfo.number : "?",
        strlen( pCore->itemInfo.name ) ? pCore->itemInfo.name : "?",
        strlen( pCore->itemInfo.alwfile ) ? pCore->itemInfo.alwfile : "*",
        reject,
        strlen( pCore->itemInfo.maxrectime ) ? pCore->itemInfo.maxrectime : "*",
        strlen( pCore->itemInfo.answerdelay ) ? pCore->itemInfo.answerdelay : "*",
        isActive,
        strlen( pCore->itemInfo.ringingwave ) ? pCore->itemInfo.ringingwave : "*",
        strlen( pCore->itemInfo.notifynumber ) ? pCore->itemInfo.notifynumber : "*");

      pCore = (PeopleSetup *) WinSendMsg( hwnd, CM_QUERYRECORD,
                              MPFROMP( pCore ),
                              MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER ) );
    }

    fclose( fh );
  }
}

MRESULT EXPENTRY addPeopleProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char number[512], name[512], alwfile[512];
  char reject_cause[512], maxrectime[512], answerdelay[512], isActive;
  char ringing[512], notifynum[512];
  HPOINTER bmp;
  FILEDLG fileDlg;

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      WinSendMsg( WinWindowFromID( hwnd, CFG_PEOPLE_EDITDLG_ACTIVE ),
                  BM_SETCHECK, MPFROMSHORT( 1 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_FILE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_RINGING, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_REJECT, REJECT_D );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_D ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_0 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_1 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_2 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_3 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_NOTIFY, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_PEOPLE_EDITDLG_FILEDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVALWFILE;
          strcpy( fileDlg.szFullFile, "*.WAV;*.ALW" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_FILE,
                               fileDlg.szFullFile );

          break;

        case CFG_PEOPLE_EDITDLG_RINGING_FDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVFILE;
          strcpy( fileDlg.szFullFile, "*.WAV" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_RINGING,
                               fileDlg.szFullFile );

          break;

        case CFG_PEOPLE_EDITDLG_OK:
          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NUMBER,
                               sizeof( number ), (PSZ) number );

          if( !strlen(number) )
          {
            MsgBox( ERRNONUM );
            break;
          }
          else if( number[0] == '*' )
          {
            MsgBox( ERRSTARINNUM );
            break;
          }

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NAME,
                               sizeof( name ), (PSZ) name );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_FILE,
                               sizeof( alwfile ), (PSZ) alwfile );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_MAXRECTIM,
                               sizeof( maxrectime ), (PSZ) maxrectime );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_DELAY,
                               sizeof( answerdelay ), (PSZ) answerdelay );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                               sizeof( reject_cause ), (PSZ) reject_cause );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_RINGING,
                               sizeof( ringing ), (PSZ) ringing );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NOTIFY,
                               sizeof( ringing ), (PSZ) notifynum );

          isActive = (char) WinSendMsg( WinWindowFromID( hwnd,
                                    CFG_PEOPLE_EDITDLG_ACTIVE), BM_QUERYCHECK,
                                    NULL, NULL );

          if( isActive )
            bmp = callersIcoActive;
          else
            bmp = callersIcoInactive;

          AddItemToPeopleCntr( WinWindowFromID( hwndPage3, CFG_NBP3_CNTR ),
                               bmp, number, name, alwfile, reject_cause,
                               maxrectime, answerdelay, ringing, notifynum );

          WinDismissDlg( hwnd, TRUE );
          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

      return 0;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

MRESULT EXPENTRY editPeopleProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char number[512], name[512], alwfile[512];
  char reject_cause[512], maxrectime[512], answerdelay[512], isActive;
  char ringing[512], notifynum[512];
  FILEDLG fileDlg;

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      if( currPerson->itemInfo.icon == callersIcoActive )
        WinSendMsg( WinWindowFromID( hwnd, CFG_PEOPLE_EDITDLG_ACTIVE ),
                    BM_SETCHECK, MPFROMSHORT( 1 ), NULL );

      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NUMBER,
                         currPerson->itemInfo.number );

      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NAME,
                         currPerson->itemInfo.name );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_FILE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_FILE,
                         currPerson->itemInfo.alwfile );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_RINGING, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_RINGING,
                         currPerson->itemInfo.ringingwave );

      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_MAXRECTIM,
                         currPerson->itemInfo.maxrectime );

      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_DELAY,
                         currPerson->itemInfo.answerdelay );

      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         currPerson->itemInfo.reject_cause );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_D ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_0 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_1 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_2 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_3 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_NOTIFY, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NOTIFY,
                         currPerson->itemInfo.notifynumber );

      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_PEOPLE_EDITDLG_FILEDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVALWFILE;
          strcpy( fileDlg.szFullFile, "*.WAV;*.ALW" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_FILE,
                               fileDlg.szFullFile );

          break;

        case CFG_PEOPLE_EDITDLG_RINGING_FDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVFILE;
          strcpy( fileDlg.szFullFile, "*.WAV" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_RINGING,
                               fileDlg.szFullFile );

          break;

        case CFG_PEOPLE_EDITDLG_OK:
          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NUMBER,
                               sizeof( number ), (PSZ) number );

          if( !strlen(number) )
          {
            MsgBox( ERRNONUM );
            break;
          }
          else if( number[0] == '*' )
          {
            MsgBox( ERRSTARINNUM );
            break;
          }

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NAME,
                               sizeof( name ), (PSZ) name );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_FILE,
                               sizeof( alwfile ), (PSZ) alwfile );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_RINGING,
                               sizeof( ringing ), (PSZ) ringing );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_MAXRECTIM,
                               sizeof( maxrectime ), (PSZ) maxrectime );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_DELAY,
                               sizeof( answerdelay ), (PSZ) answerdelay );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                               sizeof( reject_cause ), (PSZ) reject_cause );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NOTIFY,
                               sizeof( notifynum ), (PSZ) notifynum );

          strcpy( currPerson->itemInfo.number, number );
          strcpy( currPerson->itemInfo.name, name );
          strcpy( currPerson->itemInfo.alwfile, alwfile );
          strcpy( currPerson->itemInfo.reject_cause, reject_cause );
          strcpy( currPerson->itemInfo.answerdelay, answerdelay );
          strcpy( currPerson->itemInfo.maxrectime, maxrectime );
          strcpy( currPerson->itemInfo.ringingwave, ringing );
          strcpy( currPerson->itemInfo.notifynumber, notifynum );

          isActive = (char) WinSendMsg( WinWindowFromID( hwnd,
                                    CFG_PEOPLE_EDITDLG_ACTIVE), BM_QUERYCHECK,
                                    NULL, NULL );

          if( isActive )
            currPerson->itemInfo.icon = callersIcoActive;
          else
            currPerson->itemInfo.icon = callersIcoInactive;

          CheckPeopleSetup( currPerson );

          WinSendMsg( WinWindowFromID( hwndPage3, CFG_NBP3_CNTR ),
                      CM_INVALIDATERECORD, &currPerson,
                      MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );

          WinDismissDlg( hwnd, TRUE );
          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

      return 0;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

MRESULT EXPENTRY page4Proc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HWND hwndContainer;
  PCNREDITDATA editData;
  POINTL ptlMouse;

  switch( msg )
  {
    case WM_INITDLG:
      hwndContainer = WinWindowFromID( hwnd, CFG_NBP4_CNTR );

      hwndDtmfPopup = WinLoadMenu( hwndNotebook, NULLHANDLE,
                                   CFG_DTMF_POPUP );

      InitDtmfCntr( hwndContainer );
      FillDtmfCntr( hwndContainer );

      break;

    case WM_WINDOWPOSCHANGED:
      WinSetWindowPos( WinWindowFromID( hwnd, CFG_NBP4_CNTR ), 0, 0,
                       0, ( (PSWP) mp1 )->cx, ( (PSWP) mp1 )->cy,
                       SWP_MOVE | SWP_SIZE );
      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_DTMF_QEDIT:
          currDtmf = (DtmfSetup *) WinSendMsg( WinWindowFromID( hwnd,
                                  CFG_NBP4_CNTR ),
                                  CM_QUERYRECORDEMPHASIS,
                                  MPFROMSHORT( CMA_FIRST ),
                                  MPFROMSHORT( CRA_SELECTED ) );

          /* automagically jump to real edit-function */

        case CFG_DTMF_EDIT:
          if( currDtmf == NULL )
            break;

          WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) editDtmfProc,
                     NULLHANDLE, CFG_DTMF_EDITDLG, NULL );

          break;

        case CFG_DTMF_ADD:
          WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) addDtmfProc,
                     NULLHANDLE, CFG_DTMF_EDITDLG, NULL );
          break;

        case CFG_DTMF_QDEL:
          currDtmf = (DtmfSetup *) WinSendMsg( WinWindowFromID( hwnd,
                                  CFG_NBP4_CNTR ),
                                  CM_QUERYRECORDEMPHASIS,
                                  MPFROMSHORT( CMA_FIRST ),
                                  MPFROMSHORT( CRA_SELECTED ) );

          /* automagically jump to real delete-function */

        case CFG_DTMF_DEL:
          if( currDtmf == NULL )
            break;

          if( WinMessageBox( HWND_DESKTOP, hwnd,
                             DELENTRY,
                             CTELMSG, 0, MB_YESNO | MB_ICONQUESTION |
                             MB_DEFBUTTON2 | MB_APPLMODAL | MB_MOVEABLE )
              == MBID_YES )
           {
             WinSendMsg( WinWindowFromID( hwnd, CFG_NBP4_CNTR ),
                         CM_REMOVERECORD, MPFROMP( &currDtmf ),
                         MPFROM2SHORT( 1, CMA_FREE | CMA_INVALIDATE ) );
           }

          break;


        default:
          break;
      }

      return 0;

    case WM_CONTROL:
      switch( SHORT2FROMMP( mp1 ) )
      {
        case CN_ENTER:
          if( (currDtmf = (DtmfSetup *) ((PNOTIFYRECORDENTER) mp2)->pRecord)
              == NULL )
            break;

          WinPostMsg( hwnd, WM_COMMAND, MPFROMSHORT( CFG_DTMF_EDIT ), NULL );
          break;

        case CN_REALLOCPSZ:
          editData = (PCNREDITDATA) PVOIDFROMMP( mp2 );

          if( editData->pFieldInfo != NULL )
            return (MPARAM) TRUE;

          return WinDefDlgProc( hwnd, msg, mp1, mp2 );

        case CN_ENDEDIT:
          currDtmf = (DtmfSetup *)
                       (((PCNREDITDATA) PVOIDFROMMP( mp2 ))->pRecord);

          CheckDtmfSetup( currDtmf );

          break;

        case CN_CONTEXTMENU:
          currDtmf = (DtmfSetup *) PVOIDFROMMP( mp2 );

          WinQueryPointerPos( HWND_DESKTOP, &ptlMouse );
          WinMapWindowPoints( HWND_DESKTOP, hwndNotebook, &ptlMouse, 1 );

          WinPopupMenu( hwndNotebook, hwnd, hwndDtmfPopup, ptlMouse.x,
                        ptlMouse.y, 0, PU_HCONSTRAIN | PU_VCONSTRAIN |
                        PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 |
                        PU_NONE );

          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }
      break;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

void InitDtmfCntr( HWND hwnd )
{
  CNRINFO cntrInfo;
  FIELDINFOINSERT finfoInsert;
  PFIELDINFO pfiDetails, pfiCurrent, pfiSplit;
  int fields;
  ULONG version;

  /* Set Font & Size                                                     */
  DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, (PVOID) &version,
                   sizeof(ULONG) );

  if( version == 40 )
    WinSetPresParam( hwnd, PP_FONTNAMESIZE, sizeof( Defv4CntrFont ),
                     Defv4CntrFont );
  else
    WinSetPresParam( hwnd, PP_FONTNAMESIZE, sizeof( DefCntrFont ),
                     DefCntrFont );

  /* Remove all Container Items                                          */
  WinSendMsg( hwnd, CM_REMOVERECORD, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_FREE ) );

  /* Remove all Info about Details-View                                  */
  WinSendMsg( hwnd, CM_REMOVEDETAILFIELDINFO, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_FREE ) );

  /* Get Storage for Columns                                             */
  fields = 5;

  pfiDetails= (PFIELDINFO) PVOIDFROMMR( WinSendMsg(hwnd,
                                                   CM_ALLOCDETAILFIELDINFO,
                                                   MPFROMSHORT( fields ),
                                                   0 ) );

  if( pfiDetails == NULL )
    return;

  pfiCurrent             = pfiDetails;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_BITMAPORICON | CFA_FIREADONLY |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER;
  pfiCurrent->pTitleData = "";
  pfiCurrent->offStruct  = FIELDOFFSET( DtmfSetup, itemInfo.icon );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = DTMF_CODE;
  pfiCurrent->offStruct  = FIELDOFFSET( DtmfSetup, itemInfo.pcode );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER | CFA_FIREADONLY;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = ACTION;
  pfiCurrent->offStruct  = FIELDOFFSET( DtmfSetup, itemInfo.paction );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_VCENTER | CFA_HORZSEPARATOR;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = "";
  pfiCurrent->offStruct  = FIELDOFFSET( DtmfSetup, itemInfo.pparams );
  pfiCurrent->pUserData  = NULL;

  pfiCurrent             = pfiCurrent->pNextFieldInfo;
  pfiCurrent->cb         = sizeof( FIELDINFO );
  pfiCurrent->flData     = CFA_STRING | CFA_HORZSEPARATOR |
                           CFA_VCENTER;
  pfiCurrent->flTitle    = CFA_VCENTER | CFA_FITITLEREADONLY;
  pfiCurrent->pTitleData = WINDOW_TITLE;
  pfiCurrent->offStruct  = FIELDOFFSET( DtmfSetup, itemInfo.ptitle );
  pfiCurrent->pUserData  = NULL;

  /* Set up Container Info                                               */
  cntrInfo.flWindowAttr   = CV_DETAIL | CA_DETAILSVIEWTITLES;
  cntrInfo.pSortRecord    = (PVOID) sortDtmf;
  cntrInfo.cb             = sizeof( CNRINFO );
  cntrInfo.cyLineSpacing  = 1;
  cntrInfo.slBitmapOrIcon = iconSize;

  WinSendMsg( hwnd, CM_SETCNRINFO, MPFROMP( &cntrInfo ),
              MPFROMLONG( CMA_FLWINDOWATTR | CMA_LINESPACING |
              CMA_PSORTRECORD | CMA_SLBITMAPORICON ) );

  finfoInsert.cb = sizeof( FIELDINFOINSERT );
  finfoInsert.pFieldInfoOrder = (PFIELDINFO) CMA_FIRST;
  finfoInsert.fInvalidateFieldInfo = TRUE;
  finfoInsert.cFieldInfoInsert = fields;

  WinSendMsg( hwnd, CM_INSERTDETAILFIELDINFO, MPFROMP( pfiDetails ),
              MPFROMP( &finfoInsert ) );

  return;
}

SHORT EXPENTRY sortDtmf( PMINIRECORDCORE p1, PMINIRECORDCORE p2, PVOID tmp )
{
  return( strcmpi( ((DtmfSetup *)p1)->itemInfo.code,
                   ((DtmfSetup *)p2)->itemInfo.code ) );
}

void AddItemToDtmfCntr( HWND hwnd, HPOINTER bmp, char *code, char *action,
                        char *params, char *title, char *type, char *active )
{
  DtmfSetup *rec;
  RECORDINSERT riInsert;

  rec = (DtmfSetup *) PVOIDFROMMR( WinSendMsg( hwnd, CM_ALLOCRECORD,
                            MPFROMLONG(sizeof(DtmfSetup)-sizeof(MINIRECORDCORE)),
                            MPFROMSHORT( 1 ) ) );

  strcpy( rec->itemInfo.code, code );
  strcpy( rec->itemInfo.action, action );
  strcpy( rec->itemInfo.params, params );
  strcpy( rec->itemInfo.title, title );
  strcpy( rec->itemInfo.type, type );
  strcpy( rec->itemInfo.active, active );

  rec->itemInfo.pcode        = rec->itemInfo.code;
  rec->itemInfo.paction      = rec->itemInfo.action;
  rec->itemInfo.pparams      = rec->itemInfo.params;
  rec->itemInfo.ptitle       = rec->itemInfo.title;
  rec->itemInfo.ptype        = rec->itemInfo.type;
  rec->itemInfo.pactive      = rec->itemInfo.active;

  rec->itemInfo.icon         = bmp;

  rec->core.hptrIcon = (HPOINTER) bmp;
  rec->core.pszIcon  = rec->itemInfo.action;

  CheckDtmfSetup( rec );

  riInsert.cb                = sizeof( RECORDINSERT );
  riInsert.pRecordOrder      = ( PRECORDCORE ) CMA_END;
  riInsert.pRecordParent     = NULL;
  riInsert.fInvalidateRecord = TRUE;
  riInsert.zOrder            = CMA_TOP;
  riInsert.cRecordsInsert    = 1;

  WinSendMsg( hwnd,
              CM_INSERTRECORD,
              MPFROMP( rec ),
              MPFROMP( &riInsert ) );

}

void FillDtmfCntr( HWND hwnd )
{
  FILE *fh;
  char str[512], code[512], action[512], params[512],
       title[512], type[512], active[512];
  char *stok;
  HPOINTER bmp;

  if( (fh = fopen( ACTFILE, "r" )) != NULL )
  {
    while( fgets( str, 512, fh ) )
    {
      delete_cr( str );

      if( (strlen( str ) > 0) && (str[0] != '#') )
      {
        stok = strtok( str, "|" );
        if( stok ) strcpy( code, stok );
        else *code = 0;

        stok = strtok( NULL, "|" );
        if( stok )
        {
          if( !strcmp( DTMF_REMOTECONTROL, stok ) )
            strcpy( action, DTMF_TEXT_RCONTROL );
          else if( !strcmp( DTMF_REBOOT, stok ) )
            strcpy( action, DTMF_TEXT_REBOOT );
          else if( !strcmp( DTMF_DEACTIVATE, stok ) )
            strcpy( action, DTMF_TEXT_DEACT );
          else if( !strcmp( DTMF_QUIT, stok ) )
            strcpy( action, DTMF_TEXT_QUIT );
          else if( !strcmp( DTMF_SETCALLBACK, stok ) )
            strcpy( action, DTMF_TEXT_SETNOTIFY );
          else
            strcpy( action, stok );
        }
        else *action = 0;

        stok = strtok( NULL, "|" );
        if( stok ) strcpy( params, stok );
        else *params = 0;

        stok = strtok( NULL, "|" );
        if( stok ) strcpy( title, stok );
        else *title = 0;

        stok = strtok( NULL, "|" );
        if( stok ) strcpy( type, stok );
        else *type = 0;

        stok = strtok( NULL, "|" );
        if( stok ) strcpy( active, stok );
        else strcpy( active, "1" );
        bmp = atoi(active) ? actionIcoActive : actionIcoInactive;

        AddItemToDtmfCntr( hwnd, bmp, code, action, params, title,
                           type, active );
      }
    }

    fclose( fh );
  }

  return;
}

void SaveDtmfSetup( HWND hwnd )
{
  FILE *fh;
  DtmfSetup *pCore;
  char action[512];

  if( (fh = fopen( ACTFILE, "w" )) != NULL )
  {
    fprintf( fh, CREATEMAINT,
             ACTFILE, APPNAME );

    pCore = (DtmfSetup *) WinSendMsg( hwnd, CM_QUERYRECORD, NULL,
                          MPFROM2SHORT( CMA_FIRST, CMA_ITEMORDER ) );

    while( pCore )
    {
      if( !strcmp( DTMF_TEXT_RCONTROL, pCore->itemInfo.action ) )
        strcpy( action, DTMF_REMOTECONTROL );
      else if( !strcmp( DTMF_TEXT_REBOOT, pCore->itemInfo.action ) )
        strcpy( action, DTMF_REBOOT );
      else if( !strcmp( DTMF_TEXT_DEACT, pCore->itemInfo.action ) )
        strcpy( action, DTMF_DEACTIVATE );
      else if( !strcmp( DTMF_TEXT_QUIT, pCore->itemInfo.action ) )
        strcpy( action, DTMF_QUIT );
      else if( !strcmp( DTMF_TEXT_SETNOTIFY, pCore->itemInfo.action ) )
        strcpy( action, DTMF_SETCALLBACK );
      else
        strcpy( action, pCore->itemInfo.action );

      fprintf( fh, "%s|%s|%s|%s|%s|%s\n",
                                 pCore->itemInfo.code,
                                 action,
                                 pCore->itemInfo.params,
                                 pCore->itemInfo.title,
                                 pCore->itemInfo.type,
                                 pCore->itemInfo.active );

      pCore = (DtmfSetup *) WinSendMsg( hwnd, CM_QUERYRECORD,
                            MPFROMP( pCore ),
                            MPFROM2SHORT( CMA_NEXT, CMA_ITEMORDER ) );
    }

    fclose( fh );
  }
}

MRESULT EXPENTRY editDtmfProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char isActive;
  short index;

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_CODE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      if( currDtmf->itemInfo.active[0] == '1' )
        WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_ACTIVE,
                           BM_SETCHECK, MPFROMSHORT( 1 ), NULL );

      WinSetDlgItemText( hwnd, CFG_DTMF_EDITDLG_CODE,
                         currDtmf->itemInfo.code );

      if( !strcmp( DTMF_TEXT_RCONTROL, currDtmf->itemInfo.action ) )
      {
        WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_RCONTROL,
                           BM_SETCHECK, MPFROMSHORT( 1 ), NULL );
      }
      else if( !strcmp( DTMF_TEXT_REBOOT, currDtmf->itemInfo.action ) )
      {
        WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_REBOOT,
                           BM_SETCHECK, MPFROMSHORT( 1 ), NULL );
      }
      else if( !strcmp( DTMF_TEXT_DEACT, currDtmf->itemInfo.action ) )
      {
        WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_DEACT,
                           BM_SETCHECK, MPFROMSHORT( 1 ), NULL );
      }
      else if( !strcmp( DTMF_TEXT_QUIT, currDtmf->itemInfo.action ) )
      {
        WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_QUIT,
                           BM_SETCHECK, MPFROMSHORT( 1 ), NULL );
      }
      else if( !strcmp( DTMF_TEXT_SETNOTIFY, currDtmf->itemInfo.action ) )
      {
        WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_SETNOTIFY,
                           BM_SETCHECK, MPFROMSHORT( 1 ), NULL );
      }
      else
      {
        WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_EXEC,
                           BM_SETCHECK, MPFROMSHORT( 1 ), NULL );

        WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG ), TRUE );
        WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG ), TRUE );
        WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE ), TRUE );

        WinSetDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG,
                           currDtmf->itemInfo.action );

        WinSetDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG,
                           currDtmf->itemInfo.params );

        WinSetDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE,
                           currDtmf->itemInfo.title );
      }

      break;

    case WM_CONTROL:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_DTMF_EDITDLG_RB_EXEC:
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG ), TRUE );
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG ), TRUE );
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE ), TRUE );
          break;

        case CFG_DTMF_EDITDLG_RB_RCONTROL:
        case CFG_DTMF_EDITDLG_RB_REBOOT:
        case CFG_DTMF_EDITDLG_RB_DEACT:
        case CFG_DTMF_EDITDLG_RB_QUIT:
        case CFG_DTMF_EDITDLG_RB_SETNOTIFY:
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG ), FALSE );
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG ), FALSE );
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE ), FALSE );
          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

      return 0;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_DTMF_EDITDLG_OK:
          strcpy( currDtmf->itemInfo.code, " " );
          strcpy( currDtmf->itemInfo.params, " " );
          strcpy( currDtmf->itemInfo.title, " " );
          strcpy( currDtmf->itemInfo.type, " " );

          WinQueryDlgItemText( hwnd, CFG_DTMF_EDITDLG_CODE,
                               sizeof( currDtmf->itemInfo.code ),
                               (PSZ) currDtmf->itemInfo.code );

          index = (short) WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_RCONTROL,
                                     BM_QUERYCHECKINDEX, NULL, NULL );
          index+= CFG_DTMF_EDITDLG_RB_RCONTROL;

          switch( index )
          {
            case CFG_DTMF_EDITDLG_RB_RCONTROL:
              strcpy( currDtmf->itemInfo.action, DTMF_TEXT_RCONTROL );
              break;
            case CFG_DTMF_EDITDLG_RB_REBOOT:
              strcpy( currDtmf->itemInfo.action, DTMF_TEXT_REBOOT );
              break;
            case CFG_DTMF_EDITDLG_RB_DEACT:
              strcpy( currDtmf->itemInfo.action, DTMF_TEXT_DEACT );
              break;
            case CFG_DTMF_EDITDLG_RB_QUIT:
              strcpy( currDtmf->itemInfo.action, DTMF_TEXT_QUIT );
              break;
            case CFG_DTMF_EDITDLG_RB_SETNOTIFY:
              strcpy( currDtmf->itemInfo.action, DTMF_TEXT_SETNOTIFY );
              break;
            default:
              WinQueryDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG,
                                   sizeof( currDtmf->itemInfo.action ),
                                   (PSZ) currDtmf->itemInfo.action );

              WinQueryDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG,
                                   sizeof( currDtmf->itemInfo.params ),
                                   (PSZ) currDtmf->itemInfo.params );

              WinQueryDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE,
                                   sizeof( currDtmf->itemInfo.title ),
                                   (PSZ) currDtmf->itemInfo.title );
              break;
          }

          isActive = (char) WinSendMsg( WinWindowFromID( hwnd,
                                    CFG_DTMF_EDITDLG_ACTIVE), BM_QUERYCHECK,
                                    NULL, NULL );

          if( isActive )
          {
            currDtmf->itemInfo.icon      = actionIcoActive;
            strcpy( currDtmf->itemInfo.active, "1" );
          }
          else
          {
            currDtmf->itemInfo.icon = actionIcoInactive;
            strcpy( currDtmf->itemInfo.active, "0" );
          }

          CheckDtmfSetup( currDtmf );

          WinSendMsg( WinWindowFromID( hwndPage4, CFG_NBP4_CNTR ),
                      CM_INVALIDATERECORD, &currDtmf,
                      MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );

          WinDismissDlg( hwnd, TRUE );
          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

MRESULT EXPENTRY addDtmfProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char isActive;
  short index;

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_CODE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );
      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_ACTIVE,
                         BM_SETCHECK, MPFROMSHORT( 1 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_RCONTROL,
                         BM_SETCHECK, MPFROMSHORT( 1 ), NULL );
      break;

    case WM_CONTROL:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_DTMF_EDITDLG_RB_EXEC:
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG ), TRUE );
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG ), TRUE );
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE ), TRUE );
          break;

        case CFG_DTMF_EDITDLG_RB_RCONTROL:
        case CFG_DTMF_EDITDLG_RB_REBOOT:
        case CFG_DTMF_EDITDLG_RB_DEACT:
        case CFG_DTMF_EDITDLG_RB_QUIT:
        case CFG_DTMF_EDITDLG_RB_SETNOTIFY:
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG ), FALSE );
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG ), FALSE );
          WinEnableWindow( WinWindowFromID( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE ), FALSE );
          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

      return 0;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_DTMF_EDITDLG_OK:
        {
          HPOINTER bmp;
          char     code[512], action[512], params[512], title[512], active[512];

          strcpy( code, " " );
          strcpy( params, " " );
          strcpy( title, " " );

          WinQueryDlgItemText( hwnd, CFG_DTMF_EDITDLG_CODE,
                               sizeof( code ), (PSZ) code );

          index = (short) WinSendDlgItemMsg( hwnd, CFG_DTMF_EDITDLG_RB_RCONTROL,
                                     BM_QUERYCHECKINDEX, NULL, NULL );
          index+= CFG_DTMF_EDITDLG_RB_RCONTROL;

          switch( index )
          {
            case CFG_DTMF_EDITDLG_RB_RCONTROL:
              strcpy( action, DTMF_TEXT_RCONTROL );
              break;
            case CFG_DTMF_EDITDLG_RB_REBOOT:
              strcpy( action, DTMF_TEXT_REBOOT );
              break;
            case CFG_DTMF_EDITDLG_RB_DEACT:
              strcpy( action, DTMF_TEXT_DEACT );
              break;
            case CFG_DTMF_EDITDLG_RB_QUIT:
              strcpy( action, DTMF_TEXT_QUIT );
              break;
            case CFG_DTMF_EDITDLG_RB_SETNOTIFY:
              strcpy( action, DTMF_TEXT_SETNOTIFY );
              break;
            default:
              WinQueryDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_PRG,
                                   sizeof( action ), (PSZ) action );

              WinQueryDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_ARG,
                                   sizeof( params ), (PSZ) params );

              WinQueryDlgItemText( hwnd, CFG_DTMF_EDITDLG_EXEC_TITLE,
                                   sizeof( title ), (PSZ) title );
              break;
          }

          isActive = (char) WinSendMsg( WinWindowFromID( hwnd,
                                    CFG_DTMF_EDITDLG_ACTIVE), BM_QUERYCHECK,
                                    NULL, NULL );

          if( isActive )
          {
            bmp = actionIcoActive;
            strcpy( active, "1" );
          }
          else
          {
            bmp = actionIcoInactive;
            strcpy( active, "0" );
          }

          AddItemToDtmfCntr( WinWindowFromID( hwndPage4, CFG_NBP4_CNTR ),
                             bmp, code, action, params, title, "0", active );

          WinDismissDlg( hwnd, TRUE );
          break;
        }

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}

void CheckEAZSetup( EAZSetup *rec )
{
  if( !strlen( strip_blanks(rec->itemInfo.port) ) )
    strcpy( rec->itemInfo.port, "?" );

  if( !strlen( strip_blanks(rec->itemInfo.desc) ) )
    strcpy( rec->itemInfo.desc, "?" );

  if( !strlen( strip_blanks(rec->itemInfo.reject_cause) ) )
    strcpy( rec->itemInfo.reject_cause, REJECT_0 );
}

void CheckPeopleSetup( PeopleSetup *rec )
{
  if( !strlen( strip_blanks(rec->itemInfo.name) ) )
    strcpy( rec->itemInfo.name, "?" );

  if( !strlen( strip_blanks(rec->itemInfo.number) ) )
    strcpy( rec->itemInfo.number, "?" );

  if( !strlen( strip_blanks(rec->itemInfo.reject_cause) ) )
    strcpy( rec->itemInfo.reject_cause, REJECT_0 );
}

void CheckDtmfSetup( DtmfSetup *rec )
{
  if( !strlen( strip_blanks(rec->itemInfo.code) ) )
    strcpy( rec->itemInfo.code, "?" );

  if( !strlen( strip_blanks(rec->itemInfo.action) ) )
    strcpy( rec->itemInfo.action, DTMF_TEXT_RCONTROL );
}

MRESULT EXPENTRY addNewCallerProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char number[512], name[512], alwfile[512], *pt, reject;
  char reject_cause[512], maxrectime[512], answerdelay[512], isActive;
  char ringing[512], notifynum[512];
  HPOINTER bmp;
  FILEDLG fileDlg;
  FILE *fh;

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      WinSendMsg( WinWindowFromID( hwnd, CFG_PEOPLE_EDITDLG_ACTIVE ),
                  BM_SETCHECK, MPFROMSHORT( 1 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_FILE, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_RINGING, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_REJECT, REJECT_D );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_D ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_0 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_1 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_2 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                         LM_INSERTITEM, MPFROMSHORT(LIT_END),
                         MPFROMP( REJECT_3 ) );

      WinSendDlgItemMsg( hwnd, CFG_PEOPLE_EDITDLG_NOTIFY, EM_SETTEXTLIMIT,
                         MPFROMSHORT( 512 ), NULL );

      strcpy( name, currRec->itemInfo.caller );
      pt = strchr( name, '(' );
      if( pt != NULL )
        *(pt-1) = 0;
      WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NUMBER, name );

      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case CFG_PEOPLE_EDITDLG_FILEDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVALWFILE;
          strcpy( fileDlg.szFullFile, "*.WAV;*.ALW" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_FILE,
                               fileDlg.szFullFile );

          break;

        case CFG_PEOPLE_EDITDLG_RINGING_FDLG:
          memset( &fileDlg, 0, sizeof(fileDlg) );

          fileDlg.cbSize     = sizeof( fileDlg );
          fileDlg.fl         = FDS_OPEN_DIALOG | FDS_CENTER;
          fileDlg.pszTitle   = (PSZ) CHOOSEWAVFILE;
          strcpy( fileDlg.szFullFile, "*.WAV" );

          WinFileDlg( HWND_DESKTOP, hwnd, &fileDlg );

          if( fileDlg.lReturn == DID_OK )
            WinSetDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_RINGING,
                               fileDlg.szFullFile );

          break;

        case CFG_PEOPLE_EDITDLG_OK:
          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NUMBER,
                               sizeof( number ), (PSZ) number );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NAME,
                               sizeof( name ), (PSZ) name );

          if( !strlen( strip_blanks( name ) ) )
            strcpy( name, number );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_FILE,
                               sizeof( alwfile ), (PSZ) alwfile );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_MAXRECTIM,
                               sizeof( maxrectime ), (PSZ) maxrectime );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_DELAY,
                               sizeof( answerdelay ), (PSZ) answerdelay );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_REJECT,
                               sizeof( reject_cause ), (PSZ) reject_cause );

          if( strcmpi(reject_cause, REJECT_3) == 0 )       reject = '3';
          else if( strcmpi(reject_cause, REJECT_2) == 0 )  reject = '2';
          else if( strcmpi(reject_cause, REJECT_1) == 0 )  reject = '1';
          else if( strcmpi(reject_cause, REJECT_0) == 0 )  reject = '0';
          else                                             reject = '*';

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_RINGING,
                               sizeof( ringing ), (PSZ) ringing );

          WinQueryDlgItemText( hwnd, CFG_PEOPLE_EDITDLG_NOTIFY,
                               sizeof( notifynum ), (PSZ) notifynum );

          isActive = (char) WinSendMsg( WinWindowFromID( hwnd,
                                    CFG_PEOPLE_EDITDLG_ACTIVE), BM_QUERYCHECK,
                                    NULL, NULL );

          strip_blanks( number );
          strip_blanks( name );
          strip_blanks( alwfile );
          strip_blanks( number );
          strip_blanks( name );
          strip_blanks( alwfile );
          strip_blanks( ringing ),
          strip_blanks( notifynum );

          if( (fh = fopen( NAMFILE, "a" )) != NULL )
          {
            fprintf( fh, "%s|%s|%s|%c|%s|%s|%d|%s\n",
              strlen( number ) ? number : "?",
              strlen( name ) ? name : "?",
              strlen( alwfile ) ? alwfile : "*",
              reject,
              strlen( maxrectime ) ? maxrectime : "*",
              strlen( answerdelay ) ? answerdelay : "*",
              isActive,
              strlen( ringing ) ? ringing : "*",
              strlen( notifynum ) ? notifynum : "*");

            fclose( fh );

            strcpy( currRec->itemInfo.caller, name );

            WinSendMsg( hwndCntr, CM_INVALIDATERECORD, &currRec,
                        MPFROM2SHORT( 1, CMA_TEXTCHANGED ) );

          }

          WinDismissDlg( hwnd, TRUE );
          break;

        default:
          return WinDefDlgProc( hwnd, msg, mp1, mp2 );
      }

      return 0;

    default:
      return WinDefDlgProc( hwnd, msg, mp1, mp2 );
  }

  return 0;
}


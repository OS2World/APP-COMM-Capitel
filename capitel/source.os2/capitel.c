#ifdef LANG_GER
#define ACT_LANG LANGUAGE_GER
#else
#define ACT_LANG LANGUAGE_ENG
#endif

#define INCL_PM
#define INCL_DOS
#define INCL_32
#define INCL_MCIOS2
#define INCL_WINWORKPLACE

#include <os2.h>
#include <os2me.h>
#include <mciapi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <io.h>

#include "capitel.h"
#include "..\..\common\source\version.h"
#include "configex.h"
#include "toolbar.h"
#include "statbar.h"
#include "cntr.h"
#include "config.h"
#include "configrc.h"
// #include "loadmci.h"
#include "regist.h"
#include "sysfile.h"

#include "texte.h"
#include "..\..\answer\source\answer.h"
#include "..\..\common\source\global.h"
#include "..\..\..\units\common.src\cfg_file.h"
#include "..\..\util\source\dosstart.h"
#include "..\..\util\source\register.h"

#define SetAppTitle( str )    WinSetWindowText( hwndFrame, (PSZ) str );

MRESULT EXPENTRY ctelFrameProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY ctelCntrProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY cmdProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY recProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY aboutProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY inCallProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2);
MRESULT EXPENTRY recordInfoProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2);

void MsgBox( char * );
void ErrorBox( char *, HWND );
void sigFunc( short, void * );
void ScanForCalls( void );
void DeleteCall( void );
void LoadBitmaps( HWND );
void centerDialog( HWND, HWND );
void copyFile( char *, char *);
void setHeard( CntrRec * );
void delete_cr( char * );
short queryDimensions( HWND, char *, ULONG *, ULONG * );
short onlyDigits( char * );
char *strip_blanks( char * );
void shExecute( char *, BOOL );

void ctel_disc_ind( long, short );
void ctel_filenum( short );
void ctel_connect_ind( TCapiInfo * );
void ctel_convert_status( short );
void ctel_rescan( void );

int wavplay_start( HWND hwnd, char* wavFile );
int wavplay_stop( short deviceID );
short wavplay_current_deviceID = 0;

int MyMsgBox(HWND hwndParent, HWND hwndOwner,
  char* text, char* title, int style, int icon);
int MyMsgBoxEx(HWND hwndParent, HWND hwndOwner,
  char* text, char* title, char* checktext, int style, int icon,
  char* pCfgFile, char* pCfgKey);
MRESULT EXPENTRY MsgBoxProc(
  HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

PFNWP oldFrameProc, oldCntrProc;
HPOINTER cntrIcoCall, cntrIcoCallWav, cntrIcoCallHeard, cntrIcoCallDigital;
HPOINTER portsIcoActive, portsIcoInactive, callersIcoActive, callersIcoInactive;
HPOINTER actionIcoActive, actionIcoInactive;
TCallInfo CallInfo;
char do_isdn_close = 0, isRecording = 0, wasInActive = 0;
char callPending = 0;
char recFile[256], textmsg[256], defaultBarText[256];
CntrRec *currRec;

char mmpmInstalled = 1;

HWND hwndFrame, hwndCntr, hwndToolbar, hwndStatusbar, hwndPopup;
HWND hwndInCall = 0, hwndRecInfo = 0;

int main( int argc, char *argv[] )
{
  HAB hab;
  HMQ hmq;
  QMSG qmsg;
  FRAMECDATA fcd;
  int i;

  hab = WinInitialize( 0 );
  hmq = WinCreateMsgQueue( hab, 0 );

  registerToolbar( hab );
  registerStatusbar( hab );

  fcd.cb = sizeof( FRAMECDATA );
  fcd.flCreateFlags = FCF_MINMAX | FCF_SIZEBORDER | FCF_SYSMENU |
                      FCF_TITLEBAR | FCF_SHELLPOSITION | FCF_TASKLIST |
                      FCF_MENU | FCF_ICON | FCF_ACCELTABLE | FCF_CLOSEBUTTON;
  fcd.hmodResources = 0;
  fcd.idResources = ID_FRAME;

  hwndFrame = WinCreateWindow( HWND_DESKTOP, WC_FRAME, APPNAME,
                               0, 0, 0, 0, 0, 0, HWND_TOP, ID_FRAME,
                               &fcd, 0 );

  WinShowWindow( hwndFrame, FALSE );

  WinCreateWindow( hwndFrame, "pmToolbar", "", 0, 10, 10, 30, 30,
                   hwndFrame, HWND_TOP, ID_TOOLBAR, 0, 0 );

  WinCreateWindow( hwndFrame, "pmStatusbar", APPNAME, 0, 0, 0, 0, 0,
                   hwndFrame, HWND_TOP, ID_STATUSBAR, 0, 0 );

  hwndCntr = WinCreateWindow( hwndFrame, WC_CONTAINER, 0,
                              CCS_MINIRECORDCORE | CCS_READONLY |
                              CCS_SINGLESEL, 0, 0, 0, 0, hwndFrame,
                              HWND_BOTTOM, FID_CLIENT, 0, 0 );

  oldFrameProc = WinSubclassWindow( hwndFrame, ctelFrameProc );
  oldCntrProc = WinSubclassWindow( hwndCntr, ctelCntrProc );

  hwndToolbar   = WinWindowFromID( hwndFrame, ID_TOOLBAR );
  hwndStatusbar = WinWindowFromID( hwndFrame, ID_STATUSBAR );

  hwndPopup     = WinLoadMenu( hwndCntr, NULLHANDLE, ID_POPUP );

  /* Fake window creation... */
  WinPostMsg( hwndFrame, WM_CREATE, 0, 0 );

  while( WinGetMsg(hab, (PQMSG) &qmsg, 0, 0, 0) )
    WinDispatchMsg(hab, (PQMSG) &qmsg );

  WinSetWindowPos( hwndFrame, 0, 0, 0, 0, 0, SWP_HIDE );
  WinStoreWindowPos( "CapiTel", "WinPos", hwndFrame );

  WinDestroyWindow( hwndFrame );
  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );

  if( do_isdn_close )
  {
    answer_exit();
    while( answer_cannot_close() ) DosSleep (300);
  }

  return 0;
}

MRESULT EXPENTRY ctelFrameProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  MRESULT rc;
  PRECTL prectl;
  SHORT countSwp;
  PSWP  pswp, pswpClient=0, pswpToolbar=0, pswpStatusbar=0;
  int cy, cyStatus;
  SWP  swp, *sav;
  RGB rgb;
  ULONG fl;
  TRACKINFO TrackInfo;

  switch( msg )
  {
    case WM_CREATE:                             /* Faked Window Creation !!! */

      if( !initRegistration() )
      {
/*
        if( !checkSystemFile() )
        {
          MsgBox( VEREXP );

          WinDlgBox( HWND_DESKTOP, hwndFrame, (PFNWP) registrationProc,
                     NULLHANDLE, REG_DIALOG, NULL );

          if( strlen( QueryRegName() ) > 0 )
            MsgBox( RESTART );

          WinPostMsg( hwnd, WM_QUIT, 0, 0 );
          break;
        }
*/

        if (999 == config_file_read_ulong (STD_CFG_FILE,DEFAULT_ANSW_DELAY,DEFAULT_ANSW_DELAY_DEF)) {
          sprintf( defaultBarText, FREETEXT, APPNAME );
        } else {
          sprintf( defaultBarText, UNREGTEXT, APPNAME );
        }
      }
      else
        sprintf( defaultBarText, ISREGTEXT, APPNAME,
                 QueryRegName() );

      LoadBitmaps( hwnd );

      AddButton( IDI_TOGGLE );
      AddSpace( 8 );
      AddButton( ID_CALL_PLAY );
      AddButton( ID_CALL_PLAY_ALL );
      AddButton( ID_CALL_DEL );
      AddSpace( 8 );
      AddButton( IDI_SETUP );
      AddButton( IDI_PORTS );
      AddButton( IDI_PEOPLE );
      AddButton( IDI_DTMF );

      InitContainer( );

      if( !(fl = config_file_read_ulong( STD_CFG_FILE, CAPITEL_RUN_CNT, CAPITEL_RUN_CNT_DEF )) )
      {
        config_file_write_ulong( STD_CFG_FILE, CAPITEL_RUN_CNT, 1 );
        WinPostMsg( hwnd, WM_COMMAND, MPFROM2SHORT( IDI_SETUP, 0 ), NULL );
      }
      else
      {
        config_file_write_ulong( STD_CFG_FILE, CAPITEL_RUN_CNT, ++fl );
        if( config_file_read_ulong( STD_CFG_FILE, WINDOW_FRAMECTRLS_HIDDEN, WINDOW_FRAMECTRLS_HIDDEN_DEF ) )
        {
          config_file_write_ulong( STD_CFG_FILE, WINDOW_FRAMECTRLS_HIDDEN, 0 );
          WinPostMsg( hwndFrame, WM_COMMAND, MPFROM2SHORT( IDI_HIDE, 0 ), 0 );
        }
        else
        {
          WinSendMsg( WinWindowFromID( hwnd, FID_MENU ), MM_SETITEMATTR,
                      MPFROM2SHORT( IDI_HIDE, TRUE ),
                      MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );

          WinSendMsg( hwndPopup, MM_SETITEMATTR,
                      MPFROM2SHORT( IDI_HIDE, TRUE ),
                      MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );
        }
      }

      if( !WinRestoreWindowPos( "CapiTel", "WinPos", hwndFrame ) )
      {
        /* Set some defaults values */
        rgb.bBlue  = 254;
        rgb.bGreen = 254;
        rgb.bRed   = 254;
        WinSetPresParam( hwndCntr, PP_BACKGROUNDCOLOR, sizeof( RGB ), &rgb );
        WinSetPresParam( hwndCntr, PP_FONTNAMESIZE, sizeof( DefCntrFont ),
                         DefCntrFont );

        WinSetWindowPos( hwndFrame, 0, 15, 15, 450, 300,
                         SWP_MOVE | SWP_SIZE | SWP_SHOW );
      }
      else
        WinSetWindowPos( hwndFrame, HWND_TOP, 0, 0, 0, 0, SWP_SHOW |
                         SWP_ACTIVATE | SWP_ZORDER );

      WinSendMsg( WinWindowFromID( hwnd, FID_MENU ), MM_SETITEMATTR,
                  MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                  MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );

      WinSendMsg( hwndPopup, MM_SETITEMATTR,
                  MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                  MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );

//      SetStatus( "Initializing MMPM..." );
//      LoadMMPM();

      SetStatus( INITISDN );
      if( answer_init( sigFunc, 1 , ACT_LANG) )
        break;

      answer_listen();

      do_isdn_close = 1;

      SetStatus( SCANCALLS );
      ScanForCalls();

      SetStatus( defaultBarText );

      break;

    case WM_CALCFRAMERECT:
      rc = ( *oldFrameProc )( hwnd, msg, mp1, mp2 );

      if( rc && mp2 )
      {
        if( config_file_read_ulong( STD_CFG_FILE, WINDOW_FRAMECTRLS_HIDDEN, WINDOW_FRAMECTRLS_HIDDEN_DEF) )
        {
          cy = 0;
          cyStatus = 0;
        }
        else
        {
          cy = 30;
          cyStatus = 22;
        }
        prectl = (PRECTL) mp1;
        prectl->yTop -= (cy+cyStatus);
        prectl->yBottom += cyStatus;
      }

      return rc;

    case WM_FORMATFRAME:
      countSwp = (int) (*oldFrameProc)(hwnd, msg, mp1, mp2);
      pswp=(PSWP)mp1;

      if( config_file_read_ulong( STD_CFG_FILE, WINDOW_FRAMECTRLS_HIDDEN, WINDOW_FRAMECTRLS_HIDDEN_DEF ) )
      {
        cy = 0;
        cyStatus = 0;
      }
      else
      {
        cy = 30;
        cyStatus = 22;
      }

      sav=pswpToolbar=&pswp[countSwp-1];
      pswpStatusbar=&pswp[countSwp];
      pswpClient=&pswp[countSwp+1];

      *pswpStatusbar = *sav;
      *pswpClient = *sav;

      pswpClient->y  += cyStatus;
      pswpClient->cy -= cyStatus+cy;

      pswpToolbar->hwnd=WinWindowFromID(hwnd, ID_TOOLBAR);
      pswpToolbar->y  = (pswpClient->y+pswpClient->cy);
      pswpToolbar->cy = cy;

      pswpStatusbar->hwnd=WinWindowFromID(hwnd, ID_STATUSBAR);
      pswpStatusbar->y  = pswpClient->y - cyStatus;
      pswpStatusbar->cy = cyStatus;

      return MRFROMSHORT( countSwp+2 );

    case WM_QUERYFRAMECTLCOUNT:
      return MPFROMSHORT( (ULONG)((*oldFrameProc)(hwnd, msg, mp1, mp2)) +2 );

    case WM_WINDOWPOSCHANGED:
      pswp = (PSWP) PVOIDFROMMP( mp1 );
      if( pswp->fl & SWP_MINIMIZE )
        memcpy( &oldSWP, ++pswp, sizeof( SWP ) );
      return ( *oldFrameProc )( hwnd, msg, mp1, mp2 );

    case WM_TRACKFRAME:
      if( !(WinGetKeyState( HWND_DESKTOP, VK_BUTTON2 ) & 0x8000) )
        return ( *oldFrameProc )( hwnd, msg, mp1, mp2 );

      WinSetFocus( HWND_DESKTOP, hwnd );
      WinQueryWindowPos( hwnd, &swp );

      if( swp.fl & SWP_MINIMIZE )
        return ( *oldFrameProc )( hwnd, msg, mp1, mp2 );

      memset ( &TrackInfo, 0, sizeof(TrackInfo) ) ;

      TrackInfo.cxBorder = 2 ;
      TrackInfo.cyBorder = 2 ;
      TrackInfo.cxGrid = 2 ;
      TrackInfo.cyGrid = 2 ;
      TrackInfo.cxKeyboard = 8 ;
      TrackInfo.cyKeyboard = 8 ;

      WinQueryWindowPos ( hwnd, &swp ) ;
      TrackInfo.rclTrack.xLeft   = swp.x ;
      TrackInfo.rclTrack.xRight  = swp.x + swp.cx ;
      TrackInfo.rclTrack.yBottom = swp.y ;
      TrackInfo.rclTrack.yTop    = swp.y + swp.cy ;

      WinQueryWindowPos ( HWND_DESKTOP, &swp ) ;
      TrackInfo.rclBoundary.xLeft   = swp.x ;
      TrackInfo.rclBoundary.xRight  = swp.x + swp.cx ;
      TrackInfo.rclBoundary.yBottom = swp.y ;
      TrackInfo.rclBoundary.yTop    = swp.y + swp.cy ;

      TrackInfo.ptlMinTrackSize.x = 0 ;
      TrackInfo.ptlMinTrackSize.y = 0 ;
      TrackInfo.ptlMaxTrackSize.x = swp.cx ;
      TrackInfo.ptlMaxTrackSize.y = swp.cy ;

      TrackInfo.fs = TF_MOVE | TF_STANDARD | TF_ALLINBOUNDARY ;

      if ( WinTrackRect ( HWND_DESKTOP, (HPS)NULL, &TrackInfo ) )
      {
        WinSetWindowPos ( hwnd, 0, TrackInfo.rclTrack.xLeft,
                          TrackInfo.rclTrack.yBottom, 0, 0, SWP_MOVE ) ;
      }

      return MPFROMSHORT( 1 );

    case WM_COMMAND:
    case WM_CONTROL:
      return cmdProc( hwnd, msg, mp1, mp2 );

    case WM_INITMENU:
      if( SHORT1FROMMP( mp1 ) != IDS_CALL )
        break;

      currRec = (CntrRec *) WinSendMsg( hwndCntr, CM_QUERYRECORDEMPHASIS,
                            MPFROMSHORT( CMA_FIRST ),
                            MPFROMSHORT( CRA_SELECTED ) );

      WinEnableMenuItem( (HWND) mp2, ID_CALL_PLAY, currRec && currRec->itemInfo.seconds );
      WinEnableMenuItem( (HWND) mp2, ID_CALL_DEL, currRec );
      WinEnableMenuItem( (HWND) mp2, ID_ADD_CALLER, currRec && onlyDigits( currRec->itemInfo.caller ) );

      break;

    case WM_MSGBOXDISMISS:
      WinDestroyWindow( (HWND) mp1 );
      break;

    case UM_WARNING:
      ErrorBox( WARNING, hwnd );
      break;

    case UM_CRITICAL:
      ErrorBox( CRITICAL, hwnd );
      break;

    case UM_FATAL:
      ErrorBox( FATAL, hwnd );
      break;

    case UM_PLAYWAV:

      if(wavplay_current_deviceID) wavplay_stop(wavplay_current_deviceID);
      remove("TEMP.WAV");
      wavplay_current_deviceID = wavplay_start(hwnd, textmsg);

      break;

    case MM_MCINOTIFY:

      if(SHORT1FROMMP(mp1) != MCI_NOTIFY_SUCCESSFUL || SHORT1FROMMP(mp2) != wavplay_current_deviceID) break;

      wavplay_stop(wavplay_current_deviceID);
      wavplay_current_deviceID = 0;
      remove("TEMP.WAV");

      break;

    default:
      return ( *oldFrameProc )( hwnd, msg, mp1, mp2 );
  }

  return FALSE;
}


MRESULT EXPENTRY ctelCntrProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{

  switch( msg )
  {
    case WM_CLOSE:

      if( MyMsgBoxEx( HWND_DESKTOP, hwndFrame,
                      REALYEXIT, CTELMSG,  REALYEXIT_CHK,
                      MB_YESNO, SPTR_ICONQUESTION,
                      STD_CFG_FILE, CONFIRM_EXIT_PROGRAM ) != MBID_YES ) 
        return 0;

      break;
  }

  return ( *oldCntrProc )( hwnd, msg, mp1, mp2 );
}


MRESULT EXPENTRY cmdProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  POINTL ptlMouse;
  RECTL Rectangle;
  FRAMECDATA frameData;
  short i;

  switch( msg )
  {
    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case IDI_EXIT:
          WinPostMsg( WinWindowFromID(hwnd,FID_CLIENT), WM_CLOSE, 0, 0 );
          break;

        case IDI_SETUP:
        case IDI_PORTS:
        case IDI_PEOPLE:
        case IDI_DTMF:
          if( callPending || isRecording )
            break;

          i = SHORT1FROMMP( mp1 );

          WinDlgBox( HWND_DESKTOP, hwndFrame, (PFNWP) configProc,
                     NULLHANDLE, CFG_DIALOG, (PVOID) &i );

          if( config_file_read_ulong( STD_CFG_FILE, CAPITEL_ACTIVE, CAPITEL_ACTIVE_DEF ) )
            answer_listen();

          break;

        case IDI_INFO:
          WinDlgBox( HWND_DESKTOP, hwndFrame, (PFNWP) aboutProc,
                     NULLHANDLE, ABOUT_DIALOG, NULL );

          break;

        case IDI_README:   shExecute( EXEC_README, TRUE );  break;
        case IDI_ORDER:    shExecute( EXEC_ORDER, TRUE );  break;
        case IDI_ORDERBMT: shExecute( EXEC_ORDERBMT, TRUE );  break;
        case IDI_LICENSE:  shExecute( EXEC_LICENSE, TRUE );  break;
        case IDI_WHATSNEW: shExecute( EXEC_WHATSNEW, TRUE );  break;
        case IDI_HOMEPAGE: shExecute( APP_HOMEPAGE, FALSE );  break;

        case IDI_REGISTRATION:
          WinDlgBox( HWND_DESKTOP, hwndFrame, (PFNWP) registrationProc,
                     NULLHANDLE, REG_DIALOG, NULL );

          if( strlen( QueryRegName() ) > 0 )
          {
            sprintf( defaultBarText, ISREGTEXT, APPNAME,
                     QueryRegName() );
            SetStatus( defaultBarText );
          }

          break;

        case IDI_TOGGLE:
          if( callPending || isRecording )
            break;

          if( config_file_read_ulong( STD_CFG_FILE, CAPITEL_ACTIVE, CAPITEL_ACTIVE_DEF ) )
          {
//            SetBitmap( IDI_TOGGLE, ID_BMP_ONOFF0 );
            WinSendMsg( WinWindowFromID( hwnd, FID_MENU ), MM_SETITEMATTR,
                        MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                        MPFROM2SHORT( MIA_CHECKED, FALSE ) );

            WinSendMsg( hwndPopup, MM_SETITEMATTR,
                        MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                        MPFROM2SHORT( MIA_CHECKED, FALSE ) );

            SetStatus( DEACTMSG );

            config_file_write_ulong( STD_CFG_FILE, CAPITEL_ACTIVE, 0 );
          }
          else
          {
//            SetBitmap( IDI_TOGGLE, IDI_TOGGLE );

            WinSendMsg( WinWindowFromID( hwnd, FID_MENU ), MM_SETITEMATTR,
                        MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                        MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );

            WinSendMsg( hwndPopup, MM_SETITEMATTR,
                        MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                        MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );

            SetStatus( defaultBarText );

            config_file_write_ulong( STD_CFG_FILE, CAPITEL_ACTIVE, 1 );
          }

          break;

        case IDI_HIDE:
          if( config_file_read_ulong( STD_CFG_FILE, WINDOW_FRAMECTRLS_HIDDEN, WINDOW_FRAMECTRLS_HIDDEN_DEF ) )
          {
            config_file_write_ulong( STD_CFG_FILE, WINDOW_FRAMECTRLS_HIDDEN, 0 );
            frameData.cb = sizeof( FRAMECDATA );
            frameData.flCreateFlags = FCF_TITLEBAR | FCF_SYSMENU |
                                      FCF_MENU | FCF_MINMAX;
            frameData.hmodResources = 0;
            frameData.idResources   = ID_FRAME;
            WinCreateFrameControls( hwnd, &frameData, APPNAME );
            WinSendMsg( WinWindowFromID(hwnd, FID_TITLEBAR), TBM_SETHILITE,
                        MPFROMSHORT( TRUE ), 0 );

            WinSendMsg( WinWindowFromID( hwnd, FID_MENU ), MM_SETITEMATTR,
                        MPFROM2SHORT( IDI_HIDE, TRUE ),
                        MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );

            WinSendMsg( hwndPopup, MM_SETITEMATTR,
                        MPFROM2SHORT( IDI_HIDE, TRUE ),
                        MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );

            if( config_file_read_ulong( STD_CFG_FILE, CAPITEL_ACTIVE, CAPITEL_ACTIVE_DEF ) )
            {
              WinSendMsg( WinWindowFromID( hwnd, FID_MENU ), MM_SETITEMATTR,
                          MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                          MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );

              WinSendMsg( hwndPopup, MM_SETITEMATTR,
                          MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                          MPFROM2SHORT( MIA_CHECKED, MIA_CHECKED ) );
            }
            else
            {
              WinSendMsg( WinWindowFromID( hwnd, FID_MENU ), MM_SETITEMATTR,
                          MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                          MPFROM2SHORT( MIA_CHECKED, FALSE ) );

              WinSendMsg( hwndPopup, MM_SETITEMATTR,
                          MPFROM2SHORT( IDI_TOGGLE, TRUE ),
                          MPFROM2SHORT( MIA_CHECKED, FALSE ) );
            }

          }
          else
          {
            config_file_write_ulong( STD_CFG_FILE, WINDOW_FRAMECTRLS_HIDDEN, 1);
            WinDestroyWindow( WinWindowFromID(hwnd, FID_SYSMENU) );
            WinDestroyWindow( WinWindowFromID(hwnd, FID_TITLEBAR) );
            WinDestroyWindow( WinWindowFromID(hwnd, FID_MINMAX) );
            WinDestroyWindow( WinWindowFromID(hwnd, FID_MENU) );

            WinSendMsg( hwndPopup, MM_SETITEMATTR,
                        MPFROM2SHORT( IDI_HIDE, TRUE ),
                        MPFROM2SHORT( MIA_CHECKED, FALSE ) );
          }

          WinSendMsg( hwndFrame, WM_UPDATEFRAME,
                      MPFROMSHORT( FCF_TITLEBAR|FCF_SYSMENU|FCF_MENU|
                      FCF_MINMAX|ID_TOOLBAR ), 0 );

          WinSendMsg( hwndToolbar, TB_REDRAW, 0, 0 );

          break;

        case ID_CALL_PLAY:
          currRec = (CntrRec *) WinSendMsg( hwndCntr, CM_QUERYRECORDEMPHASIS,
                                MPFROMSHORT( CMA_FIRST ),
                                MPFROMSHORT( CRA_SELECTED ) );
          /* automagically jump to the real play-function */

        case ID_POPUP_PLAY:
          if( (currRec == NULL) || (!mmpmInstalled) ||
              !(currRec->itemInfo.seconds) )
            break;

          answer_stop_bell();
          if(wavplay_current_deviceID) wavplay_stop(wavplay_current_deviceID);
          copyFile( currRec->itemInfo.filename, "TEMP.WAV" );
          wavplay_current_deviceID = wavplay_start(hwnd, "TEMP.WAV");
          if(wavplay_current_deviceID) setHeard( currRec );
          break;

        case ID_CALL_PLAY_ALL:
        case ID_POPUP_PLAY_ALL:
          answer_stop_bell();
          answer_play_all();
          break;

        case ID_ADD_CALLER:
          currRec = (CntrRec *) WinSendMsg( hwndCntr, CM_QUERYRECORDEMPHASIS,
                                MPFROMSHORT( CMA_FIRST ),
                                MPFROMSHORT( CRA_SELECTED ) );
          /* automagically jump to the real add-function */

        case ID_POPUP_ADD_CALLER:
          if( !currRec || !onlyDigits( currRec->itemInfo.caller ) )
            break;

          WinDlgBox( HWND_DESKTOP, hwnd, (PFNWP) addNewCallerProc,
          NULLHANDLE, CFG_PEOPLE_EDITDLG, NULL );

          break;

        case ID_CALL_DEL:
          currRec = (CntrRec *) WinSendMsg( hwndCntr, CM_QUERYRECORDEMPHASIS,
                                MPFROMSHORT( CMA_FIRST ),
                                MPFROMSHORT( CRA_SELECTED ) );
          /* automagically jump to the real delete-function */

        case ID_POPUP_DEL:
          answer_stop_bell();
          if( currRec == NULL )
            break;

          if( !config_file_read_ulong( STD_CFG_FILE, CONFIRM_CALL_DELETE, CONFIRM_CALL_DELETE_DEF ) )
          {
            DeleteCall();
            break;
          }

          if( MyMsgBox( HWND_DESKTOP, hwndFrame,
                        DELCALL,
                        CTELMSG, MB_YESNO, SPTR_ICONQUESTION )
              == MBID_YES )

            DeleteCall();

          break;

        case ID_CALL_REC:
          if( callPending || isRecording )
            break;

          if( WinDlgBox( HWND_DESKTOP, hwndFrame, (PFNWP) recProc,
                         NULLHANDLE, ID_RECDLG, NULL ) )
            break;

          isRecording = 1;
          answer_record_welcome( recFile );
          hwndRecInfo = WinLoadDlg( HWND_DESKTOP, hwndFrame,
                                    (PFNWP) recordInfoProc,
                                    NULLHANDLE, RECORD_DIALOG, NULL );
          break;

        case ID_INCALL_START:
        {
          int iPopFlag;

          if(hwndInCall) break;

          iPopFlag = config_file_read_ulong(STD_CFG_FILE,
            SHOW_POPUP_WINDOW, SHOW_POPUP_WINDOW_DEF);
          if(!iPopFlag) break;

          hwndInCall = WinLoadDlg(HWND_DESKTOP,
            hwndFrame, (PFNWP)inCallProc, 0, INCALL_DIALOG, &iPopFlag);

          break;
        }

        case ID_INCALL_STOP:

          if(hwndInCall)
          {
            WinDestroyWindow(hwndInCall);
            hwndInCall = 0;
          }
          break;

        case ID_RECINFO_STOP:
          WinDestroyWindow( hwndRecInfo );
          break;
      }

      break;

    case WM_CONTROL:
      switch( SHORT2FROMMP( mp1 ) )
      {
        case CN_ENTER:
          if( (currRec = (CntrRec *) ((PNOTIFYRECORDENTER) mp2)->pRecord)
                == NULL )
            break;

          WinPostMsg( hwnd, WM_COMMAND, MPFROMSHORT( ID_POPUP_PLAY ), NULL );
          break;

        case CN_CONTEXTMENU:
          currRec = (CntrRec *) PVOIDFROMMP( mp2 );

          if( !currRec )
          {
            BOOL fEnable = FALSE;            /* wegen compiler warnings ;-( */

            /* Alles abschalten, weil kein richtiger Eintrag angewaehlt */
            WinEnableMenuItem( hwndPopup, ID_POPUP_PLAY, fEnable );
            WinEnableMenuItem( hwndPopup, ID_POPUP_DEL, fEnable );
            WinEnableMenuItem( hwndPopup, ID_POPUP_ADD_CALLER, fEnable );
          }
          else
          {
            BOOL fEnable = TRUE;             /* wegen compiler warnings ;-( */

            /* Delete Call geht immer */
            WinEnableMenuItem( hwndPopup, ID_POPUP_DEL, fEnable );

            /* Abspielbar ? */
            WinEnableMenuItem( hwndPopup, ID_POPUP_PLAY, (currRec->itemInfo.seconds)?TRUE:FALSE );

            /* Add moeglich? */
            WinEnableMenuItem( hwndPopup, ID_POPUP_ADD_CALLER, onlyDigits( currRec->itemInfo.caller )?TRUE:FALSE );
          }

          WinQueryPointerPos( HWND_DESKTOP, &ptlMouse );
          WinMapWindowPoints( HWND_DESKTOP, hwndCntr, &ptlMouse, 1 );

          WinPopupMenu( hwndCntr, hwnd, hwndPopup, ptlMouse.x,
                        ptlMouse.y, 0, PU_HCONSTRAIN | PU_VCONSTRAIN |
                        PU_KEYBOARD | PU_MOUSEBUTTON1 | PU_MOUSEBUTTON2 |
                        PU_NONE );

          break;
      }
      break;
  }

  return FALSE;
}

void MsgBox( char *message )
{
  MyMsgBox( HWND_DESKTOP, hwndFrame,
            (PSZ) message,
            (PSZ) CTELMSG,
            MB_OK, SPTR_ICONERROR );
  return;
}

void ErrorBox( char *title, HWND hwnd )
{
  MB2INFO boxInfo;

  boxInfo.cb         = sizeof( boxInfo );
  boxInfo.cButtons   = 1;
  boxInfo.flStyle    = MB_NONMODAL | MB_ERROR | MB_MOVEABLE;
  boxInfo.hwndNotify = hwnd;

  strcpy( boxInfo.mb2d[0].achText, OKMSG );
  boxInfo.mb2d[0].idButton = 0;
  boxInfo.mb2d[0].flStyle = BS_DEFAULT;

  WinMessageBox2( HWND_DESKTOP, hwnd, textmsg, title,
                  0, (PMB2INFO) &boxInfo );
}

void sigFunc( short num, void *msg )
{
  HAB hab;
  HMQ hmq;

  hab = WinInitialize( 0 );
  hmq = WinCreateMsgQueue( hab, 0 );

  switch( num )
  {
    case 1:                            /* Warning Message                    */
      strcpy( textmsg, msg );
      WinSendMsg( hwndFrame, UM_WARNING, 0, 0 );
      break;

    case 2:                            /* Critical Error                     */
      strcpy( textmsg, msg );
      WinSendMsg( hwndFrame, UM_CRITICAL, 0, 0 );
      break;

    case 3:                            /* Fatal Error                        */
      MsgBox( msg );
      WinPostMsg( hwndFrame, WM_QUIT, 0, 0 );
      break;

    case 4:                            /* Incoming Call                      */
      ctel_connect_ind( (TCapiInfo *) msg );
      break;

    case 5:                            /* FileName of incoming call          */
      ctel_filenum( *((short *) msg) );
      break;

    case 6:                            /* Connection closed                  */
      ctel_disc_ind( *((long *) msg), 1 );
      break;

    case 7:                            /* Idle Msg while converting          */
      ctel_convert_status( *((short *) msg) );
      break;

    case 8:                            /* Rescan directory                   */
      ctel_rescan();
      break;

    case 9:                            /* Connection closed (no entry)       */
      ctel_disc_ind( *((long *) msg), 0 );
      break;

    case 10:                           /* Play .WAV File                     */
      strcpy( textmsg, msg );
      WinSendMsg( hwndFrame, UM_PLAYWAV, 0, 0 );
      break;

    case 11:                           /* Deactivate CapiTel                 */
      SetStatus( DEACTMSG );
      config_file_write_ulong( STD_CFG_FILE, CAPITEL_ACTIVE, 0 );
      break;

    case 12:                           /* Quit CapiTel                       */
      WinPostMsg( hwndFrame, WM_QUIT, 0, 0 );
      break;

  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
}

void ctel_connect_ind( TCapiInfo *msg )
{
  struct tm *tmbuf;
  time_t tod;
  char str[150], *number, *pt;
  int i;
  HAB hab;
  HMQ hmq;
  SWP swp;

  callPending = 1;

  time( &tod );
  tmbuf = localtime( &tod );

  while (tmbuf->tm_year >= 100) tmbuf->tm_year -= 100;

  sprintf( CallInfo.date, "%02d.%02d.%02d", tmbuf->tm_mday,
                                            tmbuf->tm_mon+1, tmbuf->tm_year );
  sprintf( CallInfo.time, "%02d:%02d:%02d", tmbuf->tm_hour,
                                            tmbuf->tm_min, tmbuf->tm_sec );

  strcpy( CallInfo.calledEAZ, msg->called_name );
  strcpy( CallInfo.caller, msg->caller_name );
  CallInfo.digital = msg->is_digital;

  hab = WinInitialize( 0 );
  hmq = WinCreateMsgQueue( hab, 0 );

  if( !isRecording )
    WinSendMsg( hwndFrame, WM_COMMAND, MPFROM2SHORT( ID_INCALL_START, 0 ), 0 );
  else
    WinSetDlgItemText( hwndRecInfo, RECORD_STATUS, (PSZ)"" );

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
}

void ctel_filenum( short num )
{
  sprintf( CallInfo.filename, "CALL%04d.WAV", num );
}

void ctel_disc_ind( long secs, short addToContainer )
{
  HAB hab;
  HMQ hmq;
  FILE *fh;
  char str[256];
  int i;
  HPOINTER bmp;
  SWP swp;

  hab = WinInitialize( 0 );
  hmq = WinCreateMsgQueue( hab, 0 );

  if( isRecording )
  {
    isRecording = 0;

    WinSendMsg( hwndFrame, WM_COMMAND,
                MPFROM2SHORT( ID_RECINFO_STOP, 0 ), 0 );

    if( config_file_read_ulong( STD_CFG_FILE, CAPITEL_ACTIVE, CAPITEL_ACTIVE_DEF ) )
    {
      SetStatus( defaultBarText );
    }
    else
    {
      SetStatus( DEACTMSG );
    }

    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );
    callPending = 0;
    return;
  }

  if (CallInfo.digital)
  {
    CallInfo.seconds = 0;
    bmp = cntrIcoCallDigital;
  } else {
    switch( secs )
    {
      case -1:
      case  0:
        CallInfo.seconds = 0;
        bmp = cntrIcoCall;
        break;
      default:
        CallInfo.seconds = secs;
        bmp = cntrIcoCallWav;
        break;
    }
  }

  if( config_file_read_ulong( STD_CFG_FILE, CAPITEL_ACTIVE, CAPITEL_ACTIVE_DEF ) )
  {
    SetStatus( defaultBarText );
  }
  else
  {
    SetStatus( DEACTMSG );
  }

  WinSendMsg( hwndFrame, WM_COMMAND, MPFROM2SHORT( ID_INCALL_STOP, 0 ), 0 );

  /* Ignore empty call?? */
  if( (config_file_read_ulong( STD_CFG_FILE, IGNORE_EMPTY_CALLS, IGNORE_EMPTY_CALLS_DEF )) && (CallInfo.seconds == 0) )
  {
    remove( CallInfo.filename );
  }
  else
  {
    if( config_file_read_ulong( STD_CFG_FILE, RESTORE_WINDOW_ON_NEW_CALL, RESTORE_WINDOW_ON_NEW_CALL_DEF ) )
    {
      WinQueryWindowPos( hwndFrame, &swp );
      if( swp.fl & SWP_MINIMIZE )
        WinSetWindowPos( hwndFrame, 0, 0, 0, 0, 0, SWP_RESTORE );
    }

    if( addToContainer )
    {
      AddItem( bmp, CallInfo.caller, CallInfo.date, CallInfo.time,
               CallInfo.seconds, CallInfo.calledEAZ, CallInfo.filename, CallInfo.digital );

      strcpy( str, CallInfo.filename );

      for( i=0 ; (str[i]!='.')&&(i<strlen(str)) ; i++ )
        ;
      str[i] = 0;

      strcat( str, ".IDX" );

      fh = fopen( str, "w" );
      fprintf( fh, "%s\n%s\n%s\n%ld\n%s\n%s\n0\n%d\n", CallInfo.caller, CallInfo.date,
               CallInfo.time, CallInfo.seconds, CallInfo.calledEAZ,
               CallInfo.filename, CallInfo.digital );
      fclose( fh );
    }
    else    /* Fernabfrage */
      ctel_rescan();
  }

  callPending = 0;

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
}

void ctel_convert_status( short status )
{
  HAB hab;
  HMQ hmq;

  hab = WinInitialize( 0 );
  hmq = WinCreateMsgQueue( hab, 0 );

  if( !status )
  {
    SetStatus( defaultBarText );
  }
  else
  {
    SetStatus( DISCONV );
  }

  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );
}

void ctel_rescan( void )
{
  EmptyContainer();
  ScanForCalls();
}


void ScanForCalls( void )
{
  APIRET rc;
  HDIR hdirFindHandle = HDIR_CREATE;
  FILEFINDBUF3 FindBuffer = {0};  ULONG ulResultBufLen = sizeof( FILEFINDBUF3 );
  ULONG ulFindCount = 1;
  TCallInfo cInfo;
  char str[256];
  HPOINTER bmp;

  FILE *fh;

  rc = DosFindFirst( "*.IDX", &hdirFindHandle, FILE_NORMAL,
                     &FindBuffer, ulResultBufLen, &ulFindCount,
                     FIL_STANDARD );

  while( !rc )
  {
    fh = fopen( FindBuffer.achName, "r" );

    fgets( cInfo.caller, sizeof( cInfo.caller ), fh );
    delete_cr( cInfo.caller );

    fgets( cInfo.date, sizeof( cInfo.date ), fh );
    delete_cr( cInfo.date );

    fgets( cInfo.time, sizeof( cInfo.time ), fh );
    delete_cr( cInfo.time );

    fgets( str, sizeof( str ), fh );
    delete_cr( str );
    cInfo.seconds = (ULONG) atoi( str );

    fgets( cInfo.calledEAZ, sizeof( cInfo.calledEAZ ), fh );
    delete_cr( cInfo.calledEAZ );

    fgets( cInfo.filename, sizeof( cInfo.filename ), fh );
    delete_cr( cInfo.filename );

    fgets( str, sizeof( str ), fh );
    delete_cr( str );
    cInfo.heard = (USHORT) atoi( str );

    fgets( str, sizeof( str ), fh );
    delete_cr( str );
    cInfo.digital = (CHAR) atoi( str );

    if (cInfo.digital)
    {
      bmp = cntrIcoCallDigital;
    } else {
      if( cInfo.seconds == 0 )
        bmp = cntrIcoCall;
      else
      {
        if( cInfo.heard )
          bmp = cntrIcoCallHeard;
        else
          bmp = cntrIcoCallWav;
      }
    }

    AddItem( bmp, cInfo.caller, cInfo.date, cInfo.time,
             cInfo.seconds, cInfo.calledEAZ, cInfo.filename, CallInfo.digital );

    fclose( fh );

    rc = DosFindNext( hdirFindHandle, &FindBuffer, ulResultBufLen,
                      &ulFindCount );
  }

  DosFindClose( hdirFindHandle );
}

void DeleteCall( void )
{
  char base[256];
  int i;

  strcpy( base, currRec->itemInfo.filename );

  remove( base );                         /* remove .wav file */

  for( i=0 ; (base[i]!='.')&&(i<strlen(base)) ; i++ )
    ;
  base[i] = 0;

  strcat( base, ".IDX" );

  remove( base );                      /* remove .idx file */

  for( i=0 ; (base[i]!='.')&&(i<strlen(base)) ; i++ )
    ;
  base[i] = 0;

  strcat( base, ".ALW" );

  remove( base );                      /* remove .alw file */

  WinSendMsg( hwndCntr, CM_REMOVERECORD, MPFROMP( &currRec ),
              MPFROM2SHORT( 1, CMA_FREE | CMA_INVALIDATE ) );
}

void LoadBitmaps( HWND hwnd )
{
  cntrIcoCall        = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_CALL0 );
  cntrIcoCallWav     = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_CALL1 );
  cntrIcoCallHeard   = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_CALL2 );
  cntrIcoCallDigital = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_CALL3 );

  portsIcoActive     = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_PORT0 );
  portsIcoInactive   = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_PORT1 );

  callersIcoActive   = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_CALLER0 );
  callersIcoInactive = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_CALLER1 );

  actionIcoActive    = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_ACTION0 );
  actionIcoInactive  = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, ID_BMP_ACTION1 );
}

MRESULT EXPENTRY aboutProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
  char str[100], str2[100];

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );

      sprintf( str, "%s", APPVER_FULL );
      WinSetDlgItemText( hwnd, ABOUT_VERSION, str );

      if( queryRegistration( str2, str ) )
      {
        sprintf( str, REGTOMSG, str2 );
        WinSetDlgItemText( hwnd, ABOUT_REG, str );
      }

      break;

    default:
      return( WinDefDlgProc( hwnd, msg, mp1, mp2 ) );
  }

  return 0;
}

MRESULT EXPENTRY recProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char str[255];

  strcpy( str, DEFALWFILE );

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );
      WinSetDlgItemText( hwnd, ID_RECDLG_ENTRY, str );
      WinSendMsg( WinWindowFromID( hwnd, ID_RECDLG_ENTRY ), EM_SETTEXTLIMIT,
                  MPFROMSHORT( 255 ), NULL );
      WinSendMsg( WinWindowFromID( hwnd, ID_RECDLG_ENTRY ), EM_SETSEL,
                  MPFROM2SHORT( 0, 255 ), NULL );
      break;
    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case ID_RECDLG_OK:
          WinQueryDlgItemText( hwnd, ID_RECDLG_ENTRY,
                               sizeof( str ), (PSZ) str );
          if( !strlen( str ) )
            strcpy( recFile, DEFALWFILE );
          else
            strcpy( recFile, str );

          WinDismissDlg( hwnd, FALSE );
          break;
        case ID_RECDLG_CANCEL:
          WinDismissDlg( hwnd, TRUE );
          break;
        default:
          return( WinDefDlgProc( hwnd, msg, mp1, mp2 ) );
      }
      break;

    default:
      return( WinDefDlgProc( hwnd, msg, mp1, mp2 ) );
  }

  return 0;
}

MRESULT EXPENTRY inCallProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
  switch( msg )
  {
    case WM_CONTROL:
    case WM_COMMAND:
      return 0;

    case WM_INITDLG:
    {
      SWP swp;
      char str[100];
      int cxScreen, cyScreen;

      sprintf( str, FROM_STR, CallInfo.caller );
      WinSetDlgItemText( hwnd, INCALL_FROM, str );

      sprintf( str, ON_STR, CallInfo.calledEAZ );
      WinSetDlgItemText( hwnd, INCALL_ON, str );

      /* Set position and size */

      WinQueryWindowPos( hwndFrame, &swp );
      if( swp.fl & SWP_MINIMIZE )
      {
        cxScreen = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
        cyScreen = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
        WinQueryWindowPos( hwnd, &swp );

        switch(*(int*)mp2)
        {
          case 1: // top left
            WinSetWindowPos( hwnd, HWND_TOP, 0, cyScreen - swp.cy, 0, 0, SWP_MOVE );
            break;
          case 2: // top right
            WinSetWindowPos( hwnd, HWND_TOP, cxScreen - swp.cx, cyScreen - swp.cy, 0, 0, SWP_MOVE );
            break;
          case 3: // bottom left
            WinSetWindowPos( hwnd, HWND_TOP, 0, 0, 0, 0, SWP_MOVE );
            break;
          default: // bottom right
            WinSetWindowPos( hwnd, HWND_TOP, cxScreen - swp.cx, 0, 0, 0, SWP_MOVE );
            break;
        }
      }
      else
      {
        centerDialog( hwndFrame, hwnd );
      }

      return (MPARAM) TRUE;
    }

    default:
      break;
  }

  return( WinDefDlgProc( hwnd, msg, mp1, mp2 ) );
}

MRESULT EXPENTRY recordInfoProc(HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
{
  switch( msg )
  {
    case WM_CONTROL:
    case WM_COMMAND:
      return 0;

    case WM_INITDLG:
      centerDialog( HWND_DESKTOP, hwnd );
      return 0;

    default:
      break;
  }

  return( WinDefDlgProc( hwnd, msg, mp1, mp2 ) );
}

void centerDialog(HWND hwndFrame, HWND hWndDlg)
{
    RECTL rclWindow, rclDlg;
    LONG dWidth, dHeight, wWidth, wHeight, sBLCx, sBLCy;
    SWP swp, swpd;

    WinQueryWindowPos(HWND_DESKTOP,&swpd);  // get desktop dimensions

    WinQueryWindowRect(hwndFrame,&rclWindow);
    WinQueryWindowRect(hWndDlg,&rclDlg);
    WinQueryWindowPos(hwndFrame,&swp);
    dWidth = (LONG) (rclDlg.xRight - rclDlg.xLeft);
    dHeight = (LONG) (rclDlg.yTop - rclDlg.yBottom);
    wWidth = (LONG) (rclWindow.xRight - rclWindow.xLeft);
    wHeight = (LONG) (rclWindow.yTop - rclWindow.yBottom);
    sBLCx = swp.x + (((LONG) wWidth - dWidth) / 2);
    sBLCy = swp.y + (((LONG) wHeight - dHeight) / 2);

    // make sure the dialog doesn't run off one of the edges

    if (sBLCx < 1) sBLCx = 1;
    if (sBLCy < 1) sBLCy = 1;
    if ((sBLCx + dWidth) > swpd.cx) sBLCx = swpd.cx - (dWidth + 1);
    if ((sBLCy + dHeight) > swpd.cy) sBLCy = swpd.cy - (dHeight + 1);

    WinSetWindowPos(hWndDlg,HWND_TOP,sBLCx,sBLCy,0,0,SWP_MOVE);
}

void copyFile( char *from, char *to )
{
  char *dat;
  FILE *fh_from, *fh_to;
  long size;

  if( (fh_from = fopen( from, "rb" )) != NULL )
  {
    if( (fh_to   = fopen( to, "wb" )) != NULL )
    {
      size = _filelength( fileno( fh_from ) );

      dat = (char *) malloc( size );

      fread( dat, size, 1, fh_from );
      fwrite( dat, size, 1, fh_to );

      free( dat );

      fclose( fh_to );
    }

    fclose( fh_from );
  }
}

void setHeard( CntrRec *currRec )
{
  char base[256];
  int i;
  FILE *fh;

  strcpy( base, currRec->itemInfo.filename );

  for( i=0 ; (base[i]!='.')&&(i<strlen(base)) ; i++ )
    ;

  base[i] = 0;
  strcat( base, ".IDX" );

  fh = fopen( base, "w" );

  fprintf( fh, "%s\n%s\n%s\n%ld\n%s\n%s\n1\n%d\n", currRec->itemInfo.caller,
           currRec->itemInfo.date, currRec->itemInfo.time,
           currRec->itemInfo.seconds, currRec->itemInfo.calledEAZ,
           currRec->itemInfo.filename, currRec->itemInfo.digital );

  fclose( fh );

  if (currRec->itemInfo.digital) {
    currRec->itemInfo.icon = (HPOINTER) cntrIcoCallDigital;
  } else {
    currRec->itemInfo.icon = (HPOINTER) cntrIcoCallHeard;
  }

  WinSendMsg( hwndCntr, CM_INVALIDATERECORD, MPFROMP( NULL ),
              MPFROM2SHORT( 0, CMA_REPOSITION ) );
}

short queryDimensions( HWND hwnd, char *str, ULONG *breite, ULONG *hoehe )
{
  HPS hps;
  POINTL p[TXTBOX_COUNT];

  hps = WinGetPS( hwnd );
  GpiQueryTextBox( hps, strlen( str ), str, TXTBOX_COUNT, p );
  WinReleasePS( hps );

  *breite = p[TXTBOX_BOTTOMRIGHT].x - p[TXTBOX_BOTTOMLEFT].x;
  *hoehe  = p[TXTBOX_TOPLEFT].y - p[TXTBOX_BOTTOMLEFT].y;

  return 0;
}

short onlyDigits( char *str )
{
  char tmp[256], *pt;
  short i;

  strcpy( tmp, strip_blanks( str ) );
  if( strchr( tmp,'(' ) ) strchr( tmp, '(' )[-1] = 0;

  for( i=0 ; i < strlen( tmp ) ; i++ )
  {
    if( (tmp[i] < 48) || (tmp[i] > 57) )
      return 0;
  }

  return 1;
}

int wavplay_start( HWND hwnd, char* wavFile ) {

  int rc;
  MCI_OPEN_PARMS OpenParams;
  MCI_PLAY_PARMS PlayParams;
  MCI_GENERIC_PARMS GenericParams;

  memset(&OpenParams, 0, sizeof(OpenParams));
  OpenParams.pszElementName = wavFile;

  rc = mciSendCommand(0, MCI_OPEN, MCI_WAIT|MCI_OPEN_ELEMENT, &OpenParams, 0);
  if(SHORT1FROMMP(rc) != MCIERR_SUCCESS) return 0;

  memset(&PlayParams, 0, sizeof(PlayParams));
  PlayParams.hwndCallback = hwnd;

  rc = mciSendCommand(OpenParams.usDeviceID, MCI_PLAY, MCI_NOTIFY, &PlayParams, 0);
  if(SHORT1FROMMP(rc) != MCIERR_SUCCESS) {
    wavplay_stop(OpenParams.usDeviceID);
    return 0;
  }

  return OpenParams.usDeviceID;

}

int wavplay_stop( short deviceID )
{
  MCI_GENERIC_PARMS GenericParams;

  memset(&GenericParams, 0, sizeof(GenericParams));
  mciSendCommand(deviceID, MCI_STOP, MCI_WAIT, &GenericParams, 0);
  mciSendCommand(deviceID, MCI_CLOSE, MCI_WAIT, &GenericParams, 0);

  return 1;

}

void shExecute( char *file, BOOL addPath )
{
  HOBJECT     hObj;
  UCHAR       complete[_MAX_PATH], currDir[_MAX_PATH];
  ULONG       currDrive, driveMapping, len=_MAX_PATH;

  if( addPath )
  {
    DosQueryCurrentDisk( &currDrive, &driveMapping );
    DosQueryCurrentDir( currDrive, currDir, &len );

    sprintf( complete, "%c:\\%s\\%s", (UCHAR) 'A' + currDrive - 1,
                                      currDir, file );
  }
  else
  {
    strcpy( complete, file );
  }

  if( 0 != (hObj = WinQueryObject( (PSZ)complete )) )
    WinOpenObject( hObj, 0, TRUE );
}

typedef struct sMsgBoxStruct
{
  int extended;
  char* text;
  char* title;
  char* check_text;
  int* check_state;
  int style;
  HPOINTER icon;
}
tMsgBoxStruct;


int MyMsgBox(HWND hwndParent, HWND hwndOwner,
  char* text, char* title, int style, int icon)
{
  int rc;
  tMsgBoxStruct mb;    

  mb.extended = 0;
  mb.text = text;
  mb.title = title;
  mb.style = style;
  mb.icon = WinQuerySysPointer(HWND_DESKTOP, icon, 1);
  if(!mb.icon) mb.icon = WinLoadPointer(HWND_DESKTOP, 0, icon);

  rc = WinDlgBox(hwndParent,hwndOwner,MsgBoxProc,0,IDD_MSGBOX,&mb);

  if(mb.icon) WinDestroyPointer(mb.icon);
  return rc;                        
}

int MyMsgBoxEx(HWND hwndParent, HWND hwndOwner,
  char* text, char* title, char* checktext, int style, int icon,
  char* pCfgFile, char* pCfgKey)
{
  int rc;
  tMsgBoxStruct mb;
  int DontAskAgain = 0;
  ULONG buffsize = sizeof(DontAskAgain);

  DontAskAgain = config_file_read_ulong(pCfgFile, pCfgKey, 0);
  if(DontAskAgain) return (style == MB_YESNO) ? MBID_YES : MBID_OK;

  mb.extended = 1;
  mb.text = text;
  mb.title = title;
  mb.check_text = checktext;
  mb.check_state = &DontAskAgain;
  mb.style = style;
  mb.icon = WinQuerySysPointer(HWND_DESKTOP, icon, 1);
  if(!mb.icon) mb.icon = WinLoadPointer(HWND_DESKTOP, 0, icon);

  rc = WinDlgBox(hwndParent,hwndOwner,MsgBoxProc,0,IDD_MSGBOXEX,&mb);

  if(mb.icon) WinDestroyPointer(mb.icon);
  if((rc == MBID_YES || rc == MBID_OK) && DontAskAgain)
    config_file_write_ulong(pCfgFile, pCfgKey, 1);
  return rc;
}

MRESULT EXPENTRY MsgBoxProc(
  HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch(msg)
  {
    case WM_INITDLG:
    {
      HPS hps;
      SWP swp;
      RECTL rect;
      HWND hwndText;
      HPOINTER icon;
      int textHeight = 0;
      tMsgBoxStruct* pMb = (tMsgBoxStruct*)mp2;
      char* c,* buff, ok_btn[128], cancel_btn[128], yes_btn[128], no_btn[128];

      icon = (HPOINTER)WinSendDlgItemMsg(hwnd, 
        IDC_ICONSTATIC, SM_QUERYHANDLE, 0, 0);
      if(icon) WinDestroyPointer(icon);
      WinSendDlgItemMsg(hwnd, IDC_ICONSTATIC, 
        SM_SETHANDLE, MPFROMLONG(pMb->icon), 0);

      WinSetWindowText(hwnd, pMb->title);
      WinSetDlgItemText(hwnd, IDC_TEXT, pMb->text);

      c = pMb->text;
      hwndText = WinWindowFromID(hwnd, IDC_TEXT);
      WinQueryWindowPos(hwndText, &swp);
      hps = WinGetPS(hwndText);
      while(*c)
      {
        WinQueryWindowRect(hwndText, &rect);
        c += WinDrawText(hps, -1, c, &rect, 0, 0, 
          DT_LEFT|DT_TOP|DT_QUERYEXTENT|DT_WORDBREAK);
        textHeight += rect.yTop - rect.yBottom;
      }
      WinReleasePS(hps);
      if(textHeight > swp.cy)
      {
        textHeight -= swp.cy;
        WinSetWindowPos(hwndText,0,0,0,swp.cx,swp.cy+textHeight,SWP_SIZE);
        WinQueryWindowPos(WinWindowFromID(hwnd,IDC_ICONSTATIC), &swp);
        WinSetWindowPos(WinWindowFromID(hwnd,IDC_ICONSTATIC),
          0,swp.x,swp.y+textHeight,0,0,SWP_MOVE);
        WinQueryWindowPos(hwnd, &swp);
        WinSetWindowPos(hwnd,0,0,0,swp.cx,swp.cy+textHeight,SWP_SIZE);
      }

      if(pMb->extended == 1)
      {
        WinSetDlgItemText(hwnd, IDC_CHECK, pMb->check_text);
        WinCheckButton(hwnd, IDC_CHECK, *(pMb->check_state));
        WinSetWindowULong(hwnd, QWL_USER, (ULONG)pMb->check_state);
      }
      
      WinLoadString(0,0,STR_OK_BUTTON,sizeof(ok_btn),ok_btn);
      WinLoadString(0,0,STR_CANCEL_BUTTON,sizeof(cancel_btn),cancel_btn);
      WinLoadString(0,0,STR_YES_BUTTON,sizeof(yes_btn),yes_btn);
      WinLoadString(0,0,STR_NO_BUTTON,sizeof(no_btn),no_btn);

      switch(pMb->style)
      {
        case MB_OKCANCEL:
          WinSetDlgItemText(hwnd, DID_OK, ok_btn);
          WinSetWindowUShort(WinWindowFromID(hwnd,DID_OK),QWS_ID,MBID_OK);
          WinSetDlgItemText(hwnd, DID_CANCEL, cancel_btn);
          WinSetWindowUShort(WinWindowFromID(hwnd,DID_CANCEL),QWS_ID,MBID_CANCEL);
          break;
        case MB_YESNO:
          WinSetDlgItemText(hwnd, DID_OK, yes_btn);
          WinSetWindowUShort(WinWindowFromID(hwnd,DID_OK),QWS_ID,MBID_YES);
          WinSetDlgItemText(hwnd, DID_CANCEL, no_btn);
          WinSetWindowUShort(WinWindowFromID(hwnd,DID_CANCEL),QWS_ID,MBID_NO);
          break;
        case MB_OK:
        default:
          WinSetDlgItemText(hwnd, DID_OK, ok_btn);
          WinSetWindowUShort(WinWindowFromID(hwnd,DID_OK),QWS_ID,MBID_OK);
          WinDestroyWindow(WinWindowFromID(hwnd,DID_CANCEL));
          break;                    
      }

      centerDialog(WinQueryWindow(hwnd,QW_OWNER),hwnd);
      break;
    }

    case WM_CLOSE:
    {
      int* check_state = (int*)WinQueryWindowULong(hwnd, QWL_USER);
      if(check_state) *check_state = WinQueryButtonCheckstate(hwnd, IDC_CHECK);
      if(WinWindowFromID(hwnd,MBID_NO)) WinDismissDlg(hwnd, MBID_NO);
      else WinDismissDlg(hwnd, MBID_CANCEL);
      return 0;
    }

    case WM_COMMAND:

      switch(SHORT1FROMMP(mp1))
      {
        case MBID_OK:
        case MBID_CANCEL:
        case MBID_YES:
        case MBID_NO:
        {
          int* check_state = (int*)WinQueryWindowULong(hwnd, QWL_USER);
          if(check_state) *check_state = WinQueryButtonCheckstate(hwnd, IDC_CHECK);
          if(SHORT1FROMMP(mp1) == MBID_CANCEL && WinWindowFromID(hwnd,MBID_NO))
            WinDismissDlg(hwnd, MBID_NO);
          else WinDismissDlg(hwnd, SHORT1FROMMP(mp1));
          break;
        }
      }
      return 0;

  }

  return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

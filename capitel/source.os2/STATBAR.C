#define INCL_WIN
#define INCL_DOS

#include <os2.h>
#include <stdio.h>

#include "capitel.h"
#include "..\..\common\source\version.h"
#include "statbar.h"

#define  sBarClass      "pmStatusbar"

MRESULT EXPENTRY sBarProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

void registerStatusbar( HAB hab )
{
  WinRegisterClass( hab, sBarClass, sBarProc, 0, 0 );
}

MRESULT EXPENTRY sBarProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  int cx, cy;
  HWND hwndText;
  RECTL rect;
  HPS hps;

  switch( msg )
  {
    case WM_CREATE:
      WinCreateWindow( hwnd, WC_STATIC, APPNAME, SS_TEXT | WS_VISIBLE |
                       DT_VCENTER, 0, 0, 0, 0, hwnd, HWND_TOP,
                       ID_STATUSBAR, 0, NULL );
      break;

    case WM_SIZE:
      cx = SHORT1FROMMP( mp2 );
      cy = SHORT2FROMMP( mp2 ) - 4;

      hwndText = WinWindowFromID( hwnd, ID_STATUSBAR );
      WinSetWindowPos( hwndText, HWND_TOP, 2, 2, cx, cy, SWP_MOVE | SWP_SIZE |
                       SWP_ZORDER );
      break;

    case WM_PAINT: {
      RECTL rcl;
      POINTL pt;
      HPS hps = WinBeginPaint( hwnd, 0, 0 );

      WinQueryWindowRect( hwnd, &rcl );
      WinFillRect( hps, &rcl, SYSCLR_BUTTONMIDDLE );

      pt.x = rcl.xLeft;
      pt.y = rcl.yTop - 1;
      GpiSetColor( hps, SYSCLR_BUTTONDARK );
      GpiMove( hps, &pt );
      pt.x = rcl.xRight - 1;
      GpiLine( hps, &pt );
      pt.x = rcl.xLeft;
      pt.y = rcl.yTop - 2;
      GpiSetColor( hps, CLR_WHITE );
      GpiMove( hps, &pt );
      pt.x = rcl.xRight - 1;
      GpiLine( hps, &pt );

      WinEndPaint( hps );
    } break;

    case SB_SETTEXT:
      hwndText = WinWindowFromID( hwnd, ID_STATUSBAR );
      WinSetWindowText( hwndText, mp1 );
      break;

    default:
      return( WinDefWindowProc( hwnd, msg, mp1, mp2 ) );
  }

  return FALSE;
}


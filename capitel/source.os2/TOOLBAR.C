#define INCL_WIN
#define INCL_DOS
#define INCL_PM

#include <os2.h>
#include <stdio.h>
#include <string.h>

#include "capitel.h"
#include "toolbar.h"
#include "statbar.h"

#define  tBarClass      "pmToolbar"

PFNWP oldButProc;
char  useBubbles = 0;

short queryDimensions( HWND, char *, ULONG *, ULONG * );

MRESULT EXPENTRY tBarProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );

void registerToolbar( HAB hab )
{
  WinRegisterClass( hab, tBarClass, tBarProc, 0, sizeof( ULONG ) );
}

MRESULT EXPENTRY tButProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  HWND hwndFly = WinQueryWindowULong( hwnd, QWL_USER );

  switch( msg )
  {
    case WM_MOUSEMOVE:
    {
      if( !hwndFly && useBubbles )
        WinStartTimer( 0, hwnd, 1 /* unused id */, 500 );
      return ( *oldButProc )( hwnd, msg, mp1, mp2 );
    }
    break;

    case WM_TIMER:
    {
      RGB rgb;
      HPS hps;
      HAB hab;
      RECTL rectl;
      ULONG breite, hoehe;
      POINTL p;
      char str[200];

      if( hwndFly )
        break;

      hab = WinQueryAnchorBlock( hwnd );

      WinLoadString( hab, NULLHANDLE, WinQueryWindowUShort( hwnd, QWS_ID ),
                     sizeof( str ), str );

      hwndFly = WinCreateWindow( HWND_DESKTOP, WC_STATIC, str,
                                 SS_TEXT | DT_VCENTER | DT_CENTER,
                                 0, 0, 0, 0, hwnd, HWND_TOP, 0, 0, NULL );

      rgb.bBlue  = 0;
      rgb.bGreen = 254;
      rgb.bRed   = 254;
      WinSetPresParam( hwndFly, PP_BACKGROUNDCOLOR, sizeof( RGB ), &rgb );

      rgb.bBlue  = 0;
      rgb.bGreen = 0;
      rgb.bRed   = 0;
      WinSetPresParam( hwndFly, PP_FOREGROUNDCOLOR, sizeof( RGB ), &rgb );

      WinSetPresParam( hwndFly, PP_FONTNAMESIZE, sizeof( DefCntrFont ),
                       DefCntrFont );

      queryDimensions( hwndFly, str, &breite, &hoehe );

      WinQueryPointerPos( HWND_DESKTOP, &p );
      p.y -= WinQuerySysValue( HWND_DESKTOP, SV_CYTITLEBAR );
      p.y -= hoehe/2;
      p.x += 5;

      WinSetWindowPos( hwndFly, HWND_TOP, p.x, p.y, breite+4, hoehe+2,
                       SWP_SHOW | SWP_MOVE | SWP_SIZE );

      WinSetWindowULong( hwnd, QWL_USER, (ULONG) hwndFly );
    }
    break;

    case UM_FLYOVER_BEGIN:
      useBubbles = 1;
      break;

    case UM_FLYOVER_END:
      WinStopTimer( 0, hwnd, 1 /* unused id */ );
      if( hwndFly )
        WinDestroyWindow( hwndFly );
      WinSetWindowULong( hwnd, QWL_USER, 0 );
      break;

    default:
      return ( *oldButProc )( hwnd, msg, mp1, mp2 );
  }

  return FALSE;
}

MRESULT EXPENTRY tBarProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
  HENUM henum;
  RECTL rect;
  short height, i=0;
  HWND hwndHelp;
  char str[50];
  HPS hps;

  switch( msg )
  {
    case WM_CREATE:
    case WM_DESTROY:
    case WM_SIZE:
      break;

    case WM_COMMAND:
      WinSendMsg( WinQueryWindow( hwnd, QW_OWNER ), msg, mp1, mp2 );
      break;

    case TB_REDRAW:
      henum  = WinBeginEnumWindows( hwnd );

      while( (hwndHelp = WinGetNextWindow( henum )) != 0 )
      {
        WinSetWindowPos( hwndHelp, 0, i+2, 3, 0, 0, SWP_MOVE );
        WinQueryWindowRect( hwndHelp, &rect );
        i += rect.xRight;
      }

      break;

    case TB_ADDBUTTON:
      WinQueryWindowRect( hwnd, &rect );
      height = rect.yTop - rect.yBottom;

      sprintf( str, "#%ld", (ULONG)mp1 );

      hwndHelp = WinCreateWindow( hwnd, WC_BUTTON, str, WS_VISIBLE |
                                  BS_PUSHBUTTON | BS_BITMAP | BS_NOPOINTERFOCUS,
                                  0, 0, height-4, height-4,
                                  hwnd, HWND_BOTTOM, (ULONG)mp1, 0, NULL );

      oldButProc = WinSubclassWindow( hwndHelp, tButProc );

      WinPostMsg( hwnd, TB_REDRAW, 0, 0 );

      break;

    case TB_ADDSPACE:
      hwndHelp = WinCreateWindow( hwnd, WC_STATIC, NULL, !WS_VISIBLE,
                                  0, 0, (ULONG)mp1, 0,
                                  hwnd, HWND_BOTTOM, 0, 0, NULL );

      WinPostMsg( hwnd, TB_REDRAW, 0, 0 );

      break;

    case TB_SETBITMAP:
      sprintf( str, "#%ld", (ULONG)mp2 );
      hwndHelp = WinWindowFromID( hwnd, (USHORT)mp1 );
      WinSetWindowText( hwndHelp, (PSZ)str );
      WinPostMsg( hwnd, TB_REDRAW, 0, 0 );
      break;

    case TB_REMOVEBUTTONID:
      hwndHelp = WinWindowFromID( hwnd, (USHORT) mp1 );
      if( hwndHelp )
        WinDestroyWindow( hwndHelp );
      break;

    case TB_HWNDFROMID:
      return (MPARAM) WinWindowFromID( hwnd, (USHORT) mp1 );

    case WM_PAINT: {
      RECTL rcl;
      POINTL pt;

      HPS hps = WinBeginPaint( hwnd, 0, 0 );

      WinQueryWindowRect(hwnd,&rcl);
      WinFillRect( hps, &rcl, SYSCLR_BUTTONMIDDLE );

      pt.x = rcl.xLeft;
      pt.y = rcl.yBottom;
      GpiSetColor( hps, SYSCLR_BUTTONLIGHT );
      GpiMove( hps, &pt );
      pt.x = rcl.xRight - 1;
      GpiLine( hps, &pt );
      pt.x = rcl.xLeft;
      pt.y = rcl.yBottom + 1;
      GpiSetColor( hps, SYSCLR_BUTTONDARK );
      GpiMove( hps, &pt );
      pt.x = rcl.xRight - 1;
      GpiLine( hps, &pt );
      rcl.yBottom += 2;
      WinEndPaint( hps );
    }
    break;

    default:
      return WinDefWindowProc( hwnd, msg, mp1, mp2 );
  }

  return FALSE;
}


#define INCL_WINSHELLDATA
#define INCL_WINDIALOGS

#include <os2.h>
#include <string.h>
#include "capitel.h"
#include "..\..\common\source\global.h"
//#include "..\..\common\source\texte.h"
#include "texte.h"
//#include "..\..\..\units\common.src\cfg_file.h"
#include "..\..\util\source\register.h"

void centerDialog(HWND hwndFrame, HWND hWndDlg);
void MsgBox( char * );

MRESULT EXPENTRY registrationProc( HWND hwnd, USHORT msg,
                                   MPARAM mp1, MPARAM mp2 )
{
  char str[100], str2[100];

  switch( msg )
  {
    case WM_INITDLG:
      centerDialog( hwndFrame, hwnd );

      if( queryRegistration( str, str2 ) )
      {
        WinSetDlgItemText( hwnd, REG_NAME, str );
        WinSetDlgItemText( hwnd, REG_CODE, str2 );
      }

      break;

    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case REG_OK:
          WinQueryDlgItemText( hwnd, REG_NAME, sizeof( str ), (PSZ) str );
          WinQueryDlgItemText( hwnd, REG_CODE, sizeof( str2 ), (PSZ) str2 );
          strcpy( str2, strupr( str2 ) );

          if( setRegistration( str, str2 ) )
            MsgBox( THANKREG );
          else
            MsgBox( INVALIDREG );

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


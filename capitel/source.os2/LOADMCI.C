#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS

#include <os2.h>
#include <stdio.h>

#define DLLNAME "MCIAPI"

typedef ULONG (* EXPENTRY tPlayWavFile) ( HWND, PSZ, ULONG, PSZ, HWND );

tPlayWavFile mciPlayFile;

HMODULE MMOS2Handle = NULLHANDLE;
char mmpmInstalled=0;

short LoadMMPM( void )
{
  UCHAR  LoadError[128] = "";
  APIRET rc             = NO_ERROR;

  rc = DosLoadModule( LoadError, sizeof(LoadError), DLLNAME, &MMOS2Handle );

  if( rc != NO_ERROR)
    return 0;

  rc = DosQueryProcAddr( MMOS2Handle, 0L, (PSZ)"mciPlayFile",
                         (PFN *) &mciPlayFile );

  if( rc )
  {
    DosFreeModule( MMOS2Handle );
    return 0;
  }


  mmpmInstalled = 1;
  return 1;
}

short UnLoadMMPM( void )
{
  if( !mmpmInstalled )
    return 0;

  mmpmInstalled = 0;
  DosFreeModule( MMOS2Handle );
  MMOS2Handle = NULLHANDLE;
  return 1;
}


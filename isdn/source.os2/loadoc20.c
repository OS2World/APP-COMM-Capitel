#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>

#include "..\..\..\units\common.src\loadcapi.h"

#define DLLNAME_20     "CAPI20"

HMODULE ModuleHandle_20   = NULLHANDLE;

#define DWORD unsigned long
#define CallConv APIENTRY

typedef DWORD ( *CallConv tReg_20)( DWORD, DWORD, DWORD, DWORD, DWORD * );
typedef DWORD ( *CallConv tRel_20)( DWORD );
typedef DWORD ( *CallConv tPut_20)( DWORD, PVOID );
typedef DWORD ( *CallConv tGet_20)( DWORD, PVOID * );
typedef DWORD (* CallConv tSet_20)( DWORD, DWORD );
typedef VOID  ( *CallConv tMan_20)( char * );
typedef DWORD ( *CallConv tVer_20)( DWORD *, DWORD *, DWORD *, DWORD * );
typedef DWORD ( *CallConv tSer_20)( char * );
typedef DWORD ( *CallConv tPro_20)( PVOID, DWORD );
typedef DWORD ( *CallConv tIns_20)( VOID );

tReg_20 API_REGISTER_20 = NULL;                /* Mandatory Functions. If loading of these */
tRel_20 API_RELEASE_20 = NULL;                 /* functions fails, error will be returned. */
tPut_20 API_PUT_MESSAGE_20 = NULL;
tGet_20 API_GET_MESSAGE_20 = NULL;
tSet_20 API_SET_SIGNAL_20 = NULL;
tIns_20 API_INSTALLED_20 = NULL;

tMan_20 API_GET_MANUFACTURER_20 = NULL; /* Additional Functions. Check for NULL     */
tVer_20 API_GET_VERSION_20 = NULL;      /* before using. If loading of these fncts. */
tSer_20 API_GET_SERIAL_NUMBER_20 = NULL;/* fails, no error will be returned.        */
tPro_20 API_GET_PROFILE_20 = NULL;

short Capi20_LoadCapi( void )
{
  PSZ     ModuleName_20     = DLLNAME_20;
  UCHAR   LoadError_20[128] = "";
  APIRET  rc_20             = NO_ERROR;

  rc_20 = DosLoadModule( LoadError_20, sizeof( LoadError_20 ), ModuleName_20, &ModuleHandle_20 );

  if( rc_20 != NO_ERROR ) return 0;

  rc_20 = DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_REGISTER"   ,(PFN *)&API_REGISTER_20   );
  rc_20+= DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_RELEASE"    ,(PFN *)&API_RELEASE_20    );
  rc_20+= DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_PUT_MESSAGE",(PFN *)&API_PUT_MESSAGE_20);
  rc_20+= DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_GET_MESSAGE",(PFN *)&API_GET_MESSAGE_20);
  rc_20+= DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_INSTALLED"  ,(PFN *)&API_INSTALLED_20  );
  rc_20+= DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_SET_SIGNAL" ,(PFN *)&API_SET_SIGNAL_20 );

  if( rc_20 ) {
    DosFreeModule( ModuleHandle_20 );
    return 0;
  }

  DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_GET_MANUFACTURER" ,(PFN *)&API_GET_MANUFACTURER_20 );
  DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_GET_VERSION"      ,(PFN *)&API_GET_VERSION_20      );
  DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_GET_SERIAL_NUMBER",(PFN *)&API_GET_SERIAL_NUMBER_20);
  DosQueryProcAddr( ModuleHandle_20, 0L, (PSZ)"CAPI_GET_PROFILE"      ,(PFN *)&API_GET_PROFILE_20      );

  if (API_INSTALLED_20()) return(0);

  return 1;
}

short Capi20_UnLoadCapi( void )
{
  DosFreeModule( ModuleHandle_20 );
  ModuleHandle_20 = NULLHANDLE;
  return 1;
}


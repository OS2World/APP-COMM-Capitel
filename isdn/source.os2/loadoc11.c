// 161097 /wfehn   - support fÅr avm "capi16.dll" eingebaut

#define INCL_DOSMODULEMGR
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>

#include "..\..\..\units\common.src\loadcapi.h"

#define DLLNAME_11     "CAPI"
#define DLLNAME_AVM_11 "CAPI16"

HMODULE ModuleHandle_11   = NULLHANDLE;

typedef short (* APIENTRY16 tReg_11) ( char * _Seg16, short, short, short, short );
typedef short (* APIENTRY16 tRel_11) ( short );
typedef short (* APIENTRY16 tPut_11) ( short, char * _Seg16 );
typedef short (* APIENTRY16 tGet_11) ( short, char * _Seg16 * _Seg16 );
typedef short (* APIENTRY16 tSet_11) ( short, void (* APIENTRY16)( void ) );
typedef short (* APIENTRY16 tMan_11) ( char * _Seg16 );
typedef short (* APIENTRY16 tVer_11) ( char * _Seg16 );
typedef short (* APIENTRY16 tSer_11) ( char * _Seg16 );
typedef short (* APIENTRY16 tAdr_11) ( void );
typedef short (* APIENTRY16 tIns_11) ( void );

tReg_11 API_REGISTER_11          = NULL; /* Mandatory Functions. If loading of these */
tRel_11 API_RELEASE_11           = NULL; /* functions fails, error will be returned. */
tPut_11 API_PUT_MESSAGE_11       = NULL;
tGet_11 API_GET_MESSAGE_11       = NULL;
tSet_11 API_SET_SIGNAL_11        = NULL;
tIns_11 API_INSTALLED_11         = NULL;

tMan_11 API_GET_MANUFACTURER_11  = NULL; /* Additional Functions. Check for NULL     */
tVer_11 API_GET_VERSION_11       = NULL; /* before using. If loading of these fncts. */
tSer_11 API_GET_SERIAL_NUMBER_11 = NULL; /* fails, no error will be returned.        */
tAdr_11 API_GET_ADDRESSMODE_11   = NULL;

short LoadCapi11( void )
{
  PSZ     ModuleName_11     = DLLNAME_11;
  PSZ     ModuleNameAVM_11  = DLLNAME_AVM_11;
  UCHAR   LoadError_11[128] = "";
  APIRET  rc_11             = NO_ERROR;

  rc_11 = DosLoadModule( LoadError_11, sizeof( LoadError_11 ), ModuleNameAVM_11, &ModuleHandle_11 );

  if( rc_11 != NO_ERROR ) {
    rc_11 = DosLoadModule( LoadError_11, sizeof( LoadError_11 ), ModuleName_11, &ModuleHandle_11 );
  }

  if( rc_11 != NO_ERROR ) return 0;

  rc_11 = DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_REGISTER"   , (PFN *)&API_REGISTER_11    );
  rc_11+= DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_RELEASE"    , (PFN *)&API_RELEASE_11     );
  rc_11+= DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_PUT_MESSAGE", (PFN *)&API_PUT_MESSAGE_11 );
  rc_11+= DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_GET_MESSAGE", (PFN *)&API_GET_MESSAGE_11 );
  rc_11+= DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_SET_SIGNAL" , (PFN *)&API_SET_SIGNAL_11  );
  rc_11+= DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_INSTALLED"  , (PFN *)&API_INSTALLED_11   );

  if( rc_11 ) {
    DosFreeModule( ModuleHandle_11 );
    return 0;
  }

  DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_GET_MANUFACTURER" , (PFN *)&API_GET_MANUFACTURER_11  );
  DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_GET_VERSION"      , (PFN *)&API_GET_VERSION_11       );
  DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_GET_SERIAL_NUMBER", (PFN *)&API_GET_SERIAL_NUMBER_11 );
  DosQueryProcAddr( ModuleHandle_11, 0L, (PSZ)"API_GET_ADDRESSMODE"  , (PFN *)&API_GET_ADDRESSMODE_11   );

  return 1;
}

short UnLoadCapi11( void )
{
  DosFreeModule( ModuleHandle_11 );
  ModuleHandle_11 = NULLHANDLE;
  return 1;
}


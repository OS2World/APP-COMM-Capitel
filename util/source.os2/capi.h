#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#define INCL_DOS
#define INCL_ERRORS

#include <os2.h>

/* ------------------------------------------------------------------------ */

extern short (*APIENTRY16 API_REGISTER_11         )( char * _Seg16, short, short, short, short );
extern short (*APIENTRY16 API_RELEASE_11          )( short );
extern short (*APIENTRY16 API_PUT_MESSAGE_11      )( short, char * _Seg16 );
extern short (*APIENTRY16 API_GET_MESSAGE_11      )( short, char * _Seg16 * _Seg16 );
extern short (*APIENTRY16 API_SET_SIGNAL_11       )( short, void (* APIENTRY16)( void ) );
extern short (*APIENTRY16 API_GET_MANUFACTURER_11 )( char * _Seg16 );
extern short (*APIENTRY16 API_GET_VERSION_11      )( char * _Seg16 );
extern short (*APIENTRY16 API_GET_SERIAL_NUMBER_11)( char * _Seg16 );
extern short (*APIENTRY16 API_GET_ADDRESSMODE_11  )( void );
extern short (*APIENTRY16 API_INSTALLED_11        )( void );

/* ------------------------------------------------------------------------ */


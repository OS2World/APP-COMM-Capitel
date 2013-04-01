#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "../../isdn/source/isdn.h"
#include "../../../units/common.src/cfg_file.h"
#include "../../common/source/global.h"

#define CAPI11 1
#define CAPI20 2
#define SERIAL 3

static char device = 0;

short comm_init                 ( void (*sigf)( short, void * ))
{
  short ret;
  unsigned long detect;

  detect = config_file_read_ulong(STD_CFG_FILE, COM_DEVICE, COM_DEVICE_DEF); // os2

  switch (detect) {
    case 0: {
              /* = = = = = = = = = = = = = = = = = = = = */

              ret = comm20_init (sigf, 0); // capi 2.0? aber nicht runterfahren
              if (ret == 0) {
                device = CAPI20;
                return (0);
              }
              /* = = = = = = = = = = = = = = = = = = = = */
              ret = comm11_init (sigf, 0); // capi 1.1? aber nicht runterfahren
              if (ret == 0) {
                device = CAPI11;
                return (0);
              }
              /* = = = = = = = = = = = = = = = = = = = = */
/*
              ret = commse_init (sigf, 0); // serial?
              if (ret == 0) {
                device = SERIAL;
                return (0);
              }
*/
              /* = = = = = = = = = = = = = = = = = = = = */
              return (1);
              break;
            }
    case 1: {
              ret = comm11_init (sigf, 1); // capi 1.1? aber runterfahren
              if (ret == 0) {
                device = CAPI11;
                return (0);
              }
              return (1);
              break;
            }
    case 2: {
              ret = comm20_init (sigf, 1); // capi 2.0? aber runterfahren
              if (ret == 0) {
                device = CAPI20;
                return (0);
              }
              return (1);
              break;
            }
    case 3: {
              ret = commse_init (sigf, 1); // com? aber runterfahren
              if (ret == 0) {
                device = SERIAL;
                return (0);
              }
              return (1);
              break;
            }
  }
  return (1);
}

short comm_exit                 (void)
{
  switch (device) {
    case CAPI20: return (comm20_exit());
    case CAPI11: return (comm11_exit());
    case SERIAL: return (commse_exit());
  }
  return (1);
}

short comm_listen               (short val)
{
  switch (device) {
    case CAPI20: return (comm20_listen (val));
    case CAPI11: return (comm11_listen (val));
    case SERIAL: return (commse_listen (val));
  }
  return (1);
}

char  comm_connected            (void)
{
  switch (device) {
    case CAPI20: return (comm20_connected());
    case CAPI11: return (comm11_connected());
    case SERIAL: return (commse_connected());
  }
  return (0);
}

char  comm_data_available            (void)
{
  switch (device) {
    case CAPI20: return (comm20_data_available());
    case CAPI11: return (comm11_data_available());
    case SERIAL: return (commse_data_available());
  }
  return (0);
}

void  comm_clear_all_buffer          (void)
{
  switch (device) {
    case CAPI20: comm20_clear_all_buffer(); break;
    case CAPI11: comm11_clear_all_buffer(); break;
    case SERIAL: commse_clear_all_buffer(); break;
  }
}

long  comm_read_block           (char *buff)
{
  switch (device) {
    case CAPI20: return (comm20_read_block(buff));
    case CAPI11: return (comm11_read_block(buff));
    case SERIAL: return (commse_read_block(buff));
  }
  return (0);
}

void  comm_write_block          (char *buff, short val)
{
  switch (device) {
    case CAPI20: comm20_write_block (buff, val); break;
    case CAPI11: comm11_write_block (buff, val); break;
    case SERIAL: commse_write_block (buff, val); break;
  }
}

void  comm_begin_of_record      (void)
{
  switch (device) {
    case CAPI20: comm20_begin_of_record(); break;
    case CAPI11: comm11_begin_of_record(); break;
    case SERIAL: commse_begin_of_record(); break;
  }
}

void  comm_disc_req             (void)
{
  switch (device) {
    case CAPI20: comm20_disc_req(); break;
    case CAPI11: comm11_disc_req(); break;
    case SERIAL: commse_disc_req(); break;
  }
}

void comm_name_of_interface        (char *name)
{
  switch (device) {
    case CAPI20: comm20_name_of_interface(name); break;
    case CAPI11: comm11_name_of_interface(name); break;
    case SERIAL: commse_name_of_interface(name); break;
  }
}

#ifdef WIN32

short comm11_init               (void (*sig)(short, void *), char sd) { return (1);}
short comm11_exit               (void) { return (1);}
short comm11_listen             (short val) {return (1);}
char  comm11_connected          (void) {return (0);}
char  comm11_data_available     (void) {return (0);}
void  comm11_clear_all_buffer   (void) {}
void  comm11_clear_read_buffer  (void) {}
void  comm11_clear_write_buffer (void) {}
long  comm11_read_block         (char *buff) {return (0);}
void  comm11_write_block        (char *buff, short val) {}
char  comm11_can_write          (void) {return (0);}
void  comm11_begin_of_record    (void) {}
void  comm11_disc_req           (void) {}
void  comm11_name_of_interface  (char* name){ strcpy (name,"CAPI 1.1 DUMMY");}

#endif

#ifdef UNIX

short comm11_init               (void (*sig)(short, void *), char sd) { return (1);}
short comm11_exit               (void) { return (1);}
short comm11_listen             (short val) {return (1);}
char  comm11_connected          (void) {return (0);}
char  comm11_data_available     (void) {return (0);}
void  comm11_clear_all_buffer   (void) {}
void  comm11_clear_read_buffer  (void) {}
void  comm11_clear_write_buffer (void) {}
long  comm11_read_block         (char *buff) {return (0);}
void  comm11_write_block        (char *buff, short val) {}
char  comm11_can_write          (void) {return (0);}
void  comm11_begin_of_record    (void) {}
void  comm11_disc_req           (void) {}
void  comm11_name_of_interface  (char* name){ strcpy (name,"CAPI 1.1 DUMMY");}

short commse_init               (void (*sig)(short, void *), char sd) { return (1);}
short commse_exit               (void) { return (1);}
short commse_listen             (short val) {return (1);}
char  commse_connected          (void) {return (0);}
char  commse_data_available     (void) {return (0);}
void  commse_clear_all_buffer   (void) {}
void  commse_clear_read_buffer  (void) {}
void  commse_clear_write_buffer (void) {}
long  commse_read_block         (char *buff) {return (0);}
void  commse_write_block        (char *buff, short val) {}
char  commse_can_write          (void) {return (0);}
void  commse_begin_of_record    (void) {}
void  commse_disc_req           (void) {}
void  commse_name_of_interface  (char* name){ strcpy (name,"Serial DUMMY");}

#endif

#ifdef OS2

short commse_init               (void (*sig)(short, void *), char sd) { return (1);}
short commse_exit               (void) { return (1);}
short commse_listen             (short val) {return (1);}
char  commse_connected          (void) {return (0);}
char  commse_data_available     (void) {return (0);}
void  commse_clear_all_buffer   (void) {}
void  commse_clear_read_buffer  (void) {}
void  commse_clear_write_buffer (void) {}
long  commse_read_block         (char *buff) {return (0);}
void  commse_write_block        (char *buff, short val) {}
char  commse_can_write          (void) {return (0);}
void  commse_begin_of_record    (void) {}
void  commse_disc_req           (void) {}
void  commse_name_of_interface  (char* name){ strcpy (name,"Serial DUMMY");}

#endif


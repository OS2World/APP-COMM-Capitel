#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "..\..\..\units\win.src\w32uart.h"
#include "..\..\..\units\common.src\v24util.h"
#include "..\..\..\units\common.src\os.h"
#include "..\..\common\source\version.h"
#include "..\..\isdn\source\isdn.h"
#include "..\..\..\units\common.src\capi_chk.h"
#include "..\..\..\units\common.src\loadcapi.h"
#include "..\..\..\units\common.src\num2nam.h"
#include "..\..\wave\source\wav2alw.h"
#include "..\..\common\source\global.h"
#include "..\..\..\units\common.src\cfg_file.h"
#include "..\..\answer\source\answer.h"

#define ATASTR "ATA\r"
#define RING_TIME_OUT 3000L

static char online=0;
static char UsedCom[6] = {0};

static Thread_Id_Typ  wait_thread_id  = 0;
static Thread_Id_Typ  rec_thread_id  = 0;

static tU32 LastRing = 0;
static tU32 an_delay_next_call;
static tU32 an_delay_default;
static char msn_liste[500];
static short num_in_list = 0;
static long     max_rec_time_next_call;

static unsigned long Handle;

void (*AppSigFunc)( short, void * );

static char     out_msg[300];



void SigDiscInd (void)
{
  AppSigFunc (4,"");
  AppSigFunc (6,"");
  Sleep (200);
  xW32UartPurgeRxTxBuff (Handle);
}


static void set_active_numbers(void)
{
  char  str[205], org[205];
  FILE *msndat = NULL;

  strcpy (msn_liste,"|");
  msndat = fopen (PRTFILE,"r");

  if (msndat) {
     while (fgets(str,200,msndat)) {
       if ((str[0] != '#') && (strlen(str)) && (strchr(str,'|')!=NULL)) {
         strcpy (org,str);
         strtok(str ,"|"); strtok(NULL,"|"); strtok(NULL,"|"); strtok(NULL,"|"); strtok(NULL,"|"); strtok(NULL,"|");
         if (*strtok(NULL,"|") == '1') {
           if (strchr (org,' |')) *strchr (org,'|') = 0;
           strcat (msn_liste,org);
         }
         strcat (msn_liste,"|");
       }
     }
     fclose (msndat);
  }
}


void b3_ind  (void *Dummy)
{
    AppSigFunc (3,"");
}

void b3_ind_pre  (void)  // OH GOTT, WIE KANN ICH SOWAS MACHEN?
{
    b3_ind (NULL);
}

void b3_conf (void *Dummy)
{
    AppSigFunc (5,"");
}


static void _Optlink waitforconn( void *arg )
{
  tU32 wait_ms      = 0;
  tU32 discind_flag = 0;

  while( (wait_ms < (an_delay_next_call * 1000)) && (!discind_flag))
  {
    OsSleep( 200 );
    wait_ms += 200;
    if (((GetTickCount() - LastRing) > RING_TIME_OUT))
    {
      discind_flag = 1;
    }
  }

  if (!discind_flag)
  {
    xW32UartWriteBlock (ATASTR, strlen (ATASTR), 0, Handle);
    Sleep (1000);
    if (!online)
    {
      SigDiscInd();
    }
  }
  else
  {
    SigDiscInd();
  }
  wait_thread_id = 0;
}

void callback_ring (void)
{
  AnsConIndMsg ConIndMsg      = {0};
  tU8  Buff            [256]  = {0};
  tU8  RingStr         [256]  = {0};
  tU8  UnKnownStr      [256];
  tU8  RingTokenStr    [256];
  tU32 Len;
  tU32 active;
  char an_delay_str[20];
  char bhelp[200];
  char max_rec_time_str[20];

  set_active_numbers ();

  if ((GetTickCount()-LastRing) > 3000L)
  {
    config_file_read_string(STD_CFG_FILE, TEXT_UNKNOWN_ISDN, UnKnownStr     , TEXT_UNKNOWN_ISDN_DEF);
    config_file_read_string(STD_CFG_FILE, MODEM_RING_STR   , RingTokenStr   , MODEM_RING_STR_DEF   );

    strcpy (ConIndMsg.caller_name    , UnKnownStr);
    strcpy (ConIndMsg.caller_org_name, UnKnownStr);
    strcpy (ConIndMsg.called_name    , UnKnownStr);

    Sleep (750);
    Len = xW32UartDataAvailable(Handle);
    if (Len > sizeof(Buff)) Len = sizeof(Buff);
    xW32UartReadBlock (Buff,Len,0, Handle);
    Buff[Len] = 0;

    if (strstr(Buff, RingTokenStr))
    {
      strcpy (RingStr,strstr(Buff, RingTokenStr)+5);
      if (strstr(RingStr,"\r")) *strstr(RingStr,"\r") = 0;
      strcpy (ConIndMsg.caller_name, RingStr);
      if (strstr(ConIndMsg.caller_name,";")) *strstr(ConIndMsg.caller_name,";") = 0;
      strcpy (ConIndMsg.called_name,strstr(RingStr,";")+1);
    }

/* - - - */

    an_delay_default   = config_file_read_ulong (STD_CFG_FILE,DEFAULT_ANSW_DELAY,DEFAULT_ANSW_DELAY_DEF);
    an_delay_next_call = an_delay_default;

    an_delay_next_call = an_delay_default;
    sprintf(an_delay_str,"%d",an_delay_next_call);
    better_string (ConIndMsg.caller_name,PRTFILE,6,an_delay_str, an_delay_str);
    better_string(ConIndMsg.caller_name,NAMFILE,7,"1", bhelp);
    if (!strcmp(bhelp,"1")) {
      better_string (ConIndMsg.caller_name ,NAMFILE,6,an_delay_str, an_delay_str);
    }
    an_delay_next_call = strtol (an_delay_str,NULL,10);

/* - - - */

    max_rec_time_next_call = config_file_read_ulong(STD_CFG_FILE,DEFAULT_REC_TIME,DEFAULT_REC_TIME_DEF);
    sprintf(max_rec_time_str,"%d",max_rec_time_next_call);
    better_string (ConIndMsg.caller_name,PRTFILE,5,max_rec_time_str, max_rec_time_str);
    better_string(ConIndMsg.caller_name,NAMFILE,7,"1", bhelp);
    if (!strcmp(bhelp,"1")) {
      better_string (ConIndMsg.caller_name ,NAMFILE,5,max_rec_time_str, max_rec_time_str);
    }
    max_rec_time_next_call = strtol (max_rec_time_str,NULL,10);

/* - - - */

    active = config_file_read_ulong(STD_CFG_FILE,CAPITEL_ACTIVE     ,CAPITEL_ACTIVE_DEF)
           & config_file_read_ulong(STD_CFG_FILE,CAPITEL_ACTIVE_TIME,CAPITEL_ACTIVE_TIME_DEF);

    if ((strstr(msn_liste, ConIndMsg.called_name) == NULL) && (strlen(msn_liste) > 1)) {
      num_in_list = 0;
    } else {
      num_in_list = 1;
    }

    if ((active) && (!wait_thread_id) && (num_in_list))
    {
      strcpy (ConIndMsg.caller_org_name, ConIndMsg.caller_name);
      ConIndMsg.is_digital = 0;
      AppSigFunc (1,(void *)&ConIndMsg);
      wait_thread_id = OsStartThread( waitforconn);
    }
  }
  LastRing = GetTickCount();
  xW32UartPurgeRxTxBuff (Handle);
}


void callback_dcd_on  (void)
{
  AppSigFunc (2,"");
  online = 1;
  AppSigFunc (5,"");
  AppSigFunc (3,"");

}
void callback_dcd_off (void)
{
  online = 0;
  if (rec_thread_id) {
    OsStopThread (rec_thread_id);
    rec_thread_id = 0;
  }
  SigDiscInd();
}

short commse_listen             (short val)
{
  return (1);
}

char  commse_connected          (void)
{
  return (online);
}

char  commse_data_available     (void)
{
  if (xW32UartDataAvailable(Handle)) return (1);
  return (0);
}

void  commse_clear_all_buffer   (void)
{
  xW32UartPurgeRxTxBuff (Handle);
}

long  commse_read_block         (char *buff)
{
  long len;

  len = xW32UartDataAvailable(Handle);
  xW32UartReadBlock(buff, len, b3_ind, Handle);
  return (len);
}

void  commse_write_block        (char *buff, short val)
{
  xW32UartWriteBlock(buff, val, b3_conf, Handle);
}

void  commse_disc_req           (void)
{
}

static void _Optlink comm_ctrl_rec( void *arg )
{
  OsSleep(max_rec_time_next_call * 1000);
  commse_disc_req();
}

void  commse_begin_of_record    (void)
{
  rec_thread_id = OsStartThread( comm_ctrl_rec);
}

void commse_name_of_interface (char *name)
{
  strcpy (name, UsedCom);
}

short commse_init( void (*sigf)( short, void * ), char shutdown)
{
  char InitStr[256];
  char NameOfModem[256];

  AppSigFunc = sigf;

  if (xV24UtilSearchModem (
      config_file_read_ulong(STD_CFG_FILE, MODEM_SEARCH_FROM_COM,MODEM_SEARCH_FROM_COM_DEF),
      config_file_read_ulong(STD_CFG_FILE, MODEM_SEARCH_TO_COM  ,MODEM_SEARCH_TO_COM_DEF  ),
      UsedCom, NameOfModem))
  {
    if (shutdown) {
      sprintf(out_msg,"COMAPI: Unable to detect Modem or Ta!\nShutting down.");
      AppSigFunc( 9, (void *) out_msg );
    }
    return (1);
  }

  if (xW32UartOpen(UsedCom,
                   config_file_read_ulong(STD_CFG_FILE, MODEM_BITRATE,MODEM_BITRATE_DEF),
                   W32UART_BYTE_SIZE_8,
                   W32UART_NO_PARITY, W32UART_ONE_STOPBIT, &Handle))
  {
    sprintf(out_msg,"COMAPI: Unable to open %s!", UsedCom);
    AppSigFunc( 9, (void *) out_msg );
    return (1);
  }

  config_file_read_string(STD_CFG_FILE, MODEM_INIT_STR, InitStr, MODEM_INIT_STR_DEF);
  if (xV24UtilSentCommand (Handle, InitStr))
  {
    xW32UartClose(Handle);
    sprintf(out_msg,"COMAPI: Unable to write initstring to %s!", UsedCom);
    AppSigFunc( 9, (void *) out_msg );
    return (1);
  }

  xW32UartSetEventProc (W32UART_EVENT_RING     , callback_ring   , Handle);
  xW32UartSetEventProc (W32UART_EVENT_DCD_ON   , callback_dcd_on , Handle);
  xW32UartSetEventProc (W32UART_EVENT_DCD_OFF  , callback_dcd_off, Handle);
  xW32UartSetEventProc (W32UART_EVENT_RX80_FULL, b3_ind_pre      , Handle);

  return (0);
}

short commse_exit               (void)
{
  if (wait_thread_id) OsStopThread( waitforconn);
  strcpy (UsedCom, "");
  return ((short)xW32UartClose(Handle));
}


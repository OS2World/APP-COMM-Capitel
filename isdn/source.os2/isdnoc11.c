/********************************************************************

   291295 /cw,wf - angelegt
   301295 /wf - set_state() programmiert
   301295 /wf - s2logfile() programmiert
   311295 /wf - capitel.log in capitel.deb umbenannt
   311295 /wf - register wird 10x probiert (delay=300ms)
   010195 /wf - alle puffer vergrî·ert
   240596 /cw - Thread-Signalisierung: Polling durch Event-Sem. ersetzt
                (siehe comm_sigfunc() und comm_poll_sigfunc())
              - capitel.deb Support auskommentiert
   090796 /wf - vierte spalte (reject_cause) in capitel.nam und
                 capitel.prt implementiert.
   130796 /wf - fÅnfte spalte (maximale aufnahmedauer in capitel.
                nam und capitel.prt implementiert.
   150796 /wf - wenn eine verbindung steht -> zweite = busy.
   170796 /wf - wenn eine verbindung steht -> keine signalisierung nach
                oben bei connect_ind
              - wÅrgware fÅr teles (dosOsSleep bei signalisierung) eingebaut :(

*  200497 /cwimmer  - #defines aus cfg_file.h sind uppercase
*  111097 /wfehn    - msg.flags in data_b3_req auf 0 gesetzt (vorher 4).
*
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <process.h>

#include "..\..\..\units\common.src\bastypes.h"

#include "..\..\isdn\source\isdn.h"
#include "..\..\util\source.os2\capi.h"
#include "isdnmsg1.h"
#include "..\..\..\units\common.src\capi_chk.h"
#include "..\..\..\units\common.src\loadcapi.h"
#include "..\..\..\units\common.src\num2nam.h"
#include "..\..\wave\source\wav2alw.h"
#include "..\..\common\source\global.h"
#include "..\..\..\units\common.src\os.h"
#include "..\..\..\units\common.src\cfg_file.h"
#include "..\..\answer\source\answer.h"

#define bit_transparent   0x03
#define transparent       0x04

#define serv_info_mask  0

#define si_mask_fernsprechen   2
#define si_mask_abdienste      4
#define si_mask_daten        128

#define maxbuffer         (CAPI_NUM_B3_BLK * CAPI_NUM_B3_SIZE)

void (*AppSigFunc)( short, void * );

short NextMsgNum( void );

typedef struct st_send_buffers
{
   char buff[CAPI_NUM_B3_SIZE];
   char freeflag;
} sendbt;

short    AppID=0;
long     an_delay_next_call;
long     an_delay_default;
short    MsgNum=0;
char    *MsgBuf;
char    *stack;
char     state;
short    act_plci;
short    my_ncci;
char     buffer[maxbuffer];
char     *buff_read;
char     *buff_write;

HMTX     hmtxsem1;
HMTX     hmtxsem2;
HMTX     hmtxsem3;
HMTX     hmtxsem4;
HMTX     hmtxsem5;
HEV      eventsem;

ULONG ulhelp;
ULONG openFlag = OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS;
ULONG openMode = OPEN_ACCESS_WRITEONLY | OPEN_SHARE_DENYWRITE;
FILE*    pipe_file_info = 0;
char     info_str [300];
FILE*    pipe_file_debug = 0;

char     discind_flag=0;
sendbt   sendb[CAPI_NUM_B3_BLK];
char     out_msg[300];
long     maxread=0;
short    connect_ind_msgnum=0;
char     reject_cause;
Thread_Id_Typ  rec_thread_id  = 0;
Thread_Id_Typ  poll_thread_id = 0;
long     max_rec_time_next_call;
char bhelp[200];
long debug_data_b3 = 0;

void disc_req( void );

void set_pipename_debug (void)
{
  char help[200];

  if (pipe_file_debug) fclose(pipe_file_debug);
  config_file_read_string(STD_CFG_FILE,NAME_OF_DEBUG_FILE,help,NAME_OF_DEBUG_FILE_DEF);
  pipe_file_debug = fopen (help,"a");
}

void set_pipename_info (void)
{
  char help[200];

  if (pipe_file_info) fclose(pipe_file_info);
  config_file_read_string(STD_CFG_FILE,NAME_OF_LOG_FILE,help,NAME_OF_LOG_FILE_DEF);
  pipe_file_info = fopen(help,"a");
  if (pipe_file_info) setbuf( pipe_file_info, NULL );
  info_str[0] = 0;
}

void _Optlink comm_ctrl_rec( void *arg )
{
  OsSleep( (max_rec_time_next_call * 1000) + ((CAPI_NUM_B3_BLK+1) * CAPI_NUM_B3_SIZE / 8)); // yes. it's tricky!!!
  disc_req();
}

void comm11_begin_of_record (void)
{
  rec_thread_id = OsStartThread( comm_ctrl_rec);
}

void s2debug_pipe(char *out_msg)
{
  struct tm *tmbuf;
  time_t tod;
  char str[300];

  time( &tod );
  tmbuf = localtime( &tod );

  sprintf( str, "%02d:%02d:%02d.000  ",tmbuf->tm_hour, tmbuf->tm_min, tmbuf->tm_sec);
  strcat (str,out_msg);
  strcat (str,"\n");

  if (pipe_file_debug) fwrite(str,strlen(str),1,pipe_file_debug);
}

void s2info_pipe(char *out_msg)
{
   strcat (info_str,out_msg);
   if (strchr(info_str,'\n')) {
      fwrite (info_str,strlen(info_str),1,pipe_file_info);
      info_str[0] = 0;
   }
}

void set_state (char new_state)
{
   sprintf (out_msg,"State : changed from 0x%02x to 0x%02x", state, new_state);
   s2debug_pipe(out_msg);
   state = new_state;
}

void putmsg( short id, CapiMsgHeader * _Seg16 msg)
{
  short res;

  DosRequestMutexSem(hmtxsem5,-1);
    res = API_PUT_MESSAGE_11( id, (char * _Seg16) msg );
  DosReleaseMutexSem(hmtxsem5);

  if ((msg->cmd != 0x86) || debug_data_b3) {
    sprintf(out_msg,"PutMsg: 0x%02x 0x%02x (State: %d)", msg->cmd, msg->subcmd, state );
    s2debug_pipe(out_msg);
    c11_analyze_put_msg ((char *)msg, res);
  }

}

void connect_resp(short loc_plci)
{
static TConnectRespMsg msg;

  msg.header.length = 11;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x02;
  msg.header.subcmd = 0x03;
  msg.header.msgnum = connect_ind_msgnum;

  msg.plci          = loc_plci;

  msg.reject        = reject_cause;
  if (reject_cause == 1) msg.reject = 0xbb;  // busy
  if (reject_cause == 2) msg.reject = 0xbe;
  if (reject_cause == 3) msg.reject = 0xb9;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}


void data_b3_conf_2( TDataB3Conf * _Seg16 inmsg )
{
  DosRequestMutexSem(hmtxsem2,-1);
    sendb[inmsg->number].freeflag = 1;
  DosReleaseMutexSem(hmtxsem2);

  AppSigFunc( 5, (void *) inmsg );
}

void data_b3_req( char *outdata, short length )
{
static TDataB3Req msg;
  short blkfree;

  if( length > CAPI_NUM_B3_SIZE )
    return;

  DosRequestMutexSem(hmtxsem2,-1);

  for( blkfree=0 ; (blkfree<CAPI_NUM_B3_BLK) && (sendb[blkfree].freeflag==0) ;
       blkfree ++ ) ;

  if( blkfree == CAPI_NUM_B3_BLK ) {
    DosReleaseMutexSem(hmtxsem2);
    return;
  }

  sendb[blkfree].freeflag = 0;
  DosReleaseMutexSem(hmtxsem2);

  memcpy( &(sendb[blkfree].buff), outdata, length );

  msg.header.length = 19;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x86;
  msg.header.subcmd = 0x00;
  msg.header.msgnum = NextMsgNum();

  msg.ncci = my_ncci;
  msg.data_length = length;
  msg.data = (char * _Seg16) &(sendb[blkfree].buff);
  msg.number = blkfree;
  msg.flags = 0;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}

void disc_resp(TDiscInd * _Seg16 inmsg)
{
static TDiscResp msg;

  msg.header.length = 10;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x04;
  msg.header.subcmd = 0x03;
  msg.header.msgnum = inmsg->header.msgnum;

//  msg.plci = act_plci;
  msg.plci = inmsg->plci;

  if (inmsg->plci == act_plci) set_state (1);

  putmsg( AppID, (CapiMsgHeader *) &msg);
}

void disc_ind( TDiscInd * _Seg16 inmsg )
{
  if (inmsg->plci == act_plci) {
    discind_flag = 1;
    if (rec_thread_id) {
      OsStopThread (rec_thread_id);
      rec_thread_id = 0;
    }
  }
  disc_resp(inmsg);

  if (inmsg->plci == act_plci) AppSigFunc( 6, (void *) inmsg );
}

void disc_conf( TDiscConf * _Seg16 inmsg )
{
  inmsg = inmsg;
}

void disc_b3_resp(TDiscB3Ind * _Seg16 orgmsg)
{
static TDiscB3Resp msg;

  msg.header.length = 10;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x84;
  msg.header.subcmd = 0x03;
  msg.header.msgnum = orgmsg->header.msgnum;

  msg.ncci = my_ncci;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}

void disc_req( void )
{
static  TDiscReq msg;

  set_state (4);

  msg.header.length = 11;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x04;
  msg.header.subcmd = 0x00;
  msg.header.msgnum = NextMsgNum();

  msg.plci = act_plci;
  msg.cause = 0;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}

void disc_b3_ind_2( TDiscB3Ind * _Seg16 inmsg )
{
  AppSigFunc( 4, (void *) inmsg );
  disc_b3_resp(inmsg);
  disc_req();
}

void data_b3_ind_2( TDataB3Ind * _Seg16 inmsg )
{
  TDataB3Resp msg;
//  PBYTE i;

  DosRequestMutexSem(hmtxsem1,-1);

    if (maxbuffer > (tU32)buff_write-(tU32)&buffer+inmsg->data_length) {
       memcpy (buff_write,inmsg->data,inmsg->data_length);
       buff_write += inmsg->data_length;
    } else {
       buff_write = buffer;
       buff_read  = buffer;

       sprintf (out_msg,"ISDNAPI: Buffer Overflow");
       s2debug_pipe(out_msg);
       AppSigFunc( 7, (void *) out_msg );
    }

  DosReleaseMutexSem(hmtxsem1);

  msg.header.length = 11;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x86;
  msg.header.subcmd = 0x03;
  msg.header.msgnum = inmsg->header.msgnum;

  msg.ncci          = my_ncci;
  msg.number        = inmsg->number;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);

  DosRequestMutexSem(hmtxsem3,-1);
      AppSigFunc( 3, (void *) inmsg );
  DosReleaseMutexSem(hmtxsem3);

}

void data_b3_ind_n( TDataB3Ind * _Seg16 inmsg )
{
  TDataB3Resp msg;

  msg.header.length = 11;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x86;
  msg.header.subcmd = 0x03;
  msg.header.msgnum = inmsg->header.msgnum;

  msg.ncci          = my_ncci;
  msg.number        = inmsg->number;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);

}

void connect_b3_act_ind_1( TConnB3ActInd * _Seg16 inmsg )
{
static  TConnB3ActResp msg;

  msg.header.length = 10;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x83;
  msg.header.subcmd = 0x03;
  msg.header.msgnum = inmsg->header.msgnum;

  msg.ncci = my_ncci;

  set_state(3);
  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);

  AppSigFunc( 2, (void *) inmsg );

  OsSleep(40);

  AppSigFunc( 5, (void *) inmsg );
  AppSigFunc( 5, (void *) inmsg );
  AppSigFunc( 5, (void *) inmsg );
  AppSigFunc( 5, (void *) inmsg );
  AppSigFunc( 5, (void *) inmsg );
  AppSigFunc( 5, (void *) inmsg );
  AppSigFunc( 5, (void *) inmsg );

}

void connect_b3_resp(TConnB3Ind * _Seg16 orgmsg)
{
static  TConnB3Resp msg;

  msg.header.length = 12;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x82;
  msg.header.subcmd = 0x03;
  msg.header.msgnum = orgmsg->header.msgnum;

  msg.ncci = my_ncci;
  msg.reject = 0;
  msg.ncpi.length = 0;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}

void connect_b3_ind_1( TConnB3Ind * _Seg16 msg )
{
  my_ncci = msg->ncci;

  connect_b3_resp(msg);
}

void connect_act_resp(TConnActInd * _Seg16 orgmsg)
{
static  TConnActResp msg;

  msg.header.length = 10;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x03;
  msg.header.subcmd = 0x03;
  msg.header.msgnum = orgmsg->header.msgnum;

  msg.plci = act_plci;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}

void connect_act_ind_1( TConnActInd * _Seg16 msg )
{
  connect_act_resp(msg);
}

void listen_b3_conf_1( TListenB3Conf * _Seg16 msg )
{
  if( msg->info ) {
    sprintf(out_msg,"ISDNAPI: LISTEN B3 CONFIRM ERROR:\nINFO=0x%x", msg->info );
    s2debug_pipe(out_msg);
    AppSigFunc( 8, (void *) out_msg );
  } else
    connect_resp(act_plci);
}

void listen_b3_req(void)
{
static  TListenB3Req msg;

  msg.header.length = 10;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x81;
  msg.header.subcmd = 0x00;
  msg.header.msgnum = NextMsgNum();

  msg.plci = act_plci;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}

void select_b3_prot_req(void)
{
static  TSelB3ProtReq msg;

  msg.header.length = 12;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x80;
  msg.header.subcmd = 0x00;
  msg.header.msgnum = NextMsgNum();

  msg.plci          = act_plci;
  msg.b3_prot       = transparent;

  msg.ncpd.length   = 0;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}

void select_b2_prot_conf_1( TSelB2ProtConf * _Seg16 msg )
{
  if( msg->info ) {
    sprintf(out_msg,"ISDNAPI: SELECT B2 PROT CONFIRM ERROR:\nINFO=0x%x", msg->info );
    s2debug_pipe(out_msg);
//    AppSigFunc( 8, (void *) out_msg );
    select_b3_prot_req();
  } else
    select_b3_prot_req();
}

void select_b3_prot_conf_1( TSelB3ProtConf * _Seg16 msg )
{
  if( msg->info ) {
    sprintf(out_msg,"ISDNAPI: SELECT B3 PROT CONFIRM ERROR:\nINFO=0x%x", msg->info );
    s2debug_pipe(out_msg);
//    AppSigFunc( 8, (void *) out_msg );
  } else
    listen_b3_req();
}

void select_b2_prot_req( void )
{
static  TSelB2ProtReq msg;

  msg.header.length = 19;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x40;
  msg.header.subcmd = 0x00;
  msg.header.msgnum = NextMsgNum();

  msg.plci          = act_plci;
  msg.b2_prot       = bit_transparent;

  msg.dlpd.dlpd_length = 7;
  msg.dlpd.data_length = CAPI_NUM_B3_SIZE;
  msg.dlpd.link_a      = 3;
  msg.dlpd.link_b      = 1;
  msg.dlpd.modulo_mode = 8;
  msg.dlpd.win_size    = CAPI_NUM_B3_BLK;
  msg.dlpd.xid         = 0;

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);
}

short NextMsgNum( void )
{
  return (MsgNum++ & 0x7fff);
}

void state_error( short state, char cmd, char subcmd )
{
  sprintf(out_msg,"ISDNAPI: STATE ERROR: State=%d Cmd=0x%x SubCmd=0x%x!!!", state, cmd, subcmd );
  s2debug_pipe(out_msg);
//  AppSigFunc( 7, (void *) out_msg );
}

void listen_conf_0( TListenConfMsg * _Seg16 msg )
{
  if( msg->info ) {
    sprintf(out_msg,"ISDNAPI: LISTEN CONFIRM ERROR:\nINFO=0x%x", msg->info );
    s2debug_pipe(out_msg);
    AppSigFunc( 8, (void *) out_msg );
  } else
    set_state(1);
}

void _Optlink waitforconn( void *arg )
{
  ULONG wait_ms = 0;
  short plci;
  short cnv_flag = 0;
  unsigned long active=1;

  active = config_file_read_ulong(STD_CFG_FILE,CAPITEL_ACTIVE     ,CAPITEL_ACTIVE_DEF)
         & config_file_read_ulong(STD_CFG_FILE,CAPITEL_ACTIVE_TIME,CAPITEL_ACTIVE_TIME_DEF);

  while( (wait_ms < (an_delay_next_call * 1000)) && (!discind_flag))
  {
    OsSleep( 300 );
    if (active) wait_ms += 300;
  }

  if( !discind_flag )
  {
    while (wav2alw_convert_runs()) {
       OsSleep (250);
       cnv_flag = 1;
    }

    if (cnv_flag) OsSleep (1500);

    if (!discind_flag) {
       connect_resp(act_plci);
       select_b2_prot_req( );
    } else {
      reject_cause = 1;
      connect_resp(act_plci);
    }
  } else {
    OsSleep (200);
    reject_cause = 1;
    connect_resp(act_plci);
  }
}

void connect_ind_1( TConnectIndMsg * _Seg16 msg )
{
  char caller_id[150];
  char caller_eaz[150];
  char *number, *pt;
  long i;
  char reject_cause_str[20];
  char max_rec_time_str[20];
  char an_delay_str[20];
  struct tm *tmbuf;
  time_t tod;
  char timestr[200];
  char datestr[200];
  char caller_name[200];
  char eaz_name[200];
  AnsConIndMsg ConIndMsg;
  unsigned long ZeroBeh;
  char zhelp [200];

  set_pipename_debug ();
  set_pipename_info  ();

  debug_data_b3 = config_file_read_ulong (STD_CFG_FILE,DEBUG_DATA_B3,DEBUG_DATA_B3_DEF);

  connect_ind_msgnum = msg->header.msgnum;

  if (state == 1) {
    set_state(2);
    act_plci = msg->plci;
    discind_flag=0;

    if( msg->caller_addr_len == 0 ) {
      config_file_read_string(STD_CFG_FILE,TEXT_UNKNOWN, caller_id,TEXT_UNKNOWN_DEF);
    } else {
      pt     =  &caller_id[0];
      number = &(msg->caller_addr);
      for( i=0 ; i < (msg->caller_addr_len-1) ; i++, number++, pt++ )
        *pt = *number;
      *pt = 0;
    }

//###############
    ZeroBeh = config_file_read_ulong(STD_CFG_FILE,ZERO_BEHAVIOR,ZERO_BEHAVIOR_DEF);
    switch (ZeroBeh)
    {
      case 1: if (caller_id[0] == '0')     // delete leading zero
              {
                strcpy (zhelp, caller_id+1);
                strcpy (caller_id, zhelp);
              }
              break;
      case 2: strcpy (zhelp, "0");        // add leading zero
              strcat (zhelp, caller_id);
              strcpy (caller_id, zhelp);
              break;
    }
//###############

    strcpy (ConIndMsg.caller_name,caller_id);
    answer_cti (ConIndMsg.caller_name);
    ConIndMsg.called_name[0] = msg->req_eaz;
    ConIndMsg.called_name[1] = 0;
    if (msg->req_service == 7) {
      ConIndMsg.is_digital = 1;
    } else {
      ConIndMsg.is_digital = 0;
    }
    AppSigFunc( 1, (void *) &ConIndMsg );

    time( &tod );
    tmbuf = localtime( &tod );

    tmbuf->tm_year = tmbuf->tm_year + 1900;

    sprintf( datestr, "%02d/%02d/%04d",tmbuf->tm_mday,tmbuf->tm_mon+1, tmbuf->tm_year );
    sprintf( timestr, "%02d:%02d:%02d" ,tmbuf->tm_hour, tmbuf->tm_min, tmbuf->tm_sec);
    s2info_pipe (datestr);
    s2info_pipe (" ");
    s2info_pipe (timestr);
    s2info_pipe (", ");

    s2info_pipe (caller_id);
    s2info_pipe (", ");
    better_string(caller_id,NAMFILE,2,caller_id, caller_name);
    s2info_pipe (caller_name);
    s2info_pipe (", ");

    caller_eaz[0] = msg->req_eaz;
    caller_eaz[1] = 0;
    s2info_pipe (caller_eaz);
    s2info_pipe (", ");
    better_string(caller_eaz,PRTFILE,2,caller_eaz, eaz_name);
    s2info_pipe (eaz_name);
    s2info_pipe (", ");

    strcpy (reject_cause_str,"0");
    better_string (caller_eaz,PRTFILE,4,reject_cause_str, reject_cause_str);
    better_string(ConIndMsg.caller_name,NAMFILE,7,"1",bhelp);
    if (!strcmp(bhelp,"1")) {
      better_string (caller_id ,NAMFILE,4,reject_cause_str, reject_cause_str);
    }
    reject_cause = reject_cause_str[0]-48;
    s2info_pipe (reject_cause_str);
    s2info_pipe (", ");

    max_rec_time_next_call = config_file_read_ulong(STD_CFG_FILE,DEFAULT_REC_TIME,DEFAULT_REC_TIME_DEF);
    sprintf(max_rec_time_str,"%d",max_rec_time_next_call);
    better_string (caller_eaz,PRTFILE,5,max_rec_time_str, max_rec_time_str);
    better_string(ConIndMsg.caller_name,NAMFILE,7,"1", bhelp);
    if (!strcmp(bhelp,"1")) {
      better_string (caller_id ,NAMFILE,5,max_rec_time_str, max_rec_time_str);
    }
    max_rec_time_next_call = strtol (max_rec_time_str,NULL,10);
    s2info_pipe (max_rec_time_str);
    s2info_pipe (", ");

    an_delay_next_call = an_delay_default;
    sprintf(an_delay_str,"%d",an_delay_next_call);
    better_string (caller_eaz,PRTFILE,6,an_delay_str, an_delay_str);
    better_string(ConIndMsg.caller_name,NAMFILE,7,"1", bhelp);
    if (!strcmp(bhelp,"1")) {
      better_string (caller_id ,NAMFILE,6,an_delay_str, an_delay_str);
    }
    an_delay_next_call = strtol (an_delay_str,NULL,10);
    s2info_pipe (an_delay_str);
    s2info_pipe ("\n");

    if (reject_cause) {
      connect_resp(msg->plci);
    } else {
      OsStartThread (waitforconn);
    }

  } else {
    reject_cause = 1;
    connect_resp(msg->plci);
  }
}

void int_sigfunc(void *arg)
{
static  char * _Seg16 pt;
static  CapiMsgHeader * _Seg16 cmsg;
short res;

  while( (res = API_GET_MESSAGE_11( AppID, &pt )) == 0 )
  {
    cmsg = (CapiMsgHeader * _Seg16) pt;

    if ((cmsg->cmd != 0x86) || debug_data_b3) {
       sprintf(out_msg,"GetMsg: 0x%02x 0x%02x (State: %d)", cmsg->cmd, cmsg->subcmd, state );
       s2debug_pipe(out_msg);
       c11_analyze_get_msg ((char *)pt,res);
    }

    switch( state ) {                             /* State Machine */

      /**********l*************************************************************/
      /*** State: Answ. Machine Inactive */
      /***********************************************************************/

      case 0:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0501: listen_conf_0         ((TListenConfMsg * _Seg16) cmsg ); break;
          case 0x8602: data_b3_ind_n         ((TDataB3Ind     * _Seg16) cmsg ); break;
          default:     state_error           (state, cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Answ. Machine waiting for incoming call */
      /***********************************************************************/

      case 1:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0202: connect_ind_1         ((TConnectIndMsg * _Seg16) cmsg ); break;
          case 0x0401: disc_conf             ((TDiscConf      * _Seg16) cmsg ); break;
          case 0x0501: listen_conf_0         ((TListenConfMsg * _Seg16) cmsg ); break;
          case 0x8602: data_b3_ind_n         ((TDataB3Ind     * _Seg16) cmsg ); break;
          default:     state_error           (state, cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Answ. Machine building up connection */
      /***********************************************************************/

      case 2:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0202: connect_ind_1         ((TConnectIndMsg * _Seg16) cmsg ); break;
          case 0x4001: select_b2_prot_conf_1 ((TSelB2ProtConf * _Seg16) cmsg ); break;
          case 0x8001: select_b3_prot_conf_1 ((TSelB3ProtConf * _Seg16) cmsg ); break;
          case 0x8101: listen_b3_conf_1      ((TListenB3Conf  * _Seg16) cmsg ); break;
          case 0x0302: connect_act_ind_1     ((TConnActInd    * _Seg16) cmsg ); break;
          case 0x8202: connect_b3_ind_1      ((TConnB3Ind     * _Seg16) cmsg ); break;
          case 0x8302: connect_b3_act_ind_1  ((TConnB3ActInd  * _Seg16) cmsg ); break;
          case 0x8602: data_b3_ind_n         ((TDataB3Ind     * _Seg16) cmsg ); break;
          case 0x0402: disc_ind              ((TDiscInd       * _Seg16) cmsg ); break;
          case 0x0501: listen_conf_0         ((TListenConfMsg * _Seg16) cmsg ); break;
          case 0x0401: disc_conf             ((TDiscConf      * _Seg16) cmsg ); break;
          default:     state_error           (state, cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Data Transfer  */
      /***********************************************************************/

      case 3:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0402: disc_ind              ((TDiscInd       * _Seg16) cmsg ); break;
          case 0x0202: connect_ind_1         ((TConnectIndMsg * _Seg16) cmsg ); break;
          case 0x8602: data_b3_ind_2         ((TDataB3Ind     * _Seg16) cmsg ); break;
          case 0x8402: disc_b3_ind_2         ((TDiscB3Ind     * _Seg16) cmsg ); break;
          case 0x8601: data_b3_conf_2        ((TDataB3Conf    * _Seg16) cmsg ); break;
          default:     state_error           (state, cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Disconnecting  */
      /***********************************************************************/

      case 4:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0202: connect_ind_1         ((TConnectIndMsg * _Seg16) cmsg ); break;
          case 0x8601: data_b3_conf_2        ((TDataB3Conf    * _Seg16) cmsg ); break;
          case 0x8602: data_b3_ind_n         ((TDataB3Ind     * _Seg16) cmsg ); break;
          case 0x0401: disc_conf             ((TDiscConf      * _Seg16) cmsg ); break;
          case 0x0402: disc_ind              ((TDiscInd       * _Seg16) cmsg ); break;
          case 0x8402: disc_b3_ind_2         ((TDiscB3Ind     * _Seg16) cmsg ); break;
          default:     state_error           (state, cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Unknown  */
      /***********************************************************************/

      default:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          default:     state_error( state, cmsg->cmd, cmsg->subcmd ); break;
        }
        break;
    }
  }
  return;
}

void _Optlink comm_poll_sigfunc( void *arg )
{
  ULONG cnt;

  for (;;) {
    DosWaitEventSem( eventsem, -1 );            /* warten bis was zu tun ist */
    DosResetEventSem( eventsem, &cnt );         /* reset (sonst funktioniert */
    DosRequestMutexSem(hmtxsem4,-1);
      int_sigfunc (NULL);
    DosReleaseMutexSem(hmtxsem4);
    OsSleep (50);                            /* wÅrgware fÅr teles-capi) */
  }
}

void APIENTRY16 comm_sigfunc( void )
{
  DosPostEventSem(eventsem);             /* eigentliche sigfunc aufwecken */
}

void comm11_clear_read_buffer (void)
{
  DosRequestMutexSem(hmtxsem1,-1);
    buff_read  = buffer;
    buff_write = buffer;
  DosReleaseMutexSem(hmtxsem1);
}

void comm11_clear_write_buffer (void)
{
  long t;

  DosRequestMutexSem(hmtxsem2,-1);
    for (t=0;t<CAPI_NUM_B3_BLK;t++) sendb[t].freeflag = 1;
  DosReleaseMutexSem(hmtxsem2);
}

void comm11_clear_all_buffer (void)
{
   comm11_clear_read_buffer();
   comm11_clear_write_buffer();
}

void _Optlink poll_capi( void *arg )
{
  for (;;) {
    DosRequestMutexSem(hmtxsem4,-1);
      int_sigfunc(NULL);
    DosReleaseMutexSem(hmtxsem4);
    OsSleep (50);
  }
}


short comm11_init( void (*sigf)( short, void * ), char shutdown)
{

  short cnt;

  AppSigFunc = sigf;

  if( !LoadCapi11() )
  {
    sprintf(out_msg,"ISDNAPI: Unable to load CAPI.DLL!\nShutting down.");
    if (shutdown) {
      s2debug_pipe(out_msg);
      AppSigFunc( 9, (void *) out_msg );
    }
    return 1;
  }

  if( !API_INSTALLED_11() ) return 1;

  set_state(0);

  DosCreateMutexSem("\\sem32\\ctsem1",&hmtxsem1,0,FALSE);
  DosCreateMutexSem("\\sem32\\ctsem2",&hmtxsem2,0,FALSE);
  DosCreateMutexSem("\\sem32\\ctsem3",&hmtxsem3,0,FALSE);
  DosCreateMutexSem("\\sem32\\ctsem4",&hmtxsem4,0,FALSE);
  DosCreateMutexSem("\\sem32\\ctsem5",&hmtxsem5,0,FALSE);
  DosCreateEventSem("\\sem32\\ctevt1",&eventsem,0,FALSE );

  OsStartThread( comm_poll_sigfunc);

  comm11_clear_all_buffer();

  MsgBuf = (char *) malloc( ((180*CAPI_NUM_MSGS) +
                    (CAPI_NUM_B3_CON*CAPI_NUM_B3_BLK*CAPI_NUM_B3_SIZE)) );

  if( MsgBuf == NULL )
  {
    sprintf(out_msg,"ISDNAPI: Not enough Memory!\nShutting down.");
    s2debug_pipe(out_msg);
    AppSigFunc( 9, (void *) out_msg );
    return 1;
  }

  AppID = 0;
  cnt   = 0;

  while ((AppID == 0) && (cnt++ < 10)) {
     AppID = API_REGISTER_11( MsgBuf, CAPI_NUM_MSGS, CAPI_NUM_B3_CON,CAPI_NUM_B3_BLK, CAPI_NUM_B3_SIZE );
     if (AppID == 0) OsSleep(300);
  }

  if( !AppID )
  {
    sprintf(out_msg,"ISDNAPI: API_REGISTER failed!\nShutting down.");
    s2debug_pipe(out_msg);
    AppSigFunc( 9, (void *) out_msg );
    free( MsgBuf );
    return 1;
  }

  if (config_file_read_ulong(STD_CFG_FILE,SET_SIGNAL,SET_SIGNAL_DEF)) {
    if( API_SET_SIGNAL_11( AppID, comm_sigfunc)) {
      poll_thread_id = OsStartThread( poll_capi);
    }
  } else {
    poll_thread_id = OsStartThread( poll_capi);
  }

  set_pipename_debug ();
  set_pipename_info  ();
  capi_analyze_init (0,pipe_file_debug);

  return 0;
}

short comm11_exit( void )
{
  if (poll_thread_id){
    OsStopThread (poll_thread_id);
    poll_thread_id = 0;
  }
  if (rec_thread_id) {
    OsStopThread (rec_thread_id);
    rec_thread_id = 0;
  }
  if( AppID ) API_RELEASE_11( AppID );
  if( MsgBuf != NULL ) free( MsgBuf );
  fclose (pipe_file_debug);
  fclose (pipe_file_info);
  UnLoadCapi11();
  DosCloseMutexSem(hmtxsem1);
  DosCloseMutexSem(hmtxsem2);
  DosCloseMutexSem(hmtxsem3);
  DosCloseMutexSem(hmtxsem4);
  DosCloseMutexSem(hmtxsem5);
  DosCloseEventSem(eventsem);
  return 0;
}

short comm11_listen(short act)
{
  static  TListenReqMsg msg;
  char eaz_list[20] = "";
  char help[20];
  unsigned short serv_eaz = 0;
  short i;
  short eaz_error = 0;

  for (i='0';i<='9';i++) {
    help[0] = i;
    help[1] = 0;
    better_string(help,PRTFILE,7,"0", bhelp);
    if (!strcmp(bhelp,"1")) {
      strcat(eaz_list,help);
      strcat(eaz_list,"|");
    }
  }

  if (eaz_error) AppSigFunc( 7, (void *) "EAZ-Portsetting contain more than one digit!" );

  eaz_list[strlen(eaz_list)-1] = 0;

  for( i=0 ; i < (short)strlen(eaz_list) ; i++ ) {
    if (eaz_list[i] != '|') serv_eaz = serv_eaz | (1<<((short)((eaz_list[i])-48)));
  }

  if (serv_eaz == 0) {
    serv_eaz = 2+4+8+16+32+64+128+256+512;  // kein angeben : nehme alle an.
  }

  an_delay_default   = config_file_read_ulong (STD_CFG_FILE,DEFAULT_ANSW_DELAY,DEFAULT_ANSW_DELAY_DEF);
  an_delay_next_call = an_delay_default;

  comm11_clear_all_buffer();

  msg.header.length = 17;
  msg.header.appid  = AppID;
  msg.header.cmd    = 0x05;
  msg.header.subcmd = 0x00;
  msg.header.msgnum = NextMsgNum();

  msg.controller       = ((char)(config_file_read_ulong(STD_CFG_FILE,SERV_CONTROLLER,SERV_CONTROLLER_DEF)-1));
  msg.info_mask        = serv_info_mask;

//printf ("(eaz=%d)",serv_eaz);

  if (act) {
    msg.serviced_eaz     = serv_eaz;
    if (config_file_read_ulong(STD_CFG_FILE,CAPITEL_IS_CALLER_ID,CAPITEL_IS_CALLER_ID_DEF)) {
      msg.serviced_si_mask = si_mask_fernsprechen + si_mask_abdienste + si_mask_daten;
    } else {
      msg.serviced_si_mask = si_mask_fernsprechen + si_mask_abdienste;
    }
  } else {
    msg.serviced_eaz     = 0;
    msg.serviced_si_mask = 0;
  }

  putmsg( AppID, (CapiMsgHeader * _Seg16) &msg);

  return 0;
}

char comm11_connected( void )
{
  return (state==3);
}

char comm11_data_available (void)
{
   return (buff_read != buff_write);
}

long comm11_read_block (char *buff)
{
  long ret=0;

  if( state != 3 ) return 0;

  DosRequestMutexSem(hmtxsem1,-1);
    ret = (tU32)buff_write-(tU32)buff_read;
    memcpy (buff,buff_read,ret);
    buff_read  = buffer;
    buff_write = buffer;
  DosReleaseMutexSem(hmtxsem1);

  return (ret);
}

char comm11_can_write (void)
{
   short t;
   short found = 0;

   if( state != 3 )
     return 1;

   for (t=0;t<CAPI_NUM_B3_BLK;t++) found += sendb[t].freeflag;

   return (found > 0);
}

void comm11_write_block( char *buf, short length )
{
  if( state != 3 ) return;

  data_b3_req( buf, length );

}

void comm11_disc_req(void)
{
  disc_req();
}

void comm11_name_of_interface (char *name)
{
  strcpy (name,"CAPI 1.1");
}


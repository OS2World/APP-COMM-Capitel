/********************************************************************
*
*  280497 /cw,wf    - angelegt aus capi 1.1 modul
*  111097 /wfehn    - msg.flags in data_b3_req auf 0 gesetzt (vorher 4).
*                     jetzt l„uft capitel auch auf diehl diva.
*  011297 /wfehn    - zeigt nur anrufe an, die auch in msn-liste stehen
*  010398 /cawim    - capi20.h wurde verschoben
*
********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef UNIX
#include <process.h>
#endif

#include "../../../units/common.src/bastypes.h"

#include "../../../units/common.src/os.h"
#include "../../common/source/version.h"
#include "../../isdn/source/isdn.h"
#include "../../../units/common.src/capi20.h"
#include "../../../units/common.src/isdnmsg2.h"
#include "../../../units/common.src/capi_chk.h"
#include "../../../units/common.src/loadcapi.h"
#include "../../../units/common.src/num2nam.h"
#include "../../wave/source/wav2alw.h"
#include "../../common/source/global.h"
#include "../../../units/common.src/cfg_file.h"
#include "../../answer/source/answer.h"

#define serv_info_mask  0

#define maxbuffer         (CAPI_NUM_B3_BLK * CAPI_NUM_B3_SIZE)

#ifndef OS2
#define MAKEUSHORT(l, h) (((BYTE)(l)) | ((BYTE)(h)) << 8)
#endif


#define cip_value_speech         1
#define cip_mask_speech          2

#define cip_value_digital        2
#define cip_mask_digital         4

#define cip_value_31audio        4
#define cip_mask_31audio        16

#define cip_value_telephony     16
#define cip_mask_telephony   65536

extern int fernabfrage;
extern unsigned long calllength;

void (*AppSigFunc)( short, void * );

short NextMsgNum( void );

typedef struct st_send_buffers
{
   char buff[CAPI_NUM_B3_SIZE];
   char freeflag;
} sendbt;

static DWORD    AppID=0;
static long     an_delay_next_call;
static long     an_delay_default;
static WORD     MsgNum=0;
static DWORD    conn_state;
static DWORD    act_plci;
static DWORD    my_ncci;
static char     buffer[maxbuffer];
static char     *buff_read;
static char     *buff_write;
static unsigned long flagDoSignal;

static HMTX     hmtxsem1;
static HMTX     hmtxsem2;
static HMTX     hmtxsem3;
static HMTX     hmtxsem4;
static HMTX     hmtxsem5;
#ifdef OS2
static HEV      eventsem;
#endif

//static unsigned long ulhelp;
static FILE*    pipe_file_info = 0;
static char     info_str [300];
static FILE*    pipe_file_debug = 0;

static char     discind_flag=0;
static sendbt   sendb[CAPI_NUM_B3_BLK];
static char     out_msg[3000];
//static long     maxread=0;
static short    connect_ind_msgnum=0;
static char     reject_cause;
static Thread_Id_Typ  rec_thread_id  = 0;
static Thread_Id_Typ  poll_thread_id = 0;
static long     max_rec_time_next_call;
static char msn_liste[500];
static char bhelp[200];
static long debug_data_b3 = 0;
static short num_in_list = 0;
short is_callback = 0;

static char caller_id_org [150];
static char caller_msn_org[150];
static char online=0;
static char enter_wait_for_signal = 1;
static char pipename_debug[200] = {0};


static void disc_req( void );

static void set_pipename_debug (void)
{
  if (pipe_file_debug) fclose(pipe_file_debug);
  config_file_read_string(STD_CFG_FILE,NAME_OF_DEBUG_FILE,pipename_debug,NAME_OF_DEBUG_FILE_DEF);
  pipe_file_debug = fopen (pipename_debug,"a");
}

static void set_pipename_info (void)
{
  char help[200];

  if (pipe_file_info) fclose(pipe_file_info);
  config_file_read_string(STD_CFG_FILE,NAME_OF_LOG_FILE,help,NAME_OF_LOG_FILE_DEF);
  pipe_file_info = fopen(help,"a");
  if (pipe_file_info) setbuf( pipe_file_info, NULL );
  info_str[0] = 0;
}

static void _Optlink comm_ctrl_rec( void *arg )
{
  OsSleep( (max_rec_time_next_call * 1000) + ((CAPI_NUM_B3_BLK+1) * CAPI_NUM_B3_SIZE / 8)); // yes. it's tricky!!!
  disc_req();
}

void comm20_begin_of_record (void)
{
//problem   rec_thread_id = OsStartThread( comm_ctrl_rec);
}

static void s2debug_pipe(char *out_msg)
{
  struct tm *tmbuf;
  time_t tod;
  char str[3000];

  time( &tod );
  tmbuf = localtime( &tod );

  sprintf( str, "%02d:%02d:%02d.000  ",tmbuf->tm_hour, tmbuf->tm_min, tmbuf->tm_sec);
  strcat (str,out_msg);
  strcat (str,"\n");

  if (pipe_file_debug)
  {
    fwrite(str,strlen(str),1,pipe_file_debug);
  }
}

static void s2info_pipe(char *out_msg)
{
   strcat (info_str,out_msg);
   if (strchr(info_str,'\n')) {
      if (pipe_file_info) fwrite (info_str,strlen(info_str),1,pipe_file_info);
      info_str[0] = 0;
   }
}

static void set_state (DWORD new_state)
{
   sprintf (out_msg,"State : changed from %d to %d", (int)conn_state, (int)new_state);
   conn_state = new_state;
   s2debug_pipe(out_msg);
}

static void putmsg(CapiMsgHeader * msg)
{

  if ((msg->cmd != 0x86) || debug_data_b3) {
    sprintf(out_msg,"PutMsg: 0x%02x 0x%02x (State: %d)", msg->cmd, msg->subcmd, (int)conn_state );
    s2debug_pipe(out_msg);
    c20_analyze_put_msg ((char *)msg,0xffff);
  }

  OSRequestMutexSem(&hmtxsem5,-1);
  API_PUT_MESSAGE_20( AppID, (char *) msg );
  OSReleaseMutexSem(&hmtxsem5);
}

static void connect_conf( TConnConf *inmsg )
{
  if (inmsg->info == 0) {
    act_plci = inmsg->plci;
    set_state(2);
  }
}

static void connect_b3_conf( TConnB3Conf *inmsg )
{
  if (inmsg->info == 0) {
    my_ncci = inmsg->ncci;
  }
}


static void connect_resp(long loc_plci)
{
  static TConnectRespMsg msg;

  msg.header.length = sizeof(msg.header)+20; // problem 20;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x02;
  msg.header.subcmd = (BYTE)0x83;
  msg.header.msgnum = connect_ind_msgnum;

  msg.plci          = loc_plci;

  msg.reject        = reject_cause;
  if (reject_cause == 1) msg.reject = (BYTE) 3;  // busy
  if (reject_cause == 2) msg.reject = (BYTE) 1;  // ignore
  if (reject_cause == 3) msg.reject = (BYTE) 8;  // out of order

  msg.b_protocol_len = 9; // problem 9;

  msg.b1_protocol = 0x01;
  msg.b2_protocol = 0x01;
  msg.b3_protocol = 0;

  msg.b1_configuration = 0;
  msg.b2_configuration = 0;
  msg.b3_configuration = 0;
  
//  msg.global_configuration = 0; // problem

  msg.ext[0] = 0;
  msg.ext[1] = 0;
  msg.ext[2] = 0;
  msg.ext[3] = 0;

  putmsg((CapiMsgHeader *) &msg);
}


static void data_b3_conf_2( TDataB3Conf * inmsg )
{
  OSRequestMutexSem(&hmtxsem2,-1);
    sendb[inmsg->number].freeflag = 1;
  OSReleaseMutexSem(&hmtxsem2);
  AppSigFunc( 5, (void *) inmsg );
}

static void data_b3_req( char *outdata, short length )
{
static TDataB3Req msg;
  int  blkfree;

  if( length > CAPI_NUM_B3_SIZE )
    return;

  OSRequestMutexSem(&hmtxsem2,-1);

  for( blkfree=0 ; (blkfree<CAPI_NUM_B3_BLK) && (sendb[blkfree].freeflag==0) ; blkfree ++ ) ;

  if( blkfree == CAPI_NUM_B3_BLK )
  {
    OSReleaseMutexSem(&hmtxsem2);
    return;
  }

  sendb[blkfree].freeflag = 0;
  OSReleaseMutexSem(&hmtxsem2);

  memcpy( &(sendb[blkfree].buff), outdata, length );

  msg.header.length = sizeof(msg.header)+14;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x86;
  msg.header.subcmd = (BYTE)0x80;
  msg.header.msgnum = NextMsgNum();

  msg.ncci = my_ncci;
  msg.data = (char *) &(sendb[blkfree].buff);
  msg.data_length = length;
  msg.number = blkfree;
  msg.flags = 0;

  putmsg((CapiMsgHeader *) &msg);
}

static void disc_resp(TDiscInd *inmsg)
{
  static TDiscResp msg;
  static TConnReq  msg2;
  short i;
  AnsConIndMsg ConIndMsg;
  char num[200];
  char out_num[200];
  char active_flg;
  char active_str[200];

  msg.header.length = 12;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x04;
  msg.header.subcmd = (BYTE)0x83;
  msg.header.msgnum = inmsg->header.msgnum;

  msg.plci = inmsg->plci;

  putmsg((CapiMsgHeader *) &msg);

  if (inmsg->plci == act_plci) {
    set_state (1);

    config_file_read_string(STD_CFG_FILE,CALL_BACK_ACTIVE,active_str,CALL_BACK_ACTIVE_DEF);
    active_flg = atoi(check_time(active_str));

    if ((is_callback == 0)
    && (active_flg)
    && (calllength > config_file_read_ulong(STD_CFG_FILE,CALL_BACK_LIMIT,CALL_BACK_LIMIT_DEF))) {
      config_file_read_string(STD_CFG_FILE,CALL_BACK_NUMBER,num,CALL_BACK_NUMBER_DEF);
      better_string (caller_msn_org , PRTFILE, 9, num, num);
      better_string (caller_id_org  , NAMFILE, 9, num, num);
      strcpy (num,check_time(num));

      if (strlen(num) && online) {
        OsSleep(5000);

        is_callback = 1;

        // connect_request conn_req con_req

        msg2.header.length = 33+strlen(num);
        msg2.header.appid  = (WORD)AppID;
        msg2.header.cmd    = (BYTE)0x02;
        msg2.header.subcmd = (BYTE)0x80;
        msg2.header.msgnum = NextMsgNum();
        msg2.controller    = ((BYTE)config_file_read_ulong(STD_CFG_FILE,SERV_CONTROLLER,SERV_CONTROLLER_DEF));
        msg2.cip_value     = ((BYTE)config_file_read_ulong(STD_CFG_FILE,OUT_CIP_VALUE,OUT_CIP_VALUE_DEF));
        msg2.ext[0] = 1+strlen(num);
        msg2.ext[1] = 0x80;
        memcpy (&msg2.ext[2],num,strlen(num));

        i = strlen(num)+2;

// ################################

        config_file_read_string(STD_CFG_FILE,OUTGOING_NUMBER,out_num,OUTGOING_NUMBER_DEF);
        if (out_num[0] == 0)
        {
          msg2.ext[i   ] = 0; // no calling party number
        }
        else
        {
          msg2.ext[i   ] = strlen(out_num)+2; // length
          msg2.ext[i+ 1] = 0;    // calling party number etsi 300 102-2 byte 0
          msg2.ext[i+ 2] = 0x80; // calling party number etsi 300 102-2 byte 1
          memcpy (&msg2.ext[i+3],out_num,strlen(out_num));
          i += strlen(out_num)+2;
          msg2.header.length += strlen(out_num)+2;
        }

// ################################
        msg2.ext[i+ 1] = 0; // called party subadress
        msg2.ext[i+ 2] = 0; // calling party subadress

        msg2.ext[i+ 3] = 9;
        msg2.ext[i+ 4] = 1; // b1 = transp
        msg2.ext[i+ 5] = 0;
        msg2.ext[i+ 6] = 1; // b2 = transp
        msg2.ext[i+ 7] = 0;
        msg2.ext[i+ 8] = 0; // b3 = transp
        msg2.ext[i+ 9] = 0;
        msg2.ext[i+10] = 0; // b1 config = 0
        msg2.ext[i+11] = 0; // b2 config = 0
        msg2.ext[i+12] = 0; // b3 config = 0

        msg2.ext[i+13] = 0; // bc
        msg2.ext[i+14] = 0; // llc
        msg2.ext[i+15] = 0; // hlc
        msg2.ext[i+16] = 0; // add info

        putmsg((CapiMsgHeader *) &msg2);

        strcpy (ConIndMsg.caller_name    , APPNAME);
        strcpy (ConIndMsg.caller_org_name, APPNAME);
        strcpy (ConIndMsg.called_name    , num);
        ConIndMsg.is_digital = 0;
        AppSigFunc (1,(void *)&ConIndMsg);
        fernabfrage = 1;
      }
    }
  }
}

static void disc_ind( TDiscInd *inmsg )
{
  if (inmsg->plci == act_plci) {
    discind_flag = 1;
    if (rec_thread_id) {
      OsStopThread (rec_thread_id);
      rec_thread_id = 0;
    }
  }
  if ((inmsg->plci == act_plci) && (num_in_list)) {
    AppSigFunc( 6, (void *) inmsg );
  }  

  disc_resp(inmsg);
  if (inmsg->plci == act_plci) {
    API_RELEASE_20 ( AppID );
    OsSleep(100);
    API_REGISTER_20( CAPI_MSGBUF_SIZE, CAPI_NUM_B3_CON, CAPI_NUM_B3_BLK, CAPI_NUM_B3_SIZE, &AppID );
    comm20_listen (1);
  }
}

static void disc_conf( TDiscConf *inmsg )
{
  inmsg = inmsg;
}

static void disc_b3_resp(TDiscB3Ind *orgmsg)
{
static TDiscB3Resp msg;

  msg.header.length = 12;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x84;
  msg.header.subcmd = (BYTE)0x83;
  msg.header.msgnum = orgmsg->header.msgnum;

  msg.ncci = my_ncci;

  putmsg((CapiMsgHeader *) &msg);
}

static void disc_req( void )
{
static  TDiscReq msg;

  set_state (4);

  msg.header.length = 13;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x04;
  msg.header.subcmd = (BYTE)0x80;
  msg.header.msgnum = NextMsgNum();

  msg.plci = act_plci;

  msg.additional_info[0] = 0;

  putmsg((CapiMsgHeader *) &msg);
}

static void disc_b3_ind_2( TDiscB3Ind * inmsg )
{
  AppSigFunc( 4, (void *) inmsg );
  disc_b3_resp(inmsg);
  disc_req();
}

static void data_b3_ind_2( TDataB3Ind * inmsg )
{
  TDataB3Resp msg;

  OSRequestMutexSem(&hmtxsem1,-1);

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

  OSReleaseMutexSem(&hmtxsem1);

  msg.header.length = sizeof(msg.header)+6;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x86;
  msg.header.subcmd = (BYTE)0x83;
  msg.header.msgnum = inmsg->header.msgnum;

  msg.ncci          = my_ncci;
  msg.number        = inmsg->number;

  putmsg((CapiMsgHeader *) &msg);

  OSRequestMutexSem(&hmtxsem3,-1);
    AppSigFunc( 3, (void *) inmsg );
  OSReleaseMutexSem(&hmtxsem3);
}

static void data_b3_ind_n( TDataB3Ind * inmsg )
{
  TDataB3Resp msg;

  msg.header.length = sizeof(msg.header)+6;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x86;
  msg.header.subcmd = (BYTE)0x83;
  msg.header.msgnum = inmsg->header.msgnum;

  msg.ncci          = my_ncci;
  msg.number        = inmsg->number;

  putmsg((CapiMsgHeader *) &msg);

}

static void connect_b3_act_ind_1( TConnB3ActInd * inmsg )
{
static  TConnB3ActResp msg;

  short i;

  online = 1;

  msg.header.length = 12;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x83;
  msg.header.subcmd = (BYTE)0x83;
  msg.header.msgnum = inmsg->header.msgnum;

  msg.ncci = my_ncci;

  set_state(3);
  putmsg((CapiMsgHeader *) &msg);

  AppSigFunc( 2, (void *) inmsg );

  OsSleep(100);

  for (i=1;i<= CAPI_NUM_B3_BLK;i++) AppSigFunc( 5, (void *) inmsg ); // trick

}

static void connect_b3_resp(TConnB3Ind * orgmsg)
{
static  TConnB3Resp msg;

  msg.header.length = sizeof(msg.header)+7;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x82;
  msg.header.subcmd = (BYTE)0x83;
  msg.header.msgnum = orgmsg->header.msgnum;

  msg.ncci = my_ncci;
  msg.reject = 0;
  msg.ncpi.length = 0;

  putmsg((CapiMsgHeader *) &msg);
}

static void connect_b3_ind_1( TConnB3Ind * msg )
{
  my_ncci = msg->ncci;

  connect_b3_resp(msg);
}

static void connect_act_resp(TConnActInd *orgmsg)
{
static  TConnActResp msg;

  msg.header.length = 12;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x03;
  msg.header.subcmd = (BYTE)0x83;
  msg.header.msgnum = orgmsg->header.msgnum;

  msg.plci = act_plci;

  putmsg((CapiMsgHeader *) &msg);
}

static void connect_act_ind_1( TConnActInd *msg )
{
  static TConnB3Req msg2;

  connect_act_resp(msg);
  if (is_callback) {
    msg2.header.length = 13;
    msg2.header.appid  = (WORD)AppID;
    msg2.header.cmd    = (BYTE)0x82;
    msg2.header.subcmd = (BYTE)0x80;
    msg2.header.msgnum = NextMsgNum();
    msg2.plci = act_plci;
    msg2.ncpi[0] = 0;
    putmsg((CapiMsgHeader *) &msg2);
  }
}

short NextMsgNum( void )
{
  return (MsgNum++ & 0x7fff);
}

static void state_error(char cmd, char subcmd )
{
  sprintf(out_msg,"ISDNAPI: STATE ERROR: State=%d Cmd=0x%x SubCmd=0x%x!!!", (int)conn_state, (char)cmd, (char)subcmd );
  s2debug_pipe(out_msg);
printf ("\n%s\n",out_msg); // problem
//  AppSigFunc( 7, (void *) out_msg );
}

static void listen_conf_0( TListenConfMsg *msg )
{
  if( msg->info ) {
    sprintf(out_msg,"ISDNAPI: LISTEN CONFIRM ERROR:\nINFO=0x%x", msg->info );
    s2debug_pipe(out_msg);
    AppSigFunc( 8, (void *) out_msg );
  } else
    set_state(1);
}

static void _Optlink waitforconn( void *arg )
{
  long wait_ms = 0;
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

//*****************************************************************************
// function : etsi_str_calling_pn
// in       : zeiger auf capi-struktur
// out      : nullterminierter nummernstring

unsigned char *etsi_str_calling_pn (unsigned char *cpn)
{
  static unsigned char etsi_out[200];                 // output
  unsigned char        length;                        // länge der rufnummer
  short                i;                             // bla

  strcpy (etsi_out,"");                               // output init
  length = *cpn;                                      // länge init

  if (0 == length) return (etsi_out);

  cpn++;                                              //
  length--;                                           //

  if ((*cpn &  16) == 16) strcat (etsi_out,"00");     // international?
  if ((*cpn &  32) == 32) strcat (etsi_out, "0");     // national?
  if ((*cpn & 128) ==  0) { cpn++; length--;}   ;     // ggf. pi und si überspringen
  cpn++;                                              //

  for (i=1;i<=length;i++) strncat (etsi_out,cpn++,1); // rufnummer kopieren
                                                      //
  return (etsi_out);                                  // ergebnis
}                                                     //

//*****************************************************************************
// function : etsi_str_called_pn
// in       : zeiger auf capi-struktur
// out      : nullterminierter nummernstring

unsigned char *etsi_str_called_pn (unsigned char *cpn)
{
  static unsigned char etsi_out[200];                 // output
  unsigned char        length;                        // länge der rufnummer
  short                i;                             // bla

  strcpy (etsi_out,"");                               // output init
  length = *cpn;                                      // länge init
  cpn    += 2;                                        //
  length -= 1;                                        //

  for (i=1;i<=length;i++) strncat (etsi_out,cpn++,1); // rufnummer kopieren
                                                      //
  return (etsi_out);                                  // ergebnis
}                                                     //
//*****************************************************************************

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
//problem: früher:           if (strchr (org,' |')) *strchr (org,'|') = 0;
           if (strchr (org,'|')) *strchr (org,'|') = 0;
           strcat (msn_liste,org);
         }
         strcat (msn_liste,"|");
       }
     }
     fclose (msndat);
  }
}

static void alert_conf (void)
{
}

static void alert_req (unsigned long PLCI)
{
static  TAlertReqMsg msg;

  msg.header.length = 13;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x01;
  msg.header.subcmd = (BYTE)0x80;
  msg.header.msgnum = NextMsgNum();

  msg.PLCI = PLCI;
  msg.additional_info[0] = 0;

  putmsg((CapiMsgHeader *) &msg);
}

static void connect_ind_1( TConnectIndMsg *msg )
{
  char caller_id[150];
  char caller_msn[150];
  unsigned char *msgpt;

  char reject_cause_str[20];
  char max_rec_time_str[20];
  char an_delay_str[20];
  struct tm *tmbuf;
  time_t tod;
  char timestr[200];
  char datestr[200];
  char caller_name[200];
  char msn_name[200];
  AnsConIndMsg ConIndMsg;
  unsigned long ZeroBeh;
  char zhelp [200];

//georg
  if (config_file_read_ulong(STD_CFG_FILE,CAPITEL_ACTIVE,CAPITEL_ACTIVE_DEF)) {
    alert_req(msg->plci);
  }

  online = 0;
  set_active_numbers ();
  set_pipename_debug ();
  set_pipename_info  ();

  debug_data_b3 = config_file_read_ulong (STD_CFG_FILE,DEBUG_DATA_B3,DEBUG_DATA_B3_DEF);

  connect_ind_msgnum = msg->header.msgnum;

  if (conn_state == 1) {
    comm20_clear_all_buffer();
    set_state(2);
    act_plci = msg->plci;
    discind_flag=0;
    is_callback = 0;

    msgpt = msg->ext;            // ^called  party number
    msgpt += msg->ext[0]+1;      // ^calling party number

    if( *msgpt == 0 ) {          // keine calling party number?
      if (msg->cip_value == cip_value_telephony) // telephony (isdn)
      {
        config_file_read_string(STD_CFG_FILE,TEXT_UNKNOWN_ISDN,caller_id,TEXT_UNKNOWN_ISDN_DEF);
      }
      else
      {
        config_file_read_string(STD_CFG_FILE,TEXT_UNKNOWN_ANALOG,caller_id,TEXT_UNKNOWN_ANALOG_DEF);
      }
    } else {
      strcpy (caller_id,etsi_str_calling_pn(msgpt));
      answer_cti (caller_id);
      if (caller_id[0] == 0)
      {
        if (msg->cip_value == cip_value_telephony) // telephony (isdn)
        {
          config_file_read_string(STD_CFG_FILE,TEXT_UNKNOWN_ISDN,caller_id,TEXT_UNKNOWN_ISDN_DEF);
        }
        else
        {
          config_file_read_string(STD_CFG_FILE,TEXT_UNKNOWN_ANALOG,caller_id,TEXT_UNKNOWN_ANALOG_DEF);
        }
      }
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
    strcpy (caller_id_org, caller_id);
    better_string(caller_id,NAMFILE,2,caller_id, caller_name);
    s2info_pipe (caller_name);
    s2info_pipe (", ");

    memcpy(caller_msn, msg->ext, msg->ext[0]+1 );
    if (0 == msg->ext[0]) {
      strcpy (caller_msn,"");
    } else {
      strcpy (caller_msn,etsi_str_called_pn(caller_msn));
    }
    s2info_pipe (caller_msn);
    s2info_pipe (", ");

    if ((strstr(msn_liste,caller_msn) == NULL) && (strlen(msn_liste) > 1)) {
      num_in_list = 0;
    } else {
      num_in_list = 1;
    }

    strcpy (caller_msn_org, caller_msn);
    better_string(caller_msn,PRTFILE,2,caller_msn, msn_name);
    s2info_pipe (msn_name);
    s2info_pipe (", ");

    strcpy (ConIndMsg.caller_name, caller_id);
    strcpy (ConIndMsg.caller_org_name, caller_id);
    strcpy (ConIndMsg.called_name, caller_msn);
    if (msg->cip_value == cip_value_digital) {
      ConIndMsg.is_digital = 1;
    } else {
      ConIndMsg.is_digital = 0;
    }

    if (num_in_list) AppSigFunc (1,(void *)&ConIndMsg);
//    AppSigFunc (1,(void *)&ConIndMsg);

    strcpy (reject_cause_str,"0");
    better_string (caller_msn,PRTFILE,4,reject_cause_str, reject_cause_str);
    better_string(ConIndMsg.caller_name,NAMFILE,7,"1", bhelp);
    if (!strcmp(bhelp,"1")) {
      better_string (caller_id ,NAMFILE,4,reject_cause_str, reject_cause_str);
    }
    reject_cause = reject_cause_str[0]-48;
    s2info_pipe (reject_cause_str);
    s2info_pipe (", ");

#ifdef RECOTEL
    max_rec_time_next_call = 9999;
#else
    max_rec_time_next_call = config_file_read_ulong(STD_CFG_FILE,DEFAULT_REC_TIME,DEFAULT_REC_TIME_DEF);
#endif
    sprintf(max_rec_time_str,"%d",(int)max_rec_time_next_call);
    better_string (caller_msn,PRTFILE,5,max_rec_time_str, max_rec_time_str);
    better_string(ConIndMsg.caller_name,NAMFILE,7,"1", bhelp);
    if (!strcmp(bhelp,"1")) {
      better_string (caller_id ,NAMFILE,5,max_rec_time_str, max_rec_time_str);
    }
    max_rec_time_next_call = strtol (max_rec_time_str,NULL,10);
    s2info_pipe (max_rec_time_str);
    s2info_pipe (", ");

    an_delay_next_call = an_delay_default;
    sprintf(an_delay_str,"%d",(int)an_delay_next_call);
    better_string (caller_msn,PRTFILE,6,an_delay_str, an_delay_str);
    better_string(ConIndMsg.caller_name,NAMFILE,7,"1", bhelp);
    if (!strcmp(bhelp,"1")) {
      better_string (caller_id ,NAMFILE,6,an_delay_str, an_delay_str);
    }
    an_delay_next_call = strtol (an_delay_str,NULL,10);
    s2info_pipe (an_delay_str);
    s2info_pipe ("\n");

    strcpy (ConIndMsg.caller_name,"|");
    strcat (ConIndMsg.caller_name,ConIndMsg.called_name);
    strcat (ConIndMsg.caller_name,"|");
    strcpy (ConIndMsg.called_name,ConIndMsg.caller_name);

    if ((strstr(msn_liste,ConIndMsg.called_name) == NULL) && (strlen(msn_liste) > 1)) {
       reject_cause = 2;
    }

    if ((!config_file_read_ulong(STD_CFG_FILE,CAPITEL_ACTIVE,CAPITEL_ACTIVE_DEF)) || (msg->cip_value == cip_value_digital )) {  // inaktiv?
//georg      reject_cause = 3;               // unavailable
      reject_cause = 2;               // ignore
      connect_resp(msg->plci);        // ablehnen
    } else if (reject_cause) {        // anrufer ablehnen?
      connect_resp(msg->plci);        // ja. mit rejec_cause
    } else {
//      alert_req(msg->plci);
      OsStartThread( waitforconn);      // nein = annehmen :)
    }

  } else {
    reject_cause = 1;                 // BUSY
    connect_resp(msg->plci);          // ablehnen
  }
}

static void int_sigfunc(void)
{
static  void * pt;
static  CapiMsgHeader *cmsg;


  while( API_GET_MESSAGE_20( AppID,((void**)&pt)) == 0 ) {

    cmsg = (CapiMsgHeader *) pt;

    if ((cmsg->cmd != (BYTE)0x86) || debug_data_b3) {
       sprintf(out_msg,"GetMsg: 0x%02x 0x%02x (State: %d)", cmsg->cmd, cmsg->subcmd, (int)conn_state );
       s2debug_pipe(out_msg);
       c20_analyze_get_msg ((char *)pt,0xffff);
    }

    switch( conn_state ) {                             /* State Machine */

      /**********l*************************************************************/
      /*** State: Answ. Machine Inactive */
      /***********************************************************************/

      case 0:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0581: listen_conf_0         ((TListenConfMsg *) cmsg ); break;
          case 0x8682: data_b3_ind_n         ((TDataB3Ind     *) cmsg ); break;
          default:     state_error           (cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Answ. Machine waiting for incoming call */
      /***********************************************************************/

      case 1:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0282: connect_ind_1         ((TConnectIndMsg *) cmsg ); break;
          case 0x0481: disc_conf             ((TDiscConf      *) cmsg ); break;
          case 0x0581: listen_conf_0         ((TListenConfMsg *) cmsg ); break;
          case 0x8682: data_b3_ind_n         ((TDataB3Ind     *) cmsg ); break;
          case 0x0181: alert_conf            ()                        ; break;
          case 0x0281: connect_conf          ((TConnConf      *) cmsg) ; break;
          default:     state_error           (cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Answ. Machine building up connection */
      /***********************************************************************/

      case 2:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0282: connect_ind_1         ((TConnectIndMsg *) cmsg ); break;
          case 0x0382: connect_act_ind_1     ((TConnActInd    *) cmsg ); break;
          case 0x8282: connect_b3_ind_1      ((TConnB3Ind     *) cmsg ); break;
          case 0x8382: connect_b3_act_ind_1  ((TConnB3ActInd  *) cmsg ); break;
          case 0x8682: data_b3_ind_n         ((TDataB3Ind     *) cmsg ); break;
          case 0x0482: disc_ind              ((TDiscInd       *) cmsg ); break;
          case 0x0581: listen_conf_0         ((TListenConfMsg *) cmsg ); break;
          case 0x0481: disc_conf             ((TDiscConf      *) cmsg ); break;
          case 0x0181: alert_conf            ()                        ; break;
          case 0x8281: connect_b3_conf       ((TConnB3Conf    *) cmsg ); break;
          default:     state_error           (cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Data Transfer  */
      /***********************************************************************/

      case 3:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0482: disc_ind              ((TDiscInd       *) cmsg ); break;
          case 0x0282: connect_ind_1         ((TConnectIndMsg *) cmsg ); break;
          case 0x8682: data_b3_ind_2         ((TDataB3Ind     *) cmsg ); break;
          case 0x8482: disc_b3_ind_2         ((TDiscB3Ind     *) cmsg ); break;
          case 0x8681: data_b3_conf_2        ((TDataB3Conf    *) cmsg ); break;
          default:     state_error           (cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Disconnecting  */
      /***********************************************************************/

      case 4:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          case 0x0282: connect_ind_1         ((TConnectIndMsg *) cmsg ); break;
          case 0x8681: data_b3_conf_2        ((TDataB3Conf    *) cmsg ); break;
          case 0x8682: data_b3_ind_n         ((TDataB3Ind     *) cmsg ); break;
          case 0x0481: disc_conf             ((TDiscConf      *) cmsg ); break;
          case 0x0482: disc_ind              ((TDiscInd       *) cmsg ); break;
          case 0x8482: disc_b3_ind_2         ((TDiscB3Ind     *) cmsg ); break;
          default:     state_error           (cmsg->cmd, cmsg->subcmd ); break;
        }
        break;

      /***********************************************************************/
      /*** State: Unknown  */
      /***********************************************************************/

      default:
        switch( MAKEUSHORT(cmsg->subcmd, cmsg->cmd) ) {
          default:     state_error(cmsg->cmd, cmsg->subcmd ); break;
        }
        break;
    }
  }
  return;
}

void comm20_clear_read_buffer (void)
{
  OSRequestMutexSem(&hmtxsem1,-1);
      buff_read  = buffer;
      buff_write = buffer;
  OSReleaseMutexSem(&hmtxsem1);
}

void comm20_clear_write_buffer (void)
{
   long t;

  OSRequestMutexSem(&hmtxsem2,-1);
      for (t=0;t<CAPI_NUM_B3_BLK;t++) sendb[t].freeflag = 1;
  OSReleaseMutexSem(&hmtxsem2);
}

void comm20_clear_all_buffer (void)
{
   comm20_clear_read_buffer();
   comm20_clear_write_buffer();
}

// #############################################################################################
// #############################################################################################
// #############################################################################################

static void wait_for_signal (void)
{
#ifdef WIN32
  API_WAIT_FOR_SIGNAL_20( AppID );
#endif

#ifdef UNIX
  API_WAIT_FOR_SIGNAL_20( AppID );
#endif

#ifdef OS2
  ULONG cnt;

  DosWaitEventSem( eventsem, -1 );            /* warten bis was zu tun ist */
  DosResetEventSem( eventsem, &cnt );         /* reset (sonst funktioniert */
#endif
}

// #############################################################################################
// #############################################################################################
// #############################################################################################

static void _Optlink poll_capi( void *arg )
{

  OsSleep (1000);

  for (;;) {
    if (!enter_wait_for_signal) {
      poll_thread_id = 0;
      return;
    }
    if( flagDoSignal ) {
       wait_for_signal();
       OsSleep (50);
    } else {
      OsSleep (200);
    }
    int_sigfunc();
  }
}

// #############################################################################################
// #############################################################################################
// #############################################################################################

short comm20_init( void (*sigf)( short, void * ), char shutdown)
{
  DWORD rc;

  set_pipename_debug ();
  set_pipename_info  ();
  capi_analyze_init (0,pipe_file_debug);

  AppSigFunc = sigf;

  if( !Capi20_LoadCapi(out_msg) )
  {
#ifdef WIN32
    s2debug_pipe(out_msg);
    AppSigFunc( 9, (void *) out_msg );
#endif    
#ifdef UNIX
    s2debug_pipe(out_msg);
    AppSigFunc( 9, (void *) out_msg );
#endif    
#ifdef OS2
    sprintf(out_msg,"ISDNAPI: Unable to load CAPI20.DLL!\nShutting down.");
    if (shutdown) {
      s2debug_pipe(out_msg);
      AppSigFunc( 9, (void *) out_msg );
    }
#endif
    return 1;
  }
  else
  {
#ifdef WIN32
    s2debug_pipe(out_msg);
#endif
#ifdef UNIX
    s2debug_pipe(out_msg);
#endif
  }

  if( API_INSTALLED_20() ){
    return 1;
  }  

  set_state(0);

  OSCreateMutexSem("\\sem32\\ctsem1",&hmtxsem1 ,0,FALSE);
  OSCreateMutexSem("\\sem32\\ctsem2",&hmtxsem2 ,0,FALSE);
  OSCreateMutexSem("\\sem32\\ctsem3",&hmtxsem3 ,0,FALSE);
  OSCreateMutexSem("\\sem32\\ctsem4",&hmtxsem4 ,0,FALSE);
  OSCreateMutexSem("\\sem32\\ctsem5",&hmtxsem5 ,0,FALSE);
#ifdef OS2
  DosCreateEventSem("\\sem32\\ctevt1",&eventsem,0,FALSE );
#endif

  comm20_clear_all_buffer();

  rc = API_REGISTER_20( CAPI_MSGBUF_SIZE, CAPI_NUM_B3_CON, CAPI_NUM_B3_BLK, CAPI_NUM_B3_SIZE, &AppID );

  c20_analyze_register( NULL ,CAPI_MSGBUF_SIZE,CAPI_NUM_B3_CON,CAPI_NUM_B3_BLK,CAPI_NUM_B3_SIZE, (short)AppID);

  if( rc )
  {
    sprintf(out_msg,"ISDNAPI: API_REGISTER failed!\nShutting down.");
    s2debug_pipe(out_msg);
    AppSigFunc( 9, (void *) out_msg );
    return 1;
  }

  flagDoSignal = config_file_read_ulong(STD_CFG_FILE,SET_SIGNAL,SET_SIGNAL_DEF);

#ifdef OS2
  if( flagDoSignal ) {
    API_SET_SIGNAL_20( AppID, eventsem);
  }
#endif

  poll_thread_id = OsStartThread( poll_capi);

  return 0;
}

short comm20_exit( void )
{
  if (poll_thread_id) {
    OsStopThread (poll_thread_id);
    poll_thread_id = 0;
  }

  if( AppID ) c20_analyze_release ((unsigned short)AppID, (unsigned short)API_RELEASE_20( AppID ));

  if (pipe_file_debug) {
    fclose (pipe_file_debug);
    pipe_file_debug = 0;
  }
  if (pipe_file_info) {
    fclose (pipe_file_info);
    pipe_file_info = 0;
  }
  Capi20_UnLoadCapi();
  OSCloseMutexSem(&hmtxsem1);
  OSCloseMutexSem(&hmtxsem2);
  OSCloseMutexSem(&hmtxsem3);
  OSCloseMutexSem(&hmtxsem4);
  OSCloseMutexSem(&hmtxsem5);
#ifdef OS2
  DosCloseEventSem(eventsem);
#endif
  return 0;
}

short comm20_listen(short act)
{
  static  TListenReqMsg msg;

#ifdef RECOTEL
  an_delay_default   = 0;
#else
  an_delay_default   = config_file_read_ulong (STD_CFG_FILE,DEFAULT_ANSW_DELAY,DEFAULT_ANSW_DELAY_DEF);
#endif
  an_delay_next_call = an_delay_default;

  comm20_clear_all_buffer();

  msg.header.length = sizeof(msg.header) + 18;
  msg.header.appid  = (WORD)AppID;
  msg.header.cmd    = (BYTE)0x05;
  msg.header.subcmd = (BYTE)0x80;
  msg.header.msgnum = NextMsgNum();

  msg.controller    = ((BYTE)config_file_read_ulong(STD_CFG_FILE,SERV_CONTROLLER,SERV_CONTROLLER_DEF));

  if (act) {
    msg.info_mask   = serv_info_mask;
    if (config_file_read_ulong(STD_CFG_FILE,CAPITEL_IS_CALLER_ID,CAPITEL_IS_CALLER_ID_DEF)) {
      msg.cip_mask    = cip_mask_speech + cip_mask_31audio + cip_mask_telephony + cip_mask_digital;
    } else {
      msg.cip_mask    = cip_mask_speech + cip_mask_31audio + cip_mask_telephony;
    }
  } else {
    msg.info_mask         = 0;
    msg.cip_mask          = 0;
    enter_wait_for_signal = 0; // beende wait_for_signal()
  }

  msg.cip_mask2     = 0;

  msg.ext[0]        = 0;
  msg.ext[1]        = 0;

  putmsg((CapiMsgHeader *) &msg);

  return 0;
}

char comm20_connected( void )
{
  return (conn_state==3);
}

char comm20_data_available (void)
{
   return (buff_read != buff_write);
}

long comm20_read_block (char *buff)
{
  long ret=0;

  if( conn_state != 3 ) return 0;

  OSRequestMutexSem(&hmtxsem1,-1);

      ret = (tU32)buff_write-(tU32)buff_read;
      memcpy (buff,buff_read,ret);
      buff_read  = buffer;
      buff_write = buffer;

  OSReleaseMutexSem(&hmtxsem1);

  return (ret);
}

char comm20_can_write (void)
{
   short t;
   short found = 0;

   if( conn_state != 3 )
     return 1;

   for (t=0;t<CAPI_NUM_B3_BLK;t++) found += sendb[t].freeflag;

   return (found > 0);
}

void comm20_write_block( char *buf, short length )
{
  if( conn_state != 3 ) return;

  data_b3_req( buf, length );

}

void comm20_disc_req(void)
{
  disc_req();
}

void comm20_name_of_interface (char *name)
{
  strcpy (name,"CAPI 2.0");
}


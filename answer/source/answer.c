void sigFunc( short num, void *msg );

#include <stdio.h>

#ifndef UNIX
#include <conio.h>
#include <io.h>
#include <direct.h>
#include <process.h>
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#include "../../../units/common.src/bastypes.h"

#include "../../../units/common.src/os.h"
#include "../../isdn/source/isdn.h"

#include "../../isdn/source.os2/isdnmsg1.h"

#include "../../../units/common.src/num2nam.h"
#include "../../wave/source/alw2wav.h"
#include "../../wave/source/wav2alw.h"
#include "answer.h"
#include "../../../units/common.src/util.h"
#include "../../util/source/vorwahl.h"
#include "../../util/source/dtmf.h"
#include "../../util/source/dosstart.h"
#include "../../util/source/silence.h"
#include "../../../units/common.src/cfg_file.h"
#include "../../common/source/global.h"
#include "../../util/source/register.h"

#define DEF_CTI_MSG_ENG      "CapiTel CTI Window"
#define DEF_DISABLE_MSG_ENG  "Please register.\n\nCapitel disabled!"
#define DEF_REG_MSG_ENG      "Please register CapiTel!\n\n(or set ANSWER DELAY to 999 to use CapiTel as a freeware Caller-ID)"

#define DEF_CTI_MSG_GER      "CapiTel CTI Fenster"
#define DEF_DISABLE_MSG_GER  "Bitte registreren.\n\nCapitel deaktiviert!"
#define DEF_REG_MSG_GER      "Bitte registrieren Sie CapiTel!\n\n(oder setzen Sie VERZOEGERUNG auf 999 um CapiTel als Freeware Caller-ID zu benutzen)"

char    status_isdn=0;
char    status_conv=0;
FILE    *fh_ansage=NULL, *fh_ziel=NULL, *fh_fern=NULL;
char    fh_ziel_name[512];
char    cbuffer[14*1024];
char    next_is_welcome=0;
void    (*capitel_signal_function)( short, void * );
char    help_str[300];
char    welcome_file[300];
char    welcomeFile[300];
unsigned long calllength;
char    help_welcome_file[300];
int     raute;
int     loesch;
int     fernabfrage;
int     any_dtmf_detected;
int     dtmf_job;
char    dtmf_str[200];
char    act_call_name[300];
unsigned long use_ulaw_codec=0;
char    msg_str_cti[300];
char    msg_str_reg[300];
char    msg_str_disable[300];
tU32    play_beep=0;

Thread_Id_Typ rescan_thread_id = 0;
unsigned long rescan_time_var  = 0;

unsigned long do_dtmf_find    = 1; // default on
unsigned long do_silence_find = 0; // default on

short conn_ind_cnt = 0;
short do_nerv_message = 1;

short beep_cnt;
short max_beep_cnt;

Thread_Id_Typ beep_thread_id = 0;
short num_calls = 0;

char *beepdata= "\x43\x69\x01\xa1\x88\x70\x60\x46\x66\xf8\x18\x5f\x91"
                "\xad\x89\x80\xd0\x98\x57\x79\x07\x5b\xc9\xc1\xd9\xc8"
                "\xac\x90\xcb\x31\x31\x17\xc6\x6e\xe7\x12\x60\x70\x88"
                "\x81\xed\x01\x16\x40\x58\x92\xeb\x28\x68\xd7\x91\x91"
                "\x2f\x70\xac\xf8\x89\x59\xc7\xea\xa9\x21\x29\xa0\x2c"
                "\xb0\x47\x11\x31\x67\xe8\x46\x53\xbe\xe0\xc0\x9e\x31"
                "\xed\x21\xb8\xf0\xa0\x0b\x9f\x26\x86\x09\x71\x41\xde"
                "\x50\xac\x28\xd9\x61\xf7\x1a\xc7\xf9\xdf\x60\xac\x60"
                "\xb9\xad\x31\x83\x78\xe8\xaa\x7e\xa0\xe0\x13\x91\x2d"
                "\xb9\xe0\x10\x58\x3f\x97\x22\x6e\x49\x31\xe1\x68\x2c"
                "\xd0\x1e\x81\x01\x77\x8e\xcf\xe9\xaa\x00\x50\x78\xe1"
                "\x2d\x41\x7e\x20\x48\x7a\x8e\xf8\xf8\xf7\xd1\x51\xb7"
                "\xb0\x50\x78\x77\x09\xf3\xfa\xc9\xc1\x39\xd8\x2c\x70"
                "\x6f\x31\x41\xc7\x06\xba\x47\x0e\x00\x70\x96\x41\xed"
                "\x61\x28\x00\xf8\xca\xfa\x68\x28\x89\xd1\x71\x3a\x90"
                "\xac\x08\xc9\x99\x1f\xab\x89\xe1\x97\x60\x2c\xc0\x29"
                "\x11\xb1\xbf\x28\xde\x4f\x26\x80\xc0\xa2\x71\x2d\xd9"
                "\x98\x30\x98\x23\xaf\x36\x86\x39\x91\xc1\x96\xac\x50"
                "\x36\xa1\x21\x87\xca\xd7\x99\xef\x00\xac\x20\x19\xad"
                "\x41\x4a\x38\x96\x9b\x66\xe0\x20\x27\xd1\xad\xe9\x80"
                "\x90\x78\x1f\x07\x4e\xae\x79\xf1\xa1\x38\x2c\x90\xa2"
                "\x01\x81\x07\xae\x67\x89\x42\x40\xd0\x08\x81\x2d\x81"
                "\x86\x20\x68\xea\xa6\xd8\xf8\x29\x51\xd1\x4f\x30\xd0"
                "\x48\x17\xe9\xeb\x2a\xf9\x41\x09\x20\xec\x30\x87\xf1"
                "\x01\xa7\xe6\x33\xf7\x26\x40\xf0\xe6\x31\x2d\x21\x08"
                "\x00\x48\xab\xee\x48\xa8\xb9\x51\x31\x9e\xd0\xd0\xa8"
                "\x49\x39\x6f\x7b\xb9\x61\xc7\x00\x2c\x80\x49\xd1\xc1"
                "\x33\xa8\x52\x7f\xb6\xc0\xc0\x03\x11\x2d\xb9\xa0\xb0"
                "\xb8\x43\x0b\x56\x26\x19\x11\x81\x68\xac\x10\xde\xa1"
                "\x59\xdf\x4b\x69\x19\x3b\xb0\xac\x18\x21\xad\x81\x2e"
                "\xb8\x06\xa3\xf6\x60\x20\xf7\x51\xad\x37\x00\x70\xc8"
                "\xa7\xdf\xa6\xe2\x59\x71\xd9\xd8\x2c\xf0\x73\x01\xe1"
                "\x5f\xe2\xf7\x09\xfe\x30\xd0\x28\xc1\x2d\xe1\x76\xa0"
                "\xa8\xab\xb6\xa0\xb8\xc9\xad\x91\x6b\xf0\x90\xe8\xa9"
                "\xd7\x5a\x5b\x59\xb1\xe9\x80\xec\xc0\xd7\xf1\x61\x8f"
                "\x5e\x3f\x77\x36\x30\xf0\xee\x71\x2d\x99\xb8\x80\x68"
                "\x6b\xa6\xf8\xd6\x19\xad\xb1\xb6\x50\x90\x46\xb9\xc9"
                "\x83\x13\x99\x61\x1f\x40\x2c\x20\x79\x11\x61\xea\xd6"
                "\xca\x7f\xa8\xb0\x00\xdf\x51\xad\xe9\x20\x40\x88\xd3"
                "\xe2\xe8\x3e\x21\x51\xe1\xb8\x2c\x70\xe2\x21\x99\x0f"
                "\x43\x49\xd9\x42\xf0\xac\xb8\x61\xad\xe1\x5e\xc8\x1e"
                "\xa3\x28\x00\xa0\xe9\xad\x51\x1f\xc0\x30\xe8\xe7\xef"
                "\x86\xeb\xe1\x91\xf9\x20\x2c\xb0\xa7\x01\xa1\x8f\xca"
                "\xa9\x09\xc6\x70\xd0\xb6\xb1\xad\x59\xa8\x58\x36\x2b"
                "\xd6\xe0\xc8\x79\x2d\x71\x6e\x70\xf0\x76\xa9\xb7\x12"
                "\xef\x21\xb1\x97\xc0\xec\x80\x69\x31\x21\xa3\x3e\xe7"
                "\xf7\x28\x70\x30\x8b\x11\xad\x49\x78\x60\x56\xbb\xc6"
                "\x98\xb6\x21\xad\xc1\x28\x50\xf0\x7e\x39\xe9\x8b\x4f"
                "\xa1\x61\x03\xf0\x2c\xd8\x59\x11\x21\x32\xf6\xe3\xff"
                "\x88\x30\x80\xf7\xad\xad\x77\xe0\xc0\x56\x2f\x0e\x68"
                "\x92\x61\x51\xa1\x18\x2c\x30\x1b\xa1\xb9\x63\xef\xf9"
                "\xd9\x5e\x90\xac\x68\x01\x51\x59\x86\x88\x52\x23\x88"
                "\xc0\xd8\xb9\x2d\x11\xe3\x40\x40\x76\xe7\xfb\xb6\x43"
                "\x81\x91\x09\x80\xec\xc0\x47\x01\x99\x63\x3b\x69\x89"
                "\xa8\x10\x10\x3e\xf1\xad\xf9\xe8\x98\x26\x6b\xe8\xe0"
                "\x68\xa1\x2d\x31\x66\x90\x30\x66\xa9\x27\x4e\x7f\x61"
                "\xb1\xe7\x30\xec\xe0\x49\x31\xd9\xaa\xae\x37\xb7\x08"
                "\x90\xb0\x3f\xd1\xad\xe9\x98\x20\x46\x4b\x16\x18\xe6"
                "\x61\x2d\x81\xc8\xac\xb0\xf2\xb9\x17\x1a\xa7\x21\x61"
                "\x62\x90\x2c\x38\x21\x91\xd9\x6e\x06\x4f\xff\xb8\xf0"
                "\x60\x69\x2d\xd1\x27\x60\x80\xb6\x13\xe6\x08\x4a\x01"
                "\x51\x99\x20\x2c\x40\x0f\xa1\x89\x8b\x9f\xd9\x19\x36"
                "\xd0\xac\xf6\x41\xd1\x79\x46\x28\xaa\x5b\xb8\x40\xf8"
                "\xd9\x2d\x71\x42\xb0\xc0\x06\x27\x82\x96\xcf\xc1\x91"
                "\x57\x40\xec\x60\xd7\x81\xb9\xcb\x53\x49\x89\x88\xd0"
                "\x90\x2a\x91\x51\x09\x68\xb8\x0e\x6a\xc8\x60\x56\x61"
                "\x2d\x41\x56\x90\x40\x3e\x17\x2f\xde\x67\x01\x41\x53"
                "\x70\x2c\xd8\xf9\xb1\xf9\x3a\x4a\xa9\x47\x38\x10\x40"
                "\x37\xad\x51\x77\x18\xa0\x26\xaa\xe8\x58\xbe\xc1\x2d"
                "\x21\x98\xac\xc0\xaa\xc9\xc7\x42\xc7\x61\x61\x9e\xd0"
                "\x2c\x68\x61\x71\xf9\x4e\x9e\xe7\x0f\x98\x70\x20\xb9"
                "\x2d\x11\x33\x80\xe0\xe6\xe3\xf6\x48\x23\x41\x51\x49"
                "\x80\x2c\x80\x27\x59\xa9\x2a\x87\xa1\x99\xa8\x50\xd0"
                "\x7e\xb1\x11\x49\x36\x96\xc3\x0b\x98\xb0\x48\x21\xed"
                "\x31\x26\xb0\x80\xde\x9f\xfe\xd6\x27\xb1\x91\xc7\x30"
                "\x2c\x20\x69\x61\x89\xab\xdf\x79\x69\xb8\xac\xf0\xcf"
                "\x11\xd1\xa9\x08\xc8\x02\x22\x78\x80\x36\x01\x2d\x81"
                "\x88\x90\x00\x72\x77\xcb\xa6\xf7\x41\x41\x4a\x10\x2c"
                "\xb8\xd9\x41\xc9\xda\x63\x69\x87\xd8\xd0\x00\x29\xad"
                "\x11\xa7\x58\x98\xbe\xa2\xc8\xa0\x62\xb1\x2d\xd9\xa0"
                "\xac\x80\x73\x09\x5f\xb2\xd7\x01\xe1\x46\x50\xac\x76"
                "\x81\xf1\xc9\x7e\x2e\xc7\xf3\xa0\x90\x58\x19\x2d\xf1"
                "\xa2\x80\x20\x9e\x6b\xa8\xb8\xbf\xf1\x51\xe9\xc0\xac"
                "\xe0\x37\xd9\xf7\x6a\x97\xe1\x79\x08\xac\x10\xda\xf1"
                "\x91\xe9\x76\x46\x6f\x1a\xa0\x30\x88\x61\xed\x41\x76"
                "\xb0\xe0\x6e\xcf\x66\x56\xf7\xf1\x71\xbf\x70\x2c\x98"
                "\xc9\xe1\x29\x2a\x07\x19\xa9\x18\xac\x30\x07\xd1\x91"
                "\x47\xc8\xe8\x8a\x2e\x58\x00\xa6\x91\x81\x5a\x6a\xa2"
                "\x0e\xf2\xae\xb2\x82\xa2\x9a\x3a\x4a\x6a\x8a\xab\xea"
                "\x6b\x2a\x0b\xaa\xcb\xab\x4b\xab\x4b\xab\xcb\xab\x0b"
                "\xab\x8b\xab\x6b\xaa\x6b\xaa\xeb\xaa\x2b";

unsigned long beep_on_calls_freq;
unsigned long beep_on_calls_dura;
unsigned long beep_on_calls_delay;

char dtmf_setcallback = 0;
extern short is_callback;


//****************************************************************************

void _Optlink capitel_rescan( void *arg)
{
  for (;;) {
    OsSleep (1000*rescan_time_var);
    capitel_signal_function( 8, NULL);   // neu scannen
  }
}

void _Optlink boot (void *arg)
{
  while (comm_connected()) OsSleep (500);
  OsSleep (3000);
  OsBoot();
}

void _Optlink quit (void *arg)
{
  while (comm_connected()) OsSleep (500);
  OsSleep (3000);
  capitel_signal_function( 12, NULL);
}

//short answer_version_expired(void)
//{
//  long expdate = 828398801 + 14 * 2592000;              /* june 1st 97 */
//  time_t tod;
//
//  time( &tod );
//  if ( tod > expdate ) return 1;
//  return 0;
//}

short iswav( char *filename )
{
  FILE *fh;
  char str[5] = "1234";

  if( (fh = fopen( filename, "rb" )) != NULL ) {
    fread( &str, 4, 1, fh );
    fclose( fh );
    fh = NULL;
    str[4] = 0;
  }
  return !strcmp( str, "RIFF" );
}

long check_duration( char *filename )
{
  FILE *fh = NULL;
  long duration = 0;
  char str[256];

  if ((fh = fopen( filename, "r" )) != NULL) {
    fgets( str, sizeof( str ), fh );            /* dummy read          */
    fgets( str, sizeof( str ), fh );            /* dummy read          */
    fgets( str, sizeof( str ), fh );            /* dummy read          */
    fgets( str, sizeof( str ), fh );            /* duration in seconds */
    duration = (long) atoi( str );
    fclose( fh );
    fh = NULL;
  }
  return duration;
}

void _Optlink convert_all_alw2wav( void *arg)
{
  FileInfoTyp FileInfo;
  short         rc;
//  short         max=0;
  short         i;
  char          helpstr[200];
  short         bitmode = 0;

  bitmode     = (!config_file_read_ulong(STD_CFG_FILE,GENERATE_16_BIT_WAVES,GENERATE_16_BIT_WAVES_DEF));
  status_conv = 1;

  rc = OsFindFirst(&FileInfo,CALL_MASK_WAV);

  while( rc == 0 )
  {
        if( util_file_size(FileInfo.FileName ))
        {
          if( !iswav( FileInfo.FileName ) ) {

            strcpy (helpstr,FileInfo.FileName);
            for( i=0 ; (helpstr[i]!='.')&&(i<(short)strlen(helpstr)) ; i++ );
            helpstr[i] = 0;
            strcat( helpstr, ALW_EXT );
            util_copy_file (FileInfo.FileName, helpstr);

            rename( FileInfo.FileName, CONV_TMP );
            alw2wav( CONV_TMP, FileInfo.FileName, bitmode);
            remove( CONV_TMP );
          }
        }
  rc = OsFindNext(&FileInfo);
  }

  OsFindClose(&FileInfo);

  if (fernabfrage || dtmf_job) {
    capitel_signal_function( 9, "");                               // rufende von fernabfrage -> nicht anzeigen
    remove (act_call_name);                                                // *.wav

    for( i=0 ; (act_call_name[i]!='.')&&(i<(short)strlen(act_call_name)) ; i++ );
    act_call_name[i] = 0;
    strcat( act_call_name, ALW_EXT);
    remove (act_call_name);                                                // *.alw

    for( i=0 ; (act_call_name[i]!='.')&&(i<(short)strlen(act_call_name)) ; i++ );
    act_call_name[i] = 0;
    strcat( act_call_name, IDX_EXT );
    remove (act_call_name);                                                // *.idx

    if (loesch && fernabfrage) {
      rc = OsFindFirst(&FileInfo,CALL_MASK_IDX);
      while( rc == 0 ) {
        strcpy (helpstr,FileInfo.FileName);

        if( check_duration( helpstr ) > 0 )
        {
          remove (helpstr);                                           // *.idx

          for( i=0 ; (helpstr[i]!='.')&&(i<(short)strlen(helpstr)) ; i++ );
          helpstr[i] = 0;
          strcat( helpstr, ALW_EXT );
          remove (helpstr);                                           // *.alw

          for( i=0 ; (helpstr[i]!='.')&&(i<(short)strlen(helpstr)) ; i++ );
          helpstr[i] = 0;
          strcat( helpstr, WAV_EXT );
          remove (helpstr);                                           // *.wav
        }
        rc = OsFindNext(&FileInfo);
      }
      OsFindClose(&FileInfo);
      capitel_signal_function( 8, NULL);                             // rufe geloescht -> neu scannen
    }
  } else {
    capitel_signal_function( 6, &calllength );                       // normaler ruf -> anzeigen
  }
  status_conv = 0;
}

char *NextFileName(void)
{
   FileInfoTyp FileInfo;
   short rc;
   int help;
   short res=0;

   if(next_is_welcome) {
      strcpy( act_call_name, DEFALWFILE_NXT);
      capitel_signal_function( 5, &res );
      return act_call_name;
   }

   rc = OsFindFirst(&FileInfo,CALL_MASK_WAV);
   while( rc == 0 ) {
      sscanf (FileInfo.FileName,CALL_MAKE_MASK_WAV,&help);
      if (help > res) {
         res = help;
      }
      rc = OsFindNext(&FileInfo);
   }
   OsFindClose(&FileInfo);

   res++;
   sprintf (act_call_name,CALL_MAKE_MASK_WAV,res);
   capitel_signal_function( 5, &res );

   return act_call_name;
}

void answer_connect_ind (AnsConIndMsg *msg)
{
  static char ringring_file[200];
  TCapiInfo ConIndMsg;
  char *help;
//  short cnt;
  char helpstr[200];

  raute                 = 0;
  loesch                = 0;
  fernabfrage           = 0;
  any_dtmf_detected     = 0;
  dtmf_job              = 0;
  dtmf_str[0]           = 0;
  beep_cnt              = 0;
  dtmf_setcallback      = 0;
  num_calls++;

  dtmf_init();

  do_silence_find = config_file_read_ulong(STD_CFG_FILE,MAX_SILENCE_TIME,MAX_SILENCE_TIME_DEF);
  do_dtmf_find    = config_file_read_ulong(STD_CFG_FILE,DETECT_DTMF_TONES,DETECT_DTMF_TONES_DEF);
#ifndef RECOTEL
  play_beep       = config_file_read_ulong(STD_CFG_FILE,PLAY_BEEP,PLAY_BEEP_DEF);
#endif

  config_file_read_string(STD_CFG_FILE,WELCOME_WAVE,welcome_file, WELCOME_WAVE_DEF);
  strcpy (welcome_file,check_time(welcome_file));

  config_file_read_string(STD_CFG_FILE,RINGRING_WAVE,ringring_file, RINGRING_WAVE_DEF);
  strcpy (ringring_file,check_time(ringring_file));

  silence_reset(          config_file_read_ulong (STD_CFG_FILE,MAX_SILENCE_TIME,MAX_SILENCE_TIME_DEF));

  strcpy (ConIndMsg.caller_name    , msg->caller_name);
  strcpy (ConIndMsg.caller_org_name, msg->caller_org_name);
  strcpy (ConIndMsg.called_name    , msg->called_name);
  ConIndMsg.is_digital = msg->is_digital;

  better_string(ConIndMsg.called_name,PRTFILE,8,ringring_file , ringring_file );
  better_string(ConIndMsg.called_name,PRTFILE,3,welcome_file  , welcome_file  );
  better_string(ConIndMsg.called_name,PRTFILE,2,ConIndMsg.called_name, ConIndMsg.called_name);

  better_string(ConIndMsg.caller_name,NAMFILE,7,"1", helpstr);
  if (!strcmp(helpstr,"1")) {
    better_string(ConIndMsg.caller_name,NAMFILE,8,ringring_file , ringring_file );
    better_string(ConIndMsg.caller_name,NAMFILE,3,welcome_file  , welcome_file  );
    better_string(ConIndMsg.caller_name,NAMFILE,2,ConIndMsg.caller_name, ConIndMsg.caller_name);
  }

  if (config_file_read_ulong(STD_CFG_FILE,EXPAND_CALLER_ID,EXPAND_CALLER_ID_DEF) && !strcmp(msg->caller_name,ConIndMsg.caller_name)) strcat (ConIndMsg.caller_name,vorwahl_get_name(msg->caller_name));

  strcpy (fh_ziel_name,NextFileName());
  fh_ziel        = fopen(fh_ziel_name, "wb");

  if (strstr(welcome_file,WAV_EXT)) {
     strcpy (help_welcome_file,welcome_file);
     wav2alw_convert (help_welcome_file);
     help = strstr(welcome_file,WAV_EXT);
     *(++help) = 'a';
     *(++help) = 'l';
     *(++help) = 'w';
  }

  capitel_signal_function( 4 , (void *) &ConIndMsg );
  if (config_file_read_ulong(STD_CFG_FILE,PLAY_RINGRING_WAVE, PLAY_RINGRING_WAVE_DEF)) capitel_signal_function( 10, (void *) &ringring_file );
  status_isdn=1;
  return;
}

void answer_connect_b3_act_ind (void *msg)
{
  short cnt;

  status_isdn = 2;
  comm_clear_all_buffer();

  if( !util_file_exist( welcome_file ) )
    strcpy( welcome_file, util_strip_path(welcome_file) );
  if( !util_file_exist( welcome_file ) )
    strcpy( welcome_file,DEFALWFILE);

  fh_ansage = fopen(welcome_file, "rb");


  if (initRegistration()) max_beep_cnt = 4; else max_beep_cnt = 16;

  if (do_nerv_message) {
    if (!initRegistration()) {
      if (conn_ind_cnt++ >= CONN_CNT_DEACT) {
        capitel_signal_function( 11, NULL );
        capitel_signal_function( 1 , msg_str_disable);
        config_file_write_ulong (STD_CFG_FILE,CAPITEL_ACTIVE,0);
      } else if (conn_ind_cnt > CONN_CNT_WARN) {
        for (cnt=1;cnt <= (conn_ind_cnt-CONN_CNT_WARN); cnt++) capitel_signal_function( 1 , msg_str_reg);
      }
    }
  }

  return;
}

void answer_data_b3_ind (void *msg)
{
  long cnt;

  if( comm_data_available() ) {
    if( (cnt = comm_read_block( cbuffer)) > 0 ) {
      if( status_isdn == 4 ) {
        if( fh_ziel != NULL ) fwrite( cbuffer, cnt, 1, fh_ziel);
        if ((do_silence_find) && (!any_dtmf_detected)) silence_find (cbuffer,cnt);
      }
      if (do_dtmf_find) {
        if (use_ulaw_codec) {
          dtmf_find_ulaw (cbuffer,cnt);
        } else {
          dtmf_find_alaw (cbuffer,cnt);
        }
      }
    }
  }
  return;
}

void answer_disc_b3_ind (void *msg)
{
   status_isdn = 0;

   if( fh_ansage ) fclose( fh_ansage );
   if( fh_fern   ) fclose( fh_fern   );
   fh_ansage = fh_fern = NULL;

   return;
}

void answer_data_b3_conf(void *msg)
{
  char  cbuffer[1024];
  short readsize;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( status_isdn == 2 ) {
    if((!raute) && (fh_ansage != NULL)) {
      readsize=fread(cbuffer, 1, 1024, fh_ansage );
      if (readsize) {
        comm_write_block( cbuffer, readsize );
      } else {
        status_isdn = 3;
        answer_data_b3_conf(NULL);
      }
    } else {
      status_isdn = 3;
      answer_data_b3_conf(NULL);
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  else if( status_isdn == 3 ) {
    if (play_beep)
    {
      if (beep_cnt++ < max_beep_cnt) {
        comm_write_block( beepdata, 1024 );
      } else {
        comm_begin_of_record();
        status_isdn = 4;
      }
    }
    else
    {
      comm_begin_of_record();
      status_isdn = 4;
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  else if( status_isdn == 5 ) {
    if (fh_fern != NULL) {
      readsize = fread(cbuffer, 1, 1024, fh_fern );
      if (readsize) comm_write_block( cbuffer, readsize );
    }
  }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

}

void answer_disc_ind (void *msg)
{
  char          proc[200],parm[200],title[200];

  calllength = 0;

  if( fh_ziel == NULL )
    calllength = 0;
  else
  {
    calllength = util_file_size(fh_ziel_name)/8192;
    fclose( fh_ziel );
    fh_ziel = NULL;
  }

  if (next_is_welcome) {
   remove( welcomeFile );
   rename( DEFALWFILE_NXT, welcomeFile );
   next_is_welcome=0;
  }
  OsStartThread (convert_all_alw2wav);

  if (config_file_read_ulong(STD_CFG_FILE,START_ON_DISCONNECT,START_ON_DISCONNECT_DEF)) {
    config_file_read_string(STD_CFG_FILE,START_DISC_PROC ,proc ,START_DISC_PROC_DEF);
    config_file_read_string(STD_CFG_FILE,START_DISC_PARM ,parm ,START_DISC_PARM_DEF);
    config_file_read_string(STD_CFG_FILE,START_DISC_TITLE,title,START_DISC_TITLE_DEF);
    dos_start (proc,parm,title);
  }
}

static short dtmf_activ = 0;

void build_all_calls (void)
{
  FileInfoTyp FileInfo;
  short         rc;
  short         idx;
  FILE          *inpdat, *outdat;
  char          buff2[256];
  char          nullbeep[1024];

        memset (nullbeep, 0, 1024);

        rc = OsFindFirst(&FileInfo,CALL_MASK_ALW);

        remove (ALLFILE_ALW);
        fh_fern = fopen (ALLFILE_ALW,"wb");
        if (fh_fern) fclose (fh_fern);

        while( rc == 0 ) {
          fh_fern = fopen (ALLFILE_ALW,"ab");
          if (fh_fern) {
            fwrite (beepdata,1,1024,fh_fern);
            fclose (fh_fern);
          }
          util_append_file(FileInfo.FileName ,ALLFILE_ALW);
          rc = OsFindNext(&FileInfo);
        }
        OsFindClose(&FileInfo);
        fh_fern = fopen (ALLFILE_ALW,"ab");
        if (fh_fern) {
          for (idx=1; idx <= 5; idx++) {
            fwrite (nullbeep,1,1024,fh_fern);
            fwrite (beepdata,1,1024,fh_fern);
          }
          fclose (fh_fern);
        }

// calls als abgehoert markieren

        rc = OsFindFirst(&FileInfo,CALL_MASK_IDX);
        while( rc == 0 ) {
          if(util_file_exist(FileInfo.FileName) && util_file_size(FileInfo.FileName)) {
            if ((inpdat = fopen(FileInfo.FileName,"r")) != NULL){
              if ((outdat = fopen (CALL_TMP,"w")) != NULL) {
                for (idx=1;idx<=6;idx++) {fgets (buff2,sizeof(buff2),inpdat);fputs (buff2,outdat);};
                fputs ("1\n",outdat);
                fclose (outdat);
              };
              fclose (inpdat);
              remove (FileInfo.FileName);
              rename (CALL_TMP,FileInfo.FileName);
            }
          }
          rc = OsFindNext(&FileInfo);
        }
        OsFindClose(&FileInfo);
}

void answer_dtmf_found (void *msg)
{
  char dtmf_char[2] = {0,0};
  short         idx;
  char          pars_str[200];
  char          activ_str[200];
  char          proc[200],parm[200],title[200];

  if (dtmf_activ) return;
  dtmf_activ++;

  dtmf_char[0] = *(char*)msg;
  any_dtmf_detected=1;

  switch (dtmf_char[0]) {
    case '#': raute  = 1; dtmf_str[0] = 0;break;
    case '*': if (fernabfrage) loesch = 1;break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'A':
    case 'B':
    case 'C':
    case 'D': strcat (dtmf_str,dtmf_char);break;
  }

  if (dtmf_setcallback) {
    if (strlen(dtmf_str)) {
      if (!isdigit(dtmf_str[strlen(dtmf_str)-1])) {
        dtmf_str[strlen(dtmf_str)-1] = 0;
      }
      config_file_write_string(STD_CFG_FILE, CALL_BACK_NUMBER, dtmf_str);
      is_callback = 1;
    }
  } else if ((!dtmf_str[0]==0) && (strlen(dtmf_str) > 3)) {

    better_string(dtmf_str,ACTFILE,2,"DUMMY", pars_str);
    better_string(dtmf_str,ACTFILE,6,"DUMMY", activ_str);

    if ((strcmp("DUMMY",pars_str)==0) || (strcmp("0",activ_str)==0)) pars_str[0]=0;

    if (pars_str[0] != 0) {
      if (strcmp("REMOTECONTROL",pars_str)==0) {
        dtmf_str[0] = 0;
        fernabfrage = 1;
        raute = 1;

// calls zusammenstellen

        build_all_calls();

// calls abspielen

        fh_fern = fopen(ALLFILE_ALW  , "rb");
        status_isdn = 5;

        for (idx=1;idx<= CAPI_NUM_B3_BLK; idx++) answer_data_b3_conf(NULL);  // nur zum neuanstossen...

      } else if (strcmp("REBOOT",pars_str)==0) {
        dtmf_str[0] = 0;
        dtmf_job = 1;
        comm_disc_req();
        OsStartThread (boot);
      } else if (strcmp("DEACTIVATE",pars_str)==0) {
        dtmf_str[0] = 0;
        dtmf_job = 1;
        config_file_write_ulong (STD_CFG_FILE,CAPITEL_ACTIVE,0);
        capitel_signal_function( 11, NULL);
      } else if (strcmp("QUIT",pars_str)==0) {
        dtmf_str[0] = 0;
        dtmf_job = 1;
        comm_disc_req();
        OsStartThread (quit);
      } else if (strcmp("SETCALLBACK",pars_str)==0) {
        dtmf_str[0] = 0;
        dtmf_job = 1;
        dtmf_setcallback = 1;
      } else {
        dtmf_job = 1;
        better_string(dtmf_str,ACTFILE,2,"", proc );
        better_string(dtmf_str,ACTFILE,3,"", parm );
        better_string(dtmf_str,ACTFILE,4,"", title);
        dos_start(proc,parm,title);
        dtmf_str[0]=0;
      }
    }
  }
  dtmf_activ--;
}

void answer_silence_found (void *msg)
{
  comm_disc_req();
}

void answer_sigfunc( short num, void *msg )
{
  switch( num )  {
    case  1 : answer_connect_ind       (    msg ); break;
    case  2 : answer_connect_b3_act_ind(    msg ); break;
    case  3 : answer_data_b3_ind       (    msg ); break;
    case  4 : answer_disc_b3_ind       (    msg ); break;
    case  5 : answer_data_b3_conf      (    msg ); break;
    case  6 : answer_disc_ind          (    msg ); break;
    case  7 : capitel_signal_function  ( 1, msg ); break; // warnings
    case  8 : capitel_signal_function  ( 2, msg ); break; // critical error
    case  9 : capitel_signal_function  ( 3, msg ); break; // fatal error
    case  10: capitel_signal_function  ( 7, msg ); break; // converting wav2alw
    case  11: answer_dtmf_found        (    msg ); break; // dtmf_found
    case  12: answer_silence_found     (    msg ); break; // silence_found
    default : sprintf(help_str,"ANSWER: Unknown SigFunc: %d\n", num); capitel_signal_function( 1, help_str ); break;
  }
  return;
}

void _Optlink beep_thread ( void *arg)
{
  short i;

  for (;;) {
    OsSleep (beep_on_calls_delay);
    for (i=0;i<num_calls;i++) {
      OSBeep (beep_on_calls_freq,beep_on_calls_dura);
//    capitel_signal_function  ( 10, "pop.wav" );
      OsSleep (250);
    }
  }
}

short answer_init( void (*ctel_sigfunc)( short, void * ), short nerv_message, short language)
{
  switch (language) {
    case (LANGUAGE_GER): strcpy(msg_str_disable, DEF_DISABLE_MSG_GER);
                         strcpy(msg_str_reg    , DEF_REG_MSG_GER    );
                         strcpy(msg_str_cti    , DEF_CTI_MSG_GER    );
                         break;
    default            : strcpy(msg_str_disable, DEF_DISABLE_MSG_ENG);
                         strcpy(msg_str_reg    , DEF_REG_MSG_ENG    );
                         strcpy(msg_str_cti    , DEF_CTI_MSG_ENG    );
                         break;
  }

  OSSetPriority     (config_file_read_ulong(STD_CFG_FILE,CAPITEL_PRIORITY,CAPITEL_PRIORITY_DEF));
  OSProcessAffinity (config_file_read_ulong(STD_CFG_FILE,CAPITEL_AFFINITY,CAPITEL_AFFINITY_DEF));

  use_ulaw_codec = config_file_read_ulong(STD_CFG_FILE,CAPITEL_CODEC_ULAW,CAPITEL_CODEC_ULAW_DEF);

  if (config_file_read_ulong(STD_CFG_FILE,CAPITEL_ACTIVE_ON_STARTUP,CAPITEL_ACTIVE_ON_STARTUP_DEF)) {
    config_file_write_ulong (STD_CFG_FILE,CAPITEL_ACTIVE,1);
  }
  wav2alw_convert_all();
  do_nerv_message = nerv_message;

  rescan_time_var = config_file_read_ulong (STD_CFG_FILE,RESCAN_TIME,RESCAN_TIME_DEF);
  if (rescan_time_var) {
    rescan_thread_id = OsStartThread (capitel_rescan);
  }

  if (config_file_read_ulong(STD_CFG_FILE,BEEP_ON_CALLS,BEEP_ON_CALLS_DEF)) {
    beep_on_calls_freq  = config_file_read_ulong(STD_CFG_FILE,BEEP_ON_CALLS_FREQ ,BEEP_ON_CALLS_FREQ_DEF );
    beep_on_calls_dura  = config_file_read_ulong(STD_CFG_FILE,BEEP_ON_CALLS_DURA ,BEEP_ON_CALLS_DURA_DEF );
    beep_on_calls_delay = config_file_read_ulong(STD_CFG_FILE,BEEP_ON_CALLS_DELAY,BEEP_ON_CALLS_DELAY_DEF);
    beep_thread_id = OsStartThread (beep_thread);
  }

  capitel_signal_function = ctel_sigfunc;
  return(comm_init( answer_sigfunc));
}

void  answer_exit            (void)
{
   comm_listen(0);
   OsSleep (700);
   comm_exit();
   if (rescan_time_var) OsStopThread(rescan_thread_id);
   if (beep_thread_id ) OsStopThread(beep_thread_id  );
   while (wav2alw_convert_runs()) OsSleep (250);
}

void  answer_record_welcome  ( char *name )
{
   strcpy( welcomeFile, name );
   next_is_welcome=1;
}

void  answer_listen (void)
{
  comm_listen(1);
  use_ulaw_codec = config_file_read_ulong(STD_CFG_FILE,CAPITEL_CODEC_ULAW,CAPITEL_CODEC_ULAW_DEF);
}

short answer_cannot_close       (void)
{
  return (status_conv);
}

void answer_wav2alw_convert_all (void)
{
  wav2alw_convert_all();
  use_ulaw_codec = config_file_read_ulong(STD_CFG_FILE,CAPITEL_CODEC_ULAW,CAPITEL_CODEC_ULAW_DEF);
}

void  answer_name_of_interface   (char * name)
{
  comm_name_of_interface (name);
  return;
}

void  answer_stop_bell           (void)
{
  num_calls = 0;
}

void  answer_play_all            (void)
{
  short         bitmode = 0;

  bitmode     = (!config_file_read_ulong(STD_CFG_FILE,GENERATE_16_BIT_WAVES,GENERATE_16_BIT_WAVES_DEF));

// calls zusammenstellen

  build_all_calls();

// calls abspielen

  alw2wav (ALLFILE_ALW, ALLFILE_WAV, bitmode);
  capitel_signal_function  ( 10, ALLFILE_WAV );
  capitel_signal_function( 8, NULL);   // neu scannen

}

void  answer_cti                 (char * cti_number)
{
  char def_unknown[200];
  char cti_prg[200];

  config_file_read_string(STD_CFG_FILE,TEXT_UNKNOWN_ISDN,def_unknown,TEXT_UNKNOWN_ISDN_DEF);

  if (config_file_read_ulong(STD_CFG_FILE,CAPITEL_CTI_SUPPORT,CAPITEL_CTI_SUPPORT_DEF)) {
    config_file_read_string (STD_CFG_FILE,CAPITEL_CTI_PROGRAM,cti_prg,CAPITEL_CTI_PROGRAM_DEF);
    config_file_read_string (STD_CFG_FILE,TEXT_UNKNOWN_ISDN,def_unknown,TEXT_UNKNOWN_ISDN_DEF);
    if (!strcmp(cti_number,def_unknown)) {
      dos_start(cti_prg,         "", msg_str_cti);
    } else {
      dos_start(cti_prg, cti_number, msg_str_cti);
    }
  }

}


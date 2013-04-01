#include <stdio.h>

#ifndef unix
#include <conio.h>
#include <process.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "../../../units/common.src/bastypes.h"

#include "../../../units/common.src/os.h"
#include "../../answer/source/answer.h"
#include "../../isdn/source.os2/isdnmsg1.h"
#include "../../../units/common.src/num2nam.h"
#include "../../common/source/version.h"
#include "../../common/source/texte.h"
#include "../../util/source/dosstart.h" 
#include "../../common/source/global.h"

void capitel_number_of_call (short num)
{
  if (num >  0) { printf (CALL_MAKE_MASK_WAV, num);printf (" ");}
  if (num == 0) printf (" ->record <-");
  if (num <  0) printf (" ->offhook<-");
}

void capitel_have_no_carrier (unsigned long sec)
{
  printf (", %3ld seconds.\n", sec);
}

void capitel_remote_control (void)
{
  printf (", RC.\n");
}

void capitel_connect_ind (TCapiInfo *msg)
{
  struct tm *tmbuf;
  time_t tod;
  char cip = 'A';

  if (msg->is_digital) cip = 'D';

  time( &tod );
  tmbuf = localtime( &tod );

//printf ("\r\norg=%s\r\n",msg->caller_org_name);

  printf("%02d/%02d/%02d %02d:%02d:%02d, %s, from %s(%c)", tmbuf->tm_mday, tmbuf->tm_mon+1, tmbuf->tm_year+1900,tmbuf->tm_hour, tmbuf->tm_min, tmbuf->tm_sec, msg->called_name, msg->caller_name, cip );
  return;
}

void capitel_convert (unsigned long tip)
{
  if (tip) {
//    printf ("Capitel disabled while converting...\n");
  } else {
//    printf ("Converting done...\n");
  }
};

void capitel_deactivate      (void)
{
  printf ("Capitel deactivated!\n");
}
void capitel_exit            (void)
{
  printf ("Capitel stops\n");
  exit (99);
}

void show_copyright(void)
{
  printf ("\n");
  printf (APPSHORT);
#ifdef WIN32
  printf (" for WINDOWS 95 and WINDOWS/NT, Version ");
#endif
#ifdef OS2
  printf (" for OS/2, Version ");
#endif
#ifdef UNIX
  printf (" for UNIX, Version ");
#endif
  printf (APPVER);
  printf ("\n\n");
  printf ("%s %s  <%s>\n"     , "Copyright (c) " APPDATE " by", APP_AUTOR_2, APP_CAWIM_INET );
  printf ("                       and %s     <%s>\n\n", APP_AUTOR_1, APP_WERNER_INET);
}

char c = '#';

void sigFunc ( short num, void *msg )
{
  switch( num )  {

    case  1 : printf ("\n\nWarning Message: %s\n\n",(char*)msg);         break; // warning string
    case  2 : printf ("\n\nCritical Error : %s\n\n",(char*)msg);         break; // critical error string
    case  3 : printf ("\n\nFatal    Error : %s\n\n",(char*)msg); c = 27; break; // fatal error string
    case  4 : capitel_connect_ind     (msg);                      break; // incomming call
    case  5 : capitel_number_of_call  (*(short *)msg);            break; // filename of incomming call
    case  6 : capitel_have_no_carrier (*(unsigned long *)msg);    break; // off_hook
    case  7 : capitel_convert         (*(unsigned long *)msg);    break; // converting wav's
    case  8 :                                                     break; // rescan
    case  9 : capitel_remote_control  ();                         break; // remote_control
    case 10 :                                                     break; // play wav
    case 11 : capitel_deactivate      ();                         break; // deactivate
    case 12 : capitel_exit            ();                         break; // exit capitel
    default : printf ("Unknown capitel_sigfunc: %d\n", num);      break;
  }
  return;
}

int main(void)
{
  char device[100];

  setbuf( stdout, NULL );
  show_copyright();

  printf("Press ESC to abort or 'C' to clear screen or 'R' to record new welcome text.\n");
  printf("Starting answering machine on ");

  if(!answer_init( sigFunc, 1, LANGUAGE_ENG)) {
    answer_name_of_interface(device);
    printf ("%s ...\n\n",device);
    answer_listen();
    while( c != 27 ) {
      OsSleep (300);
#ifdef UNIX
      c = toupper(getchar());
#else      
      c = toupper(getch());
#endif      
      switch (c) {
        case 'C' : printf("[2J");
                   printf("Press ESC to abort or 'C' to clear screen or 'R' to record new welcome text.\n\n");
                   break;
        case 'R' : answer_record_welcome(DEFALWFILE);
                   printf("\n\nThe next imcomming call will be stored as new welcome text.\n");
                   break;
      }
    }
    answer_exit();

    printf("\n\nClosing %s ...\n", device);
    while (answer_cannot_close()){
      OsSleep (300);
    }  
  } else {
    printf("ERROR: Can't initialize comm-device!\n");
  }
  return (0);
}


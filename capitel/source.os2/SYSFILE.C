#define INCL_DOSMISC

#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "capitel.h"
#include "..\..\common\source\version.h"
#include "..\..\common\source\global.h"
#include "texte.h"
//#include "..\..\common\source\texte.h"

#define BASEDIR   ";]PT3]"
#define BOOTDIR   "CPPU]"
#define PDDNAME   "QPJOUEE/TZT"

#define DAYS       62         // Number of Evaluation Days

void MsgBox( char * );
void ErrorBox( char *, HWND );
char textmsg[];

short checkSystemFile( void )
{
  char filename[256], appver[5];
  short i;
  APIRET rc;
  HFILE FileHandle = 0;
  ULONG ActionTaken, bootdrive, version;
  FILESTATUS3 FileInfo = {{0}};
  struct tm *ptmbuf;
  struct tm tmbuf;
  time_t tod, past, usetime;
  short HOURMINSEC;

  strcpy( appver, APPVER );
  appver[1] = appver[2];
  appver[2] = 0;
  HOURMINSEC = (short) atoi( appver );

  HOURMINSEC &= 31;

  DosQuerySysInfo( QSV_BOOT_DRIVE, QSV_BOOT_DRIVE, &bootdrive,
                   sizeof( bootdrive ) );

  DosQuerySysInfo( QSV_VERSION_MINOR, QSV_VERSION_MINOR, &version,
                   sizeof( version ) );

  if( version < 30 )
    sprintf( filename, "%c%s%s", bootdrive+65, BASEDIR, PDDNAME );
  else
    sprintf( filename, "%c%s%s%s", bootdrive+65, BASEDIR, BOOTDIR, PDDNAME );

  for( i=0 ; i < strlen( filename ) ; i++ )
    filename[i] = filename[i]-1;

  rc = DosOpen( filename, &FileHandle, &ActionTaken, 0L, 0L,
                OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE, 0L );

  if( rc )
    return 0;

  rc = DosQueryFileInfo( FileHandle, FIL_STANDARD, &FileInfo,
                         sizeof( FILESTATUS3 ) );

  if( rc )
    return 0;

  time( &tod );
  ptmbuf = localtime( &tod );

  if( !((FileInfo.ftimeCreation.twosecs == HOURMINSEC) &&
      (FileInfo.ftimeCreation.minutes == HOURMINSEC/2) &&
      (FileInfo.ftimeCreation.hours == HOURMINSEC/3)) )
  {

    FileInfo.ftimeCreation.twosecs = HOURMINSEC;
    FileInfo.ftimeCreation.minutes = HOURMINSEC/2;
    FileInfo.ftimeCreation.hours   = HOURMINSEC/3;

    FileInfo.fdateCreation.day     = ptmbuf->tm_mday;
    FileInfo.fdateCreation.month   = ptmbuf->tm_mon+1;
    FileInfo.fdateCreation.year    = ptmbuf->tm_year-80;

    DosSetFileInfo( FileHandle, FIL_STANDARD, &FileInfo,
                    sizeof( FILESTATUS3 ) );

    DosClose( FileHandle );

    MsgBox( USECTEL );

    return 1;
  }

  DosClose( FileHandle );

  tmbuf.tm_sec  = FileInfo.ftimeCreation.twosecs;
  tmbuf.tm_min  = FileInfo.ftimeCreation.minutes;
  tmbuf.tm_hour = FileInfo.ftimeCreation.hours;
  tmbuf.tm_mday = FileInfo.fdateCreation.day;
  tmbuf.tm_mon  = FileInfo.fdateCreation.month-1;
  tmbuf.tm_year = FileInfo.fdateCreation.year+80;

  past = mktime( &tmbuf );

  usetime = (tod-past) / (24*3600);

  if( usetime > DAYS )
    return 0;

  if( DAYS-usetime <= 31 )
  {
    sprintf( textmsg, COPY_EXP_IN_N_DAYS, DAYS-usetime );
    ErrorBox( WARNING, hwndFrame );
  }

  return 1;
}


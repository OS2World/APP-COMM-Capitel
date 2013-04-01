void sigFunc( short num, void *msg );

#define INCL_DOSPROCESS   /* DOS process values - needed for DosSleep */
#define INCL_DOSSESMGR
#define INCL_DOSERRORS
#include <stdio.h>
#include <os2.h>
#include <string.h>

short is_os2 ( char *exename )
{
  ULONG apptype;
  if( DosQueryAppType( (PSZ)exename, &apptype) ) {
    return (1);
  } else {
    if( apptype & FAPPTYP_32BIT ) return(1);
  }
  return (0);
}

int dos_start(char *proc, char *parm, char *title)
{
  STARTDATA SData       = {0};
  char PgmTitle[200];                           /* Title                       */
  char PgmName[200];                            /* This starts an OS/2 session */
  APIRET    rc          = NO_ERROR;             /* Return code                 */
  PID       pid         = 0;                    /* PID returned                */
  ULONG     ulSessID    = 0;                    /* Session ID returned         */
  UCHAR     achObjBuf[100] = {0};

  strcpy (PgmTitle,title);
  strcpy (PgmName,proc);
  SData.FgBg = 0;

  if (is_os2(proc)) SData.SessionType = 2; else SData.SessionType = 7;

  SData.Length  = sizeof(STARTDATA);
  SData.Related = SSF_RELATED_CHILD;          /* start a dependent session */
  SData.TraceOpt = SSF_TRACEOPT_NONE;         /* No trace                  */
  SData.PgmTitle = PgmTitle;
  SData.PgmName = PgmName;
  SData.PgmInputs = parm;                     /* Keep session up           */
  SData.TermQ = 0;                            /* No termination queue      */
  SData.Environment = 0;                      /* No environment string     */
  SData.InheritOpt = SSF_INHERTOPT_SHELL;     /* Inherit shell's environ.  */
  SData.IconFile = 0;                         /* No icon association       */
  SData.PgmHandle = 0;
  SData.PgmControl = SSF_CONTROL_VISIBLE | SSF_CONTROL_MAXIMIZE;
  SData.InitXPos  = 30;                       /* Initial window coordinates*/
  SData.InitYPos  = 40;
  SData.InitXSize = 200;                      /* Initial window size       */
  SData.InitYSize = 140;
  SData.Reserved = 0;
  SData.ObjectBuffer  = achObjBuf;            /* Contains info if DosExecPgm fails */
  SData.ObjectBuffLen = (ULONG) sizeof(achObjBuf);

  rc = DosStartSession(&SData, &ulSessID, &pid);/* Start the session */
  return rc;
}




#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <process.h>

int dos_start(char *proc, char *parm, char *title)
{
SHELLEXECUTEINFO sShEx;

memset(&sShEx, 0, sizeof(sShEx));

sShEx.cbSize       = sizeof(sShEx);
sShEx.fMask        = SEE_MASK_FLAG_NO_UI;
sShEx.hwnd         = GetDesktopWindow();
sShEx.lpVerb       = 0;
sShEx.lpFile       = proc;
sShEx.lpParameters = parm;
sShEx.lpDirectory  = 0;
sShEx.nShow        = SW_SHOWDEFAULT;

ShellExecuteEx(&sShEx);
return(0);


/*
  int i = 1;
  char* c, * args[20];

  memset(args, 0, sizeof(args));

  args[0] = proc;

  c = strtok(parm, " ");
  while(c) {
    args[i++] = strdup(c);
    c = strtok(0, " ");
  }
  return spawnv(_P_NOWAIT,proc,args);
*/
}



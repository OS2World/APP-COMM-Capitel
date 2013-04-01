void sigFunc( short num, void *msg );

#include <stdio.h>

#ifndef UNIX
#include <conio.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "../../common/source/global.h"
#include "../../common/source/texte.h"

char *vorwahl_get_name (char *nummer)
{
   FILE *namedat;
   static char tmpstr[STD_STRING_LEN],str[STD_STRING_LEN];
   char *pt;

   namedat = fopen (NUMFILE,"r");

   if (namedat) {
      while (fgets(str,STD_STRING_LEN-1,namedat)) {
         strcpy (tmpstr,str);
         pt = strchr (tmpstr,':');
         if (pt) {
            *pt = 0;
            if (strstr(nummer,tmpstr) == nummer) {
               if (strchr(str,'\n')) { pt = strchr(str,'\n'); *pt = 0;}
               strcpy (tmpstr," (");
               strcat (tmpstr,strchr(str,':')+1);
               strcat (tmpstr,")");
               fclose (namedat);
               return (tmpstr);
            }
         }
      }
      fclose (namedat);
   } else {
      sprintf (str,STR_NOT_FOUND,NUMFILE);
      sigFunc (1,str);
   }
   return ("");
}



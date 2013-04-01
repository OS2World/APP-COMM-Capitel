/***************************************************************************/
/*** CapiTel Installation Rexx-Script                                    ***/
/*** (c) 1996-98 by Carsten Wimmer and Werner Fehn                       ***/
/*** All Rights Reserved.                                                ***/
/***************************************************************************/

Call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
Call SysLoadFuncs
Call SysCls

say ""
say " ============================================================================="
say ""
say "                 ////// /////// ////// //  /////// ////// //"
say "                //     //   // //  // //     //   //     //"
say "               //     /////// ////// //     //   ////   //"
say "              //     //   // //     //     //   //     //"
say "             ////// //   // //     //     //   ////// //////"
say ""
say "     ISDN CAPI based Answering Machine and Caller-ID for OS/2 PM or VIO"
say ""
say "                                Installation"
say ""
say " ============================================================================="
say ""
say ""

say "Do you want to install the (g)erman, (e)nglish, or (t)ext version?"
say "Moechten Sie die (d)eutsche, (e)nglische oder die (T)ext-Version"
say "installieren?"
call charout, "> "

pull answ .

say ""

select
  when ((answ = "G") | (answ = "g") | (answ = "D") | (answ = "d")) then do
    "ren ctelos2g.exe capitel.exe >NUL"
    "del ctelos2e.exe >NUL"
    "del ctelos2t.exe >NUL"
    "del license.txt > NUL"
    "del order.txt > NUL"
    "del orderbmt.txt > NUL"
    "del whatsnew.txt > NUL"
    "del readme.txt > NUL"
    end
  when ((answ = "E") | (answ = "e")) then do
    "ren ctelos2e.exe capitel.exe >NUL"
    "del ctelos2g.exe >NUL"
    "del ctelos2t.exe >NUL"
    "del lizenz.txt > NUL"
    "del bestell.txt > NUL"
    "del bestbmt.txt > NUL"
    "del neues.txt > NUL"
    "del liesmich.txt > NUL"
    end
  when ((answ = "T") | (answ = "t")) then do
    "ren ctelos2t.exe capitel.exe >NUL"
    "del ctelos2e.exe >NUL"
    "del ctelos2g.exe >NUL"
    "del lizenz.txt > NUL"
    "del bestell.txt > NUL"
    "del bestbmt.txt > NUL"
    "del neues.txt > NUL"
    "del liesmich.txt > NUL"
    end
  otherwise
    say "Aborting Installation!"
    say "Installation abgebrochen!"
    say ""
    exit
end

say "Should I create a CapiTel object on your desktop? (Y/N)"
say "Soll ich ein CapiTel Objekt auf Ihrer Arbeitsoberflaeche anlegen? (J/N)"
call charout, "> "

pull answ .

say ""

if answ <> "Y" & answ <> "y" & answ <> "J" & answ <> "j" then do
  say "No object has been created."
  say "Es wurde kein Objekt angelegt."
  say ""
  exit
end

ctEXE = "\CAPITEL.EXE"
myDir = directory()
setupString = "EXENAME="||myDir||ctEXE||";STARTUPDIR="||myDir";REPLACE"
call SysCreateObject "WPProgram", "CapiTel", "<WP_DESKTOP>", setupString

say "Object has been created."
say "Objekt wurde angelegt."

exit


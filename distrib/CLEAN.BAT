@echo off
del  distrib\exp\*.* /s /q > nul
rd distrib\exp\os2 > nul
rd distrib\exp\win> nul
rd distrib\exp > nul
del capitel\obj.os2\*.* < distrib\text\y.dat > nul
del capitel\obj.win\*.* < distrib\text\y.dat > nul
del isdn\obj.os2\*.*    < distrib\text\y.dat > nul
del isdn\obj.win\*.*    < distrib\text\y.dat > nul
del wave\obj.os2\*.*    < distrib\text\y.dat > nul
del wave\obj.win\*.*    < distrib\text\y.dat > nul
del util\obj.os2\*.*    < distrib\text\y.dat > nul
del util\obj.win\*.*    < distrib\text\y.dat > nul
del answer\obj.os2\*.*  < distrib\text\y.dat > nul
del answer\obj.win\*.*  < distrib\text\y.dat > nul
del serial\obj.win\*.*  < distrib\text\y.dat > nul
del distrib\setup.win\*.exe  < distrib\text\y.dat > nul
del distrib\setup.win\*.wsm  < distrib\text\y.dat > nul
del ..\units\win.obj\*.* < distrib\text\y.dat > nul
del ..\units\os2.obj\*.* < distrib\text\y.dat > nul
del call*.*                  < distrib\text\y.dat > nul
del *.exe                    < distrib\text\y.dat > nul
del *.pch                    < distrib\text\y.dat > nul
del *.pdb                    < distrib\text\y.dat > nul
del *.deb                    < distrib\text\y.dat > nul
del *.log                    < distrib\text\y.dat > nul
del *.zip               < distrib\text\y.dat > nul
del *.diz               < distrib\text\y.dat > nul
del *.txt               < distrib\text\y.dat > nul
del *.bak               < distrib\text\y.dat > nul
del *.wse               < distrib\text\y.dat > nul
del *.wav               < distrib\text\y.dat > nul
del *.ini               < distrib\text\y.dat > nul
del *.wsm               < distrib\text\y.dat > nul
del *.uk                < distrib\text\y.dat > nul
del *.nl                < distrib\text\y.dat > nul
del *.pdb               < distrib\text\y.dat > nul
del beep.alw            < distrib\text\y.dat > nul
del beep.wav            < distrib\text\y.dat > nul
del all_call.alw        < distrib\text\y.dat > nul
del capitel.act         < distrib\text\y.dat > nul
del capitel.cfg         < distrib\text\y.dat > nul
del capitel.nam         < distrib\text\y.dat > nul
del capitel.num         < distrib\text\y.dat > nul
del capitel.prt         < distrib\text\y.dat > nul
del capitel.prt.capi20  < distrib\text\y.dat > nul
del capitel.prt.capi11  < distrib\text\y.dat > nul
del ringing.alw         < distrib\text\y.dat > nul
del ringing.wav         < distrib\text\y.dat > nul
del welcome.alw         < distrib\text\y.dat > nul
del welcome.wav         < distrib\text\y.dat > nul
del call_all.wav        < distrib\text\y.dat > nul
del install.cmd         < distrib\text\y.dat > nul
attrib -h -s -r capitel.ini < distrib\text\y.dat > nul
del capitel.ini             < distrib\text\y.dat > nul
echo.


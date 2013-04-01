@echo off
nmake -f ctelos2t.mak
goto end






if "%user" == "cawim" goto cawim
goto end

:cawim
copy ctelos2t.exe \apps\capitel\capitel.exe
beep

:end


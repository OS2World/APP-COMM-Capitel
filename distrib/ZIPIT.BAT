@echo off
if "%user" == "cawim" goto cawim
if "%USERNAME%" == "kolja" goto kolja

if exist c:\download\ctelsrc.zip del c:\download\ctelsrc.zip
call clean
zip -r c:\download\ctelsrc *

goto end

:kolja
:cawim

if exist ctelsrc.zip del ctelsrc.zip
zip -r ctelsrc.zip *

:end


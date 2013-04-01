@echo off

md distrib\exp       > nul
md distrib\exp\os2   > nul
md distrib\exp\win   > nul

copy distrib\setup.all\*.* distrib\exp\os2   > nul
copy distrib\setup.os2\*.* distrib\exp\os2   > nul
copy ctelos2e.exe distrib\exp\os2            > nul
copy ctelos2g.exe distrib\exp\os2            > nul
copy ctelos2t.exe distrib\exp\os2            > nul

copy distrib\setup.all\*.* distrib\exp\win   > nul
copy distrib\setup.win\*.diz distrib\exp\win > nul
copy ctelwin.exe distrib\exp\win\capitel.exe > nul
copy ctelwint.exe distrib\exp\win            > nul







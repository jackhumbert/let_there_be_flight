call tools\build.bat
timeout /t 2
xcopy /s /d /y build\* "C:\Program Files (x86)\Steam\steamapps\common\Cyberpunk 2077\"
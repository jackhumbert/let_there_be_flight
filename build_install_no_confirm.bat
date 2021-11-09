call build.bat
timeout /t 2
xcopy /s /y .build\* "C:\Program Files (x86)\Steam\steamapps\common\Cyberpunk 2077\"
timeout /t 2
rmdir /s/q %~dp0.build
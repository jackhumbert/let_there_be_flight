call build.bat
xcopy /s /y .build\* "C:\Program Files (x86)\Steam\steamapps\common\Cyberpunk 2077\"
rmdir /s/q %~dp0.build
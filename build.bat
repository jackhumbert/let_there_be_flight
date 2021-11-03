if not exist ".build" mkdir .build
if not exist ".build\bin" mkdir .build\bin
if not exist ".build\bin\x64" mkdir .build\bin\x64
if not exist ".build\bin\x64\plugins" mkdir .build\bin\x64\plugins
if not exist ".build\bin\x64\plugins\flight_control" mkdir .build\bin\x64\plugins\flight_control
xcopy /y fmod_studio\.Build\Desktop\* .build\bin\x64\plugins\flight_control\
xcopy /y fmod_studio\API\* .build\bin\x64\
if not exist ".build\bin\x64\plugins\cyber_engine_tweaks" mkdir .build\bin\x64\plugins\cyber_engine_tweaks
if not exist ".build\bin\x64\plugins\cyber_engine_tweaks\mods" mkdir .build\bin\x64\plugins\cyber_engine_tweaks\mods
if not exist ".build\bin\x64\plugins\cyber_engine_tweaks\mods\flight_control" mkdir .build\bin\x64\plugins\cyber_engine_tweaks\mods\flight_control
xcopy /y cyber_engine_tweaks\* .build\bin\x64\plugins\cyber_engine_tweaks\mods\flight_control\
if not exist ".build\red4ext" mkdir .build\red4ext
if not exist ".build\red4ext\plugins" mkdir .build\red4ext\plugins
xcopy /y red4ext\build\debug\bin\flight_control.dll .build\red4ext\plugins\
if not exist ".build\r6" mkdir .build\r6
if not exist ".build\r6\scripts" mkdir .build\r6\scripts
if not exist ".build\r6\scripts\flight_control" mkdir .build\r6\scripts\flight_control
xcopy /y /e redscript\* .build\r6\scripts\flight_control\
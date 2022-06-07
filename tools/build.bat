if not exist "build" mkdir build
xcopy /y /d /s flight_effects\packed\* build\
if not exist "build\bin" mkdir build\bin
if not exist "build\bin\x64" mkdir build\bin\x64
xcopy /y /d fmod_studio\API\* build\bin\x64\
if not exist "build\r6\scripts" mkdir build\r6\scripts
@REM xcopy /y /d /s src\redscript\* build\r6\scripts\
@REM type src\redscript\flight_control\*.reds > build\r6\scripts\flight_control.reds
echo // Let There Be Flight > build\r6\scripts\flight_control.reds
echo // https://github.com/jackhumbert/let_there_be_flight >> build\r6\scripts\flight_control.reds
echo. >> build\r6\scripts\flight_control.reds
(for /f "delims=" %%a in ('dir /b /a-d src\redscript\flight_control\*.reds') do (
    echo // %%~a >> build\r6\scripts\flight_control.reds
    type src\redscript\flight_control\%%~a >> build\r6\scripts\flight_control.reds
    echo. >> build\r6\scripts\flight_control.reds
    echo. >> build\r6\scripts\flight_control.reds
  )
)
if not exist "build\r6\input" mkdir build\r6\input
xcopy /y /d /s src\input\* build\r6\input\
if not exist "build\r6\tweaks" mkdir build\r6\tweaks
xcopy /y /d /s src\tweaks\* build\r6\tweaks\
if not exist "build\red4ext" mkdir build\red4ext
if not exist "build\red4ext\plugins" mkdir build\red4ext\plugins
if not exist "build\red4ext\plugins\flight_control" mkdir build\red4ext\plugins\flight_control
xcopy /y /d fmod_studio\Build\Desktop\* build\red4ext\plugins\flight_control\
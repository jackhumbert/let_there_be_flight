if not exist "build" mkdir build
xcopy /y /d /s flight_effects\packed\* build\
if not exist "build\bin" mkdir build\bin
if not exist "build\bin\x64" mkdir build\bin\x64
xcopy /y /d fmod_studio\API\* build\bin\x64\
if not exist "build\r6\scripts" mkdir build\r6\scripts
if not exist "build\r6\scripts" mkdir build\r6\scripts\codeware
xcopy /y /d /s src\redscript\codeware build\r6\scripts\codeware\
echo // Let There Be Flight > build\r6\scripts\let_there_be_flight.reds
echo // https://github.com/jackhumbert/let_there_be_flight >> build\r6\scripts\let_there_be_flight.reds
echo. >> build\r6\scripts\let_there_be_flight.reds
(for /f "delims=" %%a in ('dir /b /a-d src\redscript\let_there_be_flight\*.reds') do (
    echo // %%~a >> build\r6\scripts\let_there_be_flight.reds
    type src\redscript\let_there_be_flight\%%~a >> build\r6\scripts\let_there_be_flight.reds
    echo. >> build\r6\scripts\let_there_be_flight.reds
    echo. >> build\r6\scripts\let_there_be_flight.reds
  )
)
if not exist "build\r6\input" mkdir build\r6\input
xcopy /y /d /s src\input\* build\r6\input\
if not exist "build\r6\tweaks" mkdir build\r6\tweaks
echo # Let There Be Flight > build\r6\tweaks\let_there_be_flight.yaml
echo # https://github.com/jackhumbert/let_there_be_flight >> build\r6\tweaks\let_there_be_flight.yaml
echo. >> build\r6\tweaks\let_there_be_flight.yaml
(for /f "delims=" %%a in ('dir /b /a-d src\tweaks\*.yaml') do (
    echo # %%~a >> build\r6\tweaks\let_there_be_flight.yaml
    type src\tweaks\%%~a >> build\r6\tweaks\let_there_be_flight.yaml
    echo. >> build\r6\tweaks\let_there_be_flight.yaml
    echo. >> build\r6\tweaks\let_there_be_flight.yaml
  )
)
if not exist "build\red4ext" mkdir build\red4ext
if not exist "build\red4ext\plugins" mkdir build\red4ext\plugins
if not exist "build\red4ext\plugins\flight_control" mkdir build\red4ext\plugins\flight_control
xcopy /y /d fmod_studio\Build\Desktop\* build\red4ext\plugins\flight_control\
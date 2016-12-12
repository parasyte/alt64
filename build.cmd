@echo off
set env="/usr/local/libdragon"

IF %1.==. GOTO default

bash --verbose -c "export N64_INST=%env%; make %1"

GOTO end

:default
bash --verbose -c "export N64_INST=%env%; make"

:end

pause
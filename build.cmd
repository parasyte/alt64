set PATH=%~dp0toolchain\gcc-toolchain-mips64\bin
@echo off

IF %1.==. (
  ::echo. "no parameter"
  make
) ELSE (
  ::echo. "parameter: %1"
  make -d %1
)

:pause
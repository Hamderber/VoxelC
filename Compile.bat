@echo off

REM Clear old object files (there shouldn't be any)
for /R %%f in (*.o) do del "%%f"

setlocal enabledelayedexpansion
set OBJFILES=

REM Compile all .c files except Main.c and UnitTester.c (they both have main())
for /R %%f in (*.c) do (
    if /I not "%%~nxf"=="Main.c" if /I not "%%~nxf"=="UnitTester.c" (
        echo Compiling %%f
        gcc -c "%%f" -o "%%~dpn.o"
        set OBJFILES=!OBJFILES! "%%~dpn.o"
    )
)

REM Build VoxelC.exe
echo Linking VoxelC.exe...
gcc %OBJFILES% Main.c -o VoxelC.exe -mconsole
echo Done VoxelC.exe
echo.

REM Build UnitTester.exe
echo Linking UnitTester.exe...
gcc %OBJFILES% UnitTester.c -o UnitTester.exe -mconsole
echo Done UnitTester.exe
echo.

REM Delete all object files
for /R %%f in (*.o) do del "%%f"

pause

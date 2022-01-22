@echo off


if exist .\output (
    echo
) else (
    md output
)

del /f /s /q .\output\*.*

go build  -o output/toy-engine.exe main.go

xcopy .\lib\*.* .\output\ /y
.\output\toy-engine.exe

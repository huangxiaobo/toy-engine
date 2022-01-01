@echo off


if exist .\output (
    echo
) else (
    md output
)

xcopy .\lib\*.* .\output\ /y

go build -o output/toy-engine.exe main.go

.\output\toy-engine.exe

PAUSE
@echo off

if exist .\output (
    echo "Empty output directory"
    del /f /s /q .\output\*.*
) else (
    echo "Create output directory"
    md output
)


echo "Start building..."
go build  -o output/toy-engine.exe main.go

echo "Copy lib file to output"
xcopy .\lib\*.* .\output\ /y


echo "Start app...."
.\output\toy-engine.exe

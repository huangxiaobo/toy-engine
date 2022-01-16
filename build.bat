@echo off

go env
::go mod download
::go mod vendor
::vend


echo "start build..."
qtdeploy build desktop main.go

echo "start run..."
xcopy .\lib\*.* .\deploy\windows\ /y
deploy\windows\toy-engine.exe

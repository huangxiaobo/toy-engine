package utils

import "os"

func GetCurrentDir() string {
	cwd, _ := os.Getwd()
	return cwd
}

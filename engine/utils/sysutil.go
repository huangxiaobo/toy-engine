package utils

import "os"

func GetCurrentDir() string {
	cwd, _ := os.Getwd()
	return cwd
}

func NextP2(val int32) int32 {
	var rVal int32 = 1
	for rVal < val {
		rVal <<= 1
	}
	return rVal
}

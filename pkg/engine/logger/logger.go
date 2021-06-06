package logger

import (
	"fmt"
	"runtime"
	"strings"

	"github.com/sirupsen/logrus"
)

var log = logrus.New()

func init() {
	log.SetLevel(logrus.TraceLevel)
	log.SetReportCaller(true)
	log.Formatter = &logrus.JSONFormatter{}
}

func fileInfo(skip int) string {
	_, file, line, ok := runtime.Caller(skip)
	if !ok {
		file = "<???>"
		line = 1
	} else {
		slash := strings.LastIndex(file, "/")
		if slash >= 0 {
			file = file[slash+1:]
		}
	}
	return fmt.Sprintf("%s:%d", file, line)
}

func Debug(args ...interface{}) {
	if log.Level >= logrus.DebugLevel {
		entry := log.WithFields(logrus.Fields{})
		entry.Data["file"] = fileInfo(2)
		entry.Debug(args...)
	}
}

func Info(args ...interface{}) {
	if log.Level >= logrus.InfoLevel {
		entry := log.WithFields(logrus.Fields{})
		entry.Data["file"] = fileInfo(2)
		entry.Info(args...)
	}
}

func Warn(args ...interface{}) {
	if log.Level >= logrus.WarnLevel {
		entry := log.WithFields(logrus.Fields{})
		entry.Data["file"] = fileInfo(2)
		entry.Warn(args...)
	}
}

func Error(args ...interface{}) {
	if log.Level >= logrus.ErrorLevel {
		entry := log.WithFields(logrus.Fields{})
		entry.Data["file"] = fileInfo(2)
		entry.Error(args...)
	}
}

func Fatal(args ...interface{}) {
	if log.Level >= logrus.FatalLevel {
		entry := log.WithFields(logrus.Fields{})
		entry.Data["file"] = fileInfo(2)
		entry.Fatal(args...)
	}
}

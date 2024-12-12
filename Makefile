export GOBIN ?= $(shell pwd)/bin

# 获取操作系统名称（Linux, Darwin for macOS, Windows_NT for Windows）
OS := $(shell uname -s)

# 获取架构信息并存储到 ARCH 变量中
ARCH := $(shell uname -m)
ifeq ($(ARCH), aarch64)
    ARCH=arm64
endif

TARGET_EXEC := toy-engine

ifeq ($(OS), Linux)
    CC = gcc
    GOOS=linux
    CFLAGS = -Wall -O2
endif
 
ifeq ($(OS), Darwin)  # macOS
    GOOS = darwin
    CC = clang
    CFLAGS = -Wall -O2
endif

.PHONY: build
build:
	GOOS=${GOOS} GOARCH=${ARCH} go build ${LDFLAGS} -o build/${TARGET_EXEC} *.go

.PHONY: clean
clean:
	rm -rf build

.PHONY: tidy
tidy:
	go mod tidy

.PHONY: test
test:
	@go test -v ./... -cover

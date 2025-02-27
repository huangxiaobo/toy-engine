#!/usr/bin/env bash

rm -rf build/*

cmake -E make_directory build
cmake -E chdir build cmake ..
cmake --build build

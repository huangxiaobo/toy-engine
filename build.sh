#!/usr/bin/env bash

cmake -E make_directory build
cmake -E chdir build cmake ..

if cmake --build build -j8; then
  echo "Build successful"
  ./build/bin/toy-engine
else
  echo "Build failed"
  exit 1
fi

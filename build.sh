#!/usr/bin/env bash

rm -rf output/*

cmake -E make_directory output
cmake -E chdir output cmake ..
cmake --build output -j8

if [ $? -eq 0 ]; then
  echo "Build successful"
  ./output/bin/toy-engine
else
  echo "Build failed"
  exit 1
fi

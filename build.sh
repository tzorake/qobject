#!/bin/bash

CXX="clang++"
CXXFLAGS="-std=c++17 -Wall -Wextra -pedantic -fsanitize=address"

SOURCE_FILE="qobject_test.cpp"
EXECUTABLE="qobject_test"

$CXX $CXXFLAGS -o $EXECUTABLE $SOURCE_FILE

if [ $? -eq 0 ]; then
    echo "Compilation successful."
    echo "Running tests..."
    ./$EXECUTABLE
else
    echo "Compilation failed."
    exit 1
fi
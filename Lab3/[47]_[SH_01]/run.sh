#!/bin/bash

echo "Compiling shell.c..."

gcc shell.c -o myshell

if [ $? -eq 0 ]; then
    echo "Compilation successful!"
    echo "Starting MyShell..."
    echo "--------------------------------"
    ./myshell
else
    echo "Compilation failed!"
fi
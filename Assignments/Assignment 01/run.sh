#!/bin/bash

gcc shell.c -o shell

if [ $? -ne 0 ]; then
    echo "Compilation failed."
    exit 1
fi

chmod +x shell
./shell
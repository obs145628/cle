#!/bin/bash

if [[ $* == *--only-build* ]]; then {
    exit 0
} fi

python3 app/app-regex-simple.py hexa "(a|b)c"
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/app-regex-simple.py hexa (a|b)c' Failed !";
    exit 1
} fi

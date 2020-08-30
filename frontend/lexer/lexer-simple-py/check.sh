#!/bin/bash

if [[ $* == *--only-build* ]]; then {
    exit 0
} fi

tmp_dir=$(mktemp -d -t regex-simple-py-XXXXXXXXXX)

python3 app/app-lexer-simple.py config/rules1.txt examples/input1.txt
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/app-lexer-simple.py config/rules1.txt examples/input1.txt' Failed !";
    exit 1
} fi

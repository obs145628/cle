#!/bin/bash

if [[ $* == *--only-build* ]]; then {
    exit 0
} fi

tmp_dir=$(mktemp -d -t regex-simple-py-XXXXXXXXXX)

python3 app/app-regex-simple.py hexa "(a|b)c" < examples/input1.txt > $tmp_dir/output1.txt && diff examples/ref1.txt $tmp_dir/output1.txt
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/app-regex-simple.py hexa (a|b)c < examples/input1.txt > $tmp_dir/output1.txt' Failed !";
    exit 1
} fi

python3 app/app-regex-simple.py hexa "((ab)+)|([0-2]c*)" < examples/input2.txt > $tmp_dir/output2.txt && diff examples/ref2.txt $tmp_dir/output2.txt
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/app-regex-simple.py hexa ((ab)+)|([0-2]c*) < examples/input2.txt > $tmp_dir/output2.txt && diff examples/ref2.txt $tmp_dir/output2.txt' Failed !";
    exit 1
} fi

python3 app/app-regex-simple.py alphanum "if|else|elif|(el+e)|(e?i?f)" < examples/input3.txt > $tmp_dir/output3.txt && diff examples/ref3.txt $tmp_dir/output3.txt
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/app-regex-simple.py alphanum if|else|elif|(el+e)|(e?i?f) < examples/input3.txt > $tmp_dir/output3.txt && diff examples/ref3.txt $tmp_dir/output3.txt' Failed !";
    exit 1
} fi

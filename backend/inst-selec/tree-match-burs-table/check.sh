#!/bin/bash

if [[ $* == *--only-build* ]]; then {
    exit 0
} fi

HIDE_DOT_GUI=1 python3 app/optree.py
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/optree.py' Failed !";
    exit 1
} fi

python3 app/rules.py
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/rules.py' Failed !";
    exit 1
} fi

HIDE_DOT_GUI=1 python3 app/match_naive.py  ./examples/ex1.ir
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/match_naive.py  ./examples/ex1.ir' Failed !";
    exit 1
} fi

HIDE_DOT_GUI=1 python3 app/match_table_full.py  ./examples/ex1.ir
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/match_table_full.py  ./examples/ex1.ir' Failed !";
    exit 1
} fi
		      
HIDE_DOT_GUI=1 python3 app/match_table_rep.py  ./examples/ex1.ir
if [ $? -ne 0 ]; then {
    echo "Test 'python3 app/match_table_rep.py  ./examples/ex1.ir' Failed !";
    exit 1
} fi

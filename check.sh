#!/bin/bash

# Basic script to run build and test command on all projects

# Use `git clean -dfx` to make sure repo is clean before runing tests

CMAKE_CMD="make -j8 && make check -j8"
if [[ $* == *--only-build* ]]; then {
  CMAKE_CMD="make -j8"
} fi

check_cmake_proj() {
    (
	echo "Testing $1 ..."
	cd ./$1;
	mkdir -p _build;
	cd _build;
	cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=1;
	eval $CMAKE_CMD

    )
    if [ $? -ne 0 ]; then {
	echo "Tests failed for project $1";
	exit 1
    } fi
}

CARGO_CMD="cargo build && cargo test"
if [[ $* == *--only-build* ]]; then {
  CARGO_CMD="cargo build"
} fi

check_cargo_proj() {
    (
	echo "Testing $1 ..."
	cd ./$1;
	eval $CARGO_CMD

    )
    if [ $? -ne 0 ]; then {
	echo "Tests failed for project $1";
	exit 1
    } fi
}

SHELL_CMD="./check.sh"
if [[ $* == *--only-build* ]]; then {
  SHELL_CMD="./check.sh --only-build"
} fi

check_shell_proj() {
    (
	echo "Testing $1 ..."
	cd ./$1;
	eval $SHELL_CMD

    )
    if [ $? -ne 0 ]; then {
	echo "Tests failed for project $1";
	exit 1
    } fi
}

check_cargo_proj extern/logia/server
check_cmake_proj extern/logia/libcpp

check_cmake_proj utils/libcpp_utils
check_cmake_proj utils/libcpp_gop10

check_cmake_proj backend/inst-sched/local-list
check_cmake_proj backend/inst-sched/local-list-eb
check_cmake_proj backend/inst-selec/tree-match-burs1
check_shell_proj backend/inst-selec/tree-match-burs-table
check_cmake_proj backend/inst-selec/tree-match-graphs
check_cmake_proj backend/inst-selec/tree-match-naive
check_cmake_proj backend/reg-alloc/block-bottomup
check_cmake_proj backend/reg-alloc/block-naive
check_cmake_proj backend/reg-alloc/color-ssa-bu
check_cmake_proj backend/reg-alloc/color-ssa-td

check_shell_proj frontend/lexer/lexer-simple-py
check_shell_proj frontend/lexer/regex-simple-py

check_cmake_proj middle-end-optis/dead-code-elim
check_cmake_proj middle-end-optis/dominance
check_cmake_proj middle-end-optis/dom-value-numbering
check_cmake_proj middle-end-optis/fun-inliner
check_cmake_proj middle-end-optis/global-code-placement
check_cmake_proj middle-end-optis/idom
check_cmake_proj middle-end-optis/interproc-constprop
check_cmake_proj middle-end-optis/lazy-code-motion
check_cmake_proj middle-end-optis/live-uninit-regs
check_cmake_proj middle-end-optis/local-value-numbering
check_cmake_proj middle-end-optis/procedure-placement
check_cmake_proj middle-end-optis/sparsecond-constprop
check_cmake_proj middle-end-optis/sparse-simple-constprop
check_cmake_proj middle-end-optis/ssa-semipruned
check_cmake_proj middle-end-optis/superblock-cloning
check_cmake_proj middle-end-optis/superlocal-value-numbering
check_cmake_proj middle-end-optis/tree-height-balancing
check_cmake_proj middle-end-optis/unssa

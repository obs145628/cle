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

check_cmake_proj utils/libcpp_utils
check_cargo_proj utils/logia/server
check_cmake_proj utils/logia/libcpp
check_cmake_proj utils/libcpp_gop10

check_cmake_proj backend/inst-sched/local-list
check_cmake_proj backend/inst-sched/local-list-eb
check_cmake_proj backend/reg-alloc/block-naive
check_cmake_proj backend/reg-alloc/block-bottomup
check_cmake_proj backend/reg-alloc/color-ssa-td
check_cmake_proj backend/reg-alloc/color-ssa-bu

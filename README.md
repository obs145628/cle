# Compiler Experiments

Several compiler experiments based on my readings of book / papers

## Content

### backend

Compiler backend

### middle-end-optis

Compiler middle-end optimizations

### utils

Several libs and other misc files used through the whole repository.

## Books

- Engineering a Compiler, Second Edition - Keith Cooper, Linda Torczon 

## Environment

Tested on Ubuntu 18.04:
- C++ compiler: g++ 7.5.0.
- Rust compiler: rustc 1.45.2
- python 3.6.9
- Cmake 3.10.2.
- LLVM 10.0.0 (d32170dbd5b0d54436537b6b75beaf44324e0c28)

## Build

Load dependencies:
```
git submodule update --init
```

Make sure rustc is set to nightly to build `logia` server:
```
cd extern/logia/server
rustup override set nightly
```

Build and run tests:
```
./check.sh
```

Build only:
```
./check.sh --only-build
```

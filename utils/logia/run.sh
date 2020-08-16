#!/bin/sh

cd app
npm install
npx webpack
if [ "$?" -ne "0" ]; then
    echo "Webpack build Failed !"
    exit 1
fi

cd ../server
cargo run

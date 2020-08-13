# MDLogger

MDLogger is a library to log program execution and statistics in more organized and graphical way than just dumping to stdout, and writing log / image files.  

All logging is written to files using Markdown syntax.  

These files are then compiled to html, and can be displayed using a WebApplication.  

## Data System

All generated markdown files lives in `./data` directory

There is a lat list of markdown documents.  
Each is represented by a directory, it contains:
- a configuration file.
- the markdown file.
- other resource files (images, dot files, etc) used by the markdown.

Documents may connect to other documents using links.

One special document is the root document. There is no related directory, it is always generated.  
It contain links to all documents with option `link-in-root` set.  

Usually, when an executable is ran, it generates one main document with `link-in-root`, 
and other documents with links from the main one.



## Build webapp

```shell
cd app
npm install
npx webpack
```

## Start Server

```shell
cd server
cargo run
```

## Run script

Build webapp, and start server

```shell
./run.sh
```

## Cleanup data

Remove all generated server files:

```shell
rm -rf www/docs/* data/doc___root/ data/config.json
```

Remove all input data:
```shell
rm -rf data/*
```

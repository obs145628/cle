# logia

logia is a library to log program execution and statistics in more organized and graphical way than just dumping to stdout, and writing log / image files.  

All logging is written to files using a formated text syntax.  

These files are then compiled to html, and can be displayed using a WebApplication.  

## Text formats:

- [cmark-gfm](https://github.com/github/cmark-gfm): Github version of Markdown


## Data System

All generated files lives in `./data` directory

There is a flat list ofdocuments.  
Each is represented by a directory, it contains:
- a configuration file.
- the main text file file.
- other resource files (images, dot files, etc) used by the document.

Documents may connect to other documents using links.

One special document is the root document. There is no related directory, it is always generated.  
It contain links to all documents with option `link-in-root` set.  

Usually, when an executable is ran, it generates one main document with `link-in-root`, 
and other documents with links from the main one.  
This can be done with the `Corpus` class in lib implementations



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

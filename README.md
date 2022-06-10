# Lines

The program that help you with counting lines in your projects, directories or files.

## Getting the repository

```console
$ git clone https://github.com/M0x1m/lines
```

## Installing

`gcc` package must be installed on your operating system for building the program.

Linux:

```console
$ cd lines
$ bash build.sh
$ sudo mv lines /usr/bin/
```

## Usage

```
$ lines [flags] [files | directories]
```
or
```
$ {any command} | lines
```
### Flags:

__-v__ &emsp; Verbose mode

Prints more information about the current goal.\
If this flag provided, the program prints current file name when counts lines in files.\
And when program is sorting diretory enries prints current position in soring.

__-L__ &emsp; Follows all symlinks

If current directory entry or file is symbolic link and this flag is provided, this symoblic link will be dereferenced and lines will be counted from it's context. Otherwise, if this flag is not provided, this symbolic link will be skiped.

__-a__ &emsp; Print all files

If this flag is provided, program will also count lines in files whose name starts with `.`.\
Otherwise, without this flag, these files will be ignored.

__-N__ &emsp; Don't print `0` lines files

If this flag in provided, program will ignore files whose do not contain any lines.

__-gl__ &emsp; Sort by more lines(default) in file

The result of sorting are elements that will be arranged by ascending order downwards.\
Last printed element will contain higher number of lines than above.

__-ll__ &emsp; Sort by less lines in file

Last printed element will contain lower number of lines than above.

__-gs__ &emsp; Sort by more size of file

Last printed element will have higher size than above.

__-ls__ &emsp; Sort by less size of file

Last printed element will have lower size than above.

__-ns__ &emsp; Print without sorting

Print all elements without sorting in arbitrary order.

__-h__ &emsp; Prints short helping message

Prints short information about flags for help.

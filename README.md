# Project3

## project structure

```
.
├── Makefile		makefile for the whole project
├── Prj3README.md	this file
├── report.pdf		project report
├── step1
│   ├── cse2303header.h	header file
│   ├── disk.c			disk simulator source file
│   └── Makefile		makefile
├── step2
│   ├── cse2303header.h	header file
│   ├── disk.c			disk simulator source file
│   ├── fs.c			file system source file
│   └── Makefile		makefile
├── step3
│   ├── client.c		client source file
│   ├── cse2303header.h	header file
│   ├── disk.c			disk simulator source file
│   ├── fs.c			file system source file
│   └── Makefile		makefile
└── typescript.md		typescript

```

## make the project

If you want to make all the project, type in this directory

```
make
```

If you want to make only the subproject in folder `($FOLDER)`, type in this directory
```
make ($FOLDER)
```

or
```
cd ($FOLDER) && make
```

step1 and step2 are semi-finished products, you can ignore them and see step3.

## Usage

### disk

a simple disk simulator.

run with

```
./disk <#cylinders> <#sector per cylinder> <track-to-track delay> <#disk-storage-filename>
```

### fs

Noted that I've implemented socket communication between fs and disk, so run the `disk` program above with an addition argument `PORT`and run this program in a different terminal, i.e.

```
./disk <#cylinders> <#sectors per cylinder> <track-to-track delay> <#disk-storage-filename> <DiskPort>
./fs <DiskPort>
```

### work together

run `disk`, `fs` and `client` together in different terminals, i.e.

```
./disk <#cylinders> <#sectors per cylinder> <track-to-track delay> <#disk-storage-filename> <DiskPort>
./fs <DiskPort> <FSPort>
./client <FSPort>
```

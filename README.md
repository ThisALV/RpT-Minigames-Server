# RpT-Minigames-Server

Backend for [RpT-Minigames WebApp](#rpt-minigames-web-application), a web application to play minigames online from your web browser. Backend uses the [RpT-WebApp](#rpt-webapp-engine-implementation-details) C++ backend engine.

## Table of content

* [RpT-Minigames-Server](#rpt-minigames-server)
  * [Table of content](#table-of-content)
  * [RpT-Minigames Web application](#rpt-minigames-web-application)
    * [RpT-WebApp engine (implementation details)](#rpt-webapp-engine-implementation-details)
  * [Install](#install)
    * [Windows](#windows)
    * [Ubuntu (or Windows with MinGW)](#ubuntu-or-windows-with-mingw)
      * [Requirements](#requirements)
      * [Install steps](#install-steps)
  * [Run](#run)
  * [Special credits](#special-credits)

## RpT-Minigames Web application

RpT-Minigames is a web application to play some minigames from web browser. 3 minigames are playable:
- Açores
- Bermudes
- Canaries

User can select multiple room from web application, each room running on exactly one server.
A server will be [launched](#run) to run one of these minigames.

### RpT-WebApp engine (implementation details)

This project is actually a proof-of-concept for RpT-WebApp. It is a web application engine
using C++ backend and Angular (TypeScript) front-end.

RpT-WebApp divides web applications into many Services which may interact with
registered & connected clients known as *actors*.

A client first give an UID and a name to be registered into server and to have
an actor. This actor piloted by client sends *Service Requests* to server,
then server replies to notify client about SR handling result on its side.
When something happens inside server, it syncs actors about its new state by
sending them *Service Events*, then continues its run.

Actors (clients) <-> Server communication uses RPTL & SER protocols.

To get every detail about these protocols or RpT library usage, please check server
Doxygen documentation available by running `cmake --build doc` from `build`
generated directory. HTML local generated files will be inside `docs/` from
repo's root directory.

## Install

### Windows

You'll need to install MinGW environment using Msys2.

[How to install MinGW using Msys2](https://www.msys2.org/wiki/MSYS2-installation/)

### Ubuntu (or Windows with MinGW)

#### Requirements

- git
- cmake >= 3.12
- g++ >= 7

Install with :

```shell
sudo apt-get install git cmake g++
```

#### Install steps

Open a terminal and run following commands :

```shell
$ git clone https://github.com/ThisALV/RpT-Minigames-Server
$ cd RpT-Minigames-Server/dist
$ sudo ./get-deps-debian.sh # or ./get-deps-msys2.sh <32|64> for Windows MinGW users depending on current system architecture
$ cd ..
$ ./build.sh # add --mingw option if you're Windows MinGW user
$ sudo ./install.sh
```

If you don't want to install RpT-Minigames-Server at system level, add `--local` option to `./build.sh`.
It will install files to ./dist/install directory.

## Run

```shell
$ minigames-server --game <a|b|c> [--port <0..65535>] [--addr <local_address>] # for Linux users
$ minigames-server.exe --game <a|b|c> [--port <0..65535>] [--addr <local_address>] # for Windows MinGW users

## If locally installed with --local, from cloned project directory
$ ./dist/install/bin/minigames-server --game <a|b|c> [--port <0..65535>] [--addr <local_address>] # for Linux users
$ ./dist/install/bin/minigames-server.exe --game <a|b|c> [--port <0..65535>] [--addr <local_address>] # for Windows MinGW users
```

Each letter a, b or c stands for following game:
- Açores
- Bermudes
- Canaries


## Special credits

Doxygen doc-style directory is [forked](https://github.com/ThisALV/doxygen-dark-theme) from [MaJerle repo](https://github.com/MaJerle/doxygen-dark-theme) adjusting some color settings like for menus or links.
# RpT-Minigames-Server

Backend for [RpT-Minigames WebApp](#rpt-minigames-web-application), a web application to play minigames online from your web browser. Backend uses the [RpT-WebApp](#rpt-webapp-engine-implementation-details) C++ backend engine.

## Table of content

* [RpTogether-Server](#rptogether-server)
  * [Table of content](#table-of-content)
  * [Roleplay-Together project](#roleplay-together-project)
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

User can select multiple room from web application, each room running on exactly one server. A server will be [launched] to run one of these minigames.

### RpT-WebApp engine (implementation details)

This project is actually a proof-of-concept for RpT-WebApp. It is a web application engine using C++ backend and Angular (TypeScript) front-end.

RpT-WebApp divides web applications into many services which ma

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
git clone https://github.com/ThisALV/RpT-Minigames-Server
cd RpT-Minigames-Server/dist
sudo ./get-deps-debian.sh # or ./get-deps-msys2.sh <32|64> for Windows MinGW users depending on current system architecture
cd ..
./build.sh # add --mingw option if you're Windows MinGW user
sudo ./install.sh
```

If you don't want to install RpT-Minigames-Server at system level, add `--local` option to `./build.sh`.
It will install files to ./dist/install directory.

## Run

```shell
minigames-server --game <a|b|c> [--port <0..65535>] [--addr <local_address>] # for Linux users
minigames-server.exe --game <a|b|c> [--port <0..65535>] [--addr <local_address>] # for Windows MinGW users

## If locally installed with --local, from cloned project directory
./dist/install/bin/minigames-server --game <a|b|c> [--port <0..65535>] [--addr <local_address>] # for Linux users
./dist/install/bin/minigames-server.exe --game <a|b|c> [--port <0..65535>] [--addr <local_address>] # for Windows MinGW users
```

Each letter a, b or c stands for following game:
- Açores
- Bermudes
- Canaries


## Special credits

Doxygen doc-style directory is [forked](https://github.com/ThisALV/doxygen-dark-theme) from [MaJerle repo](https://github.com/MaJerle/doxygen-dark-theme) adjusting some color settings like for menus or links.
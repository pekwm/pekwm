[(Basic Usage) Next >](basic-usage.md)

***

An overview of pekwm
====================

pekwm is a fast, functional, and flexible window manager which aims to
be usable, even without a mouse.

**Table of Contents**

1. [An Introduction to pekwm](#an-introduction-to-pekwm)
1. [Getting pekwm](#getting-pekwm)
1. [Compiling pekwm](#compiling-pekwm)

An Introduction to pekwm
------------------------

pekwm, a window manager written by Claes Nästén that was once based on
the aewm++ window manager, but has since evolved enough that it no
longer resembles aewm++ at all. It also has an expanded feature-set,
including window grouping (similar to ion, pwm, or fluxbox), auto
properties, Xinerama support and keygrabber that supports keychains,
and much more.

### Why pekwm?

"Why make another window manager?", some ask. This may confuse some
people, but the best answer is "Why not?". There are arguments out
there that it's better to have a single standard desktop environment,
so that our mothers can find their way around, but in all honestly, if
most of us wanted the same environment as our mothers, we probably
wouldn't be reading this anyway. The same can also be applied to Your
sister, your roommate, your wife, even your cat.

"Why should I use pekwm?", others ask. Nobody ever said you
should. However, we use it. And you're welcome to as well. You should
use the environment most suited to you. For a better answer to this
question, Check out the [pekwm Features](#pekwm-features)
section below.

### pekwm Features

Here's a short list of some of the features included in pekwm:

- Possibility to group windows in a single frame
- Configurable keygrabber that supports keychains
- Configurable mouse actions
- Configurable root- and window-menus and keybindings for all menus
- Dynamic menus that regenerate on every view from a script output
- Multi-screen support both via RandR and Xinerama
- Configurable window placement
- Theming support with images, shaping and configurable buttons.
- Autoproperties (Automatic properties such as a window's sticky state, etc.)
    

Getting pekwm
-------------

Now that you've decided to try it out, you need to get it. You're left
with two options. The first is to download and compile the source, and
the second is finding a pre-compiled package.

### Getting the pekwm source

The source code is available from Github at https://github.com/pekdon/pekwm.

Release tabralls are named pekwm-0.2.0.tar.gz and
pekwm-0.2.0.tar.bz2. Although it doesn't matter which you get, keep in
mind that the .bz2 is smaller.

Generally pekwm from GIT is stable enough for everyday use and should
in most cases be a safe bet to get all the latest functionality.

### Getting prebuilt pekwm packages

pekwm is available as a package on many Linux and BSD distributions,
see your distribution for details.

Compiling pekwm
---------------

This chapter will help you get pekwm compiled.

### Unpacking the Archive

The first step to compiling pekwm is to unpack the archive. Unpacking
it depends on which version you downloaded:

```
tar -zxvf pekwm-0.2.0.tar.gz
tar -zjvf pekwm-0.2.0.tar.bz2
```

> The '-j' option works normally on most linux systems, and as of the
> current GNU tar development version, is part of GNU tar. If your
> system does not support the -j option, you can use two things:
> **bzip2 -dc pekwm-0.2.0.tar.bz2 | tar -xvf -** or **bzip2 -d
> pekwm-0.2.0.tar.bz2** followed by **tar -xvf pekwm-0.2.0.tar**. This
> also works for the .tar.gz version using **gzip -dc** or **gzip
> -d**.

The 'v' options are optional, they show you the filenames as they're
being extracted. at this point, you should have a pekwm-0.2.0
directory. Use **cd pekwm-0.2.0** to get there.

### Installing build dependencies

Before building pekwm a C++ compiler with support for C++11 needs to
be available on the system, the CMake build system and a set of X11
and image libraries.

The below sections details how to install the required packages for
different OSes and Linux distributions.

#### Alpine

Development tools using the GCC C++ compiler:

```
# apk add cmake g++ make
```

Build dependencies:

```
# apk add fontconfig-dev jpeg-dev libxext-dev libpng-dev libxft-dev libxpm-dev
```

#### Debian

Development tools using the GCC C++ compiler:

```
# apt install cmake g++ make
```

Build dependencies:

```
# apt install libfontconfig1-dev libjpeg-dev libxext-dev libpng-dev libxft-dev libxpm-dev
```

#### OpenBSD

OpenBSD comes with X11 and a compatible C++ compiler. To add the
required packages run:

```
# pkg_add cmake jpeg libiconv png
```

#### OS X (homebrew)

OS X does not come with a X11 installation by default so first
[XQuartz](https://www.xquartz.org/) needs to be installed.

The development tools are not installed by default but can be
installed using the following command:

```
xcode-select --install
```

Assuming homebrew is installed, the only package required to build
pekwm after install XQuartz and the development tools is CMake:

```
$ brew install cmake
```

### Setting up a build directory

The first thing to do is to setup a build directory and configure
pekwm using CMake:

```
mkdir build && cd build
cmake ..
```

Pekwm support a few configuration options, each option is specified on
the cmake command line as:

```
-DOPTION=ON
```


### Common options

| Option               | Default    | Description                                                   |
|----------------------|------------|---------------------------------------------------------------|
| CMAKE_INSTALL_PREFIX | /usr/local | It may be useful to use a custom prefix to install the files. |
| DEBUG                | OFF        | Enable debug outputs and code                                 |
| PEDANTIC             | OFF        | Turn on extra compiler warnings                               |
| TESTS                | OFF        | Enable compilation of unit test programs                      |

### Options to reduce the size

| Option            | Default | Description                                                          |
|-------------------|---------|----------------------------------------------------------------------|
| ENABLE_SHAPE      | ON      | Enables the use of the Xshape extension for non-rectangular windows. |
| ENABLE_XINERAMA   | ON      | Enables Xinerama multi screen support                                |
| ENABLE_RANDR      | ON      | Enables RandR multi screen support                                   |
| ENABLE_XFT        | ON      | Enables Xft font support in pekwm (themes).                          |
| ENABLE_IMAGE_XPM  | ON      | XPM image support using libXpm.                                      |
| ENABLE_IMAGE_JPEG | ON      | JPEG image support using libjpeg.                                    |
| ENABLE_IMAGE_PNG  | ON      | PNG image support using libpng.                                      |

### Building and installing

After running cmake with any options you need, run **make**. This
should only take a few minutes. After that, become root (unless you
used a prefix in your home directory, such as
-DCMAKE_INSTALL_PREFIX=/home/you/pkg) and type **make install**

Adding **exec pekwm** to ~/.xinitrc if you start X running **startx**
or ~/.xsession if you use a display manager should usually be enough
to get pekwm running.

That's it! pekwm is installed on your computer now. Next you should
read the [Getting Started](basic-usage.md#getting-started) chapter.

***

[(Basic Usage) Next >](basic-usage.md)

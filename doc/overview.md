I. An overview of pekwm
=======================

Pekwm is a fast, functional, and flexible window manager which aims to be usable, even without a mouse.

**Table of Contents**

1\. [An Introduction to Pekwm](#overview-intro)

2\. [Getting Pekwm](#overview-getting)

3\. [Compiling Pekwm](#overview-compiling)

* * *

Chapter 1. An Introduction to Pekwm
===================================

The Pekwm Window Manager is written by Claes Nästén. The code is based on the aewm++ window manager, but it has evolved enough that it no longer resembles aewm++ at all. It also has an expanded feature-set, including window grouping (similar to ion, pwm, or fluxbox), auto properties, xinerama and keygrabber that supports keychains, and much more.

* * *

#### 1.1. Why Pekwm?

"Why make another window manager?", some ask. This may confuse some people, but the best answer is "Why not?". There are arguments out there that it's better to have a single standard desktop environment, so that our mothers can find their way around, but in all honestly, if most of us wanted the same environment as our mothers, we probably wouldn't be reading this anyway. The same can also be applied to Your sister, your roommate, your wife, even your cat.

"Why should I use pekwm?", others ask. Nobody ever said you should. However, we use it. And you're welcome to as well. You should use the environment most suited to you. For a better answer to this question, Check out the [Pekwm Features](#overview-intro-features) section below.

* * *

#### 1.2. Pekwm Features

Here's a short list of some of the features included in pekwm:

*   Possibility to group windows in a single frame
    
*   Configurable keygrabber that supports keychains
    
*   Configurable mouse actions
    
*   Configurable root- and window-menus and keybindings for all menus
    
*   Dynamic menus that regenerate on every view from a script output
    
*   Multi-screen support both via RandR and Xinerama
    
*   Configurable window placement
    
*   Theming support with images, shaping and configurable buttons.
    
*   Autoproperties (Automatic properties such as a window's sticky state, etc.)
    

* * *

Chapter 2. Getting Pekwm
========================

Now that you've decided to try it out, you need to get it. You're left with two options. The first is to download and compile the source, and the second is finding a pre-compiled package.

* * *

#### 2.1. Getting the Pekwm source

The source code is available from the pekwm website, [http://pekwm.org](http://pekwm.org).

Files are named pekwm-GIT.tar.gz and pekwm-GIT.tar.bz2. Although it doesn't matter which you get, keep in mind that the .bz2 is smaller.

* * *

#### 2.2. Getting prebuilt Pekwm packages

Links to pre-built pekwm packages are available at the pekwm website, [http://pekwm.org](http://pekwm.org).

The current version of pekwm is GIT.

If there's no package for your distribution, and you'd like to build one, Let us know! We'll gladly host or link to binary packages.

* * *

Chapter 3. Compiling Pekwm
==========================

This chapter will help you get pekwm compiled.

* * *

#### 3.1. Unpacking the Archive

The first step to compiling pekwm is to unpack the archive. Unpacking it depends on which version you downloaded:

tar -zxvf pekwm-GIT.tar.gz
tar -jxvf pekwm-GIT.tar.bz2

> The '-j' option works normally on most linux systems, and as of the current GNU tar development version, is part of GNU tar. If your system does not support the -j option, you can use two things: **bzip2 -dc pekwm-GIT.tar.bz2 | tar -xvf -** or **bzip2 -d pekwm-GIT.tar.bz2** followed by **tar -xvf pekwm-GIT.tar**. This also works for the .tar.gz version using **gzip -dc** or **gzip -d**.

The 'v' options are optional, they show you the filenames as they're being extracted. at this point, you should have a pekwm-GIT directory. Use **cd pekwm-GIT** to get there.

* * *

#### 3.2. Configuration Options

The first thing to do is to run the configure script. This configures compile options for pekwm. Here are some of the more used options and what their default values are.

**Important ./configure options:**

\--enable-shape

Enables the use of the Xshape extension for non-rectangular windows.

By default, Enabled

\--enable-xinerama

Enables Xinerama multi screen support

By default, Enabled

\--enable-xrandr

Enables RandR multi screen support

By default, Enabled

\--enable-xft

Enables Xft font support in pekwm (themes).

By default, Enabled

\--enable-image-xpm

XPM image support using libXpm.

By default, Enabled

\--enable-image-jpeg

JPEG image support using libjpeg.

By default, Enabled

\--enable-image-png

PNG image support using libpng.

By default, Enabled

\--enable-debug

Enables debugging output

By default, Disabled

\--enable-pedantic

Enables pedantic compile flags when using GCC

By default, Disabled

\--prefix=PREFIX

It may be useful to use a custom prefix to install the files.

By default, /usr/local

* * *

#### 3.3. Building and installing

After running ./configure with any options you need, run **make**. This should only take a few minutes. After that, become root (unless you used a prefix in your home directory, such as --prefix=/home/you/pkg) and type **make install**

Adding **exec pekwm** to ~/.xinitrc if you start X running **startx** or ~/.xsession if you use a display manager should usually be enough to get pekwm running.

That's it! pekwm is installed on your computer now. Next you should read the [Getting Started](#usage-gettingstarted) chapter.
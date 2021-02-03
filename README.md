![pekwm logo](pekwm_logo.svg)

README
======

The Pekwm Window Manager is written by Claes Nästén. The code is based
on the aewm++ window manager, but it has evolved enough that it no
longer resembles aewm++ at all. It also has an expanded feature-set,
including window grouping (similar to ion, pwm, or fluxbox), auto
properties, xinerama and keygrabber that supports keychains, and much
more.

For more information visit https://www.pekwm.se/

Quick Installation
==================

If you want to install pekwm from source, follow these steps:

1. Get the sources:

```
git clone https://github.com/pekdon/pekwm.git
```

2. Build with cmake:

```
cd pekwm
mkdir build && cd build
cmake ..
make
```

3. Install as root:

```
make install
```

Getting Started
===============

See the [Manual](doc/README.md) for more information on how to use and
configure pekwm.

For themes, go to the [pekwm themes](https://www.pekwm.se/themes/)
site that is based on data from the
[pekwm-theme-index](https://github.com/pekdon/pekwm-theme-index) where you
can register your own themes for inclusion.

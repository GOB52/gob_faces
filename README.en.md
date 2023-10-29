# gob_faces

[日本語](README.md)

## Overview
This library gets status on [M5Stack Faces](https://docs.m5stack.com/ja/core/face_kit) and [FacesII](https://docs.m5stack.com/ja/module/facesII).  
It does not use Arduino Wire, but uses I2C of M5Unified.  

## Required librarries
* [M5Unified](https://github.com/m5stack/M5Unified)
* [M5GFX](https://github.com/m5stack/M5GFX) (depends from M5Unified)

## How to install
Install in an appropriate way depending on your environment.
* git clone and extract into place  
or
* platformio.ini
```ini
lib_deps = https://github.com/GOB52/gob_faces.git
```

## How to make document

You can make a document of this library by [Doxygen](https://www.doxygen.nl/)  => [Doxyfile](doc/Doxyfile)  
In case you use shell script [doxy.sh](doc/doxy.sh), then output version from library.properties, revision from repository.

## Usage examples

###  Keyboard

#### [keyboad](examples/keyboad)
<img src="https://github.com/GOB52/gob_faces/assets/26270227/de252197-920e-4126-86a5-f950ea706950" width="320">

#### [M5Strek](examples/M5Strek)
<img src="https://github.com/GOB52/gob_faces/assets/26270227/53b6cab1-44a4-4eeb-9ba3-0fe11534e0fa" width="320">

### [Calculator](examples/rpn_calculator)
<img src="https://github.com/GOB52/gob_faces/assets/26270227/cbcfc3e8-1a84-4d35-a341-60691163ff54" width="320">

### [Gamepad](examples/gamepad)
<img src="https://github.com/GOB52/gob_faces/assets/26270227/5abb79b0-e201-4eda-b7f4-23346f799baf" width="320">


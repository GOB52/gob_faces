# gob_faces

[English](README.en.md)

## 概要
[M5Stack Faces](https://docs.m5stack.com/ja/core/face_kit)、[FacesII](https://docs.m5stack.com/ja/module/facesII)の情報を取り扱うライブラリです。  
Arduino Wire を使用せず、M5Unified の I2C を用いています。  

## 依存ライブラリ
* [M5Unified](https://github.com/m5stack/M5Unified)
* [M5GFX](https://github.com/m5stack/M5GFX) (M5Unified から依存)

## 導入
環境によって適切な方法でインストールしてください
* git clone して所定の位置へ展開する  
または
* platformio.ini
```ini
lib_deps = https://github.com/GOB52/gob_faces.git
```

## ドキュメントの作成方法
[Doxygen](http://www.doxygen.jp/) によりライブラリドキュメントを生成できます。 [Doxygen設定ファイル](doc/Doxyfile)  
出力する際に [doxy.sh](doc/doxy.sh) を使用すると、library.properties からVersion、repository から rev を取得し、Doxygen 出力できます。


## 使い方の例

###  Keyboard

#### [keyboard](examples/keyboard)
<img src="https://github.com/GOB52/gob_faces/assets/26270227/de252197-920e-4126-86a5-f950ea706950" width="320">

#### [M5Strek](examples/M5Strek)
<img src="https://github.com/GOB52/gob_faces/assets/26270227/53b6cab1-44a4-4eeb-9ba3-0fe11534e0fa" width="320">

### [Calculator](examples/rpn_calculator)
<img src="https://github.com/GOB52/gob_faces/assets/26270227/cbcfc3e8-1a84-4d35-a341-60691163ff54" width="320">

### [Gamepad](examples/gamepad)
<img src="https://github.com/GOB52/gob_faces/assets/26270227/5abb79b0-e201-4eda-b7f4-23346f799baf" width="320">



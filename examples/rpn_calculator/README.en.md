# RPN Calculator

[日本語](README.md)


## What is the RPN Calculator?
It is a calculator based on [Reverse Polish notation](https://en.wikipedia.org/wiki/Reverse_Polish_notation).

## Operation

|Button|Description|
---|---
|Numbers|Input number|
|=|Confirm entry|
|AC|Initialize|
|AC 長押し| Clear entry|
|.|add a decimal point|
|+/-|Reverses the sign of entry|
|+|Addition|
|-|Subtraction|
|x|Multipication|
|÷| Division|

## Example of input

```
[1][2][3][=][4][5][.][6][+/-][=][7][8][9][-][x]
```
The calculation is interpreted as **123 x (-45.6 - 789)**, yielding **-102655.8**.

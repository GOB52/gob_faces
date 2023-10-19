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
|Long press on AC| Clear entry|
|.|Add a decimal point|
|+/-|Reverses the sign of entry|
|+|Addition|
|-|Subtraction|
|x|Multipication|
|÷| Division|

## Example of input

```
[1][2][3][=][3][4][.][5][+/-][=][7][8][9][-][x]
```
The calculation is interpreted as **123 x (-34.5 - 789)**, output **-92803.5**.


## Notice
This is an example of how Faces works and does not take into account calculation errors.

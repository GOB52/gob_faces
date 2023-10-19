
/*
  RPN(Reverse Polish notation) Calculator example with gob_faces.

  See also https://en.wikipedia.org/wiki/Reverse_Polish_notation

  A calculator face is required.

  Buttons
    [0] ~ [9]          : Numbers
    [AC]               : All clear
    [Long press on AC] : Clear entry
    [=]                : Enter (store value)
    [+][-][x][÷]       : Arithmetic operation.
    [+/-]              : Change sign for entry.
    [.]                : Decimal point

    ex)
      Input : [1][2][3][=][4][5][.][6][+/-][=][7][8][9][-][x]
      --> 123 x (-45.6 - 789)
      --> -102655.8
*/
#include <M5Unified.h>
#include <gob_faces.hpp>
#include <deque>
#include <cstdlib>
#include <WString.h>

/*
通常の計算式：「　1*2+3*4=　」
    「　1,2*3,4*+　」 
*/

using namespace goblib::faces;

namespace
{
bool existsFaces{};
Calculator calc;

String entry('0');
std::deque<float> entries;

auto& lcd = M5.Display;
}

// Functions
void clearEntry()
{
    entry = '0';
}

void allClear()
{
    entries.clear();
    clearEntry();
}

void enter()
{
    char* errpos{};
    const char* s = entry.c_str();
    auto val = strtof(s, &errpos);

    //M5_LOGI("[%p] [%p]", s, errpos);
    
    if(errpos == s + strlen(s))
    {
        clearEntry();
        M5_LOGI("push:%f", val);
        entries.push_front(val);
    }
    else
    {
        M5_LOGE("Illegal number string : [%s] err:%s", s, errpos);
    }
}

void sign()
{
    if(entry.charAt(0) == '-') { entry.remove(0, 1); }
    else                       { entry = String('-') + entry; }
}

void point()
{
    if(entry.indexOf('.') < 0) { entry += '.'; }
}

//
template <class Functor> void arithmetic(Functor func)
{
    if(entries.empty()) { return; }

    enter();
    auto res = func(entries[1], entries[0]);
    M5_LOGI("arithmetic %f any %f", entries[1], entries[0]);

    entries.pop_front();
    entries.pop_front();

    entry = String(res);
}

//
void disp()
{

    lcd.setCursor(0,0);
    lcd.printf("%s", entry.c_str());

    // Show the top two on the stack.
    int32_t y = 26 * 2;
    lcd.setCursor(0,y);
    lcd.printf("Stack: %zu", entries.size());
    y += 26;

    auto sz = entries.size();
    if(sz > 2) { sz = 2; }
    for(decltype(sz) i = 0; i < sz; ++i)
    {
        lcd.setCursor(0,y);
        lcd.printf(" %f", entries[i]);
        y += 26;
    }
}

//
void setup()
{
    M5.begin();
    lcd.setFont(&fonts::Font4); // Height 26 pixel 
    
    lcd.clear(TFT_DARKGREY);
    existsFaces = goblib::faces::exists();

    if(!existsFaces)
    {
        lcd.clear(TFT_RED);
        lcd.printf("Not exists faces");
        return;
    }
    M5_LOGI("Faces exists. Is it calculator?");
    lcd.clear(0);
    calc.begin();

    disp();
}

void loop()
{
    if(!existsFaces) { delay(1000); return; }

    calc.update();
    if(!calc.available()) { return; }

    // Something was entered.
    auto in = calc.now();
    M5_LOGI("%02x", in);
    // Function
    if(calc.isFunction())
    {
        switch(in)
        {
        case Calculator::Button::ALL_CLEAR:   allClear();   break;
        case Calculator::Button::CLEAR_ENTRY: clearEntry(); break;
        case Calculator::Button::EQUALS:      enter();      break;
        case Calculator::Button::SIGN:        sign();       break;
        case Calculator::Button::POINT:       point();      break;
        case Calculator::Button::ADD:         arithmetic([](float a, float b) { return a + b; }); break;
        case Calculator::Button::SUB:         arithmetic([](float a, float b) { return a - b; }); break;
        case Calculator::Button::MUL:         arithmetic([](float a, float b) { return a * b; }); break;
        case Calculator::Button::DIV:         arithmetic([](float a, float b) { return a / b; }); break;
        }
        lcd.clear();
    }
    // Number
    else
    {
        if(entry.toInt() == 0 && entry.indexOf('.') < 0)
        {
            if(in != 0) { entry.replace('0', (char)(in + '0')); }
        }
        else
        {
            entry += (char)(in + '0');
        }
    }

    disp();
}


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
      Input : [1][2][3][=][3][4][.][5][+/-][=][7][8][9][-][x]
      --> 123 x (-34.5 - 789)
      --> -92803.5
*/
#include <M5Unified.h>
#include <gob_faces.hpp>
#include <deque>
#include <cstdlib>
#include <WString.h>

using namespace goblib::faces;

namespace
{
constexpr size_t MAX_ENTRY_LENGTH = 10;

bool existsFaces{};
Calculator calc;

String entry('0');
bool arithmeticFlag{},enterFlag{};
std::deque<float> entries;

auto& lcd = M5.Display;

bool entry2float(String& str, float& out)
{
    out = 0;

    char* errpos{};
    const char* s = str.c_str();
    auto val = strtod(s, &errpos);

    //M5_LOGV("[%p] [%p]", s, errpos);
    if(errpos == s + strlen(s))
    {
        out = (float)val;
        return true;
    }

    M5_LOGE("Illegal number string : [%s] err:%s", s, errpos);
    return false;
}

// Functions
void clearEntry()
{
    enterFlag = arithmeticFlag = false;
    entry = '0';
    entry2float(entry, entries[0]);
}

void allClear()
{
    entries.clear();
    entries.push_front(0);
    clearEntry();
}

void enter()
{
    float val{};

    if(entry2float(entry, val))
    {
        M5_LOGI("push:%f", val);
        entries.push_front(val);
        arithmeticFlag = false;
        enterFlag = true;
    }
}

void sign()
{
    if(entry.charAt(0) == '-') { entry.remove(0, 1); }
    else                       { entry = String('-') + entry; }
    entry2float(entry, entries[0]);
}

void point()
{
    if(entry.indexOf('.') < 0) {
        entry += '.';
        entry2float(entry, entries[0]);
    }
}

//
template <class Functor> void arithmetic(Functor func)
{
    if(entries.size() < 2) { return; }

    float res = func(entries[1], entries[0]);
    M5_LOGI("arithmetic %f and %f", entries[1], entries[0]);

    entries.pop_front();
    entries.pop_front();

    entries.push_front(res);
    char buf[MAX_ENTRY_LENGTH * 2];
    snprintf(buf, sizeof(buf), "%f", res);
    buf[sizeof(buf)-1] = '\0';
    entry = buf;
    M5_LOGI("result:%f [%s]", res, entry.c_str());
    
    arithmeticFlag = true;
    enterFlag = false;
}

//
void disp()
{

    lcd.setTextSize(2);
    lcd.setCursor(0,lcd.height()/2);
    lcd.printf("%s", entry.c_str());

    // stack information
    lcd.setTextSize(0.5);
    int32_t y = lcd.height()/2 - 13*2;
    lcd.setCursor(0,y);
    lcd.printf("Stack: %zu", entries.size());
    y -= 13;

    uint32_t sz = entries.size();
    if(sz > 4) { sz = 4; }
    for(uint32_t i = 0; i < sz; ++i)
    {
        lcd.setCursor(0,y);
        lcd.printf("[%u]: %f", i, entries[i]);
        y -= 13;
    }

    #if 0
    lcd.setCursor(0, lcd.height() - 13*2);
    lcd.printf("aflag:%d eflag:%d", arithmeticFlag, enterFlag);
    #endif
}

}//namespace

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

    allClear();
    disp();
}

void loop()
{
    if(!existsFaces) { delay(1000); return; }

    calc.update();
    if(!calc.available()) { return; }

    // Something was entered.
    auto in = calc.now();
    //M5_LOGI("%02x", in);

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
    }
    // Number
    else
    {
        if(arithmeticFlag) { enter(); clearEntry(); }
        if(enterFlag) { clearEntry(); }

        if(entry.length() < MAX_ENTRY_LENGTH)
        {
            // If entry is zero and no decimal point?
            if(entry.toInt() == 0 && entry.indexOf('.') < 0)
            {
                // input 1-9?
                if(in != 0)
                {
                    entry.replace('0', (char)(in + '0'));
                }
            }
            else
            {
            
                entry += (char)(in + '0');
            }
            entry2float(entry, entries[0]);
        }
    }

    lcd.clear();
    disp();
}

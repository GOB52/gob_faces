/*
  Example using gob_face in the RPN (Reverse Polish Notation) calculator.

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

#if defined(ARDUINO)
#include <WString.h>
using string_t = String;
#else
#include <string>
using string_t = std::string;
#endif

using namespace goblib::faces;

namespace
{
constexpr size_t MAX_ENTRY_LENGTH = 10;

bool existsFaces{};
Calculator calc;

string_t entry("0");
bool arithmeticFlag{},enterFlag{};
std::deque<float> entries;

auto& lcd = M5.Display;

inline void eraseString(string_t& str, const size_t pos, const size_t npos)
{
#if defined(ARDUINO)
    str.remove(pos, npos);
#else
    str.erase(pos, npos);
#endif
}

inline int indexOfString(const string_t& str, int ch)
{
#if defined(ARDUINO)
    return str.indexOf(ch);
#else
    auto res = str.find(ch);
    return res != std::string::npos ? res : -1;
#endif
}

inline int32_t string2int(const string_t& str)
{
#if defined(ARDUINO)
    return str.toInt();
#else
    return stoi(str);
#endif
}


bool entry2float(string_t& str, float& out)
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
    if(entry[0] == '-') { eraseString(entry, 0, 1);      }
    else                { entry = string_t("-") + entry; }
    entry2float(entry, entries[0]);
}

void point()
{
    M5_LOGI("[%s] %d", entry.c_str(), indexOfString(entry, '.'));
    if(indexOfString(entry, '.') < 0)
    {
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
    if(!existsFaces) { return; }

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
            if(string2int(entry) == 0 && indexOfString(entry, '.') < 0)
            {
                // input 1-9?
                if(in != 0)
                {
                    entry = (char)(in + '0');
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

#if defined ( SDL_h_ )
__attribute__((weak)) int user_func(bool* running)
{
  setup();
  do
  {
    loop();
  } while (*running);
  return 0;
}

int main(int, char**)
{
  // The second argument is effective for step execution with breakpoints.
  // You can specify the time in milliseconds to perform slow execution that ensures screen updates.
  return lgfx::Panel_sdl::main(user_func, 128);
}
#endif

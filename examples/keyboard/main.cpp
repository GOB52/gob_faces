/*
  Keyboad Face example.
*/
#include <M5Unified.h>
#include <gob_faces.hpp>
#include <vector>

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
Keyboard kbd;
auto& lcd = M5.Display;

std::vector<string_t> str = { string_t() };
string_t* curStr{&str.back()};

int32_t cx{}, cy{}, sy{}, fwid{}, fhgt{};
bool dirty{true};

int32_t calcTop()
{
    return (str.size() * fhgt > lcd.height()) ? str.size() - lcd.height() / fhgt : 0;
}

void newline()
{
    str.push_back(string_t());
    curStr = &str.back();

    cx = 0;
    ++cy;
    sy = calcTop();
}
//
}

void setup()
{
    M5.begin();
    lcd.setFont(&fonts::Font0); // width:6 height:8 (fixed)
    lcd.setTextSize(2);
    fwid = lcd.fontWidth();
    fhgt = lcd.fontHeight();
    
    if(!goblib::faces::exists())
    {
        lcd.clear(TFT_RED);
        lcd.printf("Not exists faces");
        return;
    }
    M5_LOGI("Faces exists. Is it keyboard?");
    lcd.clear(0);

    kbd.begin();
}

void loop()
{
    kbd.update();
    if(kbd.available())
    {
        dirty = true;
        auto ch = kbd.raw();

        switch(ch)
        {
        case '\n':                   newline(); break;
        default:
            if(ch < 0x7F) { (*curStr) += (char)ch; ++cx; }
            break;
        }
    }

    //
    if(dirty)
    {
        dirty = false;
        lcd.clear(TFT_BLACK);
        int32_t y{};
        for(auto it = str.begin() + sy; it < str.end(); ++it)
        {
            lcd.drawString((*it).c_str(), 0, y);
            y += fhgt;
        }
    }
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




/*
  Gamepad example using gob_faces.

  [NOTICE] A controller that SDL recognizes as a game controller is required if native.
  e.g.) Xbox, Xbox360, PS3, PS4, PS5, Switch Pro controller...

  See alseo https://github.com/libsdl-org/SDL/blob/SDL2/include/SDL_gamecontroller.h
*/
#include <M5Unified.h>
#include <gob_faces.hpp>

using namespace goblib::faces;

namespace
{
Gamepad pad;
auto& lcd = M5.Display;
//
}

void disp()
{
    int32_t x{},y{},w{},h{};

    x = lcd.width()/16;
    y = lcd.height()/16;
    w = lcd.width()/16;
    h = lcd.width()/16;

    // Cross buttons
    lcd.fillRect(x, y + h * 2, w * 2, h * 2,
                 pad.wasHold(Gamepad::Button::Left) ? TFT_BLUE : 
                 pad.isPressed(Gamepad::Button::Left)  ? TFT_YELLOW : TFT_BLACK);
    lcd.fillRect(x + w * 4, y + h * 2, w * 2, h * 2,
                 pad.wasHold(Gamepad::Button::Right) ? TFT_BLUE :
                 pad.isPressed(Gamepad::Button::Right) ? TFT_YELLOW : TFT_BLACK);
    lcd.fillRect(x + w * 2, y, w * 2, h * 2,
                 pad.isHolding(Gamepad::Button::Up) ? TFT_BLUE : 
                 pad.isPressed(Gamepad::Button::Up)    ? TFT_YELLOW : TFT_BLACK);
    lcd.fillRect(x + w * 2, y + h * 4, w * 2, h * 2,
                 pad.isHolding(Gamepad::Button::Down) ? TFT_BLUE : 
                 pad.isPressed(Gamepad::Button::Down)  ? TFT_YELLOW : TFT_BLACK);
    lcd.fillRect(x + w * 2, y + h * 2, w * 2, h * 2, TFT_BLACK);
    
    // A,B buttons
    w = lcd.width()/16;
    h = lcd.width()/16;
    lcd.fillCircle(x + w * 10, y + h * 4, w, pad.wasRepeated(Gamepad::Button::B) ? TFT_YELLOW : TFT_BLACK);
    lcd.fillCircle(x + w * 13, y + h * 3, w, pad.wasRepeated(Gamepad::Button::A) ? TFT_YELLOW : TFT_BLACK);

    // Select,Start buttons
    lcd.fillCircle(x + w * 5,  y + h * 9, w, pad.wasPressed(Gamepad::Button::Select) ? TFT_YELLOW : TFT_BLACK);
    lcd.fillCircle(x + w * 9,  y + h * 9, w, pad.wasPressed(Gamepad::Button::Start)  ? TFT_YELLOW : TFT_BLACK);
}

void setup()
{
    M5.begin();
    lcd.setFont(&fonts::Font2);

    if(!goblib::faces::exists())
    {
        lcd.clear(TFT_RED);
        lcd.printf("Not exists faces");
        return;
    }
    M5_LOGI("Faces exists. Is it gamepad?");
    lcd.clear(TFT_DARKGRAY);

    pad.begin();
    pad.setRepeatTH(Gamepad::Button::B, 100);
    pad.setRepeatTH(Gamepad::Button::A, 1000);

    pad.setHoldTH(Gamepad::Button::Left, 1000);
    pad.setHoldTH(Gamepad::Button::Right, 1500);
    pad.setHoldTH(Gamepad::Button::Up, 2000);
    pad.setHoldTH(Gamepad::Button::Down, 2500);

    disp();
}

void loop()
{
    pad.update();
    if(pad.available())
    {
        M5_LOGI("%ld: [%02x] [%02x] [%02x] [%02x] [%02x] [%02x]",
                M5.millis(), pad.now(), pad.edge(), pad.releaseEdge(), pad.hold(), pad.holdEdge(), pad.repeat());
        disp();
    }
    M5.delay(1000/30);
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
  // You can specify the time in milliseconds to perform slow execution that ensures screen update
    return lgfx::Panel_sdl::main(user_func, 128);
}
#endif






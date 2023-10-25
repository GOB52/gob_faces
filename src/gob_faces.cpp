/*!
  @file gob_faces.cpp
  @brief Get M5Stack Faces status (Keyboard / Calculator / Gamepad) without Arduino Wire.
*/
#include "gob_faces.hpp"
#include <M5Unified.h>
#if defined(SDL_h_)
#include <locale>
#include <string>
using std::string;
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#elif defined(__APPLE__)
#else
#endif

namespace
{
// Settings
constexpr uint8_t PIN_NO = 0x05;
constexpr uint32_t I2C_FREQ = 100000;
constexpr uint8_t I2C_ADDRESS = 0x08;
constexpr uint8_t I2C_REG = 0x00; // Faces firmware only returns data in response to a request, registers are not considered.

// enum to underlying_type
template <typename T> constexpr typename std::underlying_type<T>::type to_underlying(T e) noexcept
{
    return static_cast<typename std::underlying_type<T>::type>(e);
}

#if defined(SDL_h_)
// Gets the last character of UTF-8 string.
string getLastUtf8Charcter(string& s)
{
    if(s.empty()) { return string(); }
    auto cp = s.data() + s.size();
    while(--cp >= s.data() && (*cp & 0b10000000) && !(*cp & 0b01000000)) ;
    return cp >= s.data() ? string(cp) : string();
}
#endif
//
}

namespace goblib { namespace faces {

bool exists()
{
#if defined(SDL_h_)
    int numKeys{};
    auto keyState = SDL_GetKeyboardState(&numKeys);
    M5_LOGI("SDL numKey:%d joys:%d", numKeys, SDL_NumJoysticks());
    return keyState && numKeys > 0 ||  SDL_NumJoysticks() > 0;
#else
    return M5.In_I2C.writeRegister8(I2C_ADDRESS, I2C_REG, 0/*dummy data*/, I2C_FREQ); // Exists if write successful.
#endif
}

// ----------------------------------------------------------------------------
// class Face
bool Face::begin()
{
    if(_began) { return true; }

    if(exists())
    {
#if !defined(SDL_h_)
        pinMode(PIN_NO, INPUT_PULLUP);
#endif
        _began = true;
        return true;
    }
    M5_LOGE("Not exists faces.");
    return false;
}

void Face::update()
{
#if !defined(SDL_h_)
    _available = (digitalRead(PIN_NO) == LOW);
    if(_available)
    {
        _raw = 0;
        uint8_t val = M5.In_I2C.readRegister8(I2C_ADDRESS, I2C_REG, I2C_FREQ);
        if(val) { _raw = val; }
    }
#endif
}

// ----------------------------------------------------------------------------
// class Keyboard
bool Keyboard::begin()
{
#if !defined(SDL_h_)
    return Face::begin();
#else
    if(!_began)
    {
        SDL_AddEventWatch(sdl_event_filter_keyboard, this);
        _began = true;
    }
    return _began;
#endif
}

void Keyboard::update()
{
#if !defined(SDL_h_)
    Face::update();
    if(!available()) { return; }
    
    /* CR/LF is returned when Enter is clicked.
       CR/LF is treated as LF */
    if(raw() == '\r')
    {
        _raw = '\n';
        (void)M5.In_I2C.readRegister8(I2C_ADDRESS, I2C_REG, I2C_FREQ); // Read and dismiss the CR.
    }
#else
    std::lock_guard<std::mutex> lock(_mtx);
    _raw = 0;
    _available = (_input && _ch);
    _input = false;
    if(_available) { _raw = _ch; };
#endif
}

#if defined(SDL_h_)
int Keyboard::sdl_event_filter_keyboard(void* arg, SDL_Event *event)
{
    assert(arg);
    assert(event);
    Keyboard* face = (Keyboard*)arg;
    face->_sdl_event_filter_keyboard(event);
    return 0; // This function's return value is ignored
}

void Keyboard::_sdl_event_filter_keyboard( SDL_Event *event)
{
    std::lock_guard<std::mutex> lock(_mtx);
    switch(event->type)
    {
    case SDL_KEYUP:
        // Triggers when released to match Faces behavior.

        if(event->key.keysym.sym & SDLK_SCANCODE_MASK) { _ch = 0; }
        // Pick up keys
        switch(event->key.keysym.sym)
        {
        case SDLK_RETURN:    _ch = '\n'; break; // Enter
        case SDLK_ESCAPE:    _ch = 0x0B; break; // ESC (Faces default firmware never send it!)
        case SDLK_BACKSPACE: _ch = 0x08; break; // Backspace
        case SDLK_DELETE:    _ch = 0x7F; break; // Delete
        case SDLK_INSERT:    _ch = 0xB8; break; // Insert
        case SDLK_TAB:       _ch = 0x08; break; // Tab
        case SDLK_HOME:      _ch = 0xBB; break; // Home
        case SDLK_END:       _ch = 0xBC; break; // End
        case SDLK_PAGEUP:    _ch = 0xBD; break; // Page up
        case SDLK_PAGEDOWN:  _ch = 0xBE; break; // Page down
        case SDLK_UP:        _ch = 0xB7; break; // Up 
        case SDLK_LEFT:      _ch = 0xBF; break; // Left
        case SDLK_DOWN:      _ch = 0xC0; break; // Down
        case SDLK_RIGHT:     _ch = 0xC1; break; // Right
        // Reject some scancode
        // When you press the alphanumeric/kana key, SLDK_SPACE is returned. (Japanese S-SIS keyboard on Mac)
        case SDLK_SPACE:
            if(event->key.keysym.scancode >= SDL_SCANCODE_LANG1 && event->key.keysym.scancode <= SDL_SCANCODE_LANG9)
            {
                _ch = 0;
            }
            break;
        }
        
        _input = (_ch != 0);
        M5_LOGV("UP:SC:%d KC:%d Mod:%x [%d]",
                event->key.keysym.scancode, event->key.keysym.sym,
                event->key.keysym.mod, _ch);
        break;
    case SDL_TEXTINPUT:
        {
            // Get last character of text input.(UTF-8)
            _ch = 0;
            auto s = string(event->text.text);
            auto last = getLastUtf8Charcter(s);
            M5_LOGV("last:[%s]", last.c_str());
            if(last.size() == 1) { _ch = last[0]; }
        }
        break;
    }
}
#endif

// ----------------------------------------------------------------------------
// class Calculator
bool Calculator::begin()
{
#if !defined(SDL_h_)
    return Face::begin();
#else
    if(!_began)
    {
        SDL_AddEventWatch(sdl_event_filter_calculator, this);
        _began = true;
    }
    return _began;
#endif
}

void Calculator::update()
{
#if !defined(SDL_h_)
    Face::update();
    if(!available()) { return; }
#else
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _raw = 0;
        _available = (_input && _ch);
        _input = false;
        if(!available()) { return; }
        _raw = _ch;
    }

#endif    
    // data conversion
    if(_raw)
    {
        switch(_raw)
        {
        case 'A':  _now = ALL_CLEAR; break;
        case 'M':  _now = MEMORY_INPUT; break;
        case 0x08: _now = CLEAR_ENTRY; break;
        case '%':  _now = PERCENT; break;
        case '+':  _now = ADD; break;
        case '-':  _now = SUB; break;
        case '*':  _now = MUL; break;
        case '/':  _now = DIV; break;
        case '`':  _now = SIGN; break;
        case '.':  _now = POINT; break;
        case '=':  _now = EQUALS; break;
        case '\r': _now = MEMORY_RECALL; break;
        default:
            if(_raw >= '0' && _raw <= '9')
            {
                _now = _raw - '0';
            }
            else
            {
                _available = false;
            }
            break;
        }
    }
    M5_LOGV("a:%d [%02x] / <%02x>", _available, _raw, _now);
}

#if defined(SDL_h_)
int Calculator::sdl_event_filter_calculator(void* arg, SDL_Event *event)
{
    assert(arg);
    assert(event);
    Calculator* face = (Calculator*)arg;
    face->_sdl_event_filter_calculator(event);
    return 0; // This function's return value is ignored
}

// Calculator face supports numeric keypad.
void Calculator::_sdl_event_filter_calculator(SDL_Event *event)
{
    std::lock_guard<std::mutex> lock(_mtx);
    switch(event->type)
    {
    case SDL_KEYUP:
        // Triggers when released to match Faces behavior.

        if(event->key.keysym.sym & SDLK_SCANCODE_MASK) { _ch = 0; }
        // Pick up numeric keyboard key.
        switch(event->key.keysym.sym)
        {
        case SDLK_KP_ENTER: // Falls through
        case SDLK_KP_EQUALS: _ch = '='; break;

#if defined(__APPLE__)
        case SDLK_NUMLOCKCLEAR: // Falls through (Clear on Mac)
#endif
        case SDLK_KP_CLEAR:    _ch = 'A'; break;

        case SDLK_KP_DIVIDE:   _ch = '/'; break;
        case SDLK_KP_MULTIPLY: _ch = '*'; break;
        case SDLK_KP_MINUS:    _ch = '-'; break;
        case SDLK_KP_PLUS:     _ch = '+'; break;

        case SDLK_KP_1: // Falls through
        case SDLK_KP_2: // Falls through
        case SDLK_KP_3: // Falls through
        case SDLK_KP_4: // Falls through
        case SDLK_KP_5: // Falls through
        case SDLK_KP_6: // Falls through
        case SDLK_KP_7: // Falls through
        case SDLK_KP_8: // Falls through
        case SDLK_KP_9:
            _ch = '1' + event->key.keysym.sym - SDLK_KP_1; break;
        case SDLK_KP_0:        _ch = '0'; break;
        case SDLK_KP_PERIOD:   _ch = '.'; break;
        // Substitute some keys.
        case SDLK_c:        _ch = 0x08; break; // c key instead of Clear entry key.
        case SDLK_m:        _ch = 'M';  break; // m key instead of Memory input.
        case SDLK_r:        _ch = '\r'; break; // r key instead of Memory recall.
        case SDLK_s:        _ch = '`';  break;  // s key instead of Change sign.
        default: break;
        }
        
        _input = (_ch != 0);
        M5_LOGV("UP:SC:%d KC:%d Mod:%x [%c]",
                event->key.keysym.scancode, event->key.keysym.sym,
                event->key.keysym.mod, _ch);
        break;
    case SDL_TEXTINPUT:
        {
            // Get last character of text input.(UTF-8)
            _ch = 0;
            auto s = string(event->text.text);
            auto last = getLastUtf8Charcter(s);
            M5_LOGV("last:[%s]", last.c_str());
            if(last.size() == 1) { _ch = last[0]; }
        }
    }
}
#endif

// ----------------------------------------------------------------------------
// class Gamepad
bool Gamepad::begin()
{
#if !defined(SDL_h_)
    return Face::begin();
#else
    if(!_began)
    {
        auto num = SDL_NumJoysticks();
        if(num > 0)
        {
            M5_LOGV("Deteced joysticks:%d", num);

            int idx = 0;
            while(idx < num)
            {
                M5_LOGV("  [%s]", SDL_JoystickNameForIndex(idx));
                if(SDL_IsGameController(idx)) { break; }
                ++idx;
            }
            if(idx >= num) { M5_LOGE("Not detect game controller"); return false; }

            _joyIndex = idx;
            M5_LOGV("Using joystick index: %d", _joyIndex);

            auto ctrl = SDL_GameControllerOpen(_joyIndex);
            if(!ctrl)
            {
                M5_LOGE("Failed to open game controller.");
                return false;
            }
            SDL_AddEventWatch(sdl_event_filter_gamepad, this);
            _began = true;
        }
        else
        {
            M5_LOGE("No joysticks");
        }
    }
    return _began;
#endif    
}

void Gamepad::update()
{
    _last = _now;
    auto lastHold = _hold;
    Face::update();

#if defined(SDL_h_)
    if(_joyIndex >= 0)
    {
        SDL_GameController* ctrl = SDL_GameControllerFromPlayerIndex(_joyIndex);
        if(ctrl)
        {
            _available = true;
            _raw = ~0;;
            int btn{0};
            for(auto& b : buttonMap)
            {
                if(b && SDL_GameControllerGetButton(ctrl, (SDL_GameControllerButton)btn)) { _raw &= ~b;  }
                ++btn;
            }
        }
    }
#endif

    if(available())
    {
        _now = _raw ^ ~0; // Invert it so that only the one being pushed is in a state where the bit is standing.
        M5_LOGV("%02x / %02x", _raw, _now);
    }
    _available = true; // Always!

    // Detect press/release edge
    _edge = (_last ^ _now) & _now;
    _releaseEdge = (_last ^ _now) & ~_now;

    Bits k = 1;
    auto ms = M5.millis();
    for(size_t i = 0; i < NUMBER_OF_BUTTONS; ++i, k <<= 1)
    {
        // Press
        if(_edge & k)
        {
            _repeatStart[i] = _holdStart[i] = ms;
            _repeat |= k;
            continue;
        }
        // Repeat?
        if((_now & k) && ms - _repeatStart[i] >= _repeatTH[i])
        {
            _repeatStart[i] = ms;
            _repeat |= k;
        }
        else
        {
            _repeat &= ~k;
        }
        // Hold?
        if((_now & k) && ms - _holdStart[i] >= _holdTH[i])
        {
            _hold |= k;
        }
        else
        {
            _hold &= ~k;
        }
    }
    _holdEdge = (lastHold ^ _hold) & _hold;
}

#if defined(SDL_h_)
int Gamepad::sdl_event_filter_gamepad(void* arg, SDL_Event *event)
{
    assert(arg);
    assert(event);
    Gamepad* face = (Gamepad*)arg;
    face->_sdl_event_filter_gamepad(event);
    return 0; // This function's return value is ignored
}

// Gamepad supports joystick and keyboard.
void Gamepad::_sdl_event_filter_gamepad(SDL_Event *event)
{
    switch(event->type)
    {
    case SDL_QUIT:
        if(_joyIndex >= 0) { SDL_GameControllerClose(SDL_GameControllerFromPlayerIndex(_joyIndex)); }
        break;
    }
}

std::array<Gamepad::Button,SDL_CONTROLLER_BUTTON_MAX>  Gamepad::buttonMap =
{
    Gamepad::Button::A, // 0:A
    Gamepad::Button::B, // 1:B
    (Gamepad::Button)0, //  2:X
    (Gamepad::Button)0, //  3:Y
    Gamepad::Button::Select, //  4:Back
    (Gamepad::Button)0, //  5:Guide
    Gamepad::Button::Start, //  6:Start
    (Gamepad::Button)0, //  7:Left stick
    (Gamepad::Button)0, //  8:Right stick
    (Gamepad::Button)0, //  9:Left shoulder
    (Gamepad::Button)0, // 10:Right shoulder
    Gamepad::Button::Up, // 11:Up
    Gamepad::Button::Down, // 12:Down
    Gamepad::Button::Left, // 13:Left
    Gamepad::Button::Right, // 14:Right
    (Gamepad::Button)0, // 15:Misc1
    (Gamepad::Button)0, // 16:Paddle1
    (Gamepad::Button)0, // 17:Paddle2
    (Gamepad::Button)0, // 18:Paddle3
    (Gamepad::Button)0, // 19:Paddle4
    (Gamepad::Button)0, // 20:Touchpad
};
#endif

unsigned long Gamepad::getHoldTH(const Button btn) const
{
    auto idx = __builtin_ffs(to_underlying(btn));
    if(idx <= 0 || idx > _repeatTH.size())
    {
       M5_LOGE("btn %x is unexpected.\n", btn);
       return 0;
    }
    return _holdTH[--idx];
}

void Gamepad::setHoldTH(const Button btn, unsigned long ms)
{
    auto idx = __builtin_ffs(to_underlying(btn));
    if(idx <= 0 || idx > _repeatTH.size())
    {
       M5_LOGE("btn %x is unexpected.\n", btn);
       return;
    }
    --idx;
    _holdTH[idx] = ms;
}

unsigned long Gamepad::getRepeatTH(const Button btn) const
{
    auto idx = __builtin_ffs(to_underlying(btn));
    if(idx <= 0 || idx > _repeatTH.size())
    {
       M5_LOGE("btn %x is unexpected.\n", btn);
       return 0;
    }
    return _repeatTH[--idx];
}

void Gamepad::setRepeatTH(const Button btn, unsigned long ms)
{
    auto idx = __builtin_ffs(to_underlying(btn));
    if(idx <= 0 || idx > _repeatTH.size())
    {
       M5_LOGE("btn %x is unexpected.\n", btn);
       return;
    }
    --idx;
    _repeatTH[idx] = ms;
}

//
}}

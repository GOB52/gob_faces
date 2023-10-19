/*!
  @file gob_faces.cpp
  @brief Get M5Stack Faces status (Keyboard / Calculator / Gamepad) without Arduino Wire.
*/
#include "gob_faces.hpp"
#include <M5Unified.h>

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

}

namespace goblib { namespace faces {

bool exists()
{
    bool e = M5.In_I2C.writeRegister8(I2C_ADDRESS, I2C_REG, 0/*dummy data*/, I2C_FREQ); // Exists if write successful.

    // Read and dismiss stored data
    while(M5.In_I2C.readRegister8(I2C_ADDRESS, I2C_REG, I2C_FREQ)) { M5_LOGE("dismiss"); }

    return e;
}

// ----------------------------------------------------------------------------
// class Face
bool Face::begin()
{
    if(exists())
    {
        //        // Read and dismiss stored data
        //        while(M5.In_I2C.readRegister8(I2C_ADDRESS, I2C_REG, I2C_FREQ)) ;

        pinMode(PIN_NO, INPUT_PULLUP);
        return true;
    }

    M5_LOGE("Not exists faces.");
    return false;
}

void Face::update()
{
    _available = (digitalRead(PIN_NO) == LOW);

    if(_available)
    {
        uint8_t val = M5.In_I2C.readRegister8(I2C_ADDRESS, I2C_REG, I2C_FREQ);
        if(val) { _raw = val; }
    }
}

// ----------------------------------------------------------------------------
// class Keyboard
void Keyboard::update()
{
    Face::update();
    if(!available()) { return; }
    
    /* CR/LF is returned when Enter is clicked.
       CR/LF is treated as LF */
    if(_raw == '\n') { (void)M5.In_I2C.readRegister8(I2C_ADDRESS, I2C_REG, I2C_FREQ); } // Read and dismiss the CR.
}

// ----------------------------------------------------------------------------
// class Calculator
void Calculator::update()
{
    Face::update();
    if(!available()) { return; }

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
        case 0x0d: _now = MEMORY_RECALL; break;
        default:
            assert(_raw >= '0' && _raw <= '9' && "illegal raw value");
            _now = _raw - '0';
            break;
        }
    }
    M5_LOGV("%02x / %02x", _raw, _now);
}

// ----------------------------------------------------------------------------
// class Gamepad
void Gamepad::update()
{
    _last = _now;

    Face::update();
    if(available())
    {
        _now = _raw ^ 0xff; // Invert it so that only the one being pushed is in a state where the bit is standing.
        M5_LOGV("%02x / %02x", _raw, _now);
    }

    // Detect press/release edge
    _edge = (_last ^ _now) & _now;
    _edge_r = (_last ^ _now) & ~_now;
    
    // Software repeat
    uint8_t k = 1;
    for(size_t i = 0; i < _repeatCount.size(); ++i, k <<= 1)
    {
        if((_enableRepeat & k) == 0) { continue; }
        if(_edge & k)
        {
            _repeatCount[i] = _repeatCycle[i];
            _repeat |= k;
            continue;
        }
        if((_now & k) && _repeatCount[i]-- == 0)
        {
            _repeatCount[i] = _repeatCycle[i];
            _repeat |= k;
        }
        else
        {
            _repeat &= ~k;
        }
    }
}

bool Gamepad::isEnableRepeat(const Button btn) const
{
    return to_underlying(btn) & _enableRepeat;
}

void Gamepad::enableRepeat(const Button btn, const bool enable)
{
    if(enable)
    {
        _enableRepeat |= to_underlying(btn);
    }
    else
    {
        _enableRepeat &= ~to_underlying(btn);
    }
}

std::uint8_t Gamepad::getRepeatCycle(const Button btn) const
{
    auto idx = __builtin_ffs(to_underlying(btn));
    if(idx > 0 && idx < _repeatCount.size())
    {
        return _repeatCycle[idx - 1];
    }
    M5_LOGE("btn %x is unexpected.", btn);
    return 0;
}

void Gamepad::setRepeatCycle(const Button btn, const uint8_t cycle)
{
    auto mask = ~to_underlying(btn);
    _now &= mask;
    _edge &= mask;
    _edge_r &= mask;
    _repeat &= mask;

    auto idx = __builtin_ffs(to_underlying(btn));
    if(idx > 0 && idx < _repeatCount.size())
    {
        _repeatCycle[idx - 1] = cycle;
    }
    else
    {
       M5_LOGE("btn %x is unexpected.\n", btn);
    }
}

}}

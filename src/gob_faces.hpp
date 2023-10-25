/*!
  @file gob_faces.hpp
  @brief Get M5Stack Faces status (Keyboard / Calculator / Gamepad) without Arduino Wire.

  @mainpage gob_faces
  A library to get Faces status using only M5Unified functions without Arduino Wire.  
  Required M5Unified (Using M5Unified I2C instead of the Wire)  
  C++11 or later.
  
  @author GOB https://twitter.com/gob_52_gob

  @copyright 2023 GOB
  @copyright Licensed under the MIT license. See LICENSE file in the project root for full license information.
*/
#ifndef GOBLIB_FACES_HPP
#define GOBLIB_FACES_HPP

#include <cstdint>
#include <type_traits>
#include <array>
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <mutex>
#endif

/*!
  @namespace goblib
  @brief Top level namespace of mine.
*/
namespace goblib
{

/*!
  @@namespace faces
  @brief For faces.
*/

namespace faces
{
/*!
  @brief Does any faces exists?
  @warning The type of faces cannot be identified.
  @retval true Exists
  @retval false Not exists.
*/
bool exists();


/*!
  @class Face
  @brief Base class of 3 faces.
  @warning Do not use this class directly, Please use a derived class.
 */
class Face
{
  public:
    virtual bool begin();
    virtual void update();

    inline bool available() const { return _available; } //!< @brief Does the input data exist?
    inline uint8_t raw() const { return _raw; } //!< @brief Gets the raw data.
    
  protected:
    Face(){}
    virtual ~Face(){}

    uint8_t _raw{};
    bool _began{}, _available{};

#if __has_include(<SDL2/SDL.h>)
    bool _input{};
    char _ch{};
    std::mutex _mtx{};
#endif
};


/*!
  @class Keyboard
  @brief For Keyboard
  @note When the ENTER key is pressed, the firmware returns 0x0d(CR) and 0x0a(LF), but they are treated as 0x0a(LF).
  @warning ESC cannot be obtained with the default firmware.
 */
class Keyboard : public Face
{
  public:
    /*! @enum Special Key Codes */
    enum Code : uint8_t
    {
        Delete = 0x7F,   //!< @brief Delete [sym - BS]
        Insert = 0xB8,   //!< @berif Insert [Fn - L]
        Home = 0xBB,     //!< @brief Home [Fn - X]
        End = 0xBC,      //!< @brief End [Fn - C]
        Pageup = 0xBD,   //!< @brief [Fn - V]
        Pagedown = 0xBE, //!< @brief [Fn - B]
        Up = 0xB7,       //!< @brief Up [Fn - K]
        Down = 0xC0,     //!< @brief Down [Fn - M]
        Left = 0xBF,     //!< @brief [Fn - N]
        Right = 0xC1,    //!< @brief [Fn - $]
    };

    Keyboard() : Face() {}
    virtual ~Keyboard() {}

    virtual bool begin() override;
    virtual void update() override;

  protected:
#if __has_include(<SDL2/SDL.h>)
    static int sdl_event_filter_keyboard(void* arg, SDL_Event *event);
    virtual void _sdl_event_filter_keyboard(SDL_Event *event);
#endif
};


/*!
  @class Calculator
  @brief For Calculator.
 */
class Calculator : public Face
{
  public:
    /*!
      @enum FunctionButton
      @note The function names of some buttons are for convenience only, and users do not have to behave in this way.
     */
    enum Button : uint8_t
    {
        FUNCTION_BIT = 0x80,
        ALL_CLEAR = FUNCTION_BIT, //!< [AC] All clear
        CLEAR_ENTRY,              //!< [Long press on AC] Clear entry
        MEMORY_INPUT,             //!< [M] Memory input
        PERCENT,                  //!< [%] Percent
        //
        ADD,                      //!< [+] Addition
        SUB,                      //!< [-] Subtraction
        MUL,                      //!< [x] Multipication
        DIV,                      //!< [÷] Division
        //
        SIGN,                     //!< [+/-] Change sign
        POINT,                    //!< [.] Decimal point
        EQUALS,                   //!< [=] Equals
        MEMORY_RECALL,            //!< [Long press on =] Memory recall
    };

    Calculator() : Face() {}
    ~Calculator() {}
    virtual bool begin() override;
    virtual void update() override;

    inline uint8_t now()     const { return _now; } //!< @brief Gets the modified value(0~9) or enum Button.
    inline bool isFunction() const { return now() & FUNCTION_BIT; } //!< @brief Is function button?
    
  private:
    uint8_t _now{};

#if __has_include(<SDL2/SDL.h>)
  protected:
    static int sdl_event_filter_calculator(void* arg, SDL_Event *event);
    virtual void _sdl_event_filter_calculator(SDL_Event *event);
#endif
};


/*!
  @class Gamepad
  @brief For Gamepad.
  @note Supports software repeat.
 */
class Gamepad : public Face
{
  public:
    /*!
      @enum Button
      @brief Button value.
    */
    enum Button : uint8_t
    {
        Up =        0x01,   //!< Up (cross)
        Down =      0x02,   //!< Down (cross)
        Left =      0x04,   //!< Left (cross)
        Right =     0x08,   //!< Right (cross)
        A =         0x10,   //!< A (button)
        B =         0x20,   //!< B (button)
        Select =    0x40,   //!< Select (system button)
        Start =     0x80,   //!< Start (system button)
    };
    static constexpr size_t NUMBER_OF_BUTTONS = 8;
    using Bits = std::underlying_type<Button>::type;
    
    /// @name Mask for button
    /// @{
    static constexpr Bits MASK_CROSS = (Button::Up | Button::Down | Button::Left | Button::Right); //!< @brief Cross buttons.
    static constexpr Bits MASK_BUTTON = (Button::A | Button::B); //!< @brief Buttons.
    static constexpr Bits MASK_SYSTEM = (Button::Select | Button::Start); //!< @brief System buttons.
    static constexpr Bits MASK_ALL = (MASK_CROSS | MASK_BUTTON | MASK_SYSTEM); //!< @brief All buttons.
    /// @}

    /// @name Properties
    /// @{
    inline bool isPressed(const Bits& b) const          { return now() & b; }          //!< @brief Is pressed any?
    inline bool wasPressed(const Bits& b) const         { return edge()  & b; }        //!< @brief Was pressed any?
    inline bool isReleased(const Bits& b) const         { return (now() & b) ==0; }    //!< @brief Is released any?
    inline bool wasReleased(const Bits& b) const        { return releaseEdge()  & b; } //!< @brief Was released any?
    inline bool isHolding(const Bits& b) const          { return hold() & b; }         //!< @brief Is holding any?
    inline bool wasHold(const Bits& b) const            { return holdEdge() & b; }     //!< @brief Was hold any?
    inline bool wasRepeated(const Bits& b) const        { return repeat() & b; }       //!< @brief Was repeated any?
    /// @}

    /// @name Gets the raw values
    /// @{
    inline Bits now() const         { return _now; }
    inline Bits last() const        { return _last; }
    inline Bits edge() const        { return _edge; }
    inline Bits releaseEdge() const { return _releaseEdge; }
    inline Bits hold() const        { return _hold; }
    inline Bits holdEdge() const    { return _holdEdge; }
    inline Bits repeat() const      { return _repeat; }
    /// @}

    /// @name Holding related
    /// @{
    /*! @brief Gets the threshold time considered as hold. (ms) */
    unsigned long getHoldTH(const Button btn) const;
    /*! @brief Sets the threshold time considered as hold. (ms) */
    void setHoldTH(const Button btn, unsigned long ms);
    /*! @brief Sets the threshold time considered as hold to all buttons. (ms) */
    inline void setHoldTH(unsigned long ms) { _holdTH.fill(ms); }
    /// @}
    
    /// @name Software repeat related.
    /// @{
    /*! @brief Gets the threshold time considered as repeat. (ms) */
    unsigned long getRepeatTH(const Button btn) const;
    /*! @brief Sets the threshold time considered as repeat. (ms) */
    void setRepeatTH(const Button btn, unsigned long ms);
    /*! @brief Sets the threshold time considered as repeat to all buttons. (ms) */
    inline void setRepeatTH(unsigned long ms) { _repeatTH.fill(ms); }
    /// @}

    Gamepad() : Face() { _repeatTH.fill(DEFAULT_REPEAT_MS); _holdTH.fill(DEFAULT_HOLD_MS); }
    ~Gamepad(){}
    virtual bool begin() override;
    virtual void update() override;

#if __has_include(<SDL2/SDL.h>)
  protected:
    static int sdl_event_filter_gamepad(void* arg, SDL_Event *event);
    virtual void _sdl_event_filter_gamepad(SDL_Event *event);
    int _joyIndex{-1};
    static std::array<Button,SDL_CONTROLLER_BUTTON_MAX> buttonMap;
#endif
    
  private:
    Bits _now{}, _last{}, _edge{}, _releaseEdge{}, _repeat{}, _hold{}, _holdEdge{};

    static constexpr uint8_t DEFAULT_REPEAT_MS = 10;
    static constexpr uint8_t DEFAULT_HOLD_MS = 100;
    std::array<unsigned long, NUMBER_OF_BUTTONS> _repeatStart{}, _repeatTH{};
    std::array<unsigned long, NUMBER_OF_BUTTONS> _holdStart{}, _holdTH{};
};

}}
#endif

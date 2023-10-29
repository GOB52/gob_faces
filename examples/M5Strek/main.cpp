/*
  Keyboard example
  A game that looks like a text based  "Star Trek" game.
*/
#include <M5Unified.h>
#include <gob_faces.hpp>
#include <array>
#include <tuple>
#include <random>

using namespace goblib::faces;

#if defined(ARDUINO)
#include <esp_random.h>
#include <WString.h>
using string_t = String;
#else
#include <string>
using string_t = std::string;
#define DEBUG_COMMAND
#endif

// Quadrant datat bit.
constexpr uint8_t SHOW_MASK =  0x80;
constexpr uint8_t K_MASK = 0x70;
constexpr uint8_t BASE_MASK = 0x08;
constexpr uint8_t STAR_MASK = 0x07;

// Sector data
enum
{
    NONE,
    ENEMY,
    BASE,
    STAR,
    SHIP,
};

enum DamageSpot
{
    SPOT_SRSENSOR,
    SPOT_COMPUTER,
    SPOT_LRSENSOR,
    SPOT_PHASER,
    SPOT_WARP,
    SPOT_TORPEDO,
    SPOT_SHIELD,
    SPOT_MAX
};

static const char* spotTable[] =
{
    "SHORT RANGE SENSOR",
    "COMPUTER DISPLAY",
    "LONG RANGE SENSOR",
    "PHASER",
    "WARP ENGINE",
    "PHOTON TORPEDO TUBES",
    "SHIELD",
};

struct Enemy
{
    int x{}, y{}, energy{};
};


Keyboard kbd;
auto& lcd = M5.Display;

using loop_function = void(*)();
loop_function loop_func{};
void start_loop();
void prologue_loop();
void game_loop();
void clear_loop();
void gameover_loop();

int  width{}, height{},fwid{}, fhgt{}, mx{}, my{};
int qx{}, qy{}, sx{}, sy{};
int knum{}, klingon{}, klingonTotal{};
int bases{}, days{}, torpedo{}, casualty{}, energy{};
bool dock{}, showSector{};
std::array<uint8_t, 8*8> gmap;
std::array<uint8_t, 8*8> smap;
std::array<Enemy, 6> enemies;
std::array<int, SPOT_MAX> damage;

inline void delay() { M5.delay(1000/60); } // About 60 FPS.
inline int pos2idx(const int x, const int y) { return y * 8 + x; }
inline bool isShow(const uint8_t d) { return d & SHOW_MASK; }
inline uint8_t numKlingons(const uint8_t d) { return (d & K_MASK) >> 4; }
inline bool hasBase(const uint8_t d) { return d & BASE_MASK; }
inline uint8_t numStars(const uint8_t d) { return d & STAR_MASK; }
inline uint8_t makeData(uint8_t k, uint8_t b, uint8_t s) { return ((k & 0x07) << 4) | ((b & 1) << 3) | (s & 7); }

std::mt19937 rndEngine;

// 1 - maxval
int rnd(int maxval)
{
    return (rndEngine() % maxval) + 1;
}
void hitAnyKey()
{
    for(;;)
    {
        kbd.update();
        if(kbd.available()) { break; }
        delay();
    }
}

void drawString(const int32_t x, const int32_t y, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char buf[128];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    buf[sizeof(buf)-1] = '\0';
    lcd.drawString(buf, x, y);
    va_end(ap);
}

void initMessage()
{
#define MESSAGE_X (fwid)
#define MESSAGE_Y (fhgt * 22)
    mx = MESSAGE_X;
    my = MESSAGE_Y;
    lcd.fillRect(mx, my, width - mx, height - my, TFT_BLACK);
}

template<typename ...Args> void drawMessage(const char* format, Args ...args)
{
    // Out of the screen?
    if(my >= height)
    {
        auto ms = M5.millis();
        bool rr{true};
        drawString(width - fwid * 4, height - fhgt, "...");
        for(;;)
        {
            if(M5.millis() - ms > 500)
            {
                ms = M5.millis();
                rr = !rr;
                drawString(width - fwid * 4, height - fhgt, rr ? "..." : "   ");
            }
            
            kbd.update();
            if(kbd.available()) { initMessage(); break; }
        }
    }
    drawString(mx, my, format, std::forward<Args>(args)...); my += fhgt;
}

size_t getString(string_t& result, int maxcol, int x, int y)
{
    int col{};
    for(;;)
    {
        drawString(x, y, result.c_str());

        kbd.update();
        if(kbd.available())
        {
            auto ch = kbd.raw();
            // Always interprete as the state after the sym key is pressed. (For convenience)
            switch(ch)
            {
            case 'w': ch = '1'; break;
            case 'e': ch = '2'; break;
            case 'r': ch = '3'; break;
            case 's': ch = '4'; break;
            case 'd': ch = '5'; break;
            case 'f': ch = '6'; break;
            case 'z': ch = '7'; break;
            case 'x': ch = '8'; break;
            case 'c': ch = '9'; break;
            }
#if defined(DEBUG_COMMAND)
            if(col < maxcol && isalnum(ch)) { result += (char)ch; ++col; continue; }
#else
            if(col < maxcol && isdigit(ch)) { result += (char)ch; ++col; continue; }
#endif

            if(ch == '\n') { break; }
            if(ch == '\b' && result.length())
            {
#if defined(ARDUINO)
                result = result.substring(0, --col);
#else
                result = result.substr(0, --col);
#endif
                lcd.fillRect(x, y, fwid * maxcol, fhgt, 0);
            }
        }
        delay();
    }
    return result.length();
}

int getNumber(int maxcol, int x, int y)
{
    string_t s;
    auto len = getString(s, maxcol, x, y);
    int val{};
    return (len && sscanf(s.c_str(), "%d", &val) == 1) ? val : -1;
}

std::tuple<int,int,int> calcMoving(int dir)
{
    int r{}, s{}, t{};

    M5_LOGV("dir:%d s:%d", dir, (dir + 45) / 90);
    s = (dir + 45) / 90;
    dir = dir - s * 90;
    r = (45 + dir * dir) / 110 + 45;
    switch(s)
    {
    case 0:
    case 4:
        t = -45;
        s = dir;
        break;
    case 1:
        t = dir;
        s = 45;
        break;
    case 2:
        t = 45;
        s = -dir;
        break;
    case 3:
        t = -dir;
        s = -45;
        break;
    }
    return std::forward_as_tuple(r, s, t);
}

void sectorMap()
{
#define SECTOR_MAP_X (fwid)
#define SECTOR_MAP_Y (fhgt * 10)
    int x{SECTOR_MAP_X}, y{SECTOR_MAP_Y};
    
    for(int32_t yy = 0; yy < 8; ++yy)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:", yy + 1);
        buf[sizeof(buf)-1] = '\0';
        string_t s(buf);
        for(int32_t xx = 0; xx < 8; ++xx)
        {
            if(damage[SPOT_SRSENSOR] <= 0 && showSector)
            {
                switch(smap[pos2idx(xx,yy)])
                {
                case ENEMY: s += 'K'; break;
                case BASE:  s += 'B'; break;
                case STAR:  s += '*'; break;
                case SHIP:  s += 'M'; break;
                default:    s += '.'; break;
                }
            }
            else
            {
                s += '-';
            }
            s += ' ';
        }
        drawString(x, y, s.c_str()); y += fhgt;
    }
    drawString(x, y, "  - - - - - - - -"); y += fhgt;
    drawString(x, y, "  1 2 3 4 5 6 7 8");
}

void initializeSector(int gx, int gy, int ex, int ey)
{
    M5_LOGV("%s", __func__);
    auto q = gmap[pos2idx(gx,gy)];
    auto kn = numKlingons(q);
    auto bn = hasBase(q) ? 1 : 0;
    auto sn = numStars(q);
    showSector = false;
    
    knum = kn;
    smap.fill(0);
    enemies.fill({});

    smap[pos2idx(ex,ey)] = SHIP;

    while(kn)
    {
        auto xx = rnd(8) - 1;
        auto yy = rnd(8) - 1;
        if(smap[pos2idx(xx,yy)]) { continue; }
        smap[pos2idx(xx,yy)] = ENEMY;
        --kn;
        enemies[kn].x = xx;
        enemies[kn].y = yy;
        enemies[kn].energy = 300;

    }
    while(bn)
    {
        auto xx = rnd(8) - 1;
        auto yy = rnd(8) - 1;
        if(smap[pos2idx(xx,yy)]) { continue; }
        smap[yy * 8 + xx] = BASE;
        --bn;
    }
    while(sn)
    {
        auto xx = rnd(8) - 1;
        auto yy = rnd(8) - 1;
        if(smap[pos2idx(xx,yy)]) { continue; }
        smap[yy * 8 + xx] = STAR;;
        --sn;
    }
}

void galaxyMap()
{
#define GALAXY_MAP_X (fwid)
#define GALAXY_MAP_Y (0)
    int x{GALAXY_MAP_X}, y{GALAXY_MAP_Y};
    auto left = x;

    for(int32_t yy = 0; yy < 8; ++yy)
    {
        lcd.setTextColor(TFT_GREEN, TFT_BLACK);
        x = left;
        drawString(x, y, "%d:", yy + 1); x += fwid*2;
        for(int32_t xx = 0; xx < 8; ++xx)
        {
            auto o = gmap[pos2idx(xx,yy)];

            lcd.setTextColor((xx == qx && yy == qy) ? TFT_BLUE : TFT_GREEN, TFT_BLACK);
            if((o & SHOW_MASK) && !(damage[SPOT_COMPUTER]) )
            {
                drawString(x, y, "%d%d%d ", (o & 0x70) >> 4, (o & 0x08) >> 3, o & 7);
            }
            else
            {
                drawString(x, y, "*** ");
            }
            x += fwid * 4;
        }
        y += fhgt;
    }
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    drawString(0, y, "   --- --- --- --- --- --- --- ---"); y += fhgt;
    drawString(0, y, "    1   2   3   4   5   6   7   8 ");
}

void report()
{
#define REPORT_X (fwid * 4 * 9)
#define REPORT_Y (0)
    int x{REPORT_X}, y{REPORT_Y};

    lcd.fillRect(x, y, width - x, fhgt * 9, 0);
    drawString(x, y, "STARDATE  %d", 3230 - days); y += fhgt;
    drawString(x, y, "TIME LEFT %d", days); y += fhgt;
    drawString(x, y, "QUADRANT  %d-%d", qy + 1, qx + 1); y += fhgt;
    drawString(x, y, "SECTOR    %d-%d", sy + 1, sx + 1); y += fhgt;
    drawString(x, y, "ENERGY    %d", energy); y += fhgt;
    drawString(x, y, "TORPEDOES %d", torpedo); y += fhgt;
    drawString(x, y, "ENEMIES   %d", klingon); y += fhgt;
    drawString(x, y, "STARBASES %d", bases); y += fhgt;
    drawString(x, y, "CONDITION %s", dock ? "DOCKED" : (knum > 0 ? "RED" : (energy < 999 ? "YELLOW" : "GREEN")) );
}

void damageReport()
{
#define DAMAGE_REPORT_X (fwid * 20)
#define DAMAGE_REPORT_Y (fhgt * 10)
  
    int x{DAMAGE_REPORT_X}, y{DAMAGE_REPORT_Y};

    drawString(x, y, "DAMAGE REPORT: (REPAIR PERIOD)"); x += fwid; y += fhgt;

    lcd.fillRect(x, y, width - x, fhgt * SPOT_MAX, 0);
    int i = 0;
    for(auto&& e : damage)
    {
        if(e) { drawString(x, y, "%20s (%2d)", spotTable[i], e); y += fhgt; }
        ++i;
    }
}

bool repair()
{
    bool repaired{};
    for(auto& e : damage) { if(e > 0) { --e; repaired = true; } }
    return repaired;
}

void repairAll()
{
    damage.fill(0);
}

void recoverySpacecraft()
{
    energy = 4000;
    torpedo = 10;
    repairAll();
}

void damageMessage(DamageSpot ds)
{
    if(ds >= 0 && ds < SPOT_MAX)
    {
        M5_LOGV("[%s] DAMAGED", spotTable[ds]);
        drawMessage("%s DAMAGED.", spotTable[ds]);
    }
}

void casualties(int en)
{
    int cu{};
    if(damage[SPOT_SHIELD] == 0)
    {
        damage[SPOT_SHIELD] = rnd(en / 50 + 1);
        damageMessage(SPOT_SHIELD);
        return;
    }
    DamageSpot dmg = (DamageSpot)(rnd(SPOT_SHIELD) - 1);
    damage[dmg] += rnd(en / 99 + 1);
    cu = rnd(8) + 1;
    casualty += cu;
    drawMessage("TOBOZO: SICKBAY TO BRIDGE,WE SUFFERED %d CASUALTIES.", cu);
    damageMessage(dmg);
}

void drawAll()
{
    galaxyMap();
    sectorMap();
    report();
    damageReport();
}

int command()
{
#define COMMAND_X (fwid)
#define COMMAND_Y (fhgt * 20)
    int x{COMMAND_X}, y{COMMAND_Y};
    
    lcd.fillRect(x, y , width - x, fhgt, 0);
    for(;;)
    {
        y = COMMAND_Y;
        drawString(x, y, "COMMAND:?"); y += fhgt;

        string_t s;
        auto len = getString(s, 1, x + fwid * 8, COMMAND_Y);
        if(len == 0) { continue; }

        switch(s[0])
        {
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
#ifdef DEBUG_COMMAND
        case 'Z': ++damage[SPOT_SRSENSOR]; break;
        case 'X': ++damage[SPOT_COMPUTER]; break;
        case 'C': ++damage[SPOT_LRSENSOR]; break;
        case 'V': ++damage[SPOT_PHASER]; break;
        case 'B': ++damage[SPOT_WARP]; break;
        case 'N': ++damage[SPOT_TORPEDO]; break;
        case 'M': ++damage[SPOT_SHIELD]; break;
        case '<': repairAll(); break;
        case '>': drawAll(); break;
        case 'A': energy = 4000; break;
        case 'S': for(auto& d : gmap) { d |= SHOW_MASK; }; break;
        case 'E':
            gmap[pos2idx(qx,qy)] = makeData(6, 1, 1);
            initializeSector(qx, qy, sx, sy);
            galaxyMap();
            sectorMap();
            break;
        case 'P': klingon = 0; break;
        case 'O': days = 0; break;
        case 'I': energy = 0; break;
#endif
        default:
            drawString(x, y, "1:SRSENSOR 2:LRSENSOR 3:PHASER 4:TORPEDO 5:WARP"); break;
        }
    }
}

void spacebase()
{
    // Check the area around the ship.
    for(int y = sy - (sy > 0); y < sy + 1 + (sy < 7); ++y)
    {
        for(int x = sx - (sx > 0); x < sx + 1 + (sx < 7); ++x)
        {
            if(smap[pos2idx(x,y)] == BASE)
            {
                if(!dock)
                {
                    dock = true;
                    recoverySpacecraft();
                    damageReport();
                    report();
                    drawMessage("TANAKA: CAPTAIN, WE ARE DOCKED AT STARBASE.");
                }
                return;
            }
        }
    }
    dock = false;
}

void shortRangeSensor()
{
    M5_LOGV("%s", __func__);
    initMessage();
    if(damage[SPOT_SRSENSOR])
    {
        damageMessage(SPOT_SRSENSOR);
        return;
    }
    showSector = true;
    gmap[pos2idx(qx, qy)] |= SHOW_MASK;

    sectorMap();
    galaxyMap();
}

void longRangeSensor()
{
    M5_LOGV("%s", __func__);
    initMessage();
    if(damage[SPOT_LRSENSOR])
    {
        initMessage();
        damageMessage(SPOT_LRSENSOR);
        return;
    }

    for(int y = qy - (qy > 0); y < qy + 1 + (qy < 7); ++y)
    {
        for(int x = qx - (qx > 0); x < qx + 1 + (qx < 7); ++x)
        {
            gmap[pos2idx(x,y)] |= SHOW_MASK;
        }
    }
    galaxyMap();
}


int calcHit(int en, int eidx)
{
    int x{enemies[eidx].x - sx}, y{enemies[eidx].y - sy};
    return(en * 30 / (30 + x * x + y * y) + 1);
}

bool phaser()
{
    M5_LOGV("%s", __func__);
    initMessage();
    if(damage[SPOT_PHASER])
    {
        damageMessage(SPOT_PHASER);
        return false;;
    }

    int x{COMMAND_X}, y{COMMAND_Y};
    lcd.fillRect(x, y, width - x, height - y, TFT_BLACK);
    drawString(x, y, "PHASER"); y += fhgt;
    drawString(fwid, y, "ENERGIZED. UNITS TO FIRE:?    ");

    int power = getNumber(4, fwid * 26, y);
    lcd.fillRect(x, y, width - x, fhgt, TFT_BLACK);
    if(power < 1) { return false; }
    
    if(power > energy)
    {
        drawMessage("JIMMY: WE HAVE ONLY %d UNITS.", energy);
        return false;
    }
    energy -= power;
    if(knum < 1)
    {
        drawMessage("PHASER FIRED AT EMPTY SPACE.");
        return true;
    }

    power /= knum;
    if(power > 1090)
    {
        drawMessage("PHASER OVERLOADED..");
        damage[SPOT_PHASER] = 1;
        damageMessage(SPOT_PHASER);
        power = 9;
    }

    int idx{};
    for(auto& enemy : enemies)
    {
        if(enemy.energy > 0)
        {
            auto hit = calcHit(power, idx);
            enemy.energy -= hit;
            drawMessage("%d UNITS HIT, ENEMY AT S:%d-%d %s ",
                        hit, enemy.y + 1, enemy.x + 1, enemy.energy <= 0 ? "***DESTROYED***" : "***DAMAGED***");
            M5_LOGV("(%d,%d) %d, %d", enemy.x, enemy.y, hit, enemy.energy);
            if(enemy.energy <= 0)
            {
                auto d = numKlingons(gmap[pos2idx(qx, qy)]) - 1;
                gmap[pos2idx(qx, qy)] &= ~K_MASK;
                gmap[pos2idx(qx, qy)] |= (d << 4) & K_MASK;
                smap[pos2idx(enemy.x, enemy.y)] = 0;
                --knum;
                --klingon;
            }
        }
        ++idx;
    }
    return true;
}

bool photontorpedo()
{

    int cr, s, t, ss, ts;
    int x{COMMAND_X};
    int y{COMMAND_Y};
    int dir{};
    
    M5_LOGV("%s", __func__);
    initMessage();
    if(damage[SPOT_TORPEDO])
    {
        damageMessage(SPOT_TORPEDO);
        return false;;
    }

    lcd.fillRect(x, y, width - x, height - y, TFT_BLACK);
    
    if(torpedo <= 0)
    {
        drawMessage("PHOTON TORPEDO TUBE EMPTY");
        return false;
    }

    // Input direction(degree)
    for(;;)
    {
        y = COMMAND_Y;
        drawString(x, y, "TORPEDO LOADED"); y += fhgt;
        drawString(fwid, y, "COURSE (0-360) :?                 ");
        dir = getNumber(3, fwid * 17, y);
        M5_LOGV("dir:%d", dir);
        if(dir < 0 || dir > 360) { lcd.fillRect(x, y, width - x, fhgt, TFT_BLACK); return false; }
        break;
    }

    // Tracking
    lcd.fillRect(x, y, width - x, fhgt, TFT_BLACK);

    std::tie(cr, s, t) = calcMoving(dir);
    M5_LOGV("rst: %d, %d, %d", cr, s, t);

    --torpedo;
    ss = 45 * sx + 22;
    ts = 45 * sy + 22;

    M5_LOGV("Track from:%d,%d", sx, sy);
    string_t tstr("TORPEDO TRACK ");
    for(;;)
    {
        ss += s;
        ts += t;
        x = (ss + 45) / 45 - 1;
        y = (ts + 45) / 45 - 1;
        M5_LOGV("tracking:%d,%d [%d,%d]", x, y, ss, ts);    
        if(x < 0 || x >= 8 || y < 0 || y >= 8) { break; } // Out of the sector.

        char buf[32];
        snprintf(buf, sizeof(buf), "%d,%d ", y + 1, x + 1);
        buf[sizeof(buf)-1] = '\0';
        tstr += buf;
        
        switch(smap[pos2idx(x,y)])
        {
        case ENEMY:
            drawMessage(tstr.c_str());
            for(auto& e : enemies)
            {
                if(e.x == x && e.y == y)
                {
                    auto hit = rnd(99) + 280;
                    e.energy -= hit;
                    drawMessage("%d UNITS HIT, ENEMY AT S:%d-%d %s ",
                                hit, e.y + 1, e.x + 1, e.energy <= 0 ? "***DESTROYED***" : "***DAMAGED***");
                    M5_LOGV("(%d,%d) %d, %d", e.x, e.y, hit, e.energy);
                    if(e.energy <= 0)
                    {
                        auto d = numKlingons(gmap[pos2idx(qx, qy)]) - 1;
                        gmap[pos2idx(qx, qy)] &= ~K_MASK;
                        gmap[pos2idx(qx, qy)] |= (d << 4) & K_MASK;
                        smap[pos2idx(x, y)] = NONE;
                        --knum;
                        --klingon;
                    }
                    return true;
                }
            }
            M5_LOGE("Missing e...");
            break;
        case BASE:
            --bases;
            drawMessage(tstr.c_str());
            drawMessage("STARBASE DESTROYED");
            drawMessage("JIMMY: I OFTEN FIND HUMAN BEHAVIOUR FASCINATING.");
            smap[pos2idx(x,y)] = NONE;
            gmap[pos2idx(qx,qy)] &= ~BASE_MASK;
            return true;
        case STAR:
            drawMessage(tstr.c_str());
            drawMessage("HIT A STAR");
            //Absorbred?
            if(rnd(9) < 3)
            {
                drawMessage("TORPEDO ABSORBED");
                return true;
            }
            // Destroy?
            if(rnd(9) < 6)
            {
                smap[pos2idx(x,y)] = 0;
                gmap[pos2idx(qx,qy)] --;
                drawMessage("STAR DESTROYED");
            }
            else
            {
                drawMessage("IT NOVAS ***RADIATION ALARM***");
                casualties(300);
            }
            return true;
        }
    }
    tstr += "...MISSED";
    drawMessage(tstr.c_str());
    return true;
}


bool warp()
{
    M5_LOGV("%s", __func__);
    int dist{}, cr{}, s{}, t{}, ss{}, ts{}, dir{}, consume{}, x{COMMAND_X}, y{COMMAND_Y};
    lcd.fillRect(x, y, width - x, height - y, TFT_BLACK);
    
    initMessage();
  
    // Input distance and direction(degree)
    for(;;)
    {
        y = COMMAND_Y;
        drawString(x, y, "WARP"); y += fhgt;
        drawString(fwid, y, "SECTOR DISTANCE:? ");

        dist = getNumber(2, fwid * 17, y);
        M5_LOGV("dist:%d", dist);
        if(dist < 1) { lcd.fillRect(x, y, width - x, fhgt, TFT_BLACK); return false; }

        y += fhgt;
        if(damage[SPOT_WARP] > 0 && dist > 2)
        {
            drawString(x, y, "IMLIUBO: WE CAN TRY 2 AT MOST, SIR."); y += fhgt;
            continue;
        }
        if(dist > 91)
        {
            y += fhgt;
            dist = 91;
            drawString(x, y, " JIMMY: ARE YOU SURE, CAPTAIN?"); y += fhgt;
        }
        consume = (dist * dist) / 2;
        if(energy < consume)
        {
            drawString(x, y, "LOVYAN: SIR, WE DO NOT HAVE THE ENERGY."); y += fhgt;
            return false;
        }

        drawString(fwid, y, "COURSE (0-360) :?                 ");
        dir = getNumber(3, fwid * 17, y);
        M5_LOGV("dir:%d", dir);
        if(dir < 0 || dir > 360) { lcd.fillRect(x, y, width - x, fhgt, TFT_BLACK); continue; }
        break;
    }

    // Move ship
    lcd.fillRect(x, COMMAND_Y, width - x, height - COMMAND_Y, 0);

    --days;
    energy -= consume;
    if(repair()) { damageReport(); }

    std::tie(cr, s, t) = calcMoving(dir);
    M5_LOGV("rst: %d, %d, %d", cr, s, t);
  
    smap[pos2idx(sx, sy)] = 0;
    ss = 45 * sx + 22;
    ts = 45 * sy + 22;
    dist *= 45;
    M5_LOGV("d:%d ss:%d(%d)", dist, ss, ts);

    M5_LOGV("Move from:%d,%d", sx, sy);
    bool needInit{}, storm{};;
    for(;;)
    {
        dist -= cr;
        M5_LOGV("dist:%d [%d,%d] <%d,%d>", dist, ss, ts, s, t);
        if(dist < -22)
        {
            smap[pos2idx(sx, sy)] = SHIP;
            drawMessage("M5STACK IN Q:%d-%d S:%d-%d", qy + 1, qx + 1, sy + 1, sx + 1);
            if(needInit) { initializeSector(qx, qy, sx, sy); }
            return true;
        }
        ss += s;
        ts += t;
        x = (ss + 45) / 45 - 1;
        y = (ts + 45) / 45 - 1;
        M5_LOGV("to:%d,%d [%d,%d]", x, y, ss, ts);
      
        // Change Quadrant?
        if(x < 0 || x >= 8 || y < 0 || y >= 8)
        {
            M5_LOGV("old Q:%d,%d", qx, qy);
            if(x < 0) --qx;
            if(x >= 8) ++qx;
            if(y < 0) --qy;
            if(y >= 8) ++qy;
            M5_LOGV("new Q:%d,%d", qx, qy);
            // Accident?
            if(!storm && rnd(9) < 2) // 1/9
            {
                M5_LOGV("***SPACE STORM***");
                storm = true;
                drawMessage("***SPACE STORM***");
                casualties(100);
                galaxyMap();
            }
            // In the galaxy?
            if(qx >= 0 && qx < 8 && qy >= 0 && qy < 8)
            {
                sx = (x + 8) % 8;
                sy = (y + 8) % 8;
                ss = 45 * sx + 22;
                ts = 45 * sy + 22;
                smap.fill(0); // Free moving in new sector.
                needInit = true;
                M5_LOGV("continue move %d,%d", sx, sy);
                continue;
            }
            // Out of the galaxy
            M5_LOGV("Out of the galaxy");
            drawMessage("*YOU WANDERED OUTSIDE THE GALAXY**");
            drawMessage("ON BOARD COMPUTER TAKES OVER, AND SAVED YOUR LIFE");

            qx=rnd(8) - 1;
            qy=rnd(8) - 1;
            sx=rnd(8) - 1;
            sy=rnd(8) - 1;
            initializeSector(qx, qy, sx, sy);
            return true;
        }
        // Move in the self sector.
        // Cannot move due to obstacles?
        if(smap[pos2idx(x,y)])
        {
            M5_LOGV("Collision");
            drawMessage("**EMERGENCY STOP**");
            drawMessage("JIMMY: TO ERR IS HUMAN.\"");
            smap[pos2idx(sx, sy)] = SHIP;
            return true;
        }
        sx = x;
        sy = y;
    }
    return true;
}

void initializeGame(uint32_t param)
{
    gmap.fill(0);
    smap.fill(0);

    int bb, kk;
    do
    {
        klingon = 0;
        bases = 0;
        days = 30;
        for(int i=0; i<64; i++)
        {
            bb = (rnd(99) < 5);
            bases += bb;
            kk = rnd(param);
            kk = (kk < 209) + (kk < 99) + (kk < 49) + (kk < 24) + (kk < 9) + (kk < 2);
            klingon += kk;
            gmap[i] = makeData(kk, bb, rnd(7));
        }
    }while((bases < 2) || (klingon < 4));

    recoverySpacecraft();
    casualty = 0;
    klingonTotal = klingon;
    qx = rnd(8) - 1;
    qy = rnd(8) - 1;
    sx = rnd(8) - 1;
    sy = rnd(8) - 1;
    initializeSector(qx, qy, sx, sy);
}

void clear_loop()
{
#define CLEAR_X (0)
#define CLEAR_Y ((height - fhgt * 10)/2)
    int x{CLEAR_X}, y{CLEAR_Y};
    lcd.fillRect(x, y, width, fhgt * 10, TFT_BLACK);

    lcd.setTextDatum(textdatum_t::middle_center);
    y += fhgt;
    drawString(width/2, y, "========================================"); y += fhgt;
    drawString(width/2, y, "MISSION ACCOMPLISHED."); y += fhgt;
    if(days > 13) { drawString(width/2, y, "UNBELIEVABLE!"); }
    else if(days > 9) { drawString(width/2, y, "FANTASTIC!"); }
    else if(days > 5) { drawString(width/2, y, "GOOD WORK..."); }
    else { drawString(width/2, y, "BOY, YOU BARELY MADE IT."); }
    y += fhgt;
    
    auto ksc = klingonTotal * 100 / days * 10;
    auto pena = (100 * (casualty == 0)) - (5 * casualty);
    drawString(width/2, y, "%d ENEMIES IN %d STARDATES. (%d)", klingonTotal, days, ksc); y += fhgt;
    drawString(width/2, y, "%d CASUALTIES INCURRED. (%d)", casualty, pena); y += fhgt;
    drawString(width/2, y, "YOUR SCORE: %d", ksc + pena); y += fhgt;
    drawString(width/2, y, "HIT ANY KEY TO ANOTHER GAME."); y += fhgt;
    drawString(width/2, y, "========================================");
    lcd.setTextDatum(textdatum_t::top_left);

    hitAnyKey();
    loop_func = start_loop;
}

void gameover_loop()
{
    initMessage();
    if(days <= 0)
    {
        drawMessage("IT'S TOO LATE, THE FEDERATION HAS BEEN CONQUERED.");
    }
    else if(energy <= 0)
    {
        drawMessage("M5STACK DESTROYED, %s", 
                    (klingonTotal - klingon) > 9 ? "BUT YOU WERE A GOOD MAN" : "");
    }
    drawMessage("HIT ANY KEY TO ANOTHER GAME.");

    hitAnyKey();
    loop_func = start_loop;
}

int enemiesAttack()
{
    int hit{}, total{};

    if(knum <= 0) { return 0; }

    drawMessage("ENEMY ATTACK");
    if(dock)
    {
        drawMessage("STARBASE PROTECTS M5STACK");
        return 0;
    }

    int idx{};
    for(auto& e : enemies)
    {
        if(e.energy <= 0) { continue; }
        hit = calcHit(e.energy + rnd(e.energy) / 2, idx);
        total += hit;
        drawMessage("%d UNITS HIT FROM ENEMY AT S:%d-%d", hit, e.y + 1, e.x + 1);
    }

    energy -= total;
    if(energy <= 0) { drawMessage("*** BANG ***"); return total; }
    
    drawMessage("%d UNITS OF ENERGY LEFT.", energy);

    if(rnd(energy / 4) <= total)
    {
        casualties(total);
    }
    return total;
}

void game_loop()
{
    lcd.clear(0);
    drawAll();

    for(;;)
    {
        // gameclear?
        if(klingon <= 0)
        {
            hitAnyKey();
            loop_func = clear_loop;
            return;
        }
        // supply?
        spacebase();
        // enemies attack to m ship.
        if(enemiesAttack()) { drawAll(); }
        // gameover?
        if(days <= 0 || energy <= 0)
        {
            hitAnyKey();
            loop_func = gameover_loop;
            return;
        }

        // command?
        bool turnNext{};
        do
        {
            auto cmd = command();
            switch(cmd)
            {
            case 1: shortRangeSensor(); break;
            case 2: longRangeSensor(); break;
            case 3: turnNext = phaser(); break;
            case 4: turnNext = photontorpedo(); break;
            case 5: turnNext = warp(); break;
            }
        }while(!turnNext);
        drawAll();
    }
}

void prologue_loop()
{
    char str[128];
    int32_t x{width/2}, y{height/2};

    lcd.clear(0);
    lcd.setTextDatum(textdatum_t::middle_center);

    lcd.drawString("STARDATE 3200: YOUR MISSION IS ", x, y);
    y += fhgt;

    snprintf(str, sizeof(str), "TO DESTROY %d ENEMIES IN 30 STARDATES.", klingon);
    str[sizeof(str)-1] = '\0';

    lcd.drawString(str, x, y); y += fhgt;

    snprintf(str, sizeof(str), "THERE ARE %d STARBASES.", bases);
    str[sizeof(str)-1] = '\0';
    lcd.drawString(str, x, y); y += fhgt;

    y += fhgt;
    lcd.drawString("HIT ANY KEY", x, y);

    hitAnyKey();
    loop_func = game_loop;

    lcd.setTextDatum(textdatum_t::top_left);
}

void start_loop()
{
    int32_t sd{};
#if defined(ARDUINO)
    sd = esp_random(); // esp_random is hardware RNG, so using for seed.
#else
    std::random_device sg;
    sd = sg();
#endif
    M5_LOGI("seed:%d", sd);
    rndEngine.seed(sd);

    lcd.clear(0);
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.drawString("M5STREK GAME", width/2, height/2);
    lcd.drawString("DO YOU WANT A DIFFICULT GAME?", width/2, height/2 + fhgt);
    lcd.drawString("(Y / N ?)", width/2, height/2 + fhgt * 2);
    lcd.setTextDatum(textdatum_t::top_left);
    for(;;)
    {
        kbd.update();
        if(kbd.available())
        {
            auto key = kbd.raw();
            if(key == 'Y' || key == 'y') { initializeGame(999);  loop_func = prologue_loop; return; }
            if(key == 'N' || key == 'n') { initializeGame(2999); loop_func = prologue_loop; return; }
        }
        delay();
    }
}

void setup()
{
    M5.begin();
    lcd.setFont(&fonts::Font0); // 6x8 pixel

    width = lcd.width();
    height = lcd.height();
    fwid = lcd.fontWidth();
    fhgt = lcd.fontHeight();
    initMessage();

    M5_LOGV("f:%d,%d", fwid, fhgt);
    lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    
    if(!goblib::faces::exists())
    {
        lcd.clear(TFT_RED);
        lcd.printf("Not exists faces");
        return;
    }

    lcd.clear(0);
    M5_LOGV("Faces exists. Is it keyboard?");
    kbd.begin();
    kbd.update(); // Dismiss key data.
    
    loop_func = start_loop;
}

void loop()
{
    if(loop_func) { loop_func(); }
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

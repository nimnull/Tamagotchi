#pragma once
#include <U8g2lib.h>

class Game
{
public:
    Game(U8G2 *_display) : display{ _display } {};
    virtual void init(void);
    virtual void loop(void);

protected:
    U8G2 *display;
};
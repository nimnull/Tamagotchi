#pragma once

#include "game.h"
#include <U8g2lib.h>

// Mothership
#define MOTHERSHIP_HEIGHT 3
#define MOTHERSHIP_WIDTH 7
#define MOTHERSHIP_SPEED 1
#define MOTHERSHIP_SPAWN_CHANCE 150                // Was 150
#define DISPLAY_MOTHERSHIP_BONUS_TIME 20           // how long bonus stays on screen for displaying mothership
 
// Alien Settings
#define ALIEN_HEIGHT 3                             // Was 8
#define NUM_ALIEN_COLUMNS 11                       // Was 7
#define NUM_ALIEN_ROWS 5
#define X_START_OFFSET 26
#define SPACE_BETWEEN_ALIEN_COLUMNS 2
#define LARGEST_ALIEN_WIDTH 5
#define SPACE_BETWEEN_ROWS 6
#define INVADERS_DROP_BY 2                          // Was 4
#define INVADERS_SPEED 10                           // Was 6
#define INVADER_HEIGHT 3                            // Was 8 
#define EXPLOSION_GFX_TIME 10                       // Was 7
#define AMOUNT_TO_DROP_BY_PER_LEVEL 4               // NEW How much farther down aliens start per new level
#define LEVEL_TO_RESET_TO_START_HEIGHT 4            // EVERY MULTIPLE OF THIS LEVEL THE ALIEN y START POSITION WILL RESET TO TOP
#define ALIEN_X_MOVE_AMOUNT 1                       // Was 1
#define CHANCEOFBOMBDROPPING 40                     // Was 40
#define BOMB_HEIGHT 4
#define BOMB_WIDTH 2
#define MAXBOMBS 3                                  // Was 3
#define CHANCE_OF_BOMB_DAMAGE_TO_LEFT_OR_RIGHT 20   // higher more chance
#define CHANCE_OF_BOMB_PENETRATING_DOWN 1           // higher more chance
 
// Player settingsc
#define TANKGFX_WIDTH 5
#define TANKGFX_HEIGHT 3
#define PLAYER_X_MOVE_AMOUNT 2                       // Was 5
#define LIVES 3                                      // NEW
#define PLAYER_EXPLOSION_TIME 10                     // How long an ExplosionGfx remains on screen before dissapearing
#define PLAYER_Y_START 61
#define PLAYER_X_START 20
#define BASE_WIDTH 8               
#define BASE_HEIGHT 6
#define BASE_Y 53
#define NUM_BASES 4
 
#define MISSILE_HEIGHT 4
#define MISSILE_WIDTH 1
#define MISSILE_SPEED 3                              // Was 5
 
// Status of a game object constants
#define ACTIVE 0
#define EXPLODING 1
#define DESTROYED 2

// background dah dah dah sound setting
#define NOTELENGTH 1 // larger means play note longer

// graphics
// aliens

const unsigned char MotherShipGfx[] PROGMEM = {
    B01111100,
    B10111010,
    B11101110};

const unsigned char InvaderTopGfx[] PROGMEM = {
    B00000010,
    B00000111,
    B00000101,
};

const unsigned char InvaderTopGfx2[] PROGMEM = {
    B00000101,
    B00000111,
    B00000010,
};

const unsigned char PROGMEM InvaderMiddleGfx[] =
    {
        B00001010,
        B00011111,
        B00010101,
};

const unsigned char PROGMEM InvaderMiddleGfx2[] = {
    B00010101,
    B00011111,
    B00001010,
};

const unsigned char PROGMEM InvaderBottomGfx[] = {
    B00001010,
    B00010101,
    B00011011,
};

const unsigned char PROGMEM InvaderBottomGfx2[] = {
    B00011011,
    B00010101,
    B00001110,
};

static const unsigned char PROGMEM ExplosionGfx[] = {
    B00010101,
    B00001010,
    B00010101,
};

// Player grafix
const unsigned char PROGMEM TankGfx[] = {
    B00000100,
    B00011111,
    B00011111,
};

static const unsigned char PROGMEM MissileGfx[] = {
    B00000001,
    B00000001,
    B00000001,
    B00000001,
};

static const unsigned char PROGMEM AlienBombGfx[] = {
    B00000010,
    B00000001,
    B00000010,
    B00000001,
};

static const unsigned char PROGMEM BaseGfx[] = {
    B00111100,
    B01111110,
    B11111111,
    B11111111,
    B11100111,
    B11000011,
};

// 3x5 pixel 7 Seg font
const uint8_t digit[10][5] PROGMEM = {
    B00000111,
    B00000101,
    B00000101,
    B00000101,
    B00000111,
    B00000100,
    B00000100,
    B00000100,
    B00000100,
    B00000100,
    B00000111,
    B00000100,
    B00000111,
    B00000001,
    B00000111,
    B00000111,
    B00000100,
    B00000111,
    B00000100,
    B00000111,
    B00000101,
    B00000101,
    B00000111,
    B00000100,
    B00000100,
    B00000111,
    B00000001,
    B00000111,
    B00000100,
    B00000111,
    B00000111,
    B00000001,
    B00000111,
    B00000101,
    B00000111,
    B00000111,
    B00000100,
    B00000100,
    B00000100,
    B00000100,
    B00000111,
    B00000101,
    B00000111,
    B00000101,
    B00000111,
    B00000111,
    B00000101,
    B00000111,
    B00000100,
    B00000111,
};

// Game structures
struct GameObjectStruct
{
  // base object which most other objects will include
  signed int X;
  signed int Y;
  unsigned char Status; // 0 active, 1 exploding, 2 destroyed
};

struct BaseStruct
{
  GameObjectStruct Ord;
  unsigned char Gfx[6];
};

struct AlienStruct
{
  GameObjectStruct Ord;
  unsigned char ExplosionGfxCounter; // how long we want the ExplosionGfx to last
};

struct PlayerStruct
{
  GameObjectStruct Ord;
  unsigned int Score;
  unsigned char Lives;
  unsigned char Level;
  unsigned char AliensDestroyed;     // count of how many killed so far
  unsigned char AlienSpeed;          // higher the number slower they go, calculated when ever alien destroyed
  unsigned char ExplosionGfxCounter; // how long we want the ExplosionGfx to last
};

class SpaceInvaders: public Game {
    public:
        SpaceInvaders(U8G2* display): Game{display} {};
        void init(void) override;
        void loop(void) override;

    private:
        void InitBases();
        void InitAliens(int);
        void InitPlayer();
        void UpdateDisplay();
        void AttractScreen();

        void Physics();
        void MotherShipPhysics();

        void AlienControl();
        void PlayerControl();
        void MissileControl();
        
        void CheckCollisions();
        void LoseLife();
        void NewGame();
        void GameOver();
        void PlayRewardMusic();
        int16_t LeftMostPos();
        int16_t RightMostPos();
        void PlayerHit();
        void DropBomb();
        void MoveBombs();

        void MissileAndAlienCollisions();
        void MotherShipCollisions();
        void MissileAndBasesCollisions();
        void BombCollisions();
        void AlienAndBaseCollisions();
        void BombAndBasesCollision(GameObjectStruct *Bomb);

        bool Collision(GameObjectStruct Obj1, unsigned char Width1, unsigned char Height1, GameObjectStruct Obj2, unsigned char Width2, unsigned char Height2);
        void CentreText(const char *Text, unsigned char Y);
        void DisplayPlayerAndLives(PlayerStruct *Player);
        void NextLevel(PlayerStruct *Player);
        char GetAlienBaseCollisionMask(int AlienX, int AlienWidth, int BaseX);
        void DestroyBase(GameObjectStruct *Alien, BaseStruct *Base, char Mask, int BaseByteOffset);

        void draw_digit(int16_t x, int16_t y, uint8_t n);
        void print_int(int16_t x, int16_t y, uint16_t s, uint8_t digit);
};
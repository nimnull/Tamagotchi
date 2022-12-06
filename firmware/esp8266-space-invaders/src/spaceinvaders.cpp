#include "spaceinvaders.h"
#include "EEPROM.h"
#include "hardware.h"

// general global variables
unsigned long lastMillis;
unsigned long frameCount;
unsigned int framesPerSecond;

#ifdef BUZZER_ACTIVE_LOW
#define NO_TONE(pin) \
  noTone(pin);       \
  digitalWrite(pin, HIGH);
#else
#define NO_TONE(pin) noTone(pin);
#endif

// alien global vars
// The array of aliens across the screen

AlienStruct Alien[NUM_ALIEN_COLUMNS][NUM_ALIEN_ROWS];
AlienStruct MotherShip;
GameObjectStruct AlienBomb[MAXBOMBS];
BaseStruct Base[NUM_BASES];

static const int TOTAL_ALIENS = NUM_ALIEN_COLUMNS * NUM_ALIEN_ROWS; // NEW

// widths of aliens
// as aliens are the same type per row we do not need to store their graphic width per alien in the structure above
// that would take a byte per alien rather than just three entries here, 1 per row, saving significnt memory
byte AlienWidth[] = {5, 5, 5, 5, 5}; // top, middle ,bottom widths

int8_t AlienXMoveAmount = 2;
signed char InvadersMoveCounter; // counts down, when 0 move invaders, set according to how many aliens on screen
bool AnimationFrame = false;     // two frames of animation, if true show one if false show the other

// Mothership
signed char MotherShipSpeed;
unsigned int MotherShipBonus;
signed int MotherShipBonusXPos;       // pos to display bonus at
unsigned char MotherShipBonusCounter; // how long bonus amount left on screen

// Player global variables
PlayerStruct Player;
GameObjectStruct Missile;

// game variables
unsigned int HiScore;
bool GameInPlay = false;

// Sound settings and variables

// music (dah dah dah sound) control, these are the "pitch" of the four basic notes
const unsigned char Music[] = {
    160, 100, 80, 62};

unsigned char MusicIndex;   // index pointer to next note to play
unsigned MusicCounter;      // how long note plays for countdown timer, is set to
                            // NOTELENGTH define above
bool ShootCompleted = true; // stops music when this is false, so we can here shoot sound

void SpaceInvaders::init(void)
{
  display->setFont(u8g2_font_t0_11b_tf);
  display->setDrawColor(1); // set the color

#if defined(ESP8266) || defined(ESP32)
    EEPROM.begin(sizeof(unsigned int));
#endif
  EEPROM.get(0, HiScore);
  if (HiScore == 65535) // new unit never written to
  {
    HiScore = 0;
    EEPROM.put(0, HiScore);
#if defined(ESP8266) || defined(ESP32)
    EEPROM.commit();
#endif
  }

  InitAliens(0);
  InitPlayer();
}

void SpaceInvaders::loop(void)
{
  if (GameInPlay) // NEW
  {
    Physics();
    UpdateDisplay();
  }
  else
    AttractScreen();

  NO_TONE(PIN_BUZZER);
}

void SpaceInvaders::InitPlayer()
{
  Player.Ord.Y = PLAYER_Y_START;
  Player.Ord.X = PLAYER_X_START;
  Player.Ord.Status = ACTIVE;
  Player.Lives = LIVES;
  Player.Level = 0;
  Missile.Status = DESTROYED;
  Player.Score = 0;
}

void SpaceInvaders::InitBases()
{
  // Bases need to be re-built!
  byte TheByte;
  int Spacing = ((SCREEN_WIDTH - 32) - (NUM_BASES * BASE_WIDTH)) / NUM_BASES;
  for (int i = 0; i < NUM_BASES; i++)
  {
    for (int DataIdx = 0; DataIdx < BASE_HEIGHT; DataIdx++)
    {
      TheByte = pgm_read_byte(BaseGfx + DataIdx);
      Base[i].Gfx[DataIdx] = TheByte;
    }
    Base[i].Ord.X = 17 + (i * Spacing) + (i * BASE_WIDTH) + (Spacing / 2);
    Base[i].Ord.Y = BASE_Y;
    Base[i].Ord.Status = ACTIVE;
  }
}

void SpaceInvaders::InitAliens(int YStart)
{
  for (int across = 0; across < NUM_ALIEN_COLUMNS; across++)
  {
    for (int down = 0; down < NUM_ALIEN_ROWS; down++)
    {
      Alien[across][down].Ord.X = X_START_OFFSET + (across * (LARGEST_ALIEN_WIDTH + SPACE_BETWEEN_ALIEN_COLUMNS)) - (AlienWidth[down] / 2);
      Alien[across][down].Ord.Y = YStart + (down * SPACE_BETWEEN_ROWS);
      Alien[across][down].Ord.Status = ACTIVE;
      Alien[across][down].ExplosionGfxCounter = EXPLOSION_GFX_TIME;
    }
  }
  MotherShip.Ord.Y = 0;
  MotherShip.Ord.X = -MOTHERSHIP_WIDTH;
  MotherShip.Ord.Status = DESTROYED;
}

void SpaceInvaders::Physics()
{
  if (Player.Ord.Status == ACTIVE)
  {
    AlienControl();
    MotherShipPhysics();
    PlayerControl();
    MissileControl();
    CheckCollisions();
  }
}

void SpaceInvaders::UpdateDisplay()
{
  int i;

  display->clearBuffer();

  // Mothership bonus display if required
  if (MotherShipBonusCounter > 0)
  {
    print_int(MotherShipBonusXPos, 0, MotherShipBonus, 3);
    MotherShipBonusCounter--;
  }

  print_int(0, 0, Player.Score, 5); // draw score and lives, anything else can go above them
  print_int(0, 20, HiScore, 5);
  print_int(125, 0, Player.Lives, 1);
  display->drawXBMP(115, 2, TANKGFX_WIDTH, TANKGFX_HEIGHT, TankGfx);
  display->drawVLine(20, 0, 64);
  display->drawVLine(109, 0, 64);

  print_int(0, 59, framesPerSecond, 3);

  frameCount++;
  if ((millis() - lastMillis) > (1000))
  {
    framesPerSecond = (frameCount);
    frameCount = 0;
    lastMillis = millis();
  }

  // BOMBS
  //  draw bombs next as aliens have priority of overlapping them
  for (i = 0; i < MAXBOMBS; i++)
  {
    if (AlienBomb[i].Status == ACTIVE)
      display->drawXBMP(AlienBomb[i].X, AlienBomb[i].Y, 2, 4, AlienBombGfx);
    else
    { // must be destroyed
      if (AlienBomb[i].Status == EXPLODING)
        display->drawXBMP(AlienBomb[i].X - 4, AlienBomb[i].Y, 5, 3, ExplosionGfx);
      // Ensure on next draw that ExplosionGfx dissapears
      AlienBomb[i].Status = DESTROYED;
    }
  }

  // Invaders
  for (int across = 0; across < NUM_ALIEN_COLUMNS; across++)
  {
    for (int down = 0; down < NUM_ALIEN_ROWS; down++)
    {
      if (Alien[across][down].Ord.Status == ACTIVE)
      {

        switch (down)
        {
        case 0:
          if (AnimationFrame)
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderTopGfx);
          else
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderTopGfx2);
          break;
        case 1:
          if (AnimationFrame)
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderMiddleGfx);
          else
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderMiddleGfx2);
          break;
        case 2:
          if (AnimationFrame)
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderMiddleGfx);
          else
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderMiddleGfx2);
          break;
        case 3:
          if (AnimationFrame)
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderBottomGfx);
          else
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderBottomGfx2);
        case 4:
          if (AnimationFrame)
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderBottomGfx);
          else
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, AlienWidth[down], INVADER_HEIGHT, InvaderBottomGfx2);
        }
      }
      else
      {
        if (Alien[across][down].Ord.Status == EXPLODING)
        {
          Alien[across][down].ExplosionGfxCounter--;
          if (Alien[across][down].ExplosionGfxCounter > 0)
          {
            tone(PIN_BUZZER, Alien[across][down].ExplosionGfxCounter * 100, 100);
            display->drawXBMP(Alien[across][down].Ord.X, Alien[across][down].Ord.Y, 5, 3, ExplosionGfx);
          }
          else
            Alien[across][down].Ord.Status = DESTROYED;
        }
      }
    }
  }

  // player
  if (Player.Ord.Status == ACTIVE)
    display->drawXBMP(Player.Ord.X, Player.Ord.Y, TANKGFX_WIDTH, TANKGFX_HEIGHT, TankGfx);
  else
  {
    if (Player.Ord.Status == EXPLODING)
    {
      display->drawXBMP(Player.Ord.X, Player.Ord.Y, 5, 3, ExplosionGfx);
      tone(PIN_BUZZER, Player.ExplosionGfxCounter * 250, 50);
      Player.ExplosionGfxCounter--;
      if (Player.ExplosionGfxCounter == 0)
      {
        Player.Ord.Status = DESTROYED;
        LoseLife();
      }
    }
  }
  // missile
  if (Missile.Status == ACTIVE)
    display->drawXBMP(Missile.X, Missile.Y, MISSILE_WIDTH, MISSILE_HEIGHT, MissileGfx);

  // mothership (not bonus if hit)
  if (MotherShip.Ord.Status == ACTIVE)
    display->drawXBMP(MotherShip.Ord.X, MotherShip.Ord.Y, MOTHERSHIP_WIDTH, MOTHERSHIP_HEIGHT, MotherShipGfx);
  else
  {
    if (MotherShip.Ord.Status == EXPLODING)
    {
      tone(PIN_BUZZER, MotherShip.ExplosionGfxCounter * 250, 50);
      MotherShip.ExplosionGfxCounter--;
      if (MotherShip.ExplosionGfxCounter == 0)
      {
        MotherShip.Ord.Status = DESTROYED;
      }
    }
  }

  // plot bases

  for (i = 0; i < NUM_BASES; i++)
  {
    if (Base[i].Ord.Status == ACTIVE)
      display->drawXBM(Base[i].Ord.X, Base[i].Ord.Y, BASE_WIDTH, BASE_HEIGHT, Base[i].Gfx);
  }
  display->sendBuffer();
}

void SpaceInvaders::AttractScreen()
{
  display->clearBuffer();

  CentreText("Play", 8);
  CentreText("Space Invaders", 20);
  CentreText("Press Fire to start", 32);
  CentreText("Hi Score     ", 44);
  display->setCursor(80, 44);
  display->print(HiScore);

  display->sendBuffer();

  if (digitalRead(PIN_BTN_M) == PRESSED)
  {
    GameInPlay = true;
    NewGame();
  }

  // CHECK FOR HIGH SCORE RESET, PLAYER MUST HOLD LEFT AND RIGHT TOGETHER
  if ((digitalRead(PIN_BTN_L) == PRESSED) & (digitalRead(PIN_BTN_R) == PRESSED))
  {
    // Reset high score, don't need to worry about debounce as will ony write to EEPROM if changed, so writes a 0 then won't write again
    // if hiscore still 0 which it will be
    HiScore = 0;
    EEPROM.put(0, HiScore);
#if defined(ESP8266) || defined(ESP32)
    EEPROM.commit();
#endif
  }
}

unsigned char GetScoreForAlien(int RowNumber)
{
  // returns value for killing an alien at the row indicated
  switch (RowNumber)
  {
  case 0:
    return 30;
  case 1:
    return 20;
  case 2:
    return 10;
  case 3:
    return 5;
  case 4:
    return 2;
  default:
    return 0;
  }
}

void SpaceInvaders::MotherShipPhysics()
{
  if (MotherShip.Ord.Status == ACTIVE)
  { // spawned, move it
    tone(PIN_BUZZER, (MotherShip.Ord.X % 8) * 500, 200);
    MotherShip.Ord.X += MotherShipSpeed;
    if (MotherShipSpeed > 0) // going left to right , check if off right hand side
    {
      if (MotherShip.Ord.X >= (SCREEN_WIDTH - 26))
      {
        MotherShip.Ord.Status = DESTROYED;
        NO_TONE(PIN_BUZZER);
      }
    }
    else // going right to left , check if off left hand side
    {
      if (MotherShip.Ord.X + MOTHERSHIP_WIDTH < 28)
      {
        MotherShip.Ord.Status = DESTROYED;
        NO_TONE(PIN_BUZZER);
      }
    }
  }
  else
  {
    // try to spawn mothership
    if (random(MOTHERSHIP_SPAWN_CHANCE) == 1)
    {
      MotherShip.Ord.Status = ACTIVE; // Spawn a mother ship, starts just off screen at top
      if (random(2) == 1)             // need to set direction
      {
        MotherShip.Ord.X = (SCREEN_WIDTH - 20 - MOTHERSHIP_WIDTH);
        MotherShipSpeed = -MOTHERSHIP_SPEED; // if we go in here swaps to right to left
      }
      else
      {
        MotherShip.Ord.X = 20;              // -MOTHERSHIP_WIDTH;
        MotherShipSpeed = MOTHERSHIP_SPEED; // set to go left ot right
      }
    }
  }
}

void SpaceInvaders::PlayerControl()
{
  // user input checks
  if ((digitalRead(PIN_BTN_R) == PRESSED) & (Player.Ord.X + TANKGFX_WIDTH < (SCREEN_WIDTH - 20)))
    Player.Ord.X += PLAYER_X_MOVE_AMOUNT;
  if ((digitalRead(PIN_BTN_L) == PRESSED) & (Player.Ord.X > PLAYER_X_START))
    Player.Ord.X -= PLAYER_X_MOVE_AMOUNT;
  if ((digitalRead(PIN_BTN_M) == PRESSED) & (Missile.Status != ACTIVE))
  {
    tone(PIN_BUZZER, 1000, 100);
    Missile.X = Player.Ord.X + (TANKGFX_WIDTH / 2);
    Missile.Y = PLAYER_Y_START;
    Missile.Status = ACTIVE;
  }
}

void SpaceInvaders::MissileControl()
{
  if (Missile.Status == ACTIVE)
  {
    Missile.Y -= MISSILE_SPEED;
    if (Missile.Y + MISSILE_HEIGHT < 0) // If off top of screen destroy so can be used again
      Missile.Status = DESTROYED;
  }
}

void SpaceInvaders::AlienControl()
{
  if ((InvadersMoveCounter--) < 0)
  {
    bool Dropped = false;
    if ((RightMostPos() + AlienXMoveAmount >= (SCREEN_WIDTH - 18)) | (LeftMostPos() + AlienXMoveAmount <= 21)) // at edge of screen
    {
      AlienXMoveAmount = -AlienXMoveAmount; // reverse direction
      Dropped = true;                       // and indicate we are dropping
    }

    // play background music note if other higher priority sounds not playing
    if ((ShootCompleted) & (MotherShip.Ord.Status != ACTIVE))
    {
      tone(PIN_BUZZER, Music[MusicIndex], 100);
      MusicIndex++;
      if (MusicIndex == sizeof(Music))
        MusicIndex = 0;
      MusicCounter = NOTELENGTH;
    }

    // update the alien postions
    for (int Across = 0; Across < NUM_ALIEN_COLUMNS; Across++)
    {
      for (int Down = 0; Down < NUM_ALIEN_ROWS; Down++)
      {
        if (Alien[Across][Down].Ord.Status == ACTIVE)
        {
          if (Dropped == false)
          {
            Alien[Across][Down].Ord.X += AlienXMoveAmount;
          }
          else
          {
            Alien[Across][Down].Ord.Y += INVADERS_DROP_BY;
          }
        }
      }
    }
    InvadersMoveCounter = Player.AlienSpeed;
    AnimationFrame = !AnimationFrame; // swap to other frame
  }

  // should the alien drop a bomb
  if (random(CHANCEOFBOMBDROPPING) == 1)
    DropBomb();
  MoveBombs();
}

void SpaceInvaders::MoveBombs()
{
  for (int i = 0; i < MAXBOMBS; i++)
  {
    if (AlienBomb[i].Status == ACTIVE)
      AlienBomb[i].Y += 1;
  }
}

void SpaceInvaders::DropBomb()
{
  // if there is a free bomb slot then drop a bomb else nothing happens
  bool Free = false;
  unsigned char ActiveCols[NUM_ALIEN_COLUMNS];
  unsigned char BombIdx = 0;
  // find a free bomb slot
  while ((Free == false) & (BombIdx < MAXBOMBS))
  {
    if (AlienBomb[BombIdx].Status == DESTROYED)
      Free = true;
    else
      BombIdx++;
  }
  if (Free)
  {
    unsigned char Columns = 0;
    // now pick and alien at random to drop the bomb
    // we first pick a column, obviously some columns may not exist, so we count number of remaining cols
    // first, this adds time but then also picking columns randomly that don't exist may take time also
    unsigned char ActiveColCount = 0;
    signed char Row;
    unsigned char ChosenColumn;

    while (Columns < NUM_ALIEN_COLUMNS)
    {
      Row = (NUM_ALIEN_ROWS - 1);
      while (Row >= 0)
      {
        if (Alien[Columns][Row].Ord.Status == ACTIVE)
        {
          ActiveCols[ActiveColCount] = Columns;
          ActiveColCount++;
          break;
        }
        Row--;
      }
      Columns++;
    }
    // we have ActiveCols array filled with the column numbers of the active cols and we have how many
    // in ActiveColCount, now choose a column at random
    ChosenColumn = random(ActiveColCount); // random number between 0 and the amount of columns
    // we now find the first available alien in this column to drop the bomb from
    Row = (NUM_ALIEN_ROWS - 1);
    while (Row >= 0)
    {
      if (Alien[ActiveCols[ChosenColumn]][Row].Ord.Status == ACTIVE)
      {
        // Set the bomb from this alien
        AlienBomb[BombIdx].Status = ACTIVE;
        AlienBomb[BombIdx].X = Alien[ActiveCols[ChosenColumn]][Row].Ord.X + int(AlienWidth[Row] / 2);
        // above sets bomb to drop around invaders centre, here we add a litrle randomness around this pos
        AlienBomb[BombIdx].X = (AlienBomb[BombIdx].X - 2) + random(0, 4);
        AlienBomb[BombIdx].Y = Alien[ActiveCols[ChosenColumn]][Row].Ord.Y + 4;
        break;
      }
      Row--;
    }
  }
}

void SpaceInvaders::BombCollisions()
{
  // check bombs collisions
  for (int i = 0; i < MAXBOMBS; i++)
  {
    if (AlienBomb[i].Status == ACTIVE)
    {
      if (AlienBomb[i].Y > 64) // gone off bottom of screen
        AlienBomb[i].Status = DESTROYED;
      else
      {
        // HAS IT HIT PLAYERS missile
        if (Collision(AlienBomb[i], BOMB_WIDTH, BOMB_HEIGHT, Missile, MISSILE_WIDTH, MISSILE_HEIGHT))
        {
          // destroy missile and bomb
          AlienBomb[i].Status = EXPLODING;
          Missile.Status = DESTROYED;
        }
        else
        {
          // has it hit players ship
          if (Collision(AlienBomb[i], BOMB_WIDTH, BOMB_HEIGHT, Player.Ord, TANKGFX_WIDTH, TANKGFX_HEIGHT))
          {
            PlayerHit();
            AlienBomb[i].Status = DESTROYED;
          }
          else
            BombAndBasesCollision(&AlienBomb[i]);
        }
      }
    }
  }
}

void SpaceInvaders::BombAndBasesCollision(GameObjectStruct *Bomb)
{ // check and handle any bomb and base collision

  for (int i = 0; i < NUM_BASES; i++)
  {
    if (Collision(*Bomb, BOMB_WIDTH, BOMB_HEIGHT, Base[i].Ord, BASE_WIDTH, BASE_HEIGHT))
    {
      unsigned char X = Bomb->X - Base[i].Ord.X;
      signed char Bomb_Y = (Bomb->Y + BOMB_HEIGHT) - Base[i].Ord.Y;
      unsigned char Base_Y = 0;

      while ((Base_Y <= Bomb_Y) & (Base_Y < BASE_HEIGHT) & (Bomb->Status == ACTIVE))
      {
        unsigned char Idx = (Base_Y);             // this gets the index for the byte in question from the gfx array
        unsigned char TheByte = Base[i].Gfx[Idx]; // we now have the byte to inspect, but need to only look at the 2 bits where the bomb is colliding
        unsigned char Mask = B00000011;
        if (X > 6)
        {
          X = 6;
        };
        Mask = (Mask << (X));
        TheByte = TheByte & Mask;

        if (TheByte > 0)
        {
          Mask = ~Mask;
          Base[i].Gfx[Idx] = Base[i].Gfx[Idx] & Mask;
          if (random(CHANCE_OF_BOMB_PENETRATING_DOWN) == false) // if false BOMB EXPLODES else carries on destroying more
            Bomb->Status = EXPLODING;
        }
        else
          Base_Y++;
      }
    }
  }
}

void SpaceInvaders::MissileAndBasesCollisions()
{
  for (int i = 0; i < NUM_BASES; i++)
  {
    if (Collision(Missile, MISSILE_WIDTH, MISSILE_HEIGHT, Base[i].Ord, BASE_WIDTH, BASE_HEIGHT))
    {
      unsigned char X = Missile.X - Base[i].Ord.X;
      signed char Missile_Y = Missile.Y - Base[i].Ord.Y;
      signed char Base_Y = BASE_HEIGHT - 1;
      while ((Base_Y >= Missile_Y) & (Base_Y >= 0) & (Missile.Status == ACTIVE))
      {
        unsigned char Idx = (Base_Y);
        unsigned char TheByte = Base[i].Gfx[Idx];
        unsigned char Mask = B00000011;
        if (X > 6)
        {
          X = 6;
        }
        Mask = (Mask << (X));
        TheByte = TheByte & Mask;

        if (TheByte > 0)
        {
          Mask = ~Mask;
          Base[i].Gfx[Idx] = Base[i].Gfx[Idx] & Mask;
          if (random(CHANCE_OF_BOMB_PENETRATING_DOWN) == false) // if false BOMB EXPLODES else carries on destroying more
            Missile.Status = EXPLODING;
        }
        else
          Base_Y--;
      }
    }
  }
}

void SpaceInvaders::PlayerHit()
{
  Player.Ord.Status = EXPLODING;
  Player.ExplosionGfxCounter = PLAYER_EXPLOSION_TIME;
  Missile.Status = DESTROYED;
}

void SpaceInvaders::CheckCollisions()
{
  MissileAndAlienCollisions();
  MotherShipCollisions();
  MissileAndBasesCollisions();
  BombCollisions();
  AlienAndBaseCollisions();
}

char SpaceInvaders::GetAlienBaseCollisionMask(int AlienX, int AlienWidth, int BaseX)
{
  signed int DamageWidth;
  unsigned char LeftMask, RightMask;

  // this routine uses a 1 to mean remove bit and 0 to preserve, this is kind of opposite of what we would
  // normally think of, but it's done this way as when we perform bit shifting to show which bits are preserved
  // it will shift in 0's in (as that's just was the C shift operater ">>" and "<<" does
  // at the end we will flip all the bits 0 becomes 1, 1 becomes 0 etc. so that the mask then works correctly
  // with the calling code

  LeftMask = B11111111;  // initially set to remove all
  RightMask = B11111111; // unless change in code below
  // if Alien X more than base x then some start bits are unaffected
  if (AlienX > BaseX)
  {
    // we shift the bits above to the right by the amount unaffected, thus putting 0's into the bits
    // not to delete
    DamageWidth = AlienX - BaseX;
    LeftMask >>= DamageWidth;
  }

  // now work out how much of remaining byte is affected

  // if right hand side of alien is less than BaseX right hand side then some preserved at the right hand end
  if (AlienX + AlienWidth < BaseX + (BASE_WIDTH / 2))
  {
    DamageWidth = (BaseX + (BASE_WIDTH / 2)) - (AlienX + AlienWidth);
    RightMask <<= DamageWidth;
  }
  // we now have two masks, one showing which bits to preserve on left of the byte, the other the right hand side,
  // we need to combine them to one mask, the code in the brackets does this combining

  // at the moment a 0 means keep, 1 destroy, but this is actually makes it difficult to implement later on, so we flip
  // the bits to be a more logical 1= keep bit and 0 remove bit (pixel) and then return the mask
  // the tilde (~) flips the bits that resulted from combining the two masks

  return ~(LeftMask & RightMask);
}

void SpaceInvaders::DestroyBase(GameObjectStruct *Alien, BaseStruct *Base, char Mask, int BaseByteOffset)
{
  signed char Y;
  // go down "removing" bits to the depth that the alien is down into the base
  Y = (Alien->Y + ALIEN_HEIGHT) - Base->Ord.Y;
  if (Y > BASE_HEIGHT - 1)
    Y = BASE_HEIGHT - 1;
  for (; Y >= 0; Y--)
  {
    Base->Gfx[(Y) + BaseByteOffset] = Base->Gfx[(Y) + BaseByteOffset] & Mask;
  }
}

void SpaceInvaders::AlienAndBaseCollisions()
{
  unsigned char Mask;
  // checks if aliens are in collision with the tank
  // start at bottom row as they are most likely to be in contact or not and if not then none further up are either
  for (int row = (NUM_ALIEN_ROWS - 1); row >= 0; row--)
  {
    for (int column = 0; column < NUM_ALIEN_COLUMNS; column++)
    {
      if (Alien[column][row].Ord.Status == ACTIVE)
      {
        // now scan for a collision with each base in turn
        for (int BaseIdx = 0; BaseIdx < NUM_BASES; BaseIdx++)
        {
          if (Collision(Alien[column][row].Ord, AlienWidth[row], ALIEN_HEIGHT, Base[BaseIdx].Ord, BASE_WIDTH, BASE_HEIGHT))
          {
            // WE HAVE A COLLSISION, REMOVE BITS OF BUILDING
            // process left half (first byte) of base first
            Mask = GetAlienBaseCollisionMask(Alien[column][row].Ord.X, AlienWidth[row], Base[BaseIdx].Ord.X);
            DestroyBase(&Alien[column][row].Ord, &Base[BaseIdx], Mask, 0);
          }
        }
      }
    }
  }
}

void SpaceInvaders::MotherShipCollisions()
{
  if ((Missile.Status == ACTIVE) & (MotherShip.Ord.Status == ACTIVE))
  {
    if (Collision(Missile, MISSILE_WIDTH, MISSILE_HEIGHT, MotherShip.Ord, MOTHERSHIP_WIDTH, MOTHERSHIP_HEIGHT))
    {
      MotherShip.Ord.Status = EXPLODING;
      MotherShip.ExplosionGfxCounter = EXPLOSION_GFX_TIME;
      Missile.Status = DESTROYED;
      // generate the score for the mothership hit, note in the real arcade space invaders the score was not random but
      // just appeared so, a player could infulence its value with clever play, but we'll keep it a little simpler
      MotherShipBonus = random(4); // a random number between 0 and 3
      switch (MotherShipBonus)
      {
      case 0:
        MotherShipBonus = 50;
        break;
      case 1:
        MotherShipBonus = 100;
        break;
      case 2:
        MotherShipBonus = 150;
        break;
      case 3:
        MotherShipBonus = 300;
        break;
      }
      Player.Score += MotherShipBonus;
      MotherShipBonusXPos = MotherShip.Ord.X;
      if (MotherShipBonusXPos > 100) // to ensure isn't half off right hand side of screen
        MotherShipBonusXPos = 100;
      if (MotherShipBonusXPos < 0) // to ensure isn't half off right hand side of screen
        MotherShipBonusXPos = 0;
      MotherShipBonusCounter = DISPLAY_MOTHERSHIP_BONUS_TIME;
    }
  }
}

void SpaceInvaders::MissileAndAlienCollisions()
{
  for (int across = 0; across < NUM_ALIEN_COLUMNS; across++)
  {
    for (int down = 0; down < NUM_ALIEN_ROWS; down++)
    {
      if (Alien[across][down].Ord.Status == ACTIVE)
      {
        if (Missile.Status == ACTIVE)
        {
          if (Collision(Missile, MISSILE_WIDTH, MISSILE_HEIGHT, Alien[across][down].Ord, AlienWidth[down], INVADER_HEIGHT))
          {
            // missile hit
            Alien[across][down].Ord.Status = EXPLODING;
            tone(PIN_BUZZER, 700, 100);
            Missile.Status = DESTROYED;
            Player.Score += GetScoreForAlien(down);
            Player.AliensDestroyed++;
            // calc new speed of aliens, note (float) must be before TOTAL_ALIENS to force float calc else
            // you will get an incorrect result
            Player.AlienSpeed = ((1 - (Player.AliensDestroyed / (float)TOTAL_ALIENS)) * INVADERS_SPEED);
            // for very last alien make to  fast!
            if (Player.AliensDestroyed == TOTAL_ALIENS - 2)
            {
              if (AlienXMoveAmount > 0)
                AlienXMoveAmount = ALIEN_X_MOVE_AMOUNT * 2;
              else
                AlienXMoveAmount = -(ALIEN_X_MOVE_AMOUNT * 2);
            }

            // for very last alien make to super fast!
            if (Player.AliensDestroyed == TOTAL_ALIENS - 1)
            {
              if (AlienXMoveAmount > 0)
                AlienXMoveAmount = ALIEN_X_MOVE_AMOUNT * 4;
              else
                AlienXMoveAmount = -(ALIEN_X_MOVE_AMOUNT * 4);
            }

            if (Player.AliensDestroyed == TOTAL_ALIENS)
              NextLevel(&Player);
          }
        }
        if (Alien[across][down].Ord.Status == ACTIVE) // double check still active afer above code
        {
          // check if this alien is in contact with TankGfx
          if (Collision(Player.Ord, TANKGFX_WIDTH, TANKGFX_HEIGHT, Alien[across][down].Ord, AlienWidth[down], ALIEN_HEIGHT))
            PlayerHit();
          else
          {
            // check if alien is below bottom of screen
            if (Alien[across][down].Ord.Y + 8 > SCREEN_HEIGHT)
              PlayerHit();
          }
        }
      }
    }
  }
}

bool SpaceInvaders::Collision(GameObjectStruct Obj1, unsigned char Width1, unsigned char Height1, GameObjectStruct Obj2, unsigned char Width2, unsigned char Height2)
{
  return ((Obj1.X + Width1 > Obj2.X) & (Obj1.X < Obj2.X + Width2) & (Obj1.Y + Height1 > Obj2.Y) & (Obj1.Y < Obj2.Y + Height2));
}

int16_t SpaceInvaders::RightMostPos()
{
  // returns x pos of right most alien
  int Across = NUM_ALIEN_COLUMNS - 1;
  int Down;
  int16_t Largest = 0;
  while (Across >= 0)
  {
    Down = 0;
    while (Down < NUM_ALIEN_ROWS)
    {
      if (Alien[Across][Down].Ord.Status == ACTIVE)
      {
        // different aliens have different widths, add to x pos to get rightpos
        int16_t RightPos = Alien[Across][Down].Ord.X + AlienWidth[Down];
        if (RightPos > Largest)
          Largest = RightPos;
      }
      Down++;
    }
    if (Largest > 0) // we have found largest for this coloum
    {
      break;
    }
    Across--;
  }
  return Largest;
}

int16_t SpaceInvaders::LeftMostPos()
{
  // returns x pos of left most alien
  int Across = 0;
  int Down;
  int16_t Smallest = SCREEN_WIDTH * 2;
  while (Across < NUM_ALIEN_COLUMNS)
  {
    Down = 0;
    while (Down < NUM_ALIEN_ROWS)
    {
      if (Alien[Across][Down].Ord.Status == ACTIVE)
        if (Alien[Across][Down].Ord.X < Smallest)
          Smallest = Alien[Across][Down].Ord.X;
      Down++;
    }
    if (Smallest < SCREEN_WIDTH * 2) // we have found smalest for this coloum
    {
      break;
    }
    Across++;
  }
  return Smallest;
}

void SpaceInvaders::LoseLife()
{
  Player.Lives--;
  if (Player.Lives > 0)
  {
    DisplayPlayerAndLives(&Player);
    // clear alien missiles
    for (int i = 0; i < MAXBOMBS; i++)
    {
      AlienBomb[i].Status = DESTROYED;
      AlienBomb[i].Y = 0;
    }
    // ENABLE PLAYER
    Player.Ord.Status = ACTIVE;
    Player.Ord.X = PLAYER_X_START;
  }
  else
  {
    GameOver();
  }
}

void SpaceInvaders::GameOver()
{
  GameInPlay = false;
  display->clearBuffer();
  CentreText("Player 1", 8);
  CentreText("Game Over", 20);
  CentreText("Score     ", 32);
  display->setCursor(72, 32);
  display->print(Player.Score);
  if (Player.Score > HiScore)
  {
    CentreText("NEW HIGH SCORE!!!", 44);
    CentreText("**CONGRATULATIONS**", 56);
  }
  display->sendBuffer();
  if (Player.Score > HiScore)
  {
    HiScore = Player.Score;
    EEPROM.put(0, HiScore);
#if defined(ESP8266) || defined(ESP32)
    EEPROM.commit();
#endif
    PlayRewardMusic();
  }
  delay(2500);
}

void SpaceInvaders::PlayRewardMusic()
{
  unsigned char Notes[] = {26, 20, 18, 22, 20, 0, 26, 0, 26};
  unsigned char NoteDurations[] = {40, 20, 20, 40, 30, 50, 30, 10, 30};
  for (int i = 0; i < 9; i++)
  {
    tone(PIN_BUZZER, Notes[i] * 10);
    delay(NoteDurations[i] * 10); // time note plays for
    NO_TONE(PIN_BUZZER);          // stop note
    delay(20);                    // small delay between notes
  }
  NO_TONE(PIN_BUZZER);
}

void SpaceInvaders::DisplayPlayerAndLives(PlayerStruct *Player)
{
  display->clearBuffer();
  CentreText("  Player 1", 8);
  CentreText("Score ", 20);
  display->print(Player->Score);
  CentreText("Lives ", 32);
  display->print(Player->Lives);
  CentreText("Level ", 44);
  display->print(Player->Level);
  display->sendBuffer();
  delay(2000);
  Player->Ord.X = PLAYER_X_START;
}

void SpaceInvaders::CentreText(const char *Text, unsigned char Y)
{
  // centres text on screen
  display->setCursor(int((SCREEN_WIDTH - display->getStrWidth(Text)) / 2.0), Y);
  display->print(Text);
}

void SpaceInvaders::NextLevel(PlayerStruct *Player)
{ // reset any dropping bombs

  int YStart;
  for (int i = 0; i < MAXBOMBS; i++)
    AlienBomb[i].Status = DESTROYED;
  AnimationFrame = false;
  Player->Level++;
  YStart = (MOTHERSHIP_HEIGHT + 2) + ((Player->Level - 1) % LEVEL_TO_RESET_TO_START_HEIGHT) * AMOUNT_TO_DROP_BY_PER_LEVEL;
  InitAliens(YStart);
  AlienXMoveAmount = ALIEN_X_MOVE_AMOUNT;
  Player->AlienSpeed = INVADERS_SPEED;
  Player->AliensDestroyed = 0;
  MotherShip.Ord.X = -MOTHERSHIP_WIDTH;
  MotherShip.Ord.Status = DESTROYED;
  Missile.Status = DESTROYED;
  randomSeed(100);
  InitBases();
  DisplayPlayerAndLives(Player);
  MusicIndex = 0;
  MusicCounter = NOTELENGTH;
}

void SpaceInvaders::NewGame()
{
  InitPlayer();
  NextLevel(&Player);
}

void SpaceInvaders::draw_digit(int16_t x, int16_t y, uint8_t n)
{
  display->drawXBMP(x, y, 3, 5, digit[n % 10]);
}

void SpaceInvaders::print_int(int16_t x, int16_t y, uint16_t s, uint8_t digit)
{
  x += 4 * (digit - 1);
  for (; digit > 0; digit--)
  {
    draw_digit(x, y, s % 10);
    x -= 4;
    s /= 10;
  }
}
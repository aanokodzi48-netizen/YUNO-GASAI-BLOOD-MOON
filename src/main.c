#include <genesis.h>
#include "resources.h"

#define PLAYER_MIN_X 4
#define PLAYER_MAX_X 300
#define PLAYER_MIN_Y 140
#define PLAYER_MAX_Y 174
#define SPRITE_VRAM_TILES 128

typedef enum
{
    STATE_SPLASH,
    STATE_MAIN_MENU,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_PAUSED
} GameState;

typedef struct
{
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} RoomBlock;

typedef enum
{
    ROOM_OBJECT_NONE,
    ROOM_OBJECT_LAMP,
    ROOM_OBJECT_BED,
    ROOM_OBJECT_WARDROBE
} RoomObject;

/* Invisible floor-level collision footprints, aligned to the 320x224 room art. */
static const RoomBlock roomBlocks[] = {
    {  0, 138,  28,  39 }, /* entry door / frame */
    { 43, 126,  62,  47 }, /* wardrobe base down to floor */
    {178, 140,  45,  32 }, /* bedside cabinet base */
    {216, 144, 102,  32 }, /* bed frame and base */
    {287, 126,  33,  46 }  /* bookcase base */
};

static Sprite *yunoSprite = NULL;
static bool spritesInitialized = FALSE;
static s16 playerX = 144;
static s16 playerY = 154;
static u16 animClock = 0;
static u16 oldPad = 0;
static u16 runButton = BUTTON_B;
static u8 mainSelection = 0;
static u8 settingsSelection = 0;
static u16 navPrevious = 0;
static u8 navRepeat = 0;
static GameState gameState = STATE_SPLASH;

static bool lampOn = TRUE;
static u16 messageTimer = 0;
static u16 sleepTimer = 0;
static u8 lastPrompt = 255;

static const u16 goldPalette[16] = {
    0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
    0x000,0x000,0x000,0x000,0x000,0x000,0x000,
    RGB24_TO_VDPCOLOR(0xFFD84D)
};

static const u16 whitePalette[16] = {
    0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
    0x000,0x000,0x000,0x000,0x000,0x000,0x000,
    RGB24_TO_VDPCOLOR(0xF4DDF8)
};

static void drawLine(const char *label, u16 x, u16 y, bool selected)
{
    VDP_setTextPalette(selected ? PAL1 : PAL2);
    VDP_drawText(label, x, y);
}

static void clearTitleText(void)
{
    VDP_clearTextArea(0, 18, 40, 10);
}

static bool drawTitleBackground(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setTextPlane(BG_A);

    if (!VDP_drawImageEx(BG_B, &title,
                         TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                         0, 0, TRUE, TRUE))
        return FALSE;

    PAL_setPalette(PAL1, goldPalette, DMA);
    PAL_setPalette(PAL2, whitePalette, DMA);
    VDP_setTextPalette(PAL2);
    return TRUE;
}

static void showFatalError(const char *message)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setTextPalette(PAL1);
    VDP_drawText(message, 7, 13);
    while (1)
        SYS_doVBlankProcess();
}

static void drawSplash(void)
{
    if (!drawTitleBackground())
        showFatalError("TITLE IMAGE ERROR");

    clearTitleText();
    drawLine("PRESS START", 14, 26, TRUE);
    VDP_setTextPalette(PAL2);
}

static void drawMainMenu(void)
{
    clearTitleText();
    drawLine("YUNO GASAI: BLOOD MOON", 9, 20, FALSE);
    drawLine(mainSelection == 0 ? "> START" : "  START",
             16, 23, mainSelection == 0);
    drawLine(mainSelection == 1 ? "> SETTINGS" : "  SETTINGS",
             14, 25, mainSelection == 1);
    VDP_setTextPalette(PAL2);
}

static void enterMainMenuFromSplash(void)
{
    gameState = STATE_MAIN_MENU;
    mainSelection = 0;
    navPrevious = 0;
    navRepeat = 0;

    if (!drawTitleBackground())
        showFatalError("TITLE IMAGE ERROR");
    drawMainMenu();
}

static void cycleRunButton(s16 direction)
{
    u16 next;

    if (runButton == BUTTON_A)
        next = (direction > 0) ? BUTTON_B : BUTTON_C;
    else if (runButton == BUTTON_B)
        next = (direction > 0) ? BUTTON_C : BUTTON_A;
    else
        next = (direction > 0) ? BUTTON_A : BUTTON_B;

    runButton = next;
}

static char getButtonLetter(u16 button)
{
    if (button == BUTTON_A) return 'A';
    if (button == BUTTON_B) return 'B';
    return 'C';
}

/* A is the default interact button. If A is assigned to RUN, use C instead. */
static u16 getInteractionButton(void)
{
    return (runButton == BUTTON_A) ? BUTTON_C : BUTTON_A;
}

static void drawSettings(void)
{
    char line[32];

    clearTitleText();
    drawLine("SETTINGS", 16, 18, FALSE);

    sprintf(line, "%c RUN BUTTON: %c",
            settingsSelection == 0 ? '>' : ' ',
            getButtonLetter(runButton));
    drawLine(line, 12, 21, settingsSelection == 0);

    drawLine(settingsSelection == 1 ? "> RESET DEFAULT" : "  RESET DEFAULT",
             12, 23, settingsSelection == 1);
    drawLine(settingsSelection == 2 ? "> BACK" : "  BACK",
             17, 25, settingsSelection == 2);

    drawLine("A/START: SELECT  B: BACK", 8, 26, FALSE);
    drawLine("LEFT/RIGHT: CHANGE", 11, 27, FALSE);
    VDP_setTextPalette(PAL2);
}

static bool menuDirection(u16 pad, u16 *direction)
{
    u16 current = pad & (BUTTON_UP | BUTTON_DOWN);

    if (current == 0)
    {
        navPrevious = 0;
        navRepeat = 0;
        return FALSE;
    }

    if (current != navPrevious)
    {
        navPrevious = current;
        navRepeat = 0;
        *direction = current;
        return TRUE;
    }

    if (navRepeat < 12)
    {
        navRepeat++;
        return FALSE;
    }

    navRepeat = 7;
    *direction = current;
    return TRUE;
}

/* Room art is 320x224, 4-bit indexed. Both lamp states share one palette
   and are loaded into the same user-tile range; sprite tiles are reserved
   separately by SPR_initEx(128). */
static bool drawRoomBackground(void)
{
    VDP_clearPlane(BG_B, TRUE);

    if (lampOn)
    {
        PAL_setPalette(PAL3, room_on.palette->data, DMA);
        return VDP_drawImageEx(BG_B, &room_on,
                               TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                               0, 0, TRUE, TRUE);
    }

    PAL_setPalette(PAL3, room_off.palette->data, DMA);
    return VDP_drawImageEx(BG_B, &room_off,
                           TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                           0, 0, TRUE, TRUE);
}

static bool overlapsRoomBlock(s16 x, s16 y, const RoomBlock *block)
{
    /* Compact collision box around the character's feet, not the full sprite. */
    const s16 hitLeft = x + 3;
    const s16 hitTop = y + 10;
    const s16 hitWidth = 10;
    const s16 hitHeight = 6;

    return (hitLeft < block->x + block->w) &&
           (hitLeft + hitWidth > block->x) &&
           (hitTop < block->y + block->h) &&
           (hitTop + hitHeight > block->y);
}

static bool playerBlockedAt(s16 x, s16 y)
{
    u16 i;

    if ((x < PLAYER_MIN_X) || (x > PLAYER_MAX_X) ||
        (y < PLAYER_MIN_Y) || (y > PLAYER_MAX_Y))
        return TRUE;

    for (i = 0; i < (sizeof(roomBlocks) / sizeof(roomBlocks[0])); i++)
    {
        if (overlapsRoomBlock(x, y, &roomBlocks[i]))
            return TRUE;
    }

    return FALSE;
}

static void movePlayer(u16 pad)
{
    s16 speed = (pad & runButton) ? 2 : 1;
    s16 nextX = playerX;
    s16 nextY = playerY;

    if (pad & BUTTON_LEFT) nextX -= speed;
    else if (pad & BUTTON_RIGHT) nextX += speed;

    if ((nextX != playerX) && !playerBlockedAt(nextX, playerY))
        playerX = nextX;

    if (pad & BUTTON_UP) nextY -= speed;
    else if (pad & BUTTON_DOWN) nextY += speed;

    if ((nextY != playerY) && !playerBlockedAt(playerX, nextY))
        playerY = nextY;

    if (pad & (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN))
        animClock++;
    else
        animClock = 0;

    SPR_setAnimAndFrame(yunoSprite, 1, (animClock >> 3) & 3);
    SPR_setPosition(yunoSprite, playerX, playerY);
}

static s32 distanceSquared(s16 x1, s16 y1, s16 x2, s16 y2)
{
    s32 dx = x1 - x2;
    s32 dy = y1 - y2;
    return (dx * dx) + (dy * dy);
}

static RoomObject getNearbyObject(void)
{
    const s16 centerX = playerX + 8;
    const s16 centerY = playerY + 13;
    s32 distance;
    s32 bestDistance = 0x7FFFFFFF;
    RoomObject nearest = ROOM_OBJECT_NONE;

    distance = distanceSquared(centerX, centerY, 201, 143);
    if ((distance <= (48L * 48L)) && (distance < bestDistance))
    {
        bestDistance = distance;
        nearest = ROOM_OBJECT_LAMP;
    }

    distance = distanceSquared(centerX, centerY, 263, 155);
    if ((distance <= (66L * 66L)) && (distance < bestDistance))
    {
        bestDistance = distance;
        nearest = ROOM_OBJECT_BED;
    }

    distance = distanceSquared(centerX, centerY, 75, 144);
    if ((distance <= (49L * 49L)) && (distance < bestDistance))
    {
        bestDistance = distance;
        nearest = ROOM_OBJECT_WARDROBE;
    }

    return nearest;
}

static void showRoomMessage(const char *message, u16 frames)
{
    VDP_clearTextArea(0, 25, 40, 3);
    VDP_setTextPalette(PAL1);
    VDP_drawText(message, 1, 26);
    VDP_setTextPalette(PAL2);
    messageTimer = frames;
    lastPrompt = 255;
}

static void updateContextPrompt(RoomObject object)
{
    char line[32];

    if ((messageTimer > 0) || (sleepTimer > 0) || (lastPrompt == (u8)object))
        return;

    VDP_clearTextArea(0, 25, 40, 3);

    if (object == ROOM_OBJECT_LAMP)
        sprintf(line, "%c: LIGHT ON/OFF", getButtonLetter(getInteractionButton()));
    else if (object == ROOM_OBJECT_BED)
        sprintf(line, "%c: SLEEP", getButtonLetter(getInteractionButton()));
    else if (object == ROOM_OBJECT_WARDROBE)
        sprintf(line, "%c: WARDROBE (SOON)", getButtonLetter(getInteractionButton()));
    else
        line[0] = 0;

    if (line[0] != 0)
    {
        VDP_setTextPalette(PAL1);
        VDP_drawText(line, 1, 26);
        VDP_setTextPalette(PAL2);
    }

    lastPrompt = (u8)object;
}

static void wakeFromSleep(void)
{
    VDP_clearTextArea(30, 13, 5, 3);

    playerX = 205;
    playerY = 166;
    SPR_setPosition(yunoSprite, playerX, playerY);
    SPR_setVisibility(yunoSprite, VISIBLE);

    sleepTimer = 0;
    showRoomMessage("GOOD MORNING!", 90);
}

static void interactWithObject(RoomObject object)
{
    if (object == ROOM_OBJECT_LAMP)
    {
        lampOn = !lampOn;
        if (!drawRoomBackground())
            showFatalError("ROOM IMAGE ERROR");

        showRoomMessage(lampOn ? "LIGHT TURNED ON" : "LIGHT TURNED OFF", 90);
    }
    else if (object == ROOM_OBJECT_BED)
    {
        /* Hide the current placeholder sprite on the bed; add a sleep pose later. */
        lampOn = FALSE;
        if (!drawRoomBackground())
            showFatalError("ROOM IMAGE ERROR");

        playerX = 255;
        playerY = 137;
        SPR_setPosition(yunoSprite, playerX, playerY);
        SPR_setVisibility(yunoSprite, HIDDEN);

        VDP_setTextPalette(PAL1);
        VDP_drawText("Zz", 32, 14);
        VDP_setTextPalette(PAL2);

        sleepTimer = 180;
        showRoomMessage("YUNO IS RESTING...", 180);
    }
    else if (object == ROOM_OBJECT_WARDROBE)
    {
        /* Wardrobe is present and solid; its clothing menu is a later step. */
        showRoomMessage("WARDROBE COMING SOON", 90);
    }
}

static bool startGame(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    SPR_initEx(SPRITE_VRAM_TILES);
    spritesInitialized = TRUE;

    lampOn = TRUE;
    messageTimer = 0;
    sleepTimer = 0;
    lastPrompt = 255;

    if (!drawRoomBackground())
    {
        SPR_end();
        spritesInitialized = FALSE;
        return FALSE;
    }

    playerX = 144;
    playerY = 154;
    animClock = 0;

    yunoSprite = SPR_addSprite(&yuno, playerX, playerY,
                               TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    if (yunoSprite == NULL)
    {
        SPR_end();
        spritesInitialized = FALSE;
        return FALSE;
    }

    PAL_setPalette(PAL2, yuno.palette->data, DMA);
    SPR_setVisibility(yunoSprite, VISIBLE);
    gameState = STATE_PLAYING;
    return TRUE;
}

static void drawPause(void)
{
    VDP_clearTextArea(12, 11, 16, 6);
    VDP_setTextPalette(PAL1);
    VDP_drawText("---- PAUSED ----", 13, 12);
    VDP_drawText("START: RESUME", 14, 14);
    VDP_drawText("B: MAIN MENU", 15, 16);
}

static void returnToMainMenu(void)
{
    if (spritesInitialized)
    {
        SPR_end();
        spritesInitialized = FALSE;
    }

    yunoSprite = NULL;
    messageTimer = 0;
    sleepTimer = 0;
    lastPrompt = 255;
    gameState = STATE_MAIN_MENU;
    mainSelection = 0;
    navPrevious = 0;
    navRepeat = 0;

    if (!drawTitleBackground())
        showFatalError("TITLE IMAGE ERROR");
    drawMainMenu();
}

int main(bool hardReset)
{
    u16 pad, pressed, direction = 0;
    RoomObject nearby;
    (void) hardReset;

    /* Rebuild all state explicitly after a Mega Drive / MD.emu reset. */
    gameState = STATE_SPLASH;
    spritesInitialized = FALSE;
    yunoSprite = NULL;
    playerX = 144;
    playerY = 154;
    animClock = 0;
    mainSelection = 0;
    settingsSelection = 0;
    navPrevious = 0;
    navRepeat = 0;
    lampOn = TRUE;
    messageTimer = 0;
    sleepTimer = 0;
    lastPrompt = 255;

    VDP_setBackgroundColor(0);
    VDP_setTextPlane(BG_A);

    JOY_reset();
    JOY_update();
    oldPad = JOY_readJoypad(JOY_1);
    drawSplash();

    while (1)
    {
        pad = JOY_readJoypad(JOY_1);
        pressed = pad & (u16)~oldPad;
        oldPad = pad;

        if (gameState == STATE_SPLASH)
        {
            if (pad & (BUTTON_START | BUTTON_A))
                enterMainMenuFromSplash();
        }
        else if (gameState == STATE_MAIN_MENU)
        {
            if (menuDirection(pad, &direction))
            {
                if (direction & BUTTON_UP) mainSelection = 0;
                else if (direction & BUTTON_DOWN) mainSelection = 1;
                drawMainMenu();
            }

            if (pressed & (BUTTON_A | BUTTON_START))
            {
                if (mainSelection == 0)
                {
                    if (!startGame())
                        showFatalError("ROOM OR SPRITE ERROR");
                }
                else
                {
                    gameState = STATE_SETTINGS;
                    settingsSelection = 0;
                    navPrevious = 0;
                    navRepeat = 0;
                    drawSettings();
                }
            }
        }
        else if (gameState == STATE_SETTINGS)
        {
            if (pressed & BUTTON_B)
            {
                gameState = STATE_MAIN_MENU;
                mainSelection = 0;
                navPrevious = 0;
                navRepeat = 0;
                drawMainMenu();
            }
            else
            {
                if (menuDirection(pad, &direction))
                {
                    if ((direction & BUTTON_UP) && settingsSelection > 0)
                        settingsSelection--;
                    else if ((direction & BUTTON_DOWN) && settingsSelection < 2)
                        settingsSelection++;
                    drawSettings();
                }

                if ((pressed & BUTTON_LEFT) && settingsSelection == 0)
                {
                    cycleRunButton(-1);
                    drawSettings();
                }
                else if ((pressed & BUTTON_RIGHT) && settingsSelection == 0)
                {
                    cycleRunButton(1);
                    drawSettings();
                }
                else if (pressed & (BUTTON_A | BUTTON_START))
                {
                    if (settingsSelection == 0)
                    {
                        cycleRunButton(1);
                        drawSettings();
                    }
                    else if (settingsSelection == 1)
                    {
                        runButton = BUTTON_B;
                        drawSettings();
                    }
                    else
                    {
                        gameState = STATE_MAIN_MENU;
                        mainSelection = 0;
                        navPrevious = 0;
                        navRepeat = 0;
                        drawMainMenu();
                    }
                }
            }
        }
        else if (gameState == STATE_PLAYING)
        {
            if (sleepTimer > 0)
            {
                sleepTimer--;
                if (sleepTimer == 0)
                    wakeFromSleep();
            }
            else if (pressed & BUTTON_START)
            {
                gameState = STATE_PAUSED;
                drawPause();
            }
            else
            {
                movePlayer(pad);
                nearby = getNearbyObject();

                if ((messageTimer == 0) &&
                    (pressed & getInteractionButton()) &&
                    (nearby != ROOM_OBJECT_NONE))
                {
                    interactWithObject(nearby);
                }
            }

            if (messageTimer > 0)
            {
                messageTimer--;
                if ((messageTimer == 0) && (sleepTimer == 0))
                {
                    lastPrompt = 255;
                    updateContextPrompt(getNearbyObject());
                }
            }
            else if (sleepTimer == 0)
            {
                updateContextPrompt(getNearbyObject());
            }
        }
        else if (gameState == STATE_PAUSED)
        {
            if (pressed & (BUTTON_START | BUTTON_A))
            {
                gameState = STATE_PLAYING;
                VDP_clearTextArea(12, 11, 16, 6);
                lastPrompt = 255;
            }
            else if (pressed & BUTTON_B)
            {
                returnToMainMenu();
            }
        }

        if (spritesInitialized)
            SPR_update();

        SYS_doVBlankProcess();
    }

    return 0;
}

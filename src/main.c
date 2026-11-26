#include <genesis.h>
#include "resources.h"

#define PLAYER_MIN_X 8
#define PLAYER_MAX_X 286
#define PLAYER_MIN_Y 76
#define PLAYER_MAX_Y 166
#define SPRITE_VRAM_TILES 128

typedef enum
{
    STATE_SPLASH,
    STATE_MAIN_MENU,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_PAUSED
} GameState;

static Sprite *yunoSprite = NULL;
static bool spritesInitialized = FALSE;
static s16 playerX = 144;
static s16 playerY = 150;
static u16 animClock = 0;
static u16 oldPad = 0;
static u16 runButton = BUTTON_B;
static u8 mainSelection = 0;
static u8 settingsSelection = 0;
static u16 navPrevious = 0;
static u8 navRepeat = 0;
static GameState gameState = STATE_SPLASH;

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

    /* Keep menu text palettes independent from the title artwork. */
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

    /* The menu stays on the Yuno title art; the room is not loaded yet. */
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

static bool startGame(void)
{
    /* Reserve sprite VRAM before loading the room image.
       This prevents background tiles and sprite tiles from overlapping. */
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    SPR_initEx(SPRITE_VRAM_TILES);
    spritesInitialized = TRUE;

    if (!VDP_drawImageEx(BG_B, &room,
                         TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                         0, 0, TRUE, TRUE))
    {
        SPR_end();
        spritesInitialized = FALSE;
        return FALSE;
    }

    playerX = 144;
    playerY = 150;
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
    s16 speed;
    (void) hardReset;

    VDP_setBackgroundColor(0);
    VDP_setTextPlane(BG_A);

    /* SGDK initializes the joypad during system startup. */
    oldPad = 0;
    drawSplash();

    while (1)
    {
        pad = JOY_readJoypad(JOY_1);
        pressed = pad & (u16)~oldPad;
        oldPad = pad;

        if (gameState == STATE_SPLASH)
        {
            if (pressed & (BUTTON_START | BUTTON_A))
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
            if (pressed & BUTTON_START)
            {
                gameState = STATE_PAUSED;
                drawPause();
            }
            else
            {
                speed = (pad & runButton) ? 2 : 1;

                if (pad & BUTTON_LEFT)  playerX -= speed;
                if (pad & BUTTON_RIGHT) playerX += speed;
                if (pad & BUTTON_UP)    playerY -= speed;
                if (pad & BUTTON_DOWN)  playerY += speed;

                if (playerX < PLAYER_MIN_X) playerX = PLAYER_MIN_X;
                if (playerX > PLAYER_MAX_X) playerX = PLAYER_MAX_X;
                if (playerY < PLAYER_MIN_Y) playerY = PLAYER_MIN_Y;
                if (playerY > PLAYER_MAX_Y) playerY = PLAYER_MAX_Y;

                if (pad & (BUTTON_LEFT | BUTTON_RIGHT | BUTTON_UP | BUTTON_DOWN))
                    animClock++;
                else
                    animClock = 0;

                SPR_setAnimAndFrame(yunoSprite, 1, (animClock >> 3) & 3);
                SPR_setPosition(yunoSprite, playerX, playerY);
            }
        }
        else if (gameState == STATE_PAUSED)
        {
            if (pressed & (BUTTON_START | BUTTON_A))
            {
                gameState = STATE_PLAYING;
                VDP_clearTextArea(12, 11, 16, 6);
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

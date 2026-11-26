#include <genesis.h>
#include "resources.h"

#define PLAYER_MIN_X 8
#define PLAYER_MAX_X 286
#define PLAYER_MIN_Y 76
#define PLAYER_MAX_Y 166

typedef enum
{
    STATE_SPLASH,
    STATE_MAIN_MENU,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_PAUSED
} GameState;

static Sprite *yunoSprite;
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

static void clearMenuArea(void)
{
    VDP_clearTextArea(7, 4, 26, 19);
}

static void drawSplash(void)
{
    VDP_clearTextArea(0, 0, 40, 28);
    VDP_drawImageEx(BG_B, &splash,
                    TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                    0, 0, TRUE, TRUE);
    drawLine("PRESS START", 14, 26, TRUE);
    VDP_setTextPalette(PAL2);
}

static void showRoomBackground(void)
{
    VDP_clearTextArea(0, 0, 40, 28);
    VDP_drawImageEx(BG_B, &room,
                    TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                    0, 0, TRUE, TRUE);
}

static void drawMainMenu(void)
{
    clearMenuArea();
    drawLine("+--------------------------+", 7, 5, FALSE);
    drawLine("|       YUNO GASAI        |", 7, 6, TRUE);
    drawLine("|        BLOOD MOON       |", 7, 7, FALSE);
    drawLine("|                        |", 7, 8, FALSE);
    drawLine("|                        |", 7, 9, FALSE);
    drawLine("|                        |", 7, 10, FALSE);
    drawLine("|                        |", 7, 11, FALSE);
    drawLine("|                        |", 7, 12, FALSE);
    drawLine("|                        |", 7, 13, FALSE);
    drawLine("|                        |", 7, 14, FALSE);
    drawLine("|                        |", 7, 15, FALSE);
    drawLine("|                        |", 7, 16, FALSE);
    drawLine("+--------------------------+", 7, 17, FALSE);
    drawLine(mainSelection == 0 ? "> START" : "  START", 15, 11, mainSelection == 0);
    drawLine(mainSelection == 1 ? "> SETTINGS" : "  SETTINGS", 14, 13, mainSelection == 1);
    VDP_setTextPalette(PAL2);
}

static void enterMainMenu(void)
{
    gameState = STATE_MAIN_MENU;
    mainSelection = 0;
    showRoomBackground();
    drawMainMenu();
}

static void drawSettings(void)
{
    char line[32];
    const char *buttonName = (runButton == BUTTON_A) ? "A" :
                             (runButton == BUTTON_C) ? "C" : "B";

    clearMenuArea();
    drawLine("+--------------------------+", 7, 5, FALSE);
    drawLine("|         SETTINGS        |", 7, 6, TRUE);
    drawLine("|                        |", 7, 7, FALSE);
    drawLine("|                        |", 7, 8, FALSE);
    drawLine("|                        |", 7, 9, FALSE);
    drawLine("|                        |", 7, 10, FALSE);
    drawLine("|                        |", 7, 11, FALSE);
    drawLine("|                        |", 7, 12, FALSE);
    drawLine("|                        |", 7, 13, FALSE);
    drawLine("|                        |", 7, 14, FALSE);
    drawLine("|                        |", 7, 15, FALSE);
    drawLine("|                        |", 7, 16, FALSE);
    drawLine("+--------------------------+", 7, 17, FALSE);

    sprintf(line, "%c RUN BUTTON: %s", settingsSelection == 0 ? '>' : ' ', buttonName);
    drawLine(line, 11, 9, settingsSelection == 0);
    drawLine(settingsSelection == 1 ? "> RESET DEFAULT" : "  RESET DEFAULT", 12, 11, settingsSelection == 1);
    drawLine(settingsSelection == 2 ? "> BACK" : "  BACK", 16, 13, settingsSelection == 2);
    drawLine("A: SELECT  B: BACK", 11, 15, FALSE);
    drawLine("LEFT/RIGHT: CHANGE", 11, 16, FALSE);
    VDP_setTextPalette(PAL2);
}

static void cycleRunButton(void)
{
    if (runButton == BUTTON_B) runButton = BUTTON_A;
    else if (runButton == BUTTON_A) runButton = BUTTON_C;
    else runButton = BUTTON_B;
}

/* Direction repeat: immediate on press, then repeat after a short delay. */
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

    navRepeat = 8;
    *direction = current;
    return TRUE;
}

static void drawPause(void)
{
    VDP_clearTextArea(12, 11, 16, 6);
    drawLine("---- PAUSED ----", 13, 12, TRUE);
    drawLine("START: RESUME", 13, 14, FALSE);
    drawLine("B: MAIN MENU", 14, 16, FALSE);
}

int main(bool hardReset)
{
    u16 pad, pressed, direction = 0;
    s16 speed;
    (void) hardReset;

    /* Initialize the controller before the first menu input is sampled. */
    JOY_init();
    SYS_doVBlankProcess();
    oldPad = JOY_readJoypad(JOY_1);

    VDP_setBackgroundColor(0);
    PAL_setPalette(PAL1, goldPalette, DMA);
    PAL_setPalette(PAL2, whitePalette, DMA);
    VDP_setTextPalette(PAL2);

    SPR_init();
    yunoSprite = SPR_addSprite(&yuno, playerX, playerY, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    if (yunoSprite == NULL)
    {
        VDP_drawText("SPRITE ERROR", 14, 13);
        while (1) SYS_doVBlankProcess();
    }
    PAL_setPalette(PAL0, yuno.palette->data, DMA);
    SPR_setVisibility(yunoSprite, HIDDEN);

    gameState = STATE_SPLASH;
    drawSplash();

    while (1)
    {
        pad = JOY_readJoypad(JOY_1);
        pressed = pad & (u16)~oldPad;
        oldPad = pad;

        if (gameState == STATE_SPLASH)
        {
            if (pressed & (BUTTON_START | BUTTON_A))
                enterMainMenu();
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
                    gameState = STATE_PLAYING;
                    VDP_clearTextArea(0, 0, 40, 28);
                    SPR_setVisibility(yunoSprite, VISIBLE);
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

                if ((pressed & (BUTTON_LEFT | BUTTON_RIGHT)) && settingsSelection == 0)
                {
                    cycleRunButton();
                    drawSettings();
                }
                else if (pressed & (BUTTON_A | BUTTON_START))
                {
                    if (settingsSelection == 0)
                    {
                        cycleRunButton();
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
                SPR_setVisibility(yunoSprite, HIDDEN);
                enterMainMenu();
            }
        }

        SPR_update();
        SYS_doVBlankProcess();
    }

    return 0;
}

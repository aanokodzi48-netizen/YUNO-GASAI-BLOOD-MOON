#include <genesis.h>
#include "resources.h"

#define PLAYER_MIN_X 4
#define PLAYER_MAX_X 300
#define PLAYER_MIN_Y 140
#define PLAYER_MAX_Y 174
#define SPRITE_VRAM_TILES 128
#define WINDOW_TILE_X 13
#define WINDOW_TILE_Y 4
#define WINDOW_TILE_W 14
#define WINDOW_TILE_H 8
#define UI_TILE_FILL 1
#define UI_TILE_TOP 2
#define UI_TILE_BOTTOM 3
#define UI_TILE_LEFT 4
#define UI_TILE_RIGHT 5
#define UI_TILE_TOP_LEFT 6
#define UI_TILE_TOP_RIGHT 7
#define UI_TILE_BOTTOM_LEFT 8
#define UI_TILE_BOTTOM_RIGHT 9
#define UI_GLYPH_BASE 10

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
    {  8, 138,  42,  39 }, /* entry door */
    { 54, 126,  51,  47 }, /* wardrobe base */
    {169, 140,  45,  32 }, /* bedside cabinet base */
    {207, 144, 108,  32 }, /* bed frame and base */
    {280, 126,  36,  46 }  /* bookcase base */
};

static Sprite *yunoSprite = NULL;
static Sprite *lampGlowSprite = NULL;
static Sprite *skyFxSprite = NULL;
static bool spritesInitialized = FALSE;
static u16 windowTileBase = 0;
static u16 uiTileBase = 0;
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
static bool isMorning = FALSE;
static u8 clockHour = 19;
static u8 clockMinute = 45;
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

static const u16 uiPalette[16] = {
    0x0000,
    RGB24_TO_VDPCOLOR(0xFFFFDB),
    RGB24_TO_VDPCOLOR(0xDB6DB6),
    RGB24_TO_VDPCOLOR(0x000024),
    RGB24_TO_VDPCOLOR(0x49246D),
    RGB24_TO_VDPCOLOR(0x924992),
    RGB24_TO_VDPCOLOR(0xDBB692),
    RGB24_TO_VDPCOLOR(0xFFDBB6),
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
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

/* Load the static room once. Lamp and time changes only alter small
   sprites/tilemap cells; the full 320x224 background is never DMA-reloaded. */
static bool drawRoomBackground(void)
{
    u16 nextTile;

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    nextTile = TILE_USER_INDEX + room.tileset->numTile;
    if (nextTile > (TILE_USER_MAX_INDEX + 1))
        return FALSE;
    if (window_morning.tileset->numTile > (TILE_USER_MAX_INDEX + 1 - nextTile))
        return FALSE;

    if (!VDP_drawImageEx(BG_B, &room,
                         TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                         0, 0, TRUE, TRUE))
        return FALSE;

    windowTileBase = nextTile;
    if (!VDP_loadTileSet(window_morning.tileset, windowTileBase, DMA))
        return FALSE;

    nextTile = windowTileBase + window_morning.tileset->numTile;
    if (ui_tiles.numTile > (TILE_USER_MAX_INDEX + 1 - nextTile))
        return FALSE;
    uiTileBase = nextTile;
    if (!VDP_loadTileSet(&ui_tiles, uiTileBase, DMA))
        return FALSE;

    PAL_setPalette(PAL0, window_morning.palette->data, CPU);
    PAL_setPalette(PAL1, uiPalette, CPU);
    PAL_setPalette(PAL3, room.palette->data, CPU);
    return TRUE;
}

static bool drawMorningWindow(void)
{
    VDP_clearTileMapRect(BG_A, WINDOW_TILE_X, WINDOW_TILE_Y,
                         WINDOW_TILE_W, WINDOW_TILE_H);

    if (!isMorning)
        return TRUE;

    return VDP_setTileMapEx(BG_A, window_morning.tilemap,
                            TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, windowTileBase),
                            WINDOW_TILE_X, WINDOW_TILE_Y,
                            0, 0, WINDOW_TILE_W, WINDOW_TILE_H, CPU);
}

static void updateLampGlow(void)
{
    if (lampGlowSprite != NULL)
        SPR_setVisibility(lampGlowSprite, lampOn ? VISIBLE : HIDDEN);
}

static void updateSkySprite(void)
{
    if (skyFxSprite != NULL)
    {
        if (isMorning)
            SPR_setVisibility(skyFxSprite, HIDDEN);
        else
        {
            SPR_setAnimAndFrame(skyFxSprite, 0, 0);
            SPR_setVisibility(skyFxSprite, VISIBLE);
        }
    }
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

    distance = distanceSquared(centerX, centerY, 191, 120);
    if ((distance <= (50L * 50L)) && (distance < bestDistance))
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

    distance = distanceSquared(centerX, centerY, 80, 125);
    if ((distance <= (50L * 50L)) && (distance < bestDistance))
    {
        bestDistance = distance;
        nearest = ROOM_OBJECT_WARDROBE;
    }

    return nearest;
}

static u16 uiTile(u16 index)
{
    return TILE_ATTR_FULL(PAL1, TRUE, FALSE, FALSE, uiTileBase + index);
}

static u16 nextUiCodepoint(const u8 **cursor)
{
    u8 first = *(*cursor)++;

    if (first < 0x80)
        return first;

    if ((first & 0xE0) == 0xC0)
    {
        u8 second = *(*cursor)++;
        return (u16)(((first & 0x1F) << 6) | (second & 0x3F));
    }

    return ' ';
}

static u16 uiGlyphIndex(u16 codepoint)
{
    if ((codepoint >= 0x0410) && (codepoint <= 0x0415))
        return UI_GLYPH_BASE + (codepoint - 0x0410);
    if (codepoint == 0x0401)
        return UI_GLYPH_BASE + 6;
    if ((codepoint >= 0x0416) && (codepoint <= 0x042F))
        return UI_GLYPH_BASE + (codepoint - 0x0410) + 1;

    if (codepoint == 'A') return UI_GLYPH_BASE + 33;
    if (codepoint == 'B') return UI_GLYPH_BASE + 34;
    if (codepoint == 'C') return UI_GLYPH_BASE + 35;
    if ((codepoint >= '0') && (codepoint <= '9'))
        return UI_GLYPH_BASE + 36 + (codepoint - '0');
    if (codepoint == ':') return UI_GLYPH_BASE + 46;
    if (codepoint == '.') return UI_GLYPH_BASE + 47;
    if (codepoint == '-') return UI_GLYPH_BASE + 48;
    if (codepoint == '!') return UI_GLYPH_BASE + 49;
    return UI_GLYPH_BASE + 50; /* space / safe fallback */
}

static u16 uiTextLength(const char *text)
{
    const u8 *cursor = (const u8*)text;
    u16 count = 0;

    while (*cursor)
    {
        nextUiCodepoint(&cursor);
        count++;
    }

    return count;
}

static void drawUiText(const char *text, u16 x, u16 y)
{
    const u8 *cursor = (const u8*)text;

    while (*cursor && (x < 40))
    {
        u16 codepoint = nextUiCodepoint(&cursor);
        VDP_setTileMapXY(BG_A, uiTile(uiGlyphIndex(codepoint)), x, y);
        x++;
    }
}

static void drawUiPanel(u16 x, u16 y, u16 w, u16 h)
{
    if ((w < 3) || (h < 3))
        return;

    VDP_fillTileMapRect(BG_A, uiTile(UI_TILE_FILL), x, y, w, h);
    VDP_fillTileMapRect(BG_A, uiTile(UI_TILE_TOP), x + 1, y, w - 2, 1);
    VDP_fillTileMapRect(BG_A, uiTile(UI_TILE_BOTTOM), x + 1, y + h - 1, w - 2, 1);
    VDP_fillTileMapRect(BG_A, uiTile(UI_TILE_LEFT), x, y + 1, 1, h - 2);
    VDP_fillTileMapRect(BG_A, uiTile(UI_TILE_RIGHT), x + w - 1, y + 1, 1, h - 2);

    VDP_setTileMapXY(BG_A, uiTile(UI_TILE_TOP_LEFT), x, y);
    VDP_setTileMapXY(BG_A, uiTile(UI_TILE_TOP_RIGHT), x + w - 1, y);
    VDP_setTileMapXY(BG_A, uiTile(UI_TILE_BOTTOM_LEFT), x, y + h - 1);
    VDP_setTileMapXY(BG_A, uiTile(UI_TILE_BOTTOM_RIGHT), x + w - 1, y + h - 1);
}

static void clearMessagePanel(void)
{
    VDP_clearTileMapRect(BG_A, 1, 24, 38, 4);
}

static void drawMessagePanel(const char *message)
{
    u16 length = uiTextLength(message);
    u16 x = 1 + ((38 - length) / 2);

    drawUiPanel(1, 24, 38, 4);
    drawUiText(message, x, 25);
}

static void drawClockPanel(void)
{
    char line[24];
    u16 length;

    sprintf(line, "ВРЕМЯ %02d:%02d", clockHour, clockMinute);
    length = uiTextLength(line);
    drawUiPanel(26, 1, 13, 3);
    drawUiText(line, 26 + ((13 - length) / 2), 2);
}

static void showRoomMessage(const char *message, u16 frames)
{
    drawMessagePanel(message);
    messageTimer = frames;
    lastPrompt = 255;
}

static void updateContextPrompt(RoomObject object)
{
    char line[40];

    if ((messageTimer > 0) || (sleepTimer > 0) || (lastPrompt == (u8)object))
        return;

    if (object == ROOM_OBJECT_LAMP)
    {
        if (lampOn)
            sprintf(line, "%c: ВЫКЛЮЧИТЬ СВЕТ", getButtonLetter(getInteractionButton()));
        else
            sprintf(line, "%c: ВКЛЮЧИТЬ СВЕТ", getButtonLetter(getInteractionButton()));
    }
    else if (object == ROOM_OBJECT_BED)
        sprintf(line, "%c: ЛЕЧЬ СПАТЬ", getButtonLetter(getInteractionButton()));
    else if (object == ROOM_OBJECT_WARDROBE)
        sprintf(line, "%c: ОТКРЫТЬ ШКАФ", getButtonLetter(getInteractionButton()));
    else
        line[0] = 0;

    if (line[0] != 0)
        drawMessagePanel(line);
    else
        clearMessagePanel();

    lastPrompt = (u8)object;
}

static void wakeFromSleep(void)
{
    /* A short rest advances the scene from evening to morning. */
    isMorning = TRUE;
    clockHour = 7;
    clockMinute = 0;
    lampOn = FALSE;
    updateLampGlow();
    updateSkySprite();

    if (!drawMorningWindow())
        showFatalError("MORNING WINDOW ERROR");
    drawClockPanel();

    playerX = 205;
    playerY = 166;
    SPR_setPosition(yunoSprite, playerX, playerY);
    SPR_setVisibility(yunoSprite, VISIBLE);

    sleepTimer = 0;
    showRoomMessage("ДОБРОЕ УТРО ЮНА!", 90);
}

static void interactWithObject(RoomObject object)
{
    if (object == ROOM_OBJECT_LAMP)
    {
        /* Do not reload the full room bitmap: toggle only the small glow sprite. */
        lampOn = !lampOn;
        updateLampGlow();
        showRoomMessage(lampOn ? "СВЕТ ВКЛЮЧЕН" : "СВЕТ ВЫКЛЮЧЕН", 90);
    }
    else if (object == ROOM_OBJECT_BED)
    {
        lampOn = FALSE;
        updateLampGlow();

        playerX = 255;
        playerY = 137;
        SPR_setPosition(yunoSprite, playerX, playerY);
        SPR_setVisibility(yunoSprite, HIDDEN);

        sleepTimer = 180;
        showRoomMessage("ЮНА ОТДЫХАЕТ...", 180);
    }
    else if (object == ROOM_OBJECT_WARDROBE)
    {
        /* Wardrobe is present and solid; clothing selection is a later step. */
        showRoomMessage("ГАРДЕРОБ ПОЗЖЕ", 90);
    }
}

static bool startGame(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    SPR_initEx(SPRITE_VRAM_TILES);
    spritesInitialized = TRUE;

    lampOn = TRUE;
    isMorning = FALSE;
    clockHour = 19;
    clockMinute = 45;
    messageTimer = 0;
    sleepTimer = 0;
    lastPrompt = 255;
    lampGlowSprite = NULL;
    skyFxSprite = NULL;

    if (!drawRoomBackground())
    {
        SPR_end();
        spritesInitialized = FALSE;
        return FALSE;
    }

    if (!drawMorningWindow())
    {
        SPR_end();
        spritesInitialized = FALSE;
        return FALSE;
    }

    /* FX first, player last: Yuno remains visually in front of light effects. */
    lampGlowSprite = SPR_addSprite(&lamp_glow, 174, 94,
                                   TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    skyFxSprite = SPR_addSprite(&sky_fx, 176, 33,
                                TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    if ((lampGlowSprite == NULL) || (skyFxSprite == NULL))
    {
        SPR_end();
        spritesInitialized = FALSE;
        lampGlowSprite = NULL;
        skyFxSprite = NULL;
        return FALSE;
    }

    SPR_setAnimAndFrame(skyFxSprite, 0, 0);
    updateLampGlow();

    playerX = 144;
    playerY = 154;
    animClock = 0;

    yunoSprite = SPR_addSprite(&yuno, playerX, playerY,
                               TILE_ATTR(PAL2, TRUE, FALSE, FALSE));
    if (yunoSprite == NULL)
    {
        SPR_end();
        spritesInitialized = FALSE;
        lampGlowSprite = NULL;
        skyFxSprite = NULL;
        return FALSE;
    }

    PAL_setPalette(PAL2, yuno.palette->data, CPU);
    drawClockPanel();
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
    lampGlowSprite = NULL;
    skyFxSprite = NULL;
    windowTileBase = 0;
    uiTileBase = 0;
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
    lampGlowSprite = NULL;
    skyFxSprite = NULL;
    windowTileBase = 0;
    uiTileBase = 0;
    playerX = 144;
    playerY = 154;
    animClock = 0;
    mainSelection = 0;
    settingsSelection = 0;
    navPrevious = 0;
    navRepeat = 0;
    lampOn = TRUE;
    isMorning = FALSE;
    clockHour = 19;
    clockMinute = 45;
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
                    clearMessagePanel();
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

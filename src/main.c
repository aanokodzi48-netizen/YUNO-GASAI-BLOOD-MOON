#include <genesis.h>
#include "resources.h"

#define PLAYER_MIN_X 4
#define PLAYER_MAX_X 288
#define PLAYER_MIN_Y 140
#define PLAYER_MAX_Y 188
#define SPRITE_VRAM_TILES 128
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
#define FRAMES_PER_GAME_MINUTE 120
#define INTRO_FRAME_COUNT 10
#define INTRO_FRAME_TICKS 6
#define INTRO_WIDTH 208
#define INTRO_HEIGHT 144
#define INTRO_TILE_WIDTH (INTRO_WIDTH / 8)
#define INTRO_TILE_HEIGHT (INTRO_HEIGHT / 8)
#define INTRO_TILE_COUNT (INTRO_TILE_WIDTH * INTRO_TILE_HEIGHT)
#define INTRO_TILE_X 7
#define INTRO_TILE_Y 5
#define INTRO_CHUNK_TILES 128
#define INTRO_BUFFER_A TILE_USER_INDEX
#define INTRO_BUFFER_B (TILE_USER_INDEX + INTRO_TILE_COUNT)

typedef enum
{
    STATE_INTRO,
    STATE_SPLASH,
    STATE_MAIN_MENU,
    STATE_SETTINGS,
    STATE_PLAYING,
    STATE_PAUSED
} GameState;

typedef enum
{
    ROOM_OBJECT_NONE,
    ROOM_OBJECT_LAMP,
    ROOM_OBJECT_BED,
    ROOM_OBJECT_WARDROBE
} RoomObject;

static Sprite *yunoSprite = NULL;
static bool spritesInitialized = FALSE;
static const Image *introFrames[INTRO_FRAME_COUNT] = {
    &intro_00, &intro_01, &intro_02, &intro_03, &intro_04,
    &intro_05, &intro_06, &intro_07, &intro_08, &intro_09
};
static u8 introFrame = 0;              /* frame currently visible */
static u8 introFrameTick = 0;
static u8 introUploadFrame = 0;         /* frame being prepared */
static u8 introUploadBuffer = 0;        /* hidden VRAM buffer: 0 or 1 */
static u16 introUploadOffset = 0;
static u16 introUploadTileCount = 0;
static u16 introUploadBase = INTRO_BUFFER_A;
static bool introUploadActive = FALSE;
static bool introUploadReady = FALSE;
static bool introMapPending = FALSE;
static bool introVisible = FALSE;
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
static GameState gameState = STATE_INTRO;

static bool lampOn = FALSE;
static bool isMorning = FALSE;
static u8 clockHour = 19;
static u8 clockMinute = 45;
static u16 messageTimer = 0;
static u16 sleepTimer = 0;
static u8 lastPrompt = 255;
static u16 minuteFrameCounter = 0;

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

/* PAL3 matches the empty-room 16-color image palette. */
static const u16 roomLampPalette[16] = {
    0x0000,
    RGB24_TO_VDPCOLOR(0x240024),
    RGB24_TO_VDPCOLOR(0x240049),
    RGB24_TO_VDPCOLOR(0x490049),
    RGB24_TO_VDPCOLOR(0x49246D),
    RGB24_TO_VDPCOLOR(0x6D246D),
    RGB24_TO_VDPCOLOR(0x924992),
    RGB24_TO_VDPCOLOR(0xB649B6),
    RGB24_TO_VDPCOLOR(0xDB6DB6),
    RGB24_TO_VDPCOLOR(0x492400),
    RGB24_TO_VDPCOLOR(0x6D2400),
    RGB24_TO_VDPCOLOR(0x924924),
    RGB24_TO_VDPCOLOR(0xB66D24),
    RGB24_TO_VDPCOLOR(0xDB9249),
    RGB24_TO_VDPCOLOR(0xFFB66D),
    RGB24_TO_VDPCOLOR(0xFFDBB6)
};

static const u16 roomNightPalette[16] = {
    0x0000,
    RGB24_TO_VDPCOLOR(0x000024),
    RGB24_TO_VDPCOLOR(0x000049),
    RGB24_TO_VDPCOLOR(0x240024),
    RGB24_TO_VDPCOLOR(0x240049),
    RGB24_TO_VDPCOLOR(0x240049),
    RGB24_TO_VDPCOLOR(0x492449),
    RGB24_TO_VDPCOLOR(0x49246D),
    RGB24_TO_VDPCOLOR(0x6D246D),
    RGB24_TO_VDPCOLOR(0x240000),
    RGB24_TO_VDPCOLOR(0x240000),
    RGB24_TO_VDPCOLOR(0x492400),
    RGB24_TO_VDPCOLOR(0x6D2400),
    RGB24_TO_VDPCOLOR(0x924924),
    RGB24_TO_VDPCOLOR(0xB64949),
    RGB24_TO_VDPCOLOR(0xDB6D6D)
};

static const u16 roomDayPalette[16] = {
    0x0000,
    RGB24_TO_VDPCOLOR(0x492449),
    RGB24_TO_VDPCOLOR(0x49246D),
    RGB24_TO_VDPCOLOR(0x6D246D),
    RGB24_TO_VDPCOLOR(0x6D4992),
    RGB24_TO_VDPCOLOR(0x924992),
    RGB24_TO_VDPCOLOR(0xB66DB6),
    RGB24_TO_VDPCOLOR(0xDB6DDB),
    RGB24_TO_VDPCOLOR(0xFF92DB),
    RGB24_TO_VDPCOLOR(0x6D4924),
    RGB24_TO_VDPCOLOR(0x924924),
    RGB24_TO_VDPCOLOR(0xB66D49),
    RGB24_TO_VDPCOLOR(0xDB9249),
    RGB24_TO_VDPCOLOR(0xFFB66D),
    RGB24_TO_VDPCOLOR(0xFFDB92),
    RGB24_TO_VDPCOLOR(0xFFFFDB)
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

    PAL_setPalette(PAL0, title.palette->data, DMA);
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

/* ---------- Smooth centered intro with double-buffered VRAM streaming ---------- */

/* Each source frame is 208x144 = 26x18 = 468 tiles.
   We upload one 192-tile chunk (6144 bytes) per VBlank to stay below the
   NTSC DMA budget. The inactive VRAM buffer is filled while the current
   image remains visible, then the tilemap is switched at VBlank. */

static u16 introBufferBase(u8 buffer)
{
    return buffer ? INTRO_BUFFER_B : INTRO_BUFFER_A;
}

static bool validateIntroResources(void)
{
    u16 i;

    /* SGDK removes duplicate 8x8 tiles from IMAGE resources. Therefore
       numTile can be smaller than the full 26x18 tilemap. The VRAM buffers
       reserve the maximum size, but uploads must use each frame's real count. */
    if ((introFrames[0]->tileset->numTile == 0) ||
        (introFrames[0]->tileset->numTile > INTRO_TILE_COUNT))
        return FALSE;

    if ((INTRO_BUFFER_B + INTRO_TILE_COUNT) > (TILE_USER_MAX_INDEX + 1))
        return FALSE;

    for (i = 0; i < INTRO_FRAME_COUNT; i++)
    {
        if ((introFrames[i]->tileset->numTile == 0) ||
            (introFrames[i]->tileset->numTile > INTRO_TILE_COUNT))
            return FALSE;
    }

    return TRUE;
}

static void beginIntroUpload(u8 frame, u8 buffer)
{
    introUploadFrame = frame;
    introUploadBuffer = buffer;
    introUploadBase = introBufferBase(buffer);
    introUploadOffset = 0;
    introUploadTileCount = introFrames[frame]->tileset->numTile;
    introUploadActive = TRUE;
    introUploadReady = FALSE;
}

/* Queues only one small upload per game frame; SYS_doVBlankProcess flushes it. */
static void queueIntroUploadChunk(void)
{
    u16 remaining;
    u16 count;

    if (!introUploadActive || introMapPending || introUploadReady)
        return;

    remaining = introUploadTileCount - introUploadOffset;
    count = (remaining > INTRO_CHUNK_TILES) ? INTRO_CHUNK_TILES : remaining;

    VDP_loadTileData(
        FAR_SAFE(introFrames[introUploadFrame]->tileset->tiles +
                 ((u32)introUploadOffset * 8),
                 (u32)count * 32),
        introUploadBase,
        count,
        DMA_QUEUE
    );

    introUploadOffset += count;
}

/* Queue the new tilemap only after the hidden frame's tile data has arrived. */
static void queueIntroMapCommit(void)
{
    if (!introUploadReady || introMapPending)
        return;

    if (!VDP_setTileMapEx(BG_B,
                          introFrames[introUploadFrame]->tilemap,
                          TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, introUploadBase),
                          INTRO_TILE_X, INTRO_TILE_Y,
                          0, 0,
                          INTRO_TILE_WIDTH, INTRO_TILE_HEIGHT,
                          DMA_QUEUE_COPY))
        showFatalError("INTRO MAP QUEUE ERROR");

    introUploadReady = FALSE;
    introMapPending = TRUE;
}

static void leaveIntro(void)
{
    /* Cancel pending writes so no intro DMA can corrupt the splash screen. */
    DMA_clearQueue();
    introUploadActive = FALSE;
    introUploadReady = FALSE;
    introMapPending = FALSE;
    introVisible = FALSE;

    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setBackgroundColor(0);

    gameState = STATE_SPLASH;
    drawSplash();
}

/* Called immediately after SYS_doVBlankProcess: all queued writes have landed. */
static void completeIntroVBlank(void)
{
    if (introUploadActive && (introUploadOffset >= introUploadTileCount))
    {
        introUploadActive = FALSE;
        introUploadReady = TRUE;
    }

    if (introMapPending)
    {
        introMapPending = FALSE;
        introFrame = introUploadFrame;
        introVisible = TRUE;
        introFrameTick = 0;

        /* Prepare the following frame in the other buffer. */
        if (introFrame + 1 < INTRO_FRAME_COUNT)
            beginIntroUpload(introFrame + 1, (u8)(introUploadBuffer ^ 1));
    }
}

static bool startIntro(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setBackgroundColor(0);
    VDP_setTextPlane(BG_A);
    DMA_clearQueue();

    if (!validateIntroResources())
        return FALSE;

    PAL_setPalette(PAL0, intro_00.palette->data, DMA);

    introFrame = 0;
    introFrameTick = 0;
    introVisible = FALSE;
    introMapPending = FALSE;
    beginIntroUpload(0, 0);
    return TRUE;
}

static void updateIntroAnimation(void)
{
    if (!introVisible)
    {
        /* Initial frame is prepared while the screen remains clean black. */
        if (introUploadReady)
            queueIntroMapCommit();
    }
    else
    {
        introFrameTick++;

        if (introFrameTick >= INTRO_FRAME_TICKS)
        {
            introFrameTick = 0;

            if (introFrame == (INTRO_FRAME_COUNT - 1))
            {
                leaveIntro();
                return;
            }

            /* If upload has not quite finished, keep displaying the old frame.
               No blank frame, tearing, or partially uploaded artwork is shown. */
            if (introUploadReady)
                queueIntroMapCommit();
        }
    }

    queueIntroUploadChunk();
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

    if (!VDP_drawImageEx(BG_B, &room,
                         TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX),
                         0, 0, TRUE, TRUE))
        return FALSE;

    nextTile = TILE_USER_INDEX + room.tileset->numTile;
    if ((nextTile > (TILE_USER_MAX_INDEX + 1)) ||
        (ui_tiles.numTile > (TILE_USER_MAX_INDEX + 1 - nextTile)))
        return FALSE;

    uiTileBase = nextTile;
    if (!VDP_loadTileSet(&ui_tiles, uiTileBase, DMA))
        return FALSE;

    PAL_setPalette(PAL1, uiPalette, CPU);
    PAL_setPalette(PAL3, room.palette->data, CPU);
    return TRUE;
}

static void updateRoomLighting(void)
{
    /* The whole room responds to the lamp/time of day through PAL3. This is
       instantaneous and does not reload the room tiles, avoiding the old
       purple/blank-frame glitch during interaction. */
    bool daytime = (clockHour >= 7 && clockHour < 18);

    if (daytime)
        PAL_setPalette(PAL3, roomDayPalette, CPU);
    else if ((clockHour >= 18) && (clockHour < 20))
        PAL_setPalette(PAL3, roomLampPalette, CPU);
    else
        PAL_setPalette(PAL3, roomNightPalette, CPU);

}

static bool playerBlockedAt(s16 x, s16 y)
{
    /* The room is deliberately empty for this stage. Only screen/floor bounds remain. */
    return ((x < PLAYER_MIN_X) || (x > PLAYER_MAX_X) ||
            (y < PLAYER_MIN_Y) || (y > PLAYER_MAX_Y));
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

static RoomObject getNearbyObject(void)
{
    /* The room contains no interactable objects yet. */
    return ROOM_OBJECT_NONE;
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

static void updateGameClock(void)
{
    bool wasDaytime = (clockHour >= 7 && clockHour < 18);
    bool isDaytime;

    minuteFrameCounter++;
    if (minuteFrameCounter < FRAMES_PER_GAME_MINUTE)
        return;

    minuteFrameCounter = 0;
    clockMinute++;
    if (clockMinute >= 60)
    {
        clockMinute = 0;
        clockHour++;
        if (clockHour >= 24)
            clockHour = 0;
    }

    isMorning = (clockHour >= 7 && clockHour < 18);
    isDaytime = isMorning;

    /* Refresh the clock every game minute. Rebuild only the window overlay
       and palettes when crossing the day/night boundary. */
    drawClockPanel();
    if (wasDaytime != isDaytime)
    {
        updateRoomLighting();
    }
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
    minuteFrameCounter = 0;
    lampOn = FALSE;
    updateRoomLighting();
    drawClockPanel();

    playerX = 184;
    playerY = 179;
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
        updateRoomLighting();
        showRoomMessage(lampOn ? "СВЕТ ВКЛЮЧЕН" : "СВЕТ ВЫКЛЮЧЕН", 90);
    }
    else if (object == ROOM_OBJECT_BED)
    {
        lampOn = FALSE;
        updateRoomLighting();

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

    lampOn = FALSE;
    isMorning = FALSE;
    clockHour = 19;
    clockMinute = 45;
    minuteFrameCounter = 0;
    messageTimer = 0;
    sleepTimer = 0;
    lastPrompt = 255;

    if (!drawRoomBackground())
    {
        SPR_end();
        spritesInitialized = FALSE;
        return FALSE;
    }

    updateRoomLighting();

    /* The room is empty: no furniture, window, lamp or moon sprites are spawned. */

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
    gameState = STATE_INTRO;
    spritesInitialized = FALSE;
    yunoSprite = NULL;
    uiTileBase = 0;
    playerX = 144;
    playerY = 154;
    animClock = 0;
    mainSelection = 0;
    settingsSelection = 0;
    navPrevious = 0;
    navRepeat = 0;
    lampOn = FALSE;
    isMorning = FALSE;
    clockHour = 19;
    clockMinute = 45;
    minuteFrameCounter = 0;
    messageTimer = 0;
    sleepTimer = 0;
    lastPrompt = 255;

    VDP_setBackgroundColor(0);
    VDP_setTextPlane(BG_A);

    JOY_reset();
    JOY_update();
    oldPad = JOY_readJoypad(JOY_1);
    if (!startIntro())
        showFatalError("INTRO IMAGE ERROR");

    while (1)
    {
        pad = JOY_readJoypad(JOY_1);
        pressed = pad & (u16)~oldPad;
        oldPad = pad;

        if (gameState == STATE_INTRO)
        {
            if (pressed & (BUTTON_START | BUTTON_A))
                leaveIntro();
            else
                updateIntroAnimation();
        }
        else if (gameState == STATE_SPLASH)
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

            if (sleepTimer == 0)
                updateGameClock();

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

        if (gameState == STATE_INTRO)
            completeIntroVBlank();
    }

    return 0;
}

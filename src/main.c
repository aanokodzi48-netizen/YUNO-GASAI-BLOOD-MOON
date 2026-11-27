#include <genesis.h>
#include "resources.h"
#include "yuno_metasprite.h"

#define PLAYER_MIN_X 0
#define PLAYER_MIN_Y 0
#define PLAYER_MAX_Y 104
#define PLAYER_STEP 2
#define SPRITE_VRAM_TILES 128
#define RU_GLYPH_COUNT 51
#define PHONE_TILE_BASE TILE_USER_INDEX
#define PHONE_TILE_COUNT 16
#define PHONE_WIDTH 20
#define PHONE_HEIGHT 26

typedef enum {
    STATE_SPLASH,
    STATE_MAIN_MENU,
    STATE_CONTROLS,
    STATE_PLAYING,
    STATE_PHONE_CONTACTS,
    STATE_PHONE_CHAT
} GameState;

static bool spritesInitialized=FALSE;
static s16 playerX=140, playerY=52;
static u16 oldPad=0;
static u8 mainSelection=0, navRepeat=0;
static u16 navPrevious=0;
static GameState gameState=STATE_SPLASH;
static u16 ruFontTileBase=0;
static u32 ruFontTiles[RU_GLYPH_COUNT*8];

static const u8 ruGlyphRows[RU_GLYPH_COUNT][8]={
{0,48,120,120,104,252,204,0},
{0,60,32,60,38,38,60,0},
{0,60,44,60,38,38,60,0},
{0,60,32,32,32,32,32,0},
{124,108,108,108,108,254,198,0},
{0,60,32,60,32,32,60,0},
{0,0,94,124,126,219,0,0},
{0,124,12,60,12,12,124,0},
{0,76,92,92,124,108,108,0},
{56,76,92,92,124,108,108,0},
{0,76,120,112,120,76,76,0},
{0,60,44,44,108,108,204,0},
{0,102,110,126,94,90,66,0},
{0,76,76,124,76,76,76,0},
{0,120,108,198,198,108,120,0},
{0,124,76,76,76,76,76,0},
{0,60,38,38,60,32,32,0},
{0,60,54,96,96,54,60,0},
{0,126,24,24,24,24,24,0},
{0,102,60,60,28,24,48,0},
{0,24,126,219,219,126,24,0},
{0,204,120,48,56,120,204,0},
{76,76,76,76,76,126,2,0},
{0,102,102,62,6,6,6,0},
{0,0,219,219,219,255,0,0},
{0,214,214,214,254,127,0,0},
{0,240,112,60,62,54,62,0},
{0,67,67,123,79,79,123,0},
{0,32,32,60,38,38,60,0},
{120,108,62,62,12,124,0,0},
{0,222,251,243,219,222,0,0},
{0,62,54,54,30,54,54,0},
{56,60,32,60,32,32,60,0},
{0,96,120,12,120,96,0,0},
{0,0,24,24,24,24,0,0},
{0,0,56,0,0,0,0,0},
{0,0,0,16,16,0,0,0},
{0,204,120,48,56,120,204,0},
{0,60,54,96,96,54,60,0},
{0,56,108,108,108,108,56,0},
{0,56,24,24,24,24,60,0},
{0,120,12,12,24,48,124,0},
{0,60,12,60,12,12,120,0},
{0,28,28,60,108,124,12,0},
{0,60,32,60,12,12,124,0},
{0,60,48,124,124,60,60,0},
{0,124,12,24,24,24,48,0},
{0,60,108,60,108,108,60,0},
{0,56,108,108,60,12,56,0},
{0,16,120,120,16,0,0,0},
{0,60,56,124,40,252,112,0}
};

static const u16 goldPalette[16]={
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    RGB24_TO_VDPCOLOR(0xFFD84D)
};
static const u16 whitePalette[16]={
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    RGB24_TO_VDPCOLOR(0xFFFFFF)
};
static const u16 phonePalette[16]={
    RGB24_TO_VDPCOLOR(0x000000),
    RGB24_TO_VDPCOLOR(0x07121F),
    RGB24_TO_VDPCOLOR(0x101E33),
    RGB24_TO_VDPCOLOR(0x0B3B50),
    RGB24_TO_VDPCOLOR(0x147A8C),
    RGB24_TO_VDPCOLOR(0x25B9C4),
    RGB24_TO_VDPCOLOR(0x062B3A),
    RGB24_TO_VDPCOLOR(0x08121E),
    RGB24_TO_VDPCOLOR(0x145A80),
    RGB24_TO_VDPCOLOR(0x000000),
    RGB24_TO_VDPCOLOR(0x2B6572),
    RGB24_TO_VDPCOLOR(0x5DD3D5),
    RGB24_TO_VDPCOLOR(0xD6FFFF),
    RGB24_TO_VDPCOLOR(0x194B60),
    RGB24_TO_VDPCOLOR(0x3A91A0),
    RGB24_TO_VDPCOLOR(0xFFFFFF)
};

static u16 screenColumns(void){return (u16)(VDP_getScreenWidth()>>3);}
static u16 screenRows(void){return (u16)(VDP_getScreenHeight()>>3);}

static void buildRussianFont(void)
{
    u8 *dst=(u8*)ruFontTiles;
    u16 g,row,col;
    for(g=0;g<RU_GLYPH_COUNT;g++)
    {
        for(row=0;row<8;row++)
        {
            for(col=0;col<8;col++)
            {
                u8 pixel=(ruGlyphRows[g][row]&(1u<<(7u-col)))?15:0;
                u16 offset=(u16)(g*32u+row*4u+(col>>1));
                if((col&1u)==0) dst[offset]=(u8)(pixel<<4);
                else dst[offset]|=pixel;
            }
        }
    }
    ruFontTileBase=TILE_FONT_INDEX;
    VDP_loadFontData((const u32*)ruFontTiles,RU_GLYPH_COUNT,DMA);
}

static u16 decodeRussianCodepoint(const u8 **cursor)
{
    const u8 *p=*cursor;
    u16 cp;
    if(*p<0x80u){(*cursor)++;return *p;}
    if((p[0]&0xE0u)==0xC0u && (p[1]&0xC0u)==0x80u)
    {
        cp=(u16)(((p[0]&0x1Fu)<<6)|(p[1]&0x3Fu));
        *cursor+=2;
        return cp;
    }
    if((p[0]&0xF0u)==0xE0u && (p[1]&0xC0u)==0x80u && (p[2]&0xC0u)==0x80u)
    {
        cp=(u16)(((p[0]&0x0Fu)<<12)|((p[1]&0x3Fu)<<6)|(p[2]&0x3Fu));
        *cursor+=3;
        return cp;
    }
    (*cursor)++;
    return 0;
}

static s16 russianGlyph(u16 cp)
{
    if(cp>=0x0410u && cp<=0x042Fu)return (s16)(cp-0x0410u);
    if(cp==0x0401u)return 32;
    if(cp=='>')return 33;
    if(cp==':')return 34;
    if(cp=='-')return 35;
    if(cp=='.')return 36;
    if(cp=='X')return 37;
    if(cp=='C')return 38;
    if(cp>='0' && cp<='9')return (s16)(39u+(cp-'0'));
    if(cp=='*')return 49;
    if(cp=='#')return 50;
    return -1;
}

static u16 russianTextLength(const char *text)
{
    const u8 *p=(const u8*)text;
    u16 count=0;
    while(*p){decodeRussianCodepoint(&p);count++;}
    return count;
}

static void drawRussianText(const char *text,u16 x,u16 y,bool selected)
{
    const u8 *p=(const u8*)text;
    u16 palette=selected?PAL1:PAL2;
    u16 columns=screenColumns();
    while(*p && x<columns)
    {
        u16 cp=decodeRussianCodepoint(&p);
        s16 glyph=russianGlyph(cp);
        if(cp!=' ' && glyph>=0)
            VDP_setTileMapXY(BG_A,
                TILE_ATTR_FULL(palette,TRUE,FALSE,FALSE,
                    (u16)(ruFontTileBase+(u16)glyph)),x,y);
        x++;
    }
}

static void drawCenteredRussian(const char *text,u16 y,bool selected)
{
    u16 len=russianTextLength(text),cols=screenColumns();
    drawRussianText(text,(len<cols)?(u16)((cols-len)>>1):0,y,selected);
}

static void clearMenuText(void)
{
    VDP_clearTileMapRect(BG_A,0,18,screenColumns(),10);
}

static bool drawTitleBackground(void)
{
    VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A,TRUE);
    VDP_clearPlane(BG_B,TRUE);
    VDP_setTextPlane(BG_A);
    if(!VDP_drawImageEx(BG_B,&title,
        TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),
        0,0,TRUE,TRUE))
    {
        VDP_waitDMACompletion();
        VDP_setEnable(TRUE);
        return FALSE;
    }
    PAL_setPalette(PAL0,title.palette->data,DMA);
    PAL_setPalette(PAL1,goldPalette,DMA);
    PAL_setPalette(PAL2,whitePalette,DMA);
    buildRussianFont();
    VDP_setTextPriority(TRUE);
    VDP_waitDMACompletion();
    VDP_setEnable(TRUE);
    return TRUE;
}

static void showFatalError(void)
{
    VDP_setEnable(TRUE);
    VDP_clearPlane(BG_A,TRUE);
    VDP_clearPlane(BG_B,TRUE);
    VDP_drawText("TITLE IMAGE ERROR",7,13);
    while(1)SYS_doVBlankProcess();
}

static void drawSplash(void)
{
    if(!drawTitleBackground())showFatalError();
    clearMenuText();
    drawCenteredRussian("НАЖМИТЕ СТАРТ",26,TRUE);
}

static void drawMainMenu(void)
{
    clearMenuText();
    drawCenteredRussian("ЮНО ГАСАЙ: КРОВАВАЯ ЛУНА",18,FALSE);
    drawCenteredRussian(mainSelection==0?"> СТАРТ":"СТАРТ",22,mainSelection==0);
    drawCenteredRussian(mainSelection==1?"> УПРАВЛЕНИЕ":"УПРАВЛЕНИЕ",25,mainSelection==1);
}

static void drawControls(void)
{
    clearMenuText();
    drawCenteredRussian("УПРАВЛЕНИЕ",18,FALSE);
    drawCenteredRussian("СТРЕЛКИ: ДВИЖЕНИЕ",21,FALSE);
    drawCenteredRussian("СТАРТ: ОТКРЫТЬ ТЕЛЕФОН",23,FALSE);
    drawCenteredRussian("X: ВЫБРАТЬ КОНТАКТ",25,FALSE);
    drawCenteredRussian("C: НАЗАД В ЧАТЕ",27,FALSE);
}

static void enterMainMenuFromSplash(void)
{
    gameState=STATE_MAIN_MENU;
    mainSelection=0;
    navPrevious=0;
    navRepeat=0;
    drawMainMenu();
}

static bool menuDirection(u16 pad,u16 *direction)
{
    u16 cur=pad&(BUTTON_UP|BUTTON_DOWN);
    if(!cur){navPrevious=0;navRepeat=0;return FALSE;}
    if(cur!=navPrevious){navPrevious=cur;navRepeat=0;*direction=cur;return TRUE;}
    if(navRepeat<12){navRepeat++;return FALSE;}
    navRepeat=7;
    *direction=cur;
    return TRUE;
}

static void drawBlankPlayfield(void)
{
    VDP_clearPlane(BG_A,TRUE);
    VDP_clearPlane(BG_B,TRUE);
    PAL_setPalette(PAL0,whitePalette,CPU);
    VDP_setBackgroundColor(15);
    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(TRUE);
}

static bool playerBlockedAt(s16 x,s16 y)
{
    s16 maxX=(s16)VDP_getScreenWidth()-40;
    return x<PLAYER_MIN_X || x>maxX || y<PLAYER_MIN_Y || y>PLAYER_MAX_Y;
}

static void movePlayer(u16 pad)
{
    s16 nx=playerX,ny=playerY;
    if(pad&BUTTON_LEFT)nx-=PLAYER_STEP;
    else if(pad&BUTTON_RIGHT)nx+=PLAYER_STEP;
    if(nx!=playerX && !playerBlockedAt(nx,playerY))playerX=nx;
    if(pad&BUTTON_UP)ny-=PLAYER_STEP;
    else if(pad&BUTTON_DOWN)ny+=PLAYER_STEP;
    if(ny!=playerY && !playerBlockedAt(playerX,ny))playerY=ny;
    YunoMeta_draw(playerX,playerY);
}

static YunoMetaResult startGame(void)
{
    YunoMetaResult result;
    VDP_setEnable(FALSE);
    DMA_clearQueue();
    if(!SPR_isInitialized())SPR_initEx(SPRITE_VRAM_TILES);
    spritesInitialized=SPR_isInitialized();
    if(!spritesInitialized){VDP_setEnable(TRUE);return YUNO_META_ERR_ENGINE_NOT_INITIALIZED;}
    drawBlankPlayfield();
    playerX=(s16)((VDP_getScreenWidth()-40u)>>1);
    playerY=52;
    result=YunoMeta_create(playerX,playerY);
    if(result!=YUNO_META_OK){VDP_setEnable(TRUE);return result;}
    YunoMeta_setVisible(TRUE);
    SPR_update();
    VDP_waitDMACompletion();
    VDP_setEnable(TRUE);
    gameState=STATE_PLAYING;
    return YUNO_META_OK;
}

static const char *getYunoMetaErrorText(YunoMetaResult r)
{
    switch(r)
    {
        case YUNO_META_ERR_ALREADY_CREATED:return "ЮНО УЖЕ СОЗДАНА";
        case YUNO_META_ERR_ENGINE_NOT_INITIALIZED:return "ОШИБКА ДВИЖКА";
        case YUNO_META_ERR_RESOURCE_DEFINITION:return "ОШИБКА РЕСУРСА";
        case YUNO_META_ERR_PALETTE_MISMATCH:return "ОШИБКА ПАЛИТРЫ";
        case YUNO_META_ERR_UNSUPPORTED_SCREEN_WIDTH:return "НЕВЕРНЫЙ РАЗМЕР ЭКРАНА";
        case YUNO_META_ERR_HARDWARE_LIMIT:return "ЛИМИТ СПРАЙТОВ";
        case YUNO_META_ERR_HARDWARE_COST_MISMATCH:return "ОШИБКА ЧАСТЕЙ ЮНО";
        case YUNO_META_ERR_POSITION_RANGE:return "НЕВЕРНАЯ ПОЗИЦИЯ";
        case YUNO_META_ERR_ALLOCATION:return "НЕТ ПАМЯТИ СПРАЙТОВ";
        default:return "НЕИЗВЕСТНАЯ ОШИБКА";
    }
}

static void showYunoMetaError(YunoMetaResult r)
{
    u16 pad,previous;
    VDP_setEnable(TRUE);
    if(!drawTitleBackground())showFatalError();
    clearMenuText();
    drawCenteredRussian("НЕ УДАЛОСЬ СОЗДАТЬ ЮНО",20,TRUE);
    drawCenteredRussian(getYunoMetaErrorText(r),23,FALSE);
    drawCenteredRussian("НАЖМИТЕ СТАРТ",26,TRUE);
    previous=JOY_readJoypad(JOY_1);
    while(previous&BUTTON_START){SYS_doVBlankProcess();previous=JOY_readJoypad(JOY_1);}
    while(1)
    {
        SYS_doVBlankProcess();
        pad=JOY_readJoypad(JOY_1);
        if((pad&BUTTON_START)&&!(previous&BUTTON_START))break;
        previous=pad;
    }
    gameState=STATE_MAIN_MENU;
    mainSelection=0;
    drawMainMenu();
}

static void phoneTile(u16 x,u16 y,u8 color)
{
    u16 tile=(u16)(PHONE_TILE_BASE+color);
    VDP_setTileMapXY(BG_A,
        TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,tile),x,y);
}

static void phoneFillRect(u16 x,u16 y,u16 w,u16 h,u8 color)
{
    u16 xx,yy;
    for(yy=y;yy<(u16)(y+h);yy++)
        for(xx=x;xx<(u16)(x+w);xx++)
            phoneTile(xx,yy,color);
}

static bool drawPhoneBase(void)
{
    u16 cols=screenColumns(),rows=screenRows();
    u16 px=(u16)((cols-PHONE_WIDTH)>>1);
    u16 py=(u16)((rows-PHONE_HEIGHT)>>1);
    u8 c;

    if((u16)(PHONE_TILE_BASE+PHONE_TILE_COUNT-1)>=TILE_SPRITE_INDEX)
        return FALSE;

    VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A,TRUE);
    PAL_setPalette(PAL1,phonePalette,DMA);
    PAL_setPalette(PAL2,whitePalette,DMA);
    for(c=0;c<PHONE_TILE_COUNT;c++)
        VDP_fillTileData((u8)((c<<4)|c),(u16)(PHONE_TILE_BASE+c),1,TRUE);

    VDP_setTextPlane(BG_A);
    VDP_setTextPriority(TRUE);
    phoneFillRect(0,0,cols,rows,1);

    /* Тень и бирюзовый корпус раскрытой раскладушки. */
    phoneFillRect((u16)(px-1),(u16)(py-1),22,28,2);
    phoneFillRect(px,py,PHONE_WIDTH,PHONE_HEIGHT,3);
    phoneFillRect((u16)(px+1),(u16)(py+1),18,24,4);
    phoneFillRect((u16)(px+2),(u16)(py+2),16,10,6);
    phoneFillRect((u16)(px+3),(u16)(py+3),14,8,7);

    /* Динамик, индикатор и шарнир. */
    phoneFillRect((u16)(px+8),(u16)(py+1),4,1,9);
    phoneFillRect((u16)(px+13),(u16)(py+1),1,1,11);
    phoneFillRect((u16)(px+1),(u16)(py+12),18,2,2);
    phoneFillRect((u16)(px+2),(u16)(py+12),16,1,10);

    /* Нижняя половина корпуса и чёрная клавиатура. */
    phoneFillRect((u16)(px+2),(u16)(py+15),16,10,4);
    phoneFillRect((u16)(px+3),(u16)(py+16),14,8,3);

    {
        static const char *keys[4][3]={{"1","2","3"},{"4","5","6"},{"7","8","9"},{"*","0","#"}};
        u16 row,col,kx,ky;
        for(row=0;row<4;row++)
        {
            for(col=0;col<3;col++)
            {
                kx=(u16)(px+3+col*5);
                ky=(u16)(py+16+row*2);
                phoneFillRect(kx,ky,4,2,10);
                phoneFillRect((u16)(kx+1),(u16)(ky+0),2,1,9);
                drawRussianText(keys[row][col],(u16)(kx+1),ky,FALSE);
            }
        }
    }

    VDP_waitDMACompletion();
    VDP_setEnable(TRUE);
    return TRUE;
}

static void drawPhoneContacts(void)
{
    u16 cols=screenColumns();
    u16 px=(u16)((cols-PHONE_WIDTH)>>1);
    u16 py=(u16)((screenRows()-PHONE_HEIGHT)>>1);
    if(!drawPhoneBase())
    {
        VDP_clearPlane(BG_A,TRUE);
        drawCenteredRussian("ОШИБКА ПАМЯТИ ТЕЛЕФОНА",13,TRUE);
        return;
    }
    phoneFillRect((u16)(px+3),(u16)(py+5),14,2,8);
    drawRussianText("КОНТАКТЫ",(u16)(px+6),(u16)(py+3),FALSE);
    drawRussianText("> ЮКИ",(u16)(px+5),(u16)(py+5),FALSE);
    drawRussianText("X: ВЫБРАТЬ",(u16)(px+4),(u16)(py+8),FALSE);
    drawRussianText("СТАРТ: ВЫХОД",(u16)(px+4),(u16)(py+9),FALSE);
}

static void drawPhoneChat(void)
{
    u16 cols=screenColumns();
    u16 px=(u16)((cols-PHONE_WIDTH)>>1);
    u16 py=(u16)((screenRows()-PHONE_HEIGHT)>>1);
    if(!drawPhoneBase())
    {
        VDP_clearPlane(BG_A,TRUE);
        drawCenteredRussian("ОШИБКА ПАМЯТИ ТЕЛЕФОНА",13,TRUE);
        return;
    }
    drawRussianText("ЧАТ: ЮКИ",(u16)(px+6),(u16)(py+3),FALSE);
    drawRussianText("СООБЩЕНИЙ НЕТ",(u16)(px+3),(u16)(py+6),FALSE);
    drawRussianText("C: НАЗАД",(u16)(px+4),(u16)(py+9),FALSE);
    drawRussianText("СТАРТ: ВЫХОД",(u16)(px+4),(u16)(py+10),FALSE);
}

static void closePhone(void)
{
    drawBlankPlayfield();
    YunoMeta_setVisible(TRUE);
    SPR_update();
    gameState=STATE_PLAYING;
}

int main(bool hardReset)
{
    u16 pad,pressed,direction=0;
    YunoMetaResult result;
    (void)hardReset;

    YunoMeta_destroy();
    spritesInitialized=SPR_isInitialized();
    gameState=STATE_SPLASH;
    playerX=140;
    playerY=52;
    mainSelection=0;
    navPrevious=0;
    navRepeat=0;

    VDP_setEnable(FALSE);
    VDP_setBackgroundColor(0);
    JOY_reset();
    JOY_update();
    oldPad=JOY_readJoypad(JOY_1);
    drawSplash();

    while(1)
    {
        pad=JOY_readJoypad(JOY_1);
        pressed=pad&(u16)~oldPad;
        oldPad=pad;

        if(gameState==STATE_SPLASH)
        {
            if(pressed&(BUTTON_START|BUTTON_A))enterMainMenuFromSplash();
        }
        else if(gameState==STATE_MAIN_MENU)
        {
            if(menuDirection(pad,&direction))
            {
                if((direction&BUTTON_UP)&&mainSelection>0)mainSelection--;
                else if((direction&BUTTON_DOWN)&&mainSelection<1)mainSelection++;
                drawMainMenu();
            }
            if(pressed&(BUTTON_A|BUTTON_START))
            {
                if(mainSelection==0)
                {
                    result=startGame();
                    if(result!=YUNO_META_OK)showYunoMetaError(result);
                }
                else
                {
                    gameState=STATE_CONTROLS;
                    navPrevious=0;
                    navRepeat=0;
                    drawControls();
                }
            }
        }
        else if(gameState==STATE_CONTROLS)
        {
            if(pressed&(BUTTON_B|BUTTON_C|BUTTON_START))
            {
                gameState=STATE_MAIN_MENU;
                mainSelection=0;
                navPrevious=0;
                navRepeat=0;
                drawMainMenu();
            }
        }
        else if(gameState==STATE_PLAYING)
        {
            if(pressed&BUTTON_START)
            {
                gameState=STATE_PHONE_CONTACTS;
                YunoMeta_setVisible(FALSE);
                drawPhoneContacts();
            }
            else movePlayer(pad);
        }
        else if(gameState==STATE_PHONE_CONTACTS)
        {
            if(pressed&BUTTON_START)closePhone();
            else if(pressed&BUTTON_X)
            {
                gameState=STATE_PHONE_CHAT;
                drawPhoneChat();
            }
        }
        else if(gameState==STATE_PHONE_CHAT)
        {
            if(pressed&BUTTON_START)closePhone();
            else if(pressed&BUTTON_C)
            {
                gameState=STATE_PHONE_CONTACTS;
                drawPhoneContacts();
            }
        }

        if(spritesInitialized)SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}

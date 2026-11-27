#include <genesis.h>
#include "resources.h"
#include "yuno_metasprite.h"
#define PLAYER_MIN_X 0
#define PLAYER_MIN_Y 0
#define PLAYER_MAX_Y 104
#define SPRITE_VRAM_TILES 128
#define RUN_SPEED 2
#define RU_GLYPH_COUNT 37
typedef enum { STATE_SPLASH, STATE_MAIN_MENU, STATE_SETTINGS, STATE_PLAYING, STATE_PAUSE } GameState;
static bool spritesInitialized=FALSE;
static s16 playerX=140,playerY=52;
static u16 oldPad=0,runButton=BUTTON_B;
static u8 mainSelection=0,settingsSelection=0,navRepeat=0;
static u16 navPrevious=0;
static GameState gameState=STATE_SPLASH;
static u16 ruFontTileBase=0;
static u32 ruFontTiles[RU_GLYPH_COUNT*8];
static const u8 ruGlyphRows[RU_GLYPH_COUNT][7]={
{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{30,17,17,30,17,17,30},
{31,16,16,16,16,16,16},{14,17,17,17,17,31,17},{31,16,16,30,16,16,31},
{10,21,21,31,21,21,21},{31,1,2,4,8,16,31},{17,19,21,25,17,17,17},
{4,21,21,21,21,21,10},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
{17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{17,17,10,4,4,4,4},{31,1,2,4,8,16,31},{14,17,17,17,17,17,14},
{30,17,17,30,17,17,30},{31,16,16,30,16,16,31},{17,17,17,17,17,17,31},
{17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
{31,1,2,4,8,16,31},{4,4,4,4,4,4,4},{31,0,0,0,0,0,0},{0,0,0,0,0,0,0}};
static const u16 goldPalette[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,RGB24_TO_VDPCOLOR(0xFFD84D)};
static const u16 whitePalette[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,RGB24_TO_VDPCOLOR(0xFFFFFF)};
static u16 screenColumns(void){return (u16)(VDP_getScreenWidth()>>3);}
static void buildRussianFont(void){
 u8 *dst=(u8*)ruFontTiles;u16 g,row,col;
 for(g=0;g<RU_GLYPH_COUNT;g++)for(row=0;row<8;row++)for(col=0;col<8;col++){
  u8 pixel=0;
  if((row>0)&&(col>0)&&(col<6)&&(ruGlyphRows[g][row-1]&(1u<<(5u-col))))pixel=15;
  if((col&1u)==0)dst[g*32u+row*4u+(col>>1)]=(u8)(pixel<<4);
  else dst[g*32u+row*4u+(col>>1)]|=pixel;
 }
 ruFontTileBase=(u16)(TILE_USER_INDEX+title.tileset->numTile);
 if((ruFontTileBase+RU_GLYPH_COUNT)>=2048u)ruFontTileBase=1900u;
 VDP_loadTileData((const u32*)ruFontTiles,ruFontTileBase,RU_GLYPH_COUNT,DMA);
}
static u16 decodeRussianCodepoint(const u8 **cursor){
 const u8*p=*cursor;u16 cp;
 if(*p<0x80u){(*cursor)++;return *p;}
 if((p[0]&0xE0u)==0xC0u){cp=(u16)(((p[0]&0x1Fu)<<6)|(p[1]&0x3Fu));*cursor+=2;return cp;}
 (*cursor)++;return 0;
}
static s16 russianGlyph(u16 cp){
 if((cp>=0x0410u)&&(cp<=0x042Fu))return (s16)(cp-0x0410u);
 if(cp==0x0401u)return 32;
 if(cp=='>')return 33;if(cp==':')return 34;if(cp=='-')return 35;if(cp=='.')return 36;
 if(cp=='A')return 0;if(cp=='B')return 2;if(cp=='C')return 17;
 return -1;
}
static u16 russianTextLength(const char*text){
 const u8*p=(const u8*)text;u16 count=0;while(*p){decodeRussianCodepoint(&p);count++;}return count;
}
static void drawRussianText(const char*text,u16 x,u16 y,bool selected){
 const u8*p=(const u8*)text;u16 palette=selected?PAL1:PAL2,columns=screenColumns();
 while(*p&&(x<columns)){u16 cp=decodeRussianCodepoint(&p);s16 glyph=russianGlyph(cp);
  if(cp!=' '&&glyph>=0)VDP_setTileMapXY(BG_A,TILE_ATTR_FULL(palette,TRUE,FALSE,FALSE,(u16)(ruFontTileBase+(u16)glyph)),x,y);
  x++;
 }
}
static void drawCenteredRussian(const char*text,u16 y,bool selected){
 u16 len=russianTextLength(text),cols=screenColumns();drawRussianText(text,(len<cols)?(u16)((cols-len)>>1):0,y,selected);
}
static void clearMenuText(void){VDP_clearTileMapRect(BG_A,0,18,screenColumns(),10);}
static bool drawTitleBackground(void){
 VDP_setEnable(FALSE);VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_setTextPlane(BG_A);
 if(!VDP_drawImageEx(BG_B,&title,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,TRUE,TRUE)){
  VDP_waitDMACompletion();VDP_setEnable(TRUE);return FALSE;
 }
 PAL_setPalette(PAL0,title.palette->data,DMA);PAL_setPalette(PAL1,goldPalette,DMA);PAL_setPalette(PAL2,whitePalette,DMA);
 buildRussianFont();VDP_waitDMACompletion();VDP_setEnable(TRUE);return TRUE;
}
static void showFatalError(void){VDP_setEnable(TRUE);VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_drawText("TITLE IMAGE ERROR",7,13);while(1)SYS_doVBlankProcess();}
static void drawSplash(void){if(!drawTitleBackground())showFatalError();clearMenuText();drawCenteredRussian("НАЖМИТЕ СТАРТ",26,TRUE);}
static void drawMainMenu(void){
 clearMenuText();drawCenteredRussian("ЮНО ГАСАЙ: КРОВАВАЯ ЛУНА",18,FALSE);
 drawCenteredRussian(mainSelection==0?"> СТАРТ":"СТАРТ",22,mainSelection==0);
 drawCenteredRussian(mainSelection==1?"> НАСТРОЙКИ":"НАСТРОЙКИ",25,mainSelection==1);
}
static void enterMainMenuFromSplash(void){gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;drawMainMenu();}
static void cycleRunButton(s16 d){if(runButton==BUTTON_A)runButton=(d>0)?BUTTON_B:BUTTON_C;else if(runButton==BUTTON_B)runButton=(d>0)?BUTTON_C:BUTTON_A;else runButton=(d>0)?BUTTON_A:BUTTON_B;}
static char getButtonLetter(u16 b){if(b==BUTTON_A)return 'A';if(b==BUTTON_B)return 'B';return 'C';}
static void drawSettings(void){
 char line[32];clearMenuText();drawCenteredRussian("НАСТРОЙКИ",18,FALSE);
 sprintf(line,"КНОПКА БЕГА: %c",getButtonLetter(runButton));drawCenteredRussian(line,22,settingsSelection==0);
 drawCenteredRussian(settingsSelection==1?"> СБРОС НАСТРОЕК":"СБРОС НАСТРОЕК",24,settingsSelection==1);
 drawCenteredRussian(settingsSelection==2?"> НАЗАД":"НАЗАД",26,settingsSelection==2);
}
static bool menuDirection(u16 pad,u16*direction){
 u16 cur=pad&(BUTTON_UP|BUTTON_DOWN);if(!cur){navPrevious=0;navRepeat=0;return FALSE;}
 if(cur!=navPrevious){navPrevious=cur;navRepeat=0;*direction=cur;return TRUE;}
 if(navRepeat<12){navRepeat++;return FALSE;}navRepeat=7;*direction=cur;return TRUE;
}
static void drawBlankPlayfield(void){VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);PAL_setPalette(PAL0,whitePalette,CPU);VDP_setBackgroundColor(15);VDP_setTextPlane(BG_A);}
static bool playerBlockedAt(s16 x,s16 y){s16 maxX=(s16)VDP_getScreenWidth()-40;return x<PLAYER_MIN_X||x>maxX||y<PLAYER_MIN_Y||y>PLAYER_MAX_Y;}
static void movePlayer(u16 pad){
 s16 speed=(pad&runButton)?RUN_SPEED:1,nx=playerX,ny=playerY;
 if(pad&BUTTON_LEFT)nx-=speed;else if(pad&BUTTON_RIGHT)nx+=speed;
 if(nx!=playerX&&!playerBlockedAt(nx,playerY))playerX=nx;
 if(pad&BUTTON_UP)ny-=speed;else if(pad&BUTTON_DOWN)ny+=speed;
 if(ny!=playerY&&!playerBlockedAt(playerX,ny))playerY=ny;
 YunoMeta_draw(playerX,playerY);
}
static YunoMetaResult startGame(void){
 YunoMetaResult result;VDP_setEnable(FALSE);DMA_clearQueue();
 if(!SPR_isInitialized())SPR_initEx(SPRITE_VRAM_TILES);
 spritesInitialized=SPR_isInitialized();if(!spritesInitialized){VDP_setEnable(TRUE);return YUNO_META_ERR_ENGINE_NOT_INITIALIZED;}
 drawBlankPlayfield();playerX=(s16)((VDP_getScreenWidth()-40u)>>1);playerY=52;
 result=YunoMeta_create(playerX,playerY);if(result!=YUNO_META_OK){VDP_setEnable(TRUE);return result;}
 YunoMeta_setVisible(TRUE);SPR_update();VDP_waitDMACompletion();VDP_setEnable(TRUE);gameState=STATE_PLAYING;return YUNO_META_OK;
}
static const char*getYunoMetaErrorText(YunoMetaResult r){
 switch(r){
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
static void showYunoMetaError(YunoMetaResult r){
 u16 pad,previous;VDP_setEnable(TRUE);if(!drawTitleBackground())showFatalError();clearMenuText();
 drawCenteredRussian("НЕ УДАЛОСЬ СОЗДАТЬ ЮНО",20,TRUE);drawCenteredRussian(getYunoMetaErrorText(r),23,FALSE);drawCenteredRussian("НАЖМИТЕ СТАРТ",26,TRUE);
 previous=JOY_readJoypad(JOY_1);while(previous&BUTTON_START){SYS_doVBlankProcess();previous=JOY_readJoypad(JOY_1);}
 while(1){SYS_doVBlankProcess();pad=JOY_readJoypad(JOY_1);if((pad&BUTTON_START)&&!(previous&BUTTON_START))break;previous=pad;}
 gameState=STATE_MAIN_MENU;mainSelection=0;drawMainMenu();
}
int main(bool hardReset){
 u16 pad,pressed,direction=0;YunoMetaResult result;(void)hardReset;
 YunoMeta_destroy();spritesInitialized=SPR_isInitialized();gameState=STATE_SPLASH;playerX=140;playerY=52;
 mainSelection=0;settingsSelection=0;navPrevious=0;navRepeat=0;
 VDP_setEnable(FALSE);VDP_setBackgroundColor(0);JOY_reset();JOY_update();oldPad=JOY_readJoypad(JOY_1);drawSplash();
 while(1){
  pad=JOY_readJoypad(JOY_1);pressed=pad&(u16)~oldPad;oldPad=pad;
  if(gameState==STATE_SPLASH){if(pressed&(BUTTON_START|BUTTON_A))enterMainMenuFromSplash();}
  else if(gameState==STATE_MAIN_MENU){
   if(menuDirection(pad,&direction)){if((direction&BUTTON_UP)&&mainSelection>0)mainSelection--;else if((direction&BUTTON_DOWN)&&mainSelection<1)mainSelection++;drawMainMenu();}
   if(pressed&(BUTTON_A|BUTTON_START)){
    if(mainSelection==0){result=startGame();if(result!=YUNO_META_OK)showYunoMetaError(result);}
    else{gameState=STATE_SETTINGS;settingsSelection=0;navPrevious=0;navRepeat=0;drawSettings();}
   }
  }
  else if(gameState==STATE_SETTINGS){
   if(pressed&BUTTON_B){gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;drawMainMenu();}
   else{
    if(menuDirection(pad,&direction)){if((direction&BUTTON_UP)&&settingsSelection>0)settingsSelection--;else if((direction&BUTTON_DOWN)&&settingsSelection<2)settingsSelection++;drawSettings();}
    if((pressed&BUTTON_LEFT)&&settingsSelection==0){cycleRunButton(-1);drawSettings();}
    else if((pressed&BUTTON_RIGHT)&&settingsSelection==0){cycleRunButton(1);drawSettings();}
    else if(pressed&(BUTTON_A|BUTTON_START)){
     if(settingsSelection==0){cycleRunButton(1);drawSettings();}
     else if(settingsSelection==1){runButton=BUTTON_B;drawSettings();}
     else{gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;drawMainMenu();}
    }
   }
  }
  else if(gameState==STATE_PLAYING){
   if(pressed&BUTTON_START){gameState=STATE_PAUSE;YunoMeta_setVisible(FALSE);clearMenuText();drawCenteredRussian("ПАУЗА",22,TRUE);drawCenteredRussian("СТАРТ - ПРОДОЛЖИТЬ",25,FALSE);}
   else movePlayer(pad);
  }
  else if(gameState==STATE_PAUSE){
   if(pressed&BUTTON_START){gameState=STATE_PLAYING;VDP_clearPlane(BG_A,TRUE);YunoMeta_setVisible(TRUE);}
  }
  if(spritesInitialized)SPR_update();
  SYS_doVBlankProcess();
 }
 return 0;
}

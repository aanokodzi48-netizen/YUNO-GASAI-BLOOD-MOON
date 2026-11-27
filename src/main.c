#include <genesis.h>
#include "resources.h"
#define PLAYER_MIN_X 0
#define PLAYER_MAX_X 288
#define PLAYER_MIN_Y 0
#define PLAYER_MAX_Y 192
#define SPRITE_VRAM_TILES 128
typedef enum { STATE_SPLASH, STATE_MAIN_MENU, STATE_SETTINGS, STATE_PLAYING, STATE_PAUSED } GameState;
static Sprite *yunoSprite=NULL; static bool spritesInitialized=FALSE;
static s16 playerX=144,playerY=96; static u16 animClock=0,oldPad=0,runButton=BUTTON_B;
static u8 mainSelection=0,settingsSelection=0,navRepeat=0; static u16 navPrevious=0;
static GameState gameState=STATE_SPLASH;
static const u16 goldPalette[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,RGB24_TO_VDPCOLOR(0xFFD84D)};
static const u16 whitePalette[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,RGB24_TO_VDPCOLOR(0xFFFFFF)};
static const u16 pausePalette[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,RGB24_TO_VDPCOLOR(0x17172E)};
static void drawLine(const char*s,u16 x,u16 y,bool selected){VDP_setTextPalette(selected?PAL1:PAL2);VDP_drawText(s,x,y);}
static void clearTitleText(void){VDP_clearTextArea(0,18,40,10);}
static bool drawTitleBackground(void){
 /* Hide the VDP while the title image and its palettes are being replaced. */
 VDP_setEnable(FALSE);
 VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_setTextPlane(BG_A);
 if(!VDP_drawImageEx(BG_B,&title,TILE_ATTR_FULL(PAL0,FALSE,FALSE,FALSE,TILE_USER_INDEX),0,0,TRUE,TRUE)){
  VDP_waitDMACompletion();VDP_setEnable(TRUE);return FALSE;
 }
 PAL_setPalette(PAL0,title.palette->data,DMA);PAL_setPalette(PAL1,goldPalette,DMA);PAL_setPalette(PAL2,whitePalette,DMA);VDP_setTextPalette(PAL2);
 VDP_waitDMACompletion();VDP_setEnable(TRUE);return TRUE;
}
static void showFatalError(const char*s){VDP_setEnable(TRUE);VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);VDP_setTextPalette(PAL1);VDP_drawText(s,7,13);while(1)SYS_doVBlankProcess();}
static void drawSplash(void){if(!drawTitleBackground())showFatalError("TITLE IMAGE ERROR");clearTitleText();drawLine("PRESS START",14,26,TRUE);VDP_setTextPalette(PAL2);}
static void drawMainMenu(void){clearTitleText();drawLine("YUNO GASAI: BLOOD MOON",9,20,FALSE);drawLine(mainSelection==0?"> START":"  START",16,23,mainSelection==0);drawLine(mainSelection==1?"> SETTINGS":"  SETTINGS",14,25,mainSelection==1);VDP_setTextPalette(PAL2);}
static void enterMainMenuFromSplash(void){gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;/* Keep the already loaded title background; reloading it caused a visible blank-color flash. */drawMainMenu();}
static void cycleRunButton(s16 d){if(runButton==BUTTON_A)runButton=(d>0)?BUTTON_B:BUTTON_C;else if(runButton==BUTTON_B)runButton=(d>0)?BUTTON_C:BUTTON_A;else runButton=(d>0)?BUTTON_A:BUTTON_B;}
static char getButtonLetter(u16 b){if(b==BUTTON_A)return 'A';if(b==BUTTON_B)return 'B';return 'C';}
static void drawSettings(void){char line[32];clearTitleText();drawLine("SETTINGS",16,18,FALSE);sprintf(line,"%c RUN BUTTON: %c",settingsSelection==0?'>':' ',getButtonLetter(runButton));drawLine(line,12,21,settingsSelection==0);drawLine(settingsSelection==1?"> RESET DEFAULT":"  RESET DEFAULT",12,23,settingsSelection==1);drawLine(settingsSelection==2?"> BACK":"  BACK",17,25,settingsSelection==2);drawLine("A/START: SELECT  B: BACK",8,26,FALSE);drawLine("LEFT/RIGHT: CHANGE",11,27,FALSE);VDP_setTextPalette(PAL2);}
static bool menuDirection(u16 pad,u16*direction){u16 cur=pad&(BUTTON_UP|BUTTON_DOWN);if(!cur){navPrevious=0;navRepeat=0;return FALSE;}if(cur!=navPrevious){navPrevious=cur;navRepeat=0;*direction=cur;return TRUE;}if(navRepeat<12){navRepeat++;return FALSE;}navRepeat=7;*direction=cur;return TRUE;}
/* A cleared plane is transparent; the VDP backdrop supplies the solid white playfield.
   Do not tile-fill with a system-font tile: that tile contains glyph pixels, not a blank tile. */
static void drawBlankPlayfield(void){
 VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);
 PAL_setPalette(PAL0,whitePalette,CPU);VDP_setBackgroundColor(15);
 VDP_setTextPlane(BG_A);VDP_setTextPalette(PAL1);
}
static bool playerBlockedAt(s16 x,s16 y){return x<PLAYER_MIN_X||x>PLAYER_MAX_X||y<PLAYER_MIN_Y||y>PLAYER_MAX_Y;}
static void movePlayer(u16 pad){s16 speed=(pad&runButton)?2:1,nx=playerX,ny=playerY;if(pad&BUTTON_LEFT)nx-=speed;else if(pad&BUTTON_RIGHT)nx+=speed;if(nx!=playerX&&!playerBlockedAt(nx,playerY))playerX=nx;if(pad&BUTTON_UP)ny-=speed;else if(pad&BUTTON_DOWN)ny+=speed;if(ny!=playerY&&!playerBlockedAt(playerX,ny))playerY=ny;if(pad&(BUTTON_LEFT|BUTTON_RIGHT|BUTTON_UP|BUTTON_DOWN))animClock++;else animClock=0;SPR_setAnimAndFrame(yunoSprite,1,(animClock>>3)&3);SPR_setPosition(yunoSprite,playerX,playerY);}
static bool startGame(void){
 VDP_setEnable(FALSE);DMA_clearQueue();
 if(!spritesInitialized){SPR_initEx(SPRITE_VRAM_TILES);spritesInitialized=TRUE;}
 drawBlankPlayfield();playerX=144;playerY=96;animClock=0;
 yunoSprite=SPR_addSprite(&yuno,playerX,playerY,TILE_ATTR(PAL2,TRUE,FALSE,FALSE));
 if(!yunoSprite){SPR_end();spritesInitialized=FALSE;VDP_setEnable(TRUE);return FALSE;}
 PAL_setPalette(PAL2,yuno.palette->data,CPU);SPR_setVisibility(yunoSprite,VISIBLE);SPR_update();VDP_waitDMACompletion();VDP_setEnable(TRUE);gameState=STATE_PLAYING;return TRUE;
}
static void clearPauseOverlay(void){VDP_clearTextArea(10,2,20,7);}
static void drawPause(void){
 /* Keep the pause menu above the player and use a high-contrast dark font. */
 clearPauseOverlay();PAL_setPalette(PAL1,pausePalette,CPU);VDP_setTextPalette(PAL1);
 VDP_drawText("---- PAUSED ----",12,3);
 VDP_drawText("START: RESUME",14,5);
 VDP_drawText("B: MAIN MENU",15,7);
}
static void returnToMainMenu(void){if(spritesInitialized){SPR_end();spritesInitialized=FALSE;}yunoSprite=NULL;gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;if(!drawTitleBackground())showFatalError("TITLE IMAGE ERROR");drawMainMenu();}
int main(bool hardReset){
 u16 pad,pressed,direction=0;(void)hardReset;gameState=STATE_SPLASH;spritesInitialized=FALSE;yunoSprite=NULL;playerX=144;playerY=96;animClock=0;mainSelection=0;settingsSelection=0;navPrevious=0;navRepeat=0;
 VDP_setEnable(FALSE);VDP_setBackgroundColor(0);VDP_setTextPlane(BG_A);JOY_reset();JOY_update();oldPad=JOY_readJoypad(JOY_1);drawSplash();
 while(1){pad=JOY_readJoypad(JOY_1);pressed=pad&(u16)~oldPad;oldPad=pad;
 if(gameState==STATE_SPLASH){if(pressed&(BUTTON_START|BUTTON_A))enterMainMenuFromSplash();}
 else if(gameState==STATE_MAIN_MENU){if(menuDirection(pad,&direction)){if(direction&BUTTON_UP)mainSelection=0;else if(direction&BUTTON_DOWN)mainSelection=1;drawMainMenu();}if(pressed&(BUTTON_A|BUTTON_START)){if(mainSelection==0){if(!startGame())showFatalError("PLAYER SPRITE ERROR");}else{gameState=STATE_SETTINGS;settingsSelection=0;navPrevious=0;navRepeat=0;drawSettings();}}}
 else if(gameState==STATE_SETTINGS){if(pressed&BUTTON_B){gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;drawMainMenu();}else{if(menuDirection(pad,&direction)){if((direction&BUTTON_UP)&&settingsSelection>0)settingsSelection--;else if((direction&BUTTON_DOWN)&&settingsSelection<2)settingsSelection++;drawSettings();}if((pressed&BUTTON_LEFT)&&settingsSelection==0){cycleRunButton(-1);drawSettings();}else if((pressed&BUTTON_RIGHT)&&settingsSelection==0){cycleRunButton(1);drawSettings();}else if(pressed&(BUTTON_A|BUTTON_START)){if(settingsSelection==0){cycleRunButton(1);drawSettings();}else if(settingsSelection==1){runButton=BUTTON_B;drawSettings();}else{gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;drawMainMenu();}}}}
 else if(gameState==STATE_PLAYING){if(pressed&BUTTON_START){gameState=STATE_PAUSED;drawPause();}else movePlayer(pad);}
 else if(gameState==STATE_PAUSED){if(pressed&(BUTTON_START|BUTTON_A)){gameState=STATE_PLAYING;clearPauseOverlay();}else if(pressed&BUTTON_B)returnToMainMenu();}
 if(spritesInitialized)SPR_update();SYS_doVBlankProcess();
 }return 0;
}

#include <genesis.h>
#include "resources.h"
#include "yuno_metasprite.h"
#define PLAYER_MIN_X 0
#define PLAYER_MAX_X 280
#define PLAYER_MIN_Y 0
#define PLAYER_MAX_Y 104
#define SPRITE_VRAM_TILES 128
typedef enum { STATE_SPLASH, STATE_MAIN_MENU, STATE_SETTINGS, STATE_PLAYING, STATE_PHONE_CONTACTS, STATE_PHONE_CHAT } GameState;
#define PHONE_IMAGE_X_TILE 12
#define PHONE_IMAGE_Y_TILE 2
static bool spritesInitialized=FALSE;
static s16 playerX=140,playerY=52; static u16 oldPad=0,runButton=BUTTON_B;
static u16 runSpeed=2;
static u16 phonePreviousPalette[16];
static bool phonePaletteSaved=FALSE;
static u8 mainSelection=0,settingsSelection=0,navRepeat=0; static u16 navPrevious=0;
static GameState gameState=STATE_SPLASH;
static const u16 goldPalette[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,RGB24_TO_VDPCOLOR(0xFFD84D)};
static const u16 whitePalette[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,RGB24_TO_VDPCOLOR(0xFFFFFF)};
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
static void drawMainMenu(void){
 char speedLine[28];
 clearTitleText();
 drawLine("YUNO GASAI: BLOOD MOON",9,18,FALSE);
 drawLine(mainSelection==0?"> START":"  START",16,21,mainSelection==0);
 sprintf(speedLine,"%c RUN SPEED: %uX",mainSelection==1?'>':' ',runSpeed);
 drawLine(speedLine,10,23,mainSelection==1);
 drawLine(mainSelection==2?"> SETTINGS":"  SETTINGS",14,25,mainSelection==2);
 drawLine("LEFT/RIGHT: CHANGE SPEED",7,27,FALSE);
 VDP_setTextPalette(PAL2);
}
static void enterMainMenuFromSplash(void){gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;/* Keep the already loaded title background; reloading it caused a visible blank-color flash. */drawMainMenu();}
static void cycleRunButton(s16 d){if(runButton==BUTTON_A)runButton=(d>0)?BUTTON_B:BUTTON_C;else if(runButton==BUTTON_B)runButton=(d>0)?BUTTON_C:BUTTON_A;else runButton=(d>0)?BUTTON_A:BUTTON_B;}
static char getButtonLetter(u16 b){if(b==BUTTON_A)return 'A';if(b==BUTTON_B)return 'B';return 'C';}
static void cycleRunSpeed(s16 direction){
 if(direction>0) runSpeed=(runSpeed>=4)?2:(u16)(runSpeed+1);
 else runSpeed=(runSpeed<=2)?4:(u16)(runSpeed-1);
}
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
static void movePlayer(u16 pad){
 s16 speed=(pad&runButton)?(s16)runSpeed:1,nx=playerX,ny=playerY;
 if(pad&BUTTON_LEFT)nx-=speed;else if(pad&BUTTON_RIGHT)nx+=speed;
 if(nx!=playerX&&!playerBlockedAt(nx,playerY))playerX=nx;
 if(pad&BUTTON_UP)ny-=speed;else if(pad&BUTTON_DOWN)ny+=speed;
 if(ny!=playerY&&!playerBlockedAt(playerX,ny))playerY=ny;
 YunoMeta_draw(playerX,playerY);
}
static YunoMetaResult startGame(void){
 YunoMetaResult result;
 VDP_setEnable(FALSE);DMA_clearQueue();

 /* SPR_initEx() internally calls SPR_end(). Never call it on a live engine. */
 if(!SPR_isInitialized())
  SPR_initEx(SPRITE_VRAM_TILES);
 spritesInitialized=SPR_isInitialized();
 if(!spritesInitialized){
  VDP_setEnable(TRUE);
  return YUNO_META_ERR_ENGINE_NOT_INITIALIZED;
 }

 drawBlankPlayfield();playerX=140;playerY=52;
 result=YunoMeta_create(playerX,playerY);
 if(result!=YUNO_META_OK){VDP_setEnable(TRUE);return result;}
 YunoMeta_setVisible(TRUE);
 SPR_update();VDP_waitDMACompletion();VDP_setEnable(TRUE);gameState=STATE_PLAYING;
 return YUNO_META_OK;
}
/*
 * The phone is drawn on the full-screen WINDOW plane. WINDOW is above both
 * background planes and hardware sprites; text tiles are also high priority.
 * Yuno is hidden while the phone is open, but her coordinates are not changed.
 */
static bool drawPhoneFrame(void){
 /* Register 17 defaults to width zero; explicitly span the full H40/H32 width. */
 VDP_setWindowHPos(FALSE,(u16)(VDP_getScreenWidth() >> 4));
 VDP_setWindowFullScreen();
 VDP_clearPlane(WINDOW,TRUE);
 PAL_setPalette(PAL1,phone_ui.palette->data,CPU);
 if(!VDP_drawImageEx(
      WINDOW,&phone_ui,
      TILE_ATTR_FULL(PAL1,TRUE,FALSE,FALSE,TILE_USER_INDEX),
      PHONE_IMAGE_X_TILE,PHONE_IMAGE_Y_TILE,FALSE,TRUE))
  return FALSE;
 VDP_waitDMACompletion();
 VDP_setTextPlane(WINDOW);
 VDP_setTextPalette(PAL2);
 VDP_setTextPriority(TRUE);
 return TRUE;
}

static void closePhone(void){
 VDP_clearPlane(WINDOW,TRUE);
 VDP_setWindowOff();
 VDP_setTextPlane(BG_A);
 VDP_setTextPalette(PAL1);
 VDP_setTextPriority(FALSE);
 if(phonePaletteSaved){
  PAL_setPalette(PAL1,phonePreviousPalette,CPU);
  phonePaletteSaved=FALSE;
 }
 YunoMeta_setVisible(TRUE);
 gameState=STATE_PLAYING;
}

static void drawPhoneContacts(void){
 if(!drawPhoneFrame()){closePhone();return;}
 VDP_drawText("CONTACTS",17,5);
 VDP_drawText("> YUKI",18,7);
 VDP_drawText("X / A: OPEN",16,9);
 VDP_drawText("START: CLOSE",15,11);
}

static void drawPhoneChat(void){
 if(!drawPhoneFrame()){closePhone();return;}
 VDP_drawText("YUKI",18,5);
 VDP_drawText("CHAT",18,7);
 /* Intentionally empty: no messages are created in this version. */
 VDP_drawText("B/C BK",17,8);
 VDP_drawText("START",18,9);
}

static void openPhone(void){
 if(!phonePaletteSaved){
  PAL_getPalette(PAL1,phonePreviousPalette);
  phonePaletteSaved=TRUE;
 }
 gameState=STATE_PHONE_CONTACTS;
 
 YunoMeta_setVisible(FALSE);
 drawPhoneContacts();
}

static const char *getYunoMetaErrorText(YunoMetaResult result){
 switch(result){
  case YUNO_META_ERR_ALREADY_CREATED:return "ALREADY EXISTS";
  case YUNO_META_ERR_ENGINE_NOT_INITIALIZED:return "ENGINE NOT READY";
  case YUNO_META_ERR_RESOURCE_DEFINITION:return "BAD SPRITE RESOURCE";
  case YUNO_META_ERR_PALETTE_MISMATCH:return "PALETTE MISMATCH";
  case YUNO_META_ERR_UNSUPPORTED_SCREEN_WIDTH:return "BAD SCREEN WIDTH";
  case YUNO_META_ERR_HARDWARE_LIMIT:return "SPRITE LIMIT";
  case YUNO_META_ERR_HARDWARE_COST_MISMATCH:return "SPRITE COST";
  case YUNO_META_ERR_POSITION_RANGE:return "BAD POSITION";
  case YUNO_META_ERR_ALLOCATION:return "NO SPRITE MEMORY";
  default:return "UNKNOWN ERROR";
 }
}
static void showYunoMetaError(YunoMetaResult result){
 char codeLine[24];
 u16 pad,previous;
 VDP_setEnable(TRUE);VDP_clearPlane(BG_A,TRUE);VDP_clearPlane(BG_B,TRUE);
 VDP_setTextPlane(BG_A);VDP_setTextPalette(PAL1);
 VDP_drawText("YUNO CREATE FAILED",10,10);
 VDP_drawText(getYunoMetaErrorText(result),10,13);
 sprintf(codeLine,"ERROR CODE: %u",(u16)result);VDP_drawText(codeLine,10,15);
 VDP_drawText("RELEASE START, THEN PRESS",7,19);
 VDP_drawText("START TO RETURN TO MENU",8,21);
 previous=JOY_readJoypad(JOY_1);
 while(previous&BUTTON_START){SYS_doVBlankProcess();previous=JOY_readJoypad(JOY_1);}
 while(1){
  SYS_doVBlankProcess();pad=JOY_readJoypad(JOY_1);
  if((pad&BUTTON_START)&&!(previous&BUTTON_START))break;
  previous=pad;
 }
 gameState=STATE_MAIN_MENU;mainSelection=0;drawMainMenu();
}
int main(bool hardReset){
 u16 pad,pressed,direction=0;YunoMetaResult playerResult;(void)hardReset;
 /* Recover only Yuno-owned objects after a warm/soft reset. */
 YunoMeta_destroy();
 spritesInitialized=SPR_isInitialized();
 /* Reset the VDP Window overlay too, in case the warm reset happened in the phone. */
 VDP_setWindowOff();
 VDP_setTextPlane(BG_A);
 VDP_setTextPriority(FALSE);
 phonePaletteSaved=FALSE;
 
 gameState=STATE_SPLASH;playerX=140;playerY=52;mainSelection=0;settingsSelection=0;navPrevious=0;navRepeat=0;
 VDP_setEnable(FALSE);VDP_setBackgroundColor(0);VDP_setTextPlane(BG_A);JOY_reset();JOY_update();oldPad=JOY_readJoypad(JOY_1);drawSplash();
 while(1){pad=JOY_readJoypad(JOY_1);pressed=pad&(u16)~oldPad;oldPad=pad;
 if(gameState==STATE_SPLASH){if(pressed&(BUTTON_START|BUTTON_A))enterMainMenuFromSplash();}
 else if(gameState==STATE_MAIN_MENU){
 if(menuDirection(pad,&direction)){
  if((direction&BUTTON_UP)&&mainSelection>0)mainSelection--;
  else if((direction&BUTTON_DOWN)&&mainSelection<2)mainSelection++;
  drawMainMenu();
 }
 if((mainSelection==1)&&(pressed&(BUTTON_LEFT|BUTTON_RIGHT))){
  cycleRunSpeed((pressed&BUTTON_LEFT)?-1:1);drawMainMenu();
 }
 if(pressed&(BUTTON_A|BUTTON_START)){
  if(mainSelection==0){
   playerResult=startGame();
   if(playerResult!=YUNO_META_OK)showYunoMetaError(playerResult);
  }
  else if(mainSelection==1){cycleRunSpeed(1);drawMainMenu();}
  else{gameState=STATE_SETTINGS;settingsSelection=0;navPrevious=0;navRepeat=0;drawSettings();}
 }
}
 else if(gameState==STATE_SETTINGS){if(pressed&BUTTON_B){gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;drawMainMenu();}else{if(menuDirection(pad,&direction)){if((direction&BUTTON_UP)&&settingsSelection>0)settingsSelection--;else if((direction&BUTTON_DOWN)&&settingsSelection<2)settingsSelection++;drawSettings();}if((pressed&BUTTON_LEFT)&&settingsSelection==0){cycleRunButton(-1);drawSettings();}else if((pressed&BUTTON_RIGHT)&&settingsSelection==0){cycleRunButton(1);drawSettings();}else if(pressed&(BUTTON_A|BUTTON_START)){if(settingsSelection==0){cycleRunButton(1);drawSettings();}else if(settingsSelection==1){runButton=BUTTON_B;runSpeed=2;drawSettings();}else{gameState=STATE_MAIN_MENU;mainSelection=0;navPrevious=0;navRepeat=0;drawMainMenu();}}}}
 else if(gameState==STATE_PLAYING){
 if(pressed&BUTTON_START)openPhone();
 else movePlayer(pad);
 }
 else if(gameState==STATE_PHONE_CONTACTS){
  if(pressed&BUTTON_START)closePhone();
  else if(pressed&(BUTTON_X|BUTTON_A)){gameState=STATE_PHONE_CHAT;drawPhoneChat();}
 }
 else if(gameState==STATE_PHONE_CHAT){
  if(pressed&BUTTON_START)closePhone();
  else if(pressed&(BUTTON_B|BUTTON_C)){gameState=STATE_PHONE_CONTACTS;drawPhoneContacts();}
 }
 if(spritesInitialized)
  SPR_update();
 SYS_doVBlankProcess();
 }return 0;
}

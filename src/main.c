#include <genesis.h>
#include "resources.h"

#define FLOOR_Y 166
#define PLAYER_MIN_X 12
#define PLAYER_MAX_X 286
#define ENEMY_COUNT 3

typedef enum { TITLE, PLAYING, PAUSED, GAME_OVER, WIN } GameState;
static GameState state = TITLE;
static Sprite *yunoSprite;
static Sprite *ninjaSprites[ENEMY_COUNT];
static s16 playerX = 54;
static s16 enemyX[ENEMY_COUNT] = {205, 244, 280};
static u8 enemyAlive[ENEMY_COUNT] = {1,1,1};
static s16 hp = 3;
static s16 jumpTimer = 0;
static u16 animClock = 0, enemyClock = 0, hurtTimer = 0, attackTimer = 0;
static u16 petalClock = 0;
static u8 petalX[5] = {4, 11, 19, 28, 36};
static u8 petalY[5] = {5, 9, 4, 12, 7};
static u16 oldPad = 0;

static const u16 uiPalette[16] = {
    RGB24_TO_VDPCOLOR(0x080818), RGB24_TO_VDPCOLOR(0x17172E),
    RGB24_TO_VDPCOLOR(0x30304B), RGB24_TO_VDPCOLOR(0x555575),
    RGB24_TO_VDPCOLOR(0x7B416E), RGB24_TO_VDPCOLOR(0xA94B78),
    RGB24_TO_VDPCOLOR(0xE16D9B), RGB24_TO_VDPCOLOR(0xF6A9C4),
    RGB24_TO_VDPCOLOR(0xF5D9C7), RGB24_TO_VDPCOLOR(0xE9B95F),
    RGB24_TO_VDPCOLOR(0xA96C43), RGB24_TO_VDPCOLOR(0x613B49),
    RGB24_TO_VDPCOLOR(0x2A263A), RGB24_TO_VDPCOLOR(0x4D5C78),
    RGB24_TO_VDPCOLOR(0x8393A8), RGB24_TO_VDPCOLOR(0xFFF0E7)
};

static void drawBackdrop(void)
{
    VDP_clearTextArea(0,0,40,28);
    VDP_drawText("TOKYO  /  BLOOD MOON", 10, 1);
    VDP_drawText("     .          .           .", 2, 4);
    VDP_drawText("   |###|     |####|      |###|", 2, 7);
    VDP_drawText("   |###|     |####|      |###|", 2, 8);
    VDP_drawText("___|###|_____|####|______|###|___", 1, 10);
    VDP_drawText("     .       ( MOON )       .", 2, 12);
    VDP_drawText("________________________________________", 0, 22);
    VDP_drawText("A: PUNCH   C: KICK   B: RUN", 5, 25);
    VDP_drawText("UP: JUMP   DOWN: CROUCH   START: PAUSE", 1, 27);
}

static void syncSprites(void)
{
    u8 i;
    SPR_setPosition(yunoSprite, playerX, (jumpTimer > 0) ? FLOOR_Y-28 : FLOOR_Y);
    SPR_setAnimAndFrame(yunoSprite, 1, (animClock >> 3) & 3);
    for (i=0;i<ENEMY_COUNT;i++)
    {
        SPR_setVisibility(ninjaSprites[i], enemyAlive[i] ? VISIBLE : HIDDEN);
        SPR_setPosition(ninjaSprites[i], enemyX[i], FLOOR_Y);
        SPR_setAnimAndFrame(ninjaSprites[i], 1, (animClock >> 4) & 3);
    }
}

static void drawSakuraPetals(void)
{
    u8 i;
    petalClock++;
    if (petalClock >= 12)
    {
        petalClock = 0;
        for (i = 0; i < 5; i++)
        {
            petalY[i]++;
            if (i & 1) petalX[i]++;
            else if (petalX[i] > 0) petalX[i]--;
            if (petalY[i] > 20)
            {
                petalY[i] = 3;
                petalX[i] = (i * 7) + 2;
            }
            if (petalX[i] > 39) petalX[i] = 0;
        }
    }
    for (i = 0; i < 5; i++)
        VDP_drawText("*", petalX[i], petalY[i]);
}

static void drawHud(void)
{
    char buf[8];
    sprintf(buf, "%d", hp);
    VDP_drawText("HP:", 2, 2); VDP_drawText(buf, 6, 2);
    VDP_drawText("NINJA:", 30, 2);
    sprintf(buf, "%d", (enemyAlive[0]+enemyAlive[1]+enemyAlive[2]));
    VDP_drawText(buf, 37, 2);
    VDP_drawText("YUNO GASAI: BLOOD MOON", 9, 0);
}

static void resetGame(void)
{
    u8 i;
    playerX=54; hp=3; jumpTimer=0; animClock=0; enemyClock=0; hurtTimer=0; attackTimer=0;
    for(i=0;i<ENEMY_COUNT;i++){ enemyAlive[i]=1; enemyX[i]=205+(i*38); }
    state=PLAYING;
}

int main(bool hardReset)
{
    u16 pad, pressed;
    u8 i;
    s16 dx, nearest, nearestDist;
    (void)hardReset;

    VDP_setTextPalette(PAL2);
    PAL_setPalette(PAL2, uiPalette, DMA);
    VDP_setBackgroundColor(0);
    SPR_init();
    yunoSprite = SPR_addSprite(&yuno, playerX, FLOOR_Y, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    for(i=0;i<ENEMY_COUNT;i++)
        ninjaSprites[i] = SPR_addSprite(&ninja, enemyX[i], FLOOR_Y, TILE_ATTR(PAL1, TRUE, FALSE, FALSE));
    PAL_setPalette(PAL0, yuno.palette->data, DMA);
    PAL_setPalette(PAL1, ninja.palette->data, DMA);

    while(1)
    {
        pad=JOY_readJoypad(JOY_1);
        pressed=pad & (u16)~oldPad;
        oldPad=pad;

        if(state==TITLE)
        {
            VDP_clearTextArea(0,0,40,28);
            VDP_drawText("YUNO GASAI", 14, 7);
            VDP_drawText("BLOOD MOON", 14, 9);
            VDP_drawText("A 16-BIT NIGHTMARE BEGINS", 7, 13);
            VDP_drawText("PRESS START", 14, 19);
            SPR_setVisibility(yunoSprite, HIDDEN);
            for(i=0;i<ENEMY_COUNT;i++) SPR_setVisibility(ninjaSprites[i], HIDDEN);
            if(pressed & BUTTON_START){ resetGame(); }
        }
        else if(state==PLAYING)
        {
            SPR_setVisibility(yunoSprite, VISIBLE);
            if(pressed & BUTTON_START) state=PAUSED;
            else
            {
                if(hurtTimer) hurtTimer--;
                if(attackTimer) attackTimer--;
                if(jumpTimer) jumpTimer--;
                if(pad & BUTTON_LEFT) playerX -= (pad & BUTTON_B) ? 2 : 1;
                if(pad & BUTTON_RIGHT) playerX += (pad & BUTTON_B) ? 2 : 1;
                if(playerX<PLAYER_MIN_X) playerX=PLAYER_MIN_X;
                if(playerX>PLAYER_MAX_X) playerX=PLAYER_MAX_X;
                if((pressed & BUTTON_UP) && jumpTimer==0) jumpTimer=34;

                animClock++;
                enemyClock++;
                if(enemyClock>=8)
                {
                    enemyClock=0;
                    for(i=0;i<ENEMY_COUNT;i++) if(enemyAlive[i])
                    {
                        if(enemyX[i]<playerX+25) enemyX[i]++;
                        else if(enemyX[i]>playerX+25) enemyX[i]--;
                    }
                }

                nearest=-1; nearestDist=999;
                for(i=0;i<ENEMY_COUNT;i++) if(enemyAlive[i])
                {
                    dx=enemyX[i]-playerX; if(dx<0) dx=-dx;
                    if(dx<nearestDist){nearestDist=dx;nearest=i;}
                }
                if(((pressed & BUTTON_A)||(pressed & BUTTON_C)) && attackTimer==0 && nearest>=0 && nearestDist<43)
                {
                    enemyAlive[nearest]=0;
                    attackTimer=18;
                }
                if(nearestDist<23 && hurtTimer==0 && jumpTimer==0 && !(pad & BUTTON_DOWN))
                {
                    hp--; hurtTimer=65;
                    if(hp<=0) state=GAME_OVER;
                }

                drawBackdrop();
                drawHud();
                drawSakuraPetals();
                if(pad & BUTTON_DOWN) VDP_drawText("PRISSEL!", 17, 19);
                if(jumpTimer>0) VDP_drawText("JUMP!", 18, 18);
                syncSprites();
            }
        }
        else if(state==PAUSED)
        {
            VDP_clearTextArea(0,0,40,28);
            VDP_drawText("PAUSED",16,10);
            VDP_drawText("PRESS START TO CONTINUE",8,15);
            if(pressed & BUTTON_START) state=PLAYING;
        }
        else
        {
            VDP_clearTextArea(0,0,40,28);
            if(state==GAME_OVER) VDP_drawText("YUNO HAS FALLEN...",10,10);
            else VDP_drawText("THE NIGHT IS YOURS",10,10);
            VDP_drawText("PRESS START TO PLAY AGAIN",7,17);
            if(pressed & BUTTON_START) resetGame();
        }
        SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}

#include <genesis.h>
#include "resources.h"

#define PLAYER_MIN_X 8
#define PLAYER_MAX_X 286
#define PLAYER_MIN_Y 76
#define PLAYER_MAX_Y 166

static Sprite *yunoSprite;
static s16 playerX = 144;
static s16 playerY = 150;
static u16 animClock = 0;
static u16 oldPad = 0;
static bool paused = FALSE;

int main(bool hardReset)
{
    u16 pad, pressed;
    s16 speed;
    (void) hardReset;

    VDP_setBackgroundColor(0);
    VDP_drawImageEx(BG_B, &room, TILE_ATTR_FULL(PAL3, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, TRUE, TRUE);

    SPR_init();
    yunoSprite = SPR_addSprite(&yuno, playerX, playerY, TILE_ATTR(PAL0, TRUE, FALSE, FALSE));
    if (yunoSprite == NULL)
    {
        VDP_drawText("SPRITE ERROR", 14, 13);
        while (1) SYS_doVBlankProcess();
    }
    PAL_setPalette(PAL0, yuno.palette->data, DMA);
    SPR_setVisibility(yunoSprite, VISIBLE);

    while (1)
    {
        pad = JOY_readJoypad(JOY_1);
        pressed = pad & (u16)~oldPad;
        oldPad = pad;

        if (pressed & BUTTON_START)
        {
            paused = !paused;
            if (paused)
            {
                VDP_drawText("PAUSED", 17, 13);
                VDP_drawText("START TO RESUME", 12, 15);
            }
            else
            {
                VDP_clearTextArea(0, 12, 40, 5);
            }
        }

        if (!paused)
        {
            speed = (pad & BUTTON_B) ? 2 : 1;
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

        SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}

#include <genesis.h>

#define SCREEN_MIN_X 1
#define SCREEN_MAX_X 37

typedef enum
{
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_WIN
} GameState;

static GameState state = STATE_TITLE;
static s16 playerX = 5;
static s16 enemyX = 30;
static s16 playerHP = 3;
static s16 enemyHP = 3;
static s16 wavesCleared = 0;
static s16 jumpFrames = 0;
static u16 enemyClock = 0;
static u16 attackCooldown = 0;
static u16 hurtCooldown = 0;
static u16 transitionClock = 0;
static u16 previousPad = 0;

static void drawScene(void)
{
    char playerLine[2] = "Y";
    char enemyLine[2] = "N";

    VDP_clearTextArea(0, 0, 40, 28);
    VDP_drawText("YUNO GASAI: BLOOD MOON", 7, 1);
    VDP_drawText("NIGHT STREET - TOKYO", 10, 3);

    VDP_drawText("HP:", 2, 5);
    if (playerHP >= 3) VDP_drawText("3", 6, 5);
    else if (playerHP == 2) VDP_drawText("2", 6, 5);
    else if (playerHP == 1) VDP_drawText("1", 6, 5);
    else VDP_drawText("0", 6, 5);

    VDP_drawText("NINJA:", 27, 5);
    if (enemyHP >= 3) VDP_drawText("3", 34, 5);
    else if (enemyHP == 2) VDP_drawText("2", 34, 5);
    else if (enemyHP == 1) VDP_drawText("1", 34, 5);
    else VDP_drawText("0", 34, 5);

    VDP_drawText("  .  *       .       *       .", 2, 9);
    VDP_drawText("     |       |       |", 2, 11);
    VDP_drawText("========================================", 0, 22);
    VDP_drawText("A: PUNCH   C: KICK   B: RUN", 4, 24);
    VDP_drawText("UP: JUMP   DOWN: CROUCH   START: PAUSE", 1, 26);

    if (jumpFrames > 0)
    {
        VDP_drawText("^", playerX, 16);
        VDP_drawText(playerLine, playerX, 17);
    }
    else
    {
        VDP_drawText(playerLine, playerX, 20);
    }

    VDP_drawText(enemyLine, enemyX, 20);

    if (wavesCleared > 0)
    {
        VDP_drawText("WAVE CLEARED!", 14, 14);
    }
}

static void resetGame(void)
{
    playerX = 5;
    enemyX = 30;
    playerHP = 3;
    enemyHP = 3;
    wavesCleared = 0;
    jumpFrames = 0;
    enemyClock = 0;
    attackCooldown = 0;
    hurtCooldown = 0;
    transitionClock = 0;
    state = STATE_PLAYING;
}

int main(bool hardReset)
{
    u16 pad;
    u16 pressed;
    s16 distance;

    (void) hardReset;
    VDP_setTextPalette(PAL0);
    VDP_clearTextArea(0, 0, 40, 28);

    while (1)
    {
        pad = JOY_readJoypad(JOY_1);
        pressed = pad & (u16)~previousPad;
        previousPad = pad;

        if (state == STATE_TITLE)
        {
            VDP_clearTextArea(0, 0, 40, 28);
            VDP_drawText("YUNO GASAI", 14, 8);
            VDP_drawText("BLOOD MOON", 14, 10);
            VDP_drawText("A 16-BIT NIGHTMARE BEGINS", 7, 14);
            VDP_drawText("PRESS START", 14, 20);
            if (pressed & BUTTON_START) resetGame();
        }
        else if (state == STATE_PLAYING)
        {
            if (pressed & BUTTON_START)
            {
                state = STATE_PAUSED;
            }
            else
            {
                if (attackCooldown > 0) attackCooldown--;
                if (hurtCooldown > 0) hurtCooldown--;
                if (jumpFrames > 0) jumpFrames--;

                if (pad & BUTTON_LEFT)
                {
                    playerX -= (pad & BUTTON_B) ? 2 : 1;
                }
                if (pad & BUTTON_RIGHT)
                {
                    playerX += (pad & BUTTON_B) ? 2 : 1;
                }
                if (playerX < SCREEN_MIN_X) playerX = SCREEN_MIN_X;
                if (playerX > SCREEN_MAX_X) playerX = SCREEN_MAX_X;

                if ((pressed & BUTTON_UP) && jumpFrames == 0)
                {
                    jumpFrames = 14;
                }

                distance = enemyX - playerX;
                if (distance < 0) distance = -distance;

                if (((pressed & BUTTON_A) || (pressed & BUTTON_C)) &&
                    attackCooldown == 0 && distance <= 3)
                {
                    enemyHP--;
                    attackCooldown = 12;
                    if (enemyHP <= 0)
                    {
                        wavesCleared++;
                        if (wavesCleared >= 3)
                        {
                            state = STATE_WIN;
                        }
                        else
                        {
                            enemyHP = 3;
                            enemyX = (playerX < 19) ? 34 : 3;
                            transitionClock = 20;
                        }
                    }
                }

                if (transitionClock > 0) transitionClock--;

                enemyClock++;
                if (enemyClock >= 28)
                {
                    enemyClock = 0;
                    if (enemyX < playerX) enemyX++;
                    else if (enemyX > playerX) enemyX--;
                }

                distance = enemyX - playerX;
                if (distance < 0) distance = -distance;
                if (distance <= 1 && hurtCooldown == 0 && jumpFrames == 0 &&
                    !(pad & BUTTON_DOWN))
                {
                    playerHP--;
                    hurtCooldown = 45;
                    if (playerHP <= 0) state = STATE_GAME_OVER;
                }

                drawScene();
                if (pad & BUTTON_DOWN)
                {
                    VDP_drawText("CROUCH", 17, 18);
                }
                if (jumpFrames > 0)
                {
                    VDP_drawText("JUMP!", 17, 18);
                }
            }
        }
        else if (state == STATE_PAUSED)
        {
            VDP_clearTextArea(0, 0, 40, 28);
            VDP_drawText("PAUSED", 16, 10);
            VDP_drawText("PRESS START TO CONTINUE", 8, 15);
            if (pressed & BUTTON_START) state = STATE_PLAYING;
        }
        else if (state == STATE_GAME_OVER)
        {
            VDP_clearTextArea(0, 0, 40, 28);
            VDP_drawText("YUNO HAS FALLEN...", 10, 10);
            VDP_drawText("PRESS START TO RETRY", 10, 16);
            if (pressed & BUTTON_START) resetGame();
        }
        else if (state == STATE_WIN)
        {
            VDP_clearTextArea(0, 0, 40, 28);
            VDP_drawText("THE NIGHT IS YOURS", 10, 9);
            VDP_drawText("THREE ENEMIES DEFEATED", 8, 13);
            VDP_drawText("PRESS START TO PLAY AGAIN", 7, 19);
            if (pressed & BUTTON_START) resetGame();
        }

        SYS_doVBlankProcess();
    }

    return 0;
}

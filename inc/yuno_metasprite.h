#ifndef YUNO_METASPRITE_H
#define YUNO_METASPRITE_H

#include <genesis.h>

#define YUNO_META_PART_COUNT       6u
#define YUNO_META_HARDWARE_COST    6u

#ifndef YUNO_META_PALETTE
#define YUNO_META_PALETTE PAL3
#endif

typedef enum
{
    YUNO_META_OK = 0,
    YUNO_META_ERR_ALREADY_CREATED,
    YUNO_META_ERR_ENGINE_NOT_INITIALIZED,
    YUNO_META_ERR_RESOURCE_DEFINITION,
    YUNO_META_ERR_PALETTE_MISMATCH,
    YUNO_META_ERR_UNSUPPORTED_SCREEN_WIDTH,
    YUNO_META_ERR_HARDWARE_LIMIT,
    YUNO_META_ERR_HARDWARE_COST_MISMATCH,
    YUNO_META_ERR_POSITION_RANGE,
    YUNO_META_ERR_ALLOCATION
} YunoMetaResult;

/* Call only after the application's existing Sprite Engine initialization. */
YunoMetaResult YunoMeta_create(s16 x, s16 y);
void YunoMeta_draw(s16 x, s16 y);
void YunoMeta_setVisible(bool visible);
void YunoMeta_destroy(void);
u16 YunoMeta_getHardwareCost(void);

#endif

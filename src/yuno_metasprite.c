#include <genesis.h>
#include "resources.h"
#include "yuno_metasprite.h"

/*
 * ISOLATION CONTRACT:
 * - Owns exactly six Sprite* handles and releases only those handles.
 * - Never calls SPR_init(), SPR_end(), SPR_reset(), SPR_clear() or SPR_update().
 * - Never touches any other actor's Sprite* or SpriteDefinition.
 * - PAL3 is reserved for Yuno while this metasprite exists; its previous colors
 *   are saved and restored when this module is destroyed.
 */

typedef struct
{
    const SpriteDefinition *definition;
    s16 offsetX;
    s16 offsetY;
    u16 expectedWidth;
    u16 expectedHeight;
} YunoMetaPartDefinition;

/* Offsets are relative to the 40x120 source canvas top-left corner. */
static const YunoMetaPartDefinition yunoMetaPartDefinitions[YUNO_META_PART_COUNT] =
{
    { &yuno_meta_head,        0,   0, 32, 32 },
    { &yuno_meta_body_upper,  0,  32, 32, 32 },
    { &yuno_meta_hair_right, 32,  32,  8, 32 },
    { &yuno_meta_body_lower,  0,  64, 32, 32 },
    { &yuno_meta_legs,        0,  96, 32, 24 },
    { &yuno_meta_shoe_right, 32,  96,  8, 24 }
};

static Sprite *yunoMetaSprites[YUNO_META_PART_COUNT] =
{
    NULL, NULL, NULL, NULL, NULL, NULL
};

static bool yunoMetaCreated = FALSE;
static bool yunoMetaPaletteSaved = FALSE;
static bool yunoMetaPaletteInstalled = FALSE;
static u16 yunoMetaPreviousPalette[16];

static u16 yunoMetaGetHardwareLimit(void)
{
    const u16 screenWidth = VDP_getScreenWidth();

    if (screenWidth == 320u) return 80u; /* H40 */
    if (screenWidth == 256u) return 64u; /* H32 */
    return 0u;
}

static YunoMetaResult yunoMetaValidateResources(void)
{
    const Palette *referencePalette;
    u16 partIndex;
    u16 colorIndex;
    u16 compareIndex;

    if (yunoMetaPartDefinitions[0].definition == NULL)
        return YUNO_META_ERR_RESOURCE_DEFINITION;

    referencePalette = yunoMetaPartDefinitions[0].definition->palette;

    if ((referencePalette == NULL) ||
        (referencePalette->data == NULL) ||
        (referencePalette->length != 16u))
        return YUNO_META_ERR_PALETTE_MISMATCH;

    /* Palette indices 1..15 must contain the 15 unique opaque colors. */
    for (colorIndex = 1u; colorIndex < 16u; colorIndex++)
    {
        for (compareIndex = (u16)(colorIndex + 1u);
             compareIndex < 16u; compareIndex++)
        {
            if (referencePalette->data[colorIndex] ==
                referencePalette->data[compareIndex])
                return YUNO_META_ERR_PALETTE_MISMATCH;
        }
    }

    for (partIndex = 0u; partIndex < YUNO_META_PART_COUNT; partIndex++)
    {
        const SpriteDefinition *definition =
            yunoMetaPartDefinitions[partIndex].definition;
        const Palette *palette;

        if ((definition == NULL) || (definition->palette == NULL))
            return YUNO_META_ERR_RESOURCE_DEFINITION;

        palette = definition->palette;
        if ((palette->data == NULL) || (palette->length != 16u))
            return YUNO_META_ERR_PALETTE_MISMATCH;

        /* Every part must render as exactly one VDP sprite. */
        if (definition->maxNumSprite != 1u)
            return YUNO_META_ERR_RESOURCE_DEFINITION;

        if ((definition->w != yunoMetaPartDefinitions[partIndex].expectedWidth) ||
            (definition->h != yunoMetaPartDefinitions[partIndex].expectedHeight))
            return YUNO_META_ERR_RESOURCE_DEFINITION;

        for (colorIndex = 0u; colorIndex < 16u; colorIndex++)
        {
            if (palette->data[colorIndex] != referencePalette->data[colorIndex])
                return YUNO_META_ERR_PALETTE_MISMATCH;
        }
    }

    return YUNO_META_OK;
}

static void yunoMetaReleaseOwnedParts(void)
{
    u16 partIndex;

    for (partIndex = 0u; partIndex < YUNO_META_PART_COUNT; partIndex++)
    {
        if (yunoMetaSprites[partIndex] != NULL)
        {
            SPR_releaseSprite(yunoMetaSprites[partIndex]);
            yunoMetaSprites[partIndex] = NULL;
        }
    }

    yunoMetaCreated = FALSE;
}

static void yunoMetaRestorePalette(void)
{
    if (yunoMetaPaletteSaved && yunoMetaPaletteInstalled)
        PAL_setPalette(YUNO_META_PALETTE, yunoMetaPreviousPalette, CPU);

    yunoMetaPaletteSaved = FALSE;
    yunoMetaPaletteInstalled = FALSE;
}

YunoMetaResult YunoMeta_create(s16 x, s16 y)
{
    u16 hardwareLimit;
    u16 usedBefore;
    const u16 spriteAttributes =
        TILE_ATTR(YUNO_META_PALETTE, TRUE, FALSE, FALSE);
    YunoMetaResult validationResult;
    u16 partIndex;

    if (yunoMetaCreated)
        return YUNO_META_ERR_ALREADY_CREATED;

    /* Keep x+32 and y+96 representable in signed 16-bit screen coordinates. */
    if ((x > 32735) || (y > 32671))
        return YUNO_META_ERR_POSITION_RANGE;

    if (!SPR_isInitialized())
        return YUNO_META_ERR_ENGINE_NOT_INITIALIZED;

    validationResult = yunoMetaValidateResources();
    if (validationResult != YUNO_META_OK)
        return validationResult;

    hardwareLimit = yunoMetaGetHardwareLimit();
    if (hardwareLimit == 0u)
        return YUNO_META_ERR_UNSUPPORTED_SCREEN_WIDTH;

    usedBefore = SPR_getUsedVDPSprite();

    /* Check the complete sprite-table budget before allocating any part. */
    if ((usedBefore > hardwareLimit) ||
        (YUNO_META_HARDWARE_COST > (u16)(hardwareLimit - usedBefore)))
        return YUNO_META_ERR_HARDWARE_LIMIT;

    /*
     * Do not use SPR_addSpriteSafe(): it may defragment shared sprite VRAM and
     * relocate other actors. A normal allocation either succeeds or returns NULL.
     */
    for (partIndex = 0u; partIndex < YUNO_META_PART_COUNT; partIndex++)
    {
        const YunoMetaPartDefinition *part =
            &yunoMetaPartDefinitions[partIndex];
        const u16 expectedUsed =
            (u16)(usedBefore + partIndex + 1u);
        u16 usedAfter;

        yunoMetaSprites[partIndex] = SPR_addSprite(
            part->definition,
            (s16)(x + part->offsetX),
            (s16)(y + part->offsetY),
            spriteAttributes
        );

        if (yunoMetaSprites[partIndex] == NULL)
        {
            yunoMetaReleaseOwnedParts();
            return YUNO_META_ERR_ALLOCATION;
        }

        /* Verify that this one resource consumed exactly one VDP sprite. */
        usedAfter = SPR_getUsedVDPSprite();
        if (usedAfter != expectedUsed)
        {
            yunoMetaReleaseOwnedParts();
            return YUNO_META_ERR_HARDWARE_COST_MISMATCH;
        }

        if (usedAfter > hardwareLimit)
        {
            yunoMetaReleaseOwnedParts();
            return YUNO_META_ERR_HARDWARE_LIMIT;
        }
    }

    PAL_getPalette(YUNO_META_PALETTE, yunoMetaPreviousPalette);
    yunoMetaPaletteSaved = TRUE;

    PAL_setPalette(
        YUNO_META_PALETTE,
        yunoMetaPartDefinitions[0].definition->palette->data,
        DMA
    );
    yunoMetaPaletteInstalled = TRUE;
    yunoMetaCreated = TRUE;

    return YUNO_META_OK;
}

void YunoMeta_draw(s16 x, s16 y)
{
    u16 partIndex;

    if (!yunoMetaCreated)
        return;

    if ((x > 32735) || (y > 32671))
        return;

    for (partIndex = 0u; partIndex < YUNO_META_PART_COUNT; partIndex++)
    {
        SPR_setPosition(
            yunoMetaSprites[partIndex],
            (s16)(x + yunoMetaPartDefinitions[partIndex].offsetX),
            (s16)(y + yunoMetaPartDefinitions[partIndex].offsetY)
        );
    }
}

void YunoMeta_setVisible(bool visible)
{
    const SpriteVisibility state = visible ? VISIBLE : HIDDEN;
    u16 partIndex;

    if (!yunoMetaCreated)
        return;

    for (partIndex = 0u; partIndex < YUNO_META_PART_COUNT; partIndex++)
        SPR_setVisibility(yunoMetaSprites[partIndex], state);
}

void YunoMeta_destroy(void)
{
    yunoMetaReleaseOwnedParts();
    yunoMetaRestorePalette();
}

u16 YunoMeta_getHardwareCost(void)
{
    return YUNO_META_HARDWARE_COST;
}

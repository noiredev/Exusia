#if !defined(FIRSTGAME_H)
#include "prototype_platform.h"
#include "prototype_intrinsics.h"

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

#if FIRSTGAME_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32_t SafeTruncateUInt64(uint64_t Value) {
    // TODO: Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32_t Result = (uint32_t)Value;
    return Result;
}

inline game_controller_input *GetController(game_input *Input, int ControllerIndex) {
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return Result;
}

struct canonical_position {
#if 1
    int32_t TileMapX;
    int32_t TileMapY;

    int32_t TileX;
    int32_t TileY;
#else
    // Tile-relative x and y
    uint32_t _TileX;
    uint32_t _TileY;
#endif
    float TileRelX;
    float TileRelY;
};

struct tile_map {
    uint32_t *Tiles;
};

struct world {
    float TileSideInMeters;
    int32_t TileSideInPixels;
    float MetersToPixels;

    int32_t CountX;
    int32_t CountY;
    
    float UpperLeftX;
    float UpperLeftY;

    int32_t TileMapCountX;
    int32_t TileMapCountY;
    
    tile_map *TileMaps;
};

struct game_state {
    canonical_position PlayerP;
};

#define FIRSTGAME_H
#endif
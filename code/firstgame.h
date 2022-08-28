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

struct tile_chunk_position {
    uint32_t TileChunkX;
    uint32_t TileChunkY;

    uint32_t RelTileX;
    uint32_t RelTileY;
};

struct world_position {
    uint32_t AbsTileX;
    uint32_t AbsTileY;

    float TileRelX;
    float TileRelY;
};

struct tile_chunk {
    uint32_t *Tiles;
};

struct world {
    uint32_t ChunkShift;
    uint32_t ChunkMask;
    uint32_t ChunkDim;

    float TileSideInMeters;
    int32_t TileSideInPixels;
    float MetersToPixels;
    
    int32_t TileChunkCountX;
    int32_t TileChunkCountY;
    
    tile_chunk *TileChunks;
};

struct game_state {
    world_position PlayerP;
};

#define FIRSTGAME_H
#endif
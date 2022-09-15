#if !defined(FIRSTGAME_H)
#include "prototype_platform.h"


struct memory_arena {
    memory_index Size;
    uint8_t *Base;
    memory_index Used;
};

internal void InitializeArena(memory_arena *Arena, memory_index Size, uint8_t *Base) {
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
void * PushSize_(memory_arena *Arena, memory_index Size) {
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;    
}

#include "prototype_math.h"
#include "prototype_intrinsics.h"
#include "prototype_tile.h"

struct world {
    tile_map *TileMap;
};

struct loaded_bitmap {
    int32_t Width;
    int32_t Height;
    uint32_t *Pixels;
};

struct hero_bitmaps {
    int32_t AlignX;
    int32_t AlignY;
    loaded_bitmap Head;
    loaded_bitmap Cape;
    loaded_bitmap Torso;
};

struct entity {
    bool32 Exists;
    tile_map_position P;
    v2 dP;
}

struct game_state {
    memory_arena WorldArena;
    world *World;
    
    uint32_t CameraFollowingEntityIndex;
    tile_map_position CameraP;

    uint32_t PlayerIndexForController[ArrayCount(((game_input *)0)->Controllers];
    uint32_t EntityCount;
    entity Entities[256];

    loaded_bitmap Backdrop;
    uint32_t HeroFacingDirection;
    hero_bitmaps HeroBitmaps[4];
};

#define FIRSTGAME_H
#endif
#if !defined(PROTOTYPE_WORLD_H)
struct world_position {
    // TODO: How to get rid of abstile* here and still allow references to entities to be able to know where they are in
    int32 ChunkX;
    int32 ChunkY;
    int32 ChunkZ;

    // NOTE: These are the offsets from the center of a tile
    v3 Offset_;
};

// TODO: Could make this just world_chunk and then allow multiple tiles chunks per x/y/z
struct world_entity_block {
    uint32 EntityCount;
    uint32 LowEntityIndex[16];
    world_entity_block *Next;
};

struct world_chunk {
    int32 ChunkX;
    int32 ChunkY;
    int32 ChunkZ;

    // TODO: Profile this and determine if a pointer would be better
    world_entity_block FirstBlock;

    world_chunk *NextInHash;
};

struct world {

    real32 TileSideInMeters;
    real32 TileDepthInMeters;
    v3 ChunkDimInMeters;

    // TODO: WorldChunkHash should switch to pointers if
    // title entity blocks continute to be stored en masse directly in the tile chunk
    // NOTE: At the moment this must be a power of 2
    world_chunk ChunkHash[4096];

    world_entity_block *FirstFree;
};
#define PROTOTYPE_WORLD_H
#endif

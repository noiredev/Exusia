#if !defined(PROTOTYPE_TILE_H)

struct tile_map_position {
    uint32_t AbsTileX;
    uint32_t AbsTileY;

    float TileRelX;
    float TileRelY;
};

struct tile_chunk_position {
    uint32_t TileChunkX;
    uint32_t TileChunkY;

    uint32_t RelTileX;
    uint32_t RelTileY;
};

struct tile_chunk {
    uint32_t *Tiles;
};

struct tile_map {
    uint32_t ChunkShift;
    uint32_t ChunkMask;
    uint32_t ChunkDim;

    float TileSideInMeters;
    int32_t TileSideInPixels;
    float MetersToPixels;
    
    uint32_t TileChunkCountX;
    uint32_t TileChunkCountY;
    
    tile_chunk *TileChunks;
};

#define PROTOTYPE_TILE_H
#endif
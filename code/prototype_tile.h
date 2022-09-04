#if !defined(PROTOTYPE_TILE_H)

struct tile_map_difference {
    float dX;
    float dY;
    float dZ;
};

struct tile_map_position {
    uint32_t AbsTileX;
    uint32_t AbsTileY;
    uint32_t AbsTileZ;

    // These are the offsets from the tile center
    float OffsetX;
    float OffsetY;
};

struct tile_chunk_position {
    uint32_t TileChunkX;
    uint32_t TileChunkY;
    uint32_t TileChunkZ;

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
    
    uint32_t TileChunkCountX;
    uint32_t TileChunkCountY;
    uint32_t TileChunkCountZ;
    
    tile_chunk *TileChunks;
};

#define PROTOTYPE_TILE_H
#endif
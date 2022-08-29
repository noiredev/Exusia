
inline tile_chunk *GetTileChunk(tile_map *TileMap, uint32_t TileChunkX, uint32_t TileChunkY) {
    tile_chunk *TileChunk = 0;

    if((TileChunkX >= 0) && (TileChunkX < TileMap->TileChunkCountX) && (TileChunkY >= 0) && (TileChunkY < TileMap->TileChunkCountY)) {
        TileChunk = &TileMap->TileChunks[TileChunkY*TileMap->TileChunkCountX + TileChunkX];
    }

    return TileChunk;
}

inline uint32_t GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TileX, uint32_t TileY) {
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);

    uint32_t TileChunkValue = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];
    return TileChunkValue;
}

inline void SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TileX, uint32_t TileY, uint32_t TileValue) {
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);
    
    TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;
}

inline uint32_t GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TestTileX, uint32_t TestTileY) {
    uint32_t TileChunkValue = 0;

    if(TileChunk) {
        TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    }

    return TileChunkValue;
}

inline void SetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32_t TestTileX, uint32_t TestTileY, uint32_t TileValue) {
    uint32_t TileChunkValue = 0;
    
    if(TileChunk)
    {
        SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
    }
}

inline tile_chunk_position GetChunkPositionFor(tile_map *TileMap, uint32_t AbsTileX, uint32_t AbsTileY) {
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY = AbsTileY & TileMap->ChunkMask;

    return Result;
}

internal uint32_t GetTileValue(tile_map *TileMap, uint32_t AbsTileX, uint32_t AbsTileY) {
    bool32 Empty = false;

    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
    uint32_t TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

    return TileChunkValue;
}

internal bool32 IsTileMapPointEmpty(tile_map *TileMap, tile_map_position CanPos) {
    uint32_t TileChunkValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY);
    bool32 Empty = (TileChunkValue == 0);

    return Empty;
}

internal void SetTileValue(memory_arena *Arena, tile_map *TileMap, uint32_t AbsTileX, uint32_t AbsTileY, uint32_t TileValue) {
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);

    Assert(TileChunk);

    SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}
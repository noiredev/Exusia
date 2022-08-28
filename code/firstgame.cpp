#include "firstgame.h"

internal void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz) {
    int16_t ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
#if 0
        float SineValue = sinf(GameState->tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
#else
        int16_t SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
#if 0
        GameState->tSine += 2.0f*Pi32*1.0f / (float)WavePeriod;
        if(GameState->tSine > 2.0f*Pi32) {
            GameState->tSine -= 2.0f*Pi32;
        }
#endif
    } 
}

internal void DrawRectangle(game_offscreen_buffer *Buffer, float FloatMinX, float FloatMinY, float FloatMaxX, float FloatMaxY, float R, float G, float B) {
    int32_t MinX = RoundFloatToInt32(FloatMinX);
    int32_t MinY = RoundFloatToInt32(FloatMinY);
    int32_t MaxX = RoundFloatToInt32(FloatMaxX);
    int32_t MaxY = RoundFloatToInt32(FloatMaxY);

    if(MinX < 0) {
        MinX = 0;
    }
    if(MinY < 0) {
        MinY = 0;
    }
    
    if(MaxX > Buffer->Width) {
        MaxX = Buffer->Width;
    }
    if(MaxY > Buffer->Height) {
        MaxY = Buffer->Height;
    }
    
    uint32_t Color = (RoundFloatToUInt32(R * 255.0f) << 16) | (RoundFloatToUInt32(G * 255.0f) << 8) | RoundFloatToUInt32(B * 255.0f);

    uint8_t *Row = ((uint8_t *)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch);
    for(int Y = MinY; Y < MaxY; ++Y) {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = MinX; X < MaxX; ++X) {
            *Pixel++ = Color;
        }

        Row += Buffer->Pitch;
    }
}

inline tile_chunk *GetTileChunk(world *World, int32_t TileChunkX, int32_t TileChunkY) {
    tile_chunk *TileChunk = 0;

    if((TileChunkX >= 0) && (TileChunkX < World->TileChunkCountX) && (TileChunkY >= 0) && (TileChunkY < World->TileChunkCountY)) {
        TileChunk = &World->TileChunks[TileChunkY*World->TileChunkCountX + TileChunkX];
    }

    return TileChunk;
}

inline uint32_t GetTileValueUnchecked(world *World, tile_chunk *TileChunk, uint32_t TileX, uint32_t TileY) {
    Assert(TileChunk);
    Assert(TileX < World->ChunkDim);
    Assert(TileY < World->ChunkDim);

    uint32_t TileChunkValue = TileChunk->Tiles[TileY*World->ChunkDim + TileX];
    return TileChunkValue;
}

inline uint32_t GetTileValue(world *World, tile_chunk *TileChunk, uint32_t TestTileX, uint32_t TestTileY) {
    uint32_t TileChunkValue = 0;

    if(TileChunk) {
        TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
    }

    return TileChunkValue;
}

inline void RecanonicalizeCoord(world *World, uint32_t *Tile, float *TileRel) {
    int32_t Offset = FloorFloatToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*World->TileSideInMeters;

    Assert(*TileRel >= 0);
    Assert(*TileRel < World->TileSideInMeters);
}

inline tile_chunk_position GetChunkPositionFor(world *World, uint32_t AbsTileX, uint32_t AbsTileY) {
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> World->ChunkShift;
    Result.TileChunkY = AbsTileY >> World->ChunkShift;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY = AbsTileY & World->ChunkMask;

    return Result;
}

inline world_position RecanonicalizePosition(world *World, world_position Pos) {
    world_position Result = Pos;

    RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
    RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);

    return Result;
}

internal uint32_t GetTileValue(world *World, uint32_t AbsTileX, uint32_t AbsTileY)
{
    bool32 Empty = false;

    tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
    tile_chunk *TileMap = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
    uint32_t TileChunkValue = GetTileValue(World, TileMap, ChunkPos.RelTileX, ChunkPos.RelTileY);

    return TileChunkValue;
}

internal bool32 IsWorldPointEmpty(world *World, world_position CanPos) {
    uint32_t TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
    bool32 Empty = (TileChunkValue == 0);

    return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256
    uint32_t TempTiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
    };
    

    world World;
    // NOTE: This is set to using 256x256 tile chunks.
    World.ChunkShift = 8;
    World.ChunkMask = 0xFF;
    World.ChunkDim = 256;

    World.TileChunkCountX = 2;
    World.TileChunkCountY = 2;

    tile_chunk TileChunk;
    TileChunk.Tiles = (uint32_t *)TempTiles;
    World.TileChunks = &TileChunk;

    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (float)(World.TileSideInPixels / World.TileSideInMeters);

    float PlayerHeight = 1.4f;
    float PlayerWidth = 0.75f*PlayerHeight;

    float LowerLeftX = -(float)World.TileSideInPixels/2;
    float LowerLeftY = (float)Buffer->Height;
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized) {
        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 3;
        GameState->PlayerP.TileRelX = 5.0f;
        GameState->PlayerP.TileRelY = 5.0f;

        Memory->IsInitialized = true;
    }

    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex) {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if(Controller->IsAnalog) {
            // NOTE: Use analog movement tuning
        }
        else
        {
            // NOTE: Use digital movement tuning
            float dPlayerX = 0.0f; // pixels/second
            float dPlayerY = 0.0f; // pixels/second
            
            if(Controller->MoveUp.EndedDown) {
                dPlayerY = 1.0f;
            }
            if(Controller->MoveDown.EndedDown) {
                dPlayerY = -1.0f;
            }
            if(Controller->MoveLeft.EndedDown) {
                dPlayerX = -1.0f;
            }
            if(Controller->MoveRight.EndedDown) {
                dPlayerX = 1.0f;
            }
            dPlayerX *= 2.0f;
            dPlayerY *= 2.0f;

            world_position NewPlayerP = GameState->PlayerP;
            NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
            NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
            NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);

            world_position PlayerLeft = NewPlayerP;
            PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
            PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);

            world_position PlayerRight = NewPlayerP;
            PlayerRight.TileRelX += 0.5f*PlayerWidth;
            PlayerRight = RecanonicalizePosition(&World, PlayerRight);

            if(IsWorldPointEmpty(&World, NewPlayerP) && IsWorldPointEmpty(&World, PlayerLeft) && IsWorldPointEmpty(&World, PlayerRight)) {
                GameState->PlayerP = NewPlayerP;
            }
        }
    }
    
    DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 1.0f, 0.0f, 1.0f);

    float CenterX = 0.5f*(float)Buffer->Width;
    float CenterY = 0.5f*(float)Buffer->Height;

    for(int32_t RelRow = -10; RelRow < 10; ++RelRow) {
        for(int32_t RelColumn = -20; RelColumn < 20; ++RelColumn) {
            uint32_t Column = GameState->PlayerP.AbsTileX + RelColumn;
            uint32_t Row = GameState->PlayerP.AbsTileY + RelRow;
            uint32_t TileID = GetTileValue(&World, Column, Row);
            float Gray = 0.5f;
            if(TileID == 1) {
                Gray = 1.0f;
            }

            if((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY)) {
                Gray = 0.0f;
            }

            float MinX = CenterX + ((float)RelColumn)*World.TileSideInPixels;
            float MinY = CenterY - ((float)RelRow)*World.TileSideInPixels;
            float MaxX = MinX + World.TileSideInPixels;
            float MaxY = MinY - World.TileSideInPixels;
            DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, Gray, Gray, Gray);
        }
    }

    float PlayerR = 1.0f;
    float PlayerG = 1.0f;
    float PlayerB = 0.0f;
    float PlayerLeft = CenterX + World.MetersToPixels*GameState->PlayerP.TileRelX - 0.5f*World.MetersToPixels*PlayerWidth;
    float PlayerTop = CenterY - World.MetersToPixels*GameState->PlayerP.TileRelY - World.MetersToPixels*PlayerHeight;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + World.MetersToPixels*PlayerWidth, PlayerTop + World.MetersToPixels*PlayerHeight, PlayerR, PlayerG, PlayerB);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, 480);
}

// internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset) {
//     uint8_t *Row = (uint8_t *)Buffer->Memory;
//     for(int Y = 0; Y < Buffer->Height; ++Y) {
//         uint32_t *Pixel = (uint32_t *)Row;
//         for(int X = 0; X < Buffer->Width; ++X) {
//             /*
//               Pixel in memory: BB GG RR xx
//               Little endian architecture

//               When you load it into memory it becomes backwards so it is:
//               0x xxBBGGRR
//               However, windows did not like this so they reversed it.
//               0x xxRRGGBB
//             */
//             uint8_t Blue = (uint8_t)(X + BlueOffset);
//             uint8_t Green = (uint8_t)(Y + GreenOffset);

//             /*
//               Memory:   BB GG RR xx
//               Register: xx RR GG BB

//               Pixel (32 - bits)
//             */

//             *Pixel++ = ((Green << 8) | Blue);
//         }

//         Row += Buffer->Pitch;
//     }
// }
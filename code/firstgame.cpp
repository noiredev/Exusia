#include "firstgame.h"

#include "prototype_tile.cpp"

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

inline void RecanonicalizeCoord(tile_map *TileMap, uint32_t *Tile, float *TileRel) {
    int32_t Offset = RoundFloatToInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*TileMap->TileSideInMeters;

    Assert(*TileRel >= -0.5f*TileMap->TileSideInMeters);
    Assert(*TileRel <= 0.5f*TileMap->TileSideInMeters);
}

inline tile_map_position RecanonicalizePosition(tile_map *TileMap, tile_map_position Pos) {
    tile_map_position Result = Pos;

    RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
    RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);

    return Result;
}

internal void InitalizeArena(memory_arena *Arena, memory_index Size, uint8_t *Base) {
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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256

    float PlayerHeight = 1.4f;
    float PlayerWidth = 0.75f*PlayerHeight;

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized) {
        GameState->PlayerP.AbsTileX = 0;
        GameState->PlayerP.AbsTileY = 0;
        GameState->PlayerP.TileRelX = 5.0f;
        GameState->PlayerP.TileRelY = 5.0f;

        InitalizeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8_t *)Memory->PermanentStorage + sizeof(game_state));

        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);

        tile_map *TileMap = World->TileMap;

        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);

        TileMap->TileChunkCountX = 128;
        TileMap->TileChunkCountY = 128;

        TileMap->TileChunks = PushArray(&GameState->WorldArena, TileMap->TileChunkCountX*TileMap->TileChunkCountY, tile_chunk);

        for(uint32_t Y = 0; Y < TileMap->TileChunkCountY; ++Y) {
            for(uint32_t X = 0; X < TileMap->TileChunkCountX; ++X) {
                TileMap->TileChunks[Y*TileMap->TileChunkCountX + X].Tiles = PushArray(&GameState->WorldArena, TileMap->ChunkDim*TileMap->ChunkDim, uint32_t);
            }
        }

        TileMap->TileSideInMeters = 1.4f;
        TileMap->TileSideInPixels = 60;
        TileMap->MetersToPixels = (float)(TileMap->TileSideInPixels / TileMap->TileSideInMeters);

        float LowerLeftX = -(float)TileMap->TileSideInPixels/2;
        float LowerLeftY = (float)Buffer->Height;

        uint32_t TilesPerWidth = 17;
        uint32_t TilesPerHeight = 9;
        for(uint32_t ScreenY = 0; ScreenY < 32; ++ScreenY) {
            for(uint32_t ScreenX = 0; ScreenX < 32; ++ScreenX) {
                for(uint32_t TileY = 0; TileY < TilesPerWidth; ++TileY) {
                    for(uint32_t TileX = 0; TileX < TilesPerHeight; ++TileX) {
                        uint32_t AbsTileX = ScreenX*TilesPerWidth + TileX;
                        uint32_t AbsTileY = ScreenY*TilesPerHeight + TileY;

                        SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, (TileX == TileY) && (TileY % 2) ? 1 : 0);
                    }
                }
            }
        }

        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

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
            float PlayerSpeed = 2.0f;
            dPlayerX *= PlayerSpeed;
            dPlayerY *= PlayerSpeed;

            tile_map_position NewPlayerP = GameState->PlayerP;
            NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
            NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
            NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

            tile_map_position PlayerLeft = NewPlayerP;
            PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
            PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

            tile_map_position PlayerRight = NewPlayerP;
            PlayerRight.TileRelX += 0.5f*PlayerWidth;
            PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

            if(IsTileMapPointEmpty(TileMap, NewPlayerP) && IsTileMapPointEmpty(TileMap, PlayerLeft) && IsTileMapPointEmpty(TileMap, PlayerRight)) {
                GameState->PlayerP = NewPlayerP;
            }
        }
    }
    
    DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 1.0f, 0.0f, 1.0f);

    float ScreenCenterX = 0.5f*(float)Buffer->Width;
    float ScreenCenterY = 0.5f*(float)Buffer->Height;

    for(int32_t RelRow = -10; RelRow < 10; ++RelRow) {
        for(int32_t RelColumn = -20; RelColumn < 20; ++RelColumn) {
            uint32_t Column = GameState->PlayerP.AbsTileX + RelColumn;
            uint32_t Row = GameState->PlayerP.AbsTileY + RelRow;
            uint32_t TileID = GetTileValue(TileMap, Column, Row);
            float Gray = 0.5f;
            if(TileID == 1) {
                Gray = 1.0f;
            }

            if((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY)) {
                Gray = 0.0f;
            }

            float CenterX = ScreenCenterX - TileMap->MetersToPixels*GameState->PlayerP.TileRelX + ((float)RelColumn)*TileMap->TileSideInPixels;
            float CenterY = ScreenCenterY + TileMap->MetersToPixels*GameState->PlayerP.TileRelY - ((float)RelRow)*TileMap->TileSideInPixels;
            float MinX = CenterX - 0.5f*TileMap->TileSideInPixels;
            float MinY = CenterY - 0.5f*TileMap->TileSideInPixels;
            float MaxX = CenterX + 0.5f*TileMap->TileSideInPixels;
            float MaxY = CenterY + 0.5f*TileMap->TileSideInPixels;
            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    float PlayerR = 1.0f;
    float PlayerG = 1.0f;
    float PlayerB = 0.0f;
    float PlayerLeft = ScreenCenterX - 0.5f*TileMap->MetersToPixels*PlayerWidth;
    float PlayerTop = ScreenCenterY - TileMap->MetersToPixels*PlayerHeight;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + TileMap->MetersToPixels*PlayerWidth, PlayerTop + TileMap->MetersToPixels*PlayerHeight, PlayerR, PlayerG, PlayerB);
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
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

inline tile_map *GetTileMap(world *World, int32_t TileMapX, int32_t TileMapY) {
    tile_map *TileMap = 0;

    if((TileMapX >= 0) && (TileMapX < World->TileMapCountX) && (TileMapY >= 0) && (TileMapY < World->TileMapCountY)) {
        TileMap = &World->TileMaps[TileMapY*World->TileMapCountX + TileMapX];
    }

    return TileMap;
}

inline uint32_t GetTileValueUnchecked(world *World, tile_map *TileMap, int32_t TileX, int32_t TileY) {
    Assert(TileMap);
    Assert((TileX >= 0) && (TileX < World->CountX) && (TileY >= 0) && (TileY < World->CountY));

    uint32_t TileMapValue = TileMap->Tiles[TileY*World->CountX + TileX];
    return TileMapValue;
}

inline bool32 IsTileMapPointEmpty(world *World, tile_map *TileMap, float TestTileX, float TestTileY) {
    bool32 Empty = false;

    if(TileMap) {
        if((TestTileX >= 0) && (TestTileX < World->CountX) && (TestTileY >= 0) && (TestTileY < World->CountY)) {
            uint32_t TileMapValue = GetTileValueUnchecked(World, TileMap, TestTileX, TestTileY);
            Empty = (TileMapValue == 0);
        }
    }

    return Empty;
}

inline void RecanonicalizeCoord(world *World, int32_t TileCount, int32_t *TileMap, int32_t *Tile, float *TileRel) {
    int32_t Offset = FloorFloatToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*World->TileSideInMeters;

    Assert(*TileRel >= 0);
    Assert(*TileRel < World->TileSideInMeters);

    if(*Tile < 0) {
        *Tile = TileCount + *Tile;
        --*TileMap;
    }
    
    if(*Tile >= TileCount) {
        *Tile = *Tile - TileCount;
        ++*TileMap;
    }
}

inline canonical_position RecanonicalizePosition(world *World, canonical_position Pos) {
    canonical_position Result = Pos;

    RecanonicalizeCoord(World, World->CountX, &Result.TileMapX, &Result.TileX, &Result.TileRelX);
    RecanonicalizeCoord(World, World->CountY, &Result.TileMapY, &Result.TileY, &Result.TileRelY);

    return Result;
}

internal bool32 IsWorldPointEmpty(world *World, canonical_position CanPos) {
    bool32 Empty = false;

    tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
    Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);
    return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
    uint32_t Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
        {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 0},
        {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    };
    
    uint32_t Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
    };
    
    uint32_t Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
    };
    
    uint32_t Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
    };

    tile_map TileMaps[2][2];

    TileMaps[0][0].Tiles = (uint32_t *)Tiles00;
    TileMaps[0][1].Tiles = (uint32_t *)Tiles01;
    TileMaps[1][0].Tiles = (uint32_t *)Tiles10;
    TileMaps[1][1].Tiles = (uint32_t *)Tiles11;

    world World;
    World.TileMapCountX = 2;
    World.TileMapCountY = 2;

    World.CountX = TILE_MAP_COUNT_X;
    World.CountY = TILE_MAP_COUNT_Y;
    
    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (float)(World.TileSideInPixels / World.TileSideInMeters);

    World.UpperLeftX = -(float)World.TileSideInPixels/2;
    World.UpperLeftY = 0;

    float PlayerHeight = 1.4f;
    float PlayerWidth = 0.75f*PlayerHeight;

    World.TileMaps = (tile_map *)TileMaps;
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized) {
        GameState->PlayerP.TileMapX = 0;
        GameState->PlayerP.TileMapY = 0;
        GameState->PlayerP.TileX = 3;
        GameState->PlayerP.TileY = 3;
        GameState->PlayerP.TileRelX = 5.0f;
        GameState->PlayerP.TileRelY = 5.0f;

        Memory->IsInitialized = true;
    }

    tile_map *TileMap = GetTileMap(&World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY);
    Assert(TileMap);

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
                dPlayerY = -1.0f;
            }
            if(Controller->MoveDown.EndedDown) {
                dPlayerY = 1.0f;
            }
            if(Controller->MoveLeft.EndedDown) {
                dPlayerX = -1.0f;
            }
            if(Controller->MoveRight.EndedDown) {
                dPlayerX = 1.0f;
            }
            dPlayerX *= 2.0f;
            dPlayerY *= 2.0f;

            canonical_position NewPlayerP = GameState->PlayerP;
            NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
            NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
            NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);

            canonical_position PlayerLeft = NewPlayerP;
            PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
            PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);

            canonical_position PlayerRight = NewPlayerP;
            PlayerRight.TileRelX += 0.5f*PlayerWidth;
            PlayerRight = RecanonicalizePosition(&World, PlayerRight);

            if(IsWorldPointEmpty(&World, NewPlayerP) && IsWorldPointEmpty(&World, PlayerLeft) && IsWorldPointEmpty(&World, PlayerRight)) {
                GameState->PlayerP = NewPlayerP;
            }
        }
    }
    
    DrawRectangle(Buffer, 0.0f, 0.0f, (float)Buffer->Width, (float)Buffer->Height, 1.0f, 0.0f, 1.0f);

    for(int Row = 0; Row < 9; ++Row) {
        for(int Column = 0; Column < 17; ++Column) {
            uint32_t TileID = GetTileValueUnchecked(&World, TileMap, Column, Row);
            float Gray = 0.5f;
            if(TileID == 1) {
                Gray = 1.0f;
            }

            if((Column == GameState->PlayerP.TileX) && (Row == GameState->PlayerP.TileY)) {
                Gray = 0.0f;
            }

            float MinX = World.UpperLeftX + ((float)Column)*World.TileSideInPixels;
            float MinY = World.UpperLeftY + ((float)Row)*World.TileSideInPixels;
            float MaxX = MinX + World.TileSideInPixels;
            float MaxY = MinY + World.TileSideInPixels;
            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }

    float PlayerR = 1.0f;
    float PlayerG = 1.0f;
    float PlayerB = 0.0f;
    float PlayerLeft = World.UpperLeftX + World.TileSideInPixels*GameState->PlayerP.TileX + World.MetersToPixels*GameState->PlayerP.TileRelX - 0.5f * World.MetersToPixels*PlayerWidth;
    float PlayerTop = World.UpperLeftY + World.TileSideInPixels*GameState->PlayerP.TileY + World.MetersToPixels*GameState->PlayerP.TileRelY - World.MetersToPixels*PlayerHeight;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + World.MetersToPixels*PlayerWidth, PlayerTop +  World.MetersToPixels*PlayerHeight, PlayerR, PlayerG, PlayerB);
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
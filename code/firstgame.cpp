#include "firstgame.h"
#include "prototype_random.h"

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

internal void InitalizeArena(memory_arena *Arena, memory_index Size, uint8_t *Base) {
    Arena->Size = Size;
    Arena->Base = Base;
    Arena->Used = 0;
}

#pragma pack(push, 1)
struct bitmap_header
{
    uint16_t FileType;
    uint32_t FileSize;
    uint16_t Reserved1;
    uint16_t Reserved2;
    uint32_t BitmapOffset;
    uint32_t Size;
    int32_t Width;
    int32_t Height;
    uint16_t Planes;
    uint16_t BitsPerPixel;
};

internal uint32_t * DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName) {
    uint32_t *Result = 0;
    
    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);    
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32_t *Pixels = (uint32_t *)((uint8_t *)ReadResult.Contents + Header->BitmapOffset);
        Result = Pixels;
    }

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
        DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");

        GameState->PlayerP.AbsTileX = 0;
        GameState->PlayerP.AbsTileY = 0;
        GameState->PlayerP.OffsetX = 5.0f;
        GameState->PlayerP.OffsetY = 5.0f;

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
        TileMap->TileChunkCountZ = 2;

        TileMap->TileChunks = PushArray(&GameState->WorldArena, TileMap->TileChunkCountX*TileMap->TileChunkCountY*TileMap->TileChunkCountZ, tile_chunk);

        TileMap->TileSideInMeters = 1.4f;

        uint32_t RandomNumberIndex = 0;

        uint32_t TilesPerWidth = 17;
        uint32_t TilesPerHeight = 9;
        uint32_t ScreenY = 0;
        uint32_t ScreenX = 0;
        uint32_t AbsTileZ = 0;

        bool32 DoorLeft = false;
        bool32 DoorRight = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;
        bool32 DoorUp = false;
        bool32 DoorDown = false;
        for(uint32_t ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex) {
            // TODO: Random number generator!
            Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));

            uint32_t RandomChoice;
            if(DoorUp || DoorDown) {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
            }
            else {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
            }

            bool32 CreatedZDoor = false;
            if(RandomChoice == 2) {
                CreatedZDoor = true;
                if(AbsTileZ == 0) {
                    DoorUp = true;
                }
                else {
                    DoorDown = true;
                }
            }
            else if(RandomChoice == 1) {
                DoorRight = true;
            }
            else {
                DoorTop = true;
            }

            for(uint32_t TileY = 0; TileY < TilesPerHeight; ++TileY) {
                for(uint32_t TileX = 0; TileX < TilesPerWidth; ++TileX) {
                    uint32_t AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32_t AbsTileY = ScreenY*TilesPerHeight + TileY;
                    
                    uint32_t TileValue = 1;
                    if((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2)))) {
                        TileValue = 2;
                    }

                    if((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2)))) {
                        TileValue = 2;
                    }
                    
                    if((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2)))) {
                        TileValue = 2;
                    }

                    if((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2)))) {
                        TileValue = 2;
                    }

                    if((TileX == 10) && (TileY == 6)) {
                        if(DoorUp) {
                            TileValue = 3;
                        }

                        if(DoorDown) {
                            TileValue = 4;
                        }
                    }
                        
                    SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
                }
            }

            DoorLeft = DoorRight;
            DoorBottom = DoorTop;

            if(CreatedZDoor) {
                DoorDown = !DoorDown;
                DoorUp = !DoorUp;
            }
            else {
                DoorUp = false;
                DoorDown = false;
            }

            DoorRight = false;
            DoorTop = false;

            if(RandomChoice == 2) {
                if(AbsTileZ == 0) {
                    AbsTileZ = 1;
                } else {
                    AbsTileZ = 0;
                }                
            } else if(RandomChoice == 1) {
                ScreenX += 1;
            } else {
                ScreenY += 1;
            }
        }
        
        Memory->IsInitialized = true;
    }

    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;

    int32_t TileSideInPixels = 60;
    float MetersToPixels = (float)(TileSideInPixels / TileMap->TileSideInMeters);

    float LowerLeftX = -(float)TileSideInPixels/2;
    float LowerLeftY = (float)Buffer->Height;

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
            NewPlayerP.OffsetX += Input->dtForFrame*dPlayerX;
            NewPlayerP.OffsetY += Input->dtForFrame*dPlayerY;
            NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

            tile_map_position PlayerLeft = NewPlayerP;
            PlayerLeft.OffsetX -= 0.5f*PlayerWidth;
            PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

            tile_map_position PlayerRight = NewPlayerP;
            PlayerRight.OffsetX += 0.5f*PlayerWidth;
            PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

            if(IsTileMapPointEmpty(TileMap, NewPlayerP) && IsTileMapPointEmpty(TileMap, PlayerLeft) && IsTileMapPointEmpty(TileMap, PlayerRight)) {
                if(AreOnSametile(&GameState->PlayerP, &NewPlayerP)) {
                    uint32_t NewTileValue = GetTileValue(TileMap, NewPlayerP);
                    if(NewTileValue == 3) {
                        ++NewPlayerP.AbsTileZ;
                    } else if(NewTileValue == 4) {
                        --NewPlayerP.AbsTileZ;
                    }
                }
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
            uint32_t TileID = GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ);

            if(TileID > 0) {
                float Gray = 0.5f;
                if(TileID == 2) {
                    Gray = 1.0f;
                }

                if(TileID > 2) {
                    Gray = 0.25f;
                } 

                if((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY)) {
                    Gray = 0.0f;
                }

                float CenterX = ScreenCenterX - MetersToPixels*GameState->PlayerP.OffsetX + ((float)RelColumn)*TileSideInPixels;
                float CenterY = ScreenCenterY + MetersToPixels*GameState->PlayerP.OffsetY - ((float)RelRow)*TileSideInPixels;
                float MinX = CenterX - 0.5f*TileSideInPixels;
                float MinY = CenterY - 0.5f*TileSideInPixels;
                float MaxX = CenterX + 0.5f*TileSideInPixels;
                float MaxY = CenterY + 0.5f*TileSideInPixels;
                DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
            }
        }
    }

    float PlayerR = 1.0f;
    float PlayerG = 1.0f;
    float PlayerB = 0.0f;
    float PlayerLeft = ScreenCenterX - 0.5f*MetersToPixels*PlayerWidth;
    float PlayerTop = ScreenCenterY - MetersToPixels*PlayerHeight;
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + MetersToPixels*PlayerWidth, PlayerTop + MetersToPixels*PlayerHeight, PlayerR, PlayerG, PlayerB);
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
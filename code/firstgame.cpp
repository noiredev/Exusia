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

internal void DrawRectangle(game_offscreen_buffer *Buffer, v2 vMin, v2 vMax, float R, float G, float B) {
    int32_t MinX = RoundFloatToInt32(vMin.X);
    int32_t MinY = RoundFloatToInt32(vMin.Y);
    int32_t MaxX = RoundFloatToInt32(vMax.X);
    int32_t MaxY = RoundFloatToInt32(vMax.Y);

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

internal void DrawBitmap(game_offscreen_buffer *Buffer, loaded_bitmap *Bitmap, float RealX, float RealY, int32_t AlignX = 0, int32_t AlignY = 0) {
    RealX -= (float)AlignX;
    RealY -= (float)AlignY;

    int32_t MinX = RoundFloatToInt32(RealX);
    int32_t MinY = RoundFloatToInt32(RealY);
    int32_t MaxX = RoundFloatToInt32(RealX + (float)Bitmap->Width);
    int32_t MaxY = RoundFloatToInt32(RealY + (float)Bitmap->Height);

    int32_t SourceOffsetX = 0;
    if(MinX < 0) {
        SourceOffsetX = -MinX;
        MinX = 0;
    }

    int32_t SourceOffsetY = 0;
    if(MinY < 0) {
        SourceOffsetY = -MinY;
        MinY = 0;
    }

    if(MaxX > Buffer->Width) {
        MaxX = Buffer->Width;
    }

    if(MaxY > Buffer->Height) {
        MaxY = Buffer->Height;
    }

    uint32_t *SourceRow = Bitmap->Pixels + Bitmap->Width*(Bitmap->Height - 1);
    SourceRow += -Bitmap->Width*SourceOffsetY + SourceOffsetX;
    uint8_t *DestRow = ((uint8_t *)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch);
    for(int Y = MinY; Y < MaxY; ++Y) {
        uint32_t *Dest = (uint32_t *)DestRow;
        uint32_t *Source = SourceRow;
        for(int X = MinX; X < MaxX; ++X) {   
            // Need to optimise in the future
            float A = (float)((*Source >> 24) & 0xFF) / 255.0f;
            float SR = (float)((*Source >> 16) & 0xFF);
            float SG = (float)((*Source >> 8) & 0xFF);
            float SB = (float)((*Source >> 0) & 0xFF);

            float DR = (float)((*Dest >> 16) & 0xFF);
            float DG = (float)((*Dest >> 8) & 0xFF);
            float DB = (float)((*Dest >> 0) & 0xFF);

            float R = (1 - A)*DR + A*SR;
            float G = (1 - A)*DG + A*SG;
            float B = (1 - A)*DB + A*SB;

            *Dest = (((uint32_t)(R + 0.5f) << 16) | 
                    ((uint32_t)(G + 0.5f) << 8) | 
                    ((uint32_t)(B + 0.5f) << 0)); 

            ++Dest;
            ++Source;
        }

        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap->Width;
    }
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
    uint32_t Compression;
    uint32_t SizeOfBitmap;
    int32_t HorzResolution;
    int32_t VertResolution;
    uint32_t ColorsUsed;
    uint32_t ColorsImportant;

    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap DEBUGLoadBMP(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *FileName) {
    loaded_bitmap Result = {};
    
    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);    
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        uint32_t *Pixels = (uint32_t *)((uint8_t *)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;

        Assert(Header->Compression == 3);

        uint32_t RedMask = Header->RedMask;
        uint32_t GreenMask = Header->GreenMask;
        uint32_t BlueMask = Header->BlueMask;
        uint32_t AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(AlphaShift.Found);

        uint32_t *SourceDest = Pixels;
        for(int32_t Y = 0; Y < Header->Height; ++Y) {
            for(int32_t X = 0; X < Header->Width; ++X) {
                uint32_t C = *SourceDest;
                *SourceDest++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) | 
                                 (((C >> RedShift.Index) & 0xFF) << 16) | 
                                 (((C >> GreenShift.Index) & 0xFF) << 8) | 
                                 (((C >> BlueShift.Index) & 0xFF) << 0));
            }
        }
    }

    return Result;
}

inline entity *GetEntity(game_state *GameState, uint32_t Index) {
    entity *Entity = 0;

    if((Index > 0) && (Index < ArrayCount(GameState->Entities))) {
        Entity = &GameState->Entites[Index];
    }

    return Entity;
}

internal void InitializePlayer(entity *Entity) {
    Entity = {};

    Entity->Exists = true;
    Entity->P.AbsTileX = 0;
    Entity->P.AbsTileY = 0;
    Entity->P.Offset.X = 5.0f;
    Entity->P.Offset.Y = 5.0f;
}

internal void MovePlayer() {
    tile_map_position OldPlayerP = GameState->PlayerP;

    float PlayerSpeed = 10.0f;
    ddPlayer *= PlayerSpeed;

    ddPlayer += -1.5f*GameState->dPlayerP;

    if((ddPlayer.X != 0.0f) && (ddPlayer.Y != 0.0f)) {
        ddPlayer *= 0.707106781187f;
    }

    tile_map_position NewPlayerP = GameState->PlayerP;
    v2 PlayerDelta = 0.75f*ddPlayer*Square(Input->dtForFrame) + GameState->dPlayerP*Input->dtForFrame
    NewPlayerP.Offset += PlayerDelta;
    GameState->dPlayerP += ddPlayer*Input->dtForFrame;
    NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

#if 0            
    tile_map_position PlayerLeft = NewPlayerP;
    PlayerLeft.Offset.X -= 0.5f*PlayerWidth;
    PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);

    tile_map_position PlayerRight = NewPlayerP;
    PlayerRight.Offset.X += 0.5f*PlayerWidth;
    PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

    bool32 Collided = false;
    tile_map_position ColP = {};
    if(!IsTileMapPointEmpty(TileMap, NewPlayerP)) {
        ColP = NewPlayerP;
        Collided = true;
    }
    if(!IsTileMapPointEmpty(TileMap, PlayerLeft)) {
        ColP = PlayerLeft;
        Collided = true;
    }
    if(!IsTileMapPointEmpty(TileMap, PlayerRight)) {
        ColP = PlayerRight;
        Collided = true;
    }
    
    if(Collided) {
        v2 r = {0,0};
        if(ColP.AbsTileX < GameState->PlayerP.AbsTileX) {
            r = v2{1, 0};
        }
        if(ColP.AbsTileX > GameState->PlayerP.AbsTileX) {
            r = v2{-1, 0};
        }
        if(ColP.AbsTileY < GameState->PlayerP.AbsTileY) {
            r = v2{0, 1};
        }
        if(ColP.AbsTileY > GameState->PlayerP.AbsTileY) {
            r = v2{0, -1};
        }

        GameState->dPlayerP = GameState->dPlayerP - 1*InnerProduct(GameState->dPlayerP, r)*r;
    }
    else {
        GameState->PlayerP = NewPlayerP;
    }
#else
    uint32_t MinTileX = 0;
    uint32_t MinTileY = 0;
    uint32_t OnePastMaxTileX = 0;
    uint32_t OnePastMaxTileY = 0;
    uint32_t AbsTileZ = GameState->PlayerP.AbsTileZ;
    tile_map_position = BestPlayerP = GameState->PlayerP;
    float BestDistanceSq = LengthSq(PlayerDelta);
    for(uint32_t AbsTileY = MinTileY; AbsTileY != OnePastMaxTileY; ++AbsTileY) {
        for(uint32_t AbsTileX = MinTileX; AbsTileX != OnePastMaxtileX; ++AbsTileX) {
            tile_map_position TestTileP = CenteredTilePoint(AbsTileX, AbsTileY, AbsTileZ);
            uint32_t TileValue = GetTileValue(TileMap, TestTileP);
            if(IsTileValueEmpty(TileValue)) {
                v2 MinCorner = -0.5f*v2{TileMap->TileSideInMeters, TileMap->TileSideInMeters};
                v2 MaxCorner = 0.5f*v2{TileMap->TileSideInMeters, TileMap->TileSideInMeters};

                tile_map_difference RelNewPlayerP = Subtract(TileMap, &TestTile, &NewPlayerP);
                v2 TestP = ClosestPointInRectangle(MinCorner, MaxCorner, RelNewPlayerP);
                if() {
                    
                }
            }
        }
    }

    if(!AreOnSameTile(&OldPlayerP, &GameState->PlayerP)) {
    uint32_t NewTileValue = GetTileValue(TileMap, GameState->PlayerP);

    if(NewTileValue == 3) {
        ++GameState->PlayerP.AbsTileZ;
    }
    else if(NewTileValue == 4) {
        --GameState->PlayerP.AbsTileZ;
    }    
}

#endif
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
        GameState->Backdrop = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");

        hero_bitmaps *Bitmap;

        Bitmap = GameState->HeroBitmaps;
        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;
        ++Bitmap;

        Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
        Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
        Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
        Bitmap->AlignX = 72;
        Bitmap->AlignY = 182;
        ++Bitmap;

        GameState->CameraP.AbsTileX = 17/2;
        GameState->CameraP.AbsTileY = 9/2;

        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), (uint8_t *)Memory->PermanentStorage + sizeof(game_state));

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
            } else {
                RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;
            }

            bool32 CreatedZDoor = false;
            if(RandomChoice == 2) {
                CreatedZDoor = true;
                if(AbsTileZ == 0) {
                    DoorUp = true;
                } else {
                    DoorDown = true;
                }
            } else if(RandomChoice == 1) {
                DoorRight = true;
            } else {
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
            } else {
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
        } else {
            // NOTE: Use digital movement tuning
            v2 ddPlayer = {};
            
            if(Controller->MoveUp.EndedDown) {
                GameState->HeroFacingDirection = 1;
                ddPlayer.Y = 1.0f;
            }
            if(Controller->MoveDown.EndedDown) {
                GameState->HeroFacingDirection = 3;
                ddPlayer.Y = -1.0f;
            }
            if(Controller->MoveLeft.EndedDown) {
                GameState->HeroFacingDirection = 2;
                ddPlayer.X = -1.0f;
            }
            if(Controller->MoveRight.EndedDown) {
                GameState->HeroFacingDirection = 0;
                ddPlayer.X = 1.0f;
            }
            

                
        }
    }

    entity *CameraFollowingEntity = GetEntity(GameState, GameState->CameraFollowingEntityIndex)
    if(CameraFollowingEntity) {
        GameState->CameraP.AbsTileZ = CameraFollowingEntity->P.AbsTileZ;
    
        tile_map_difference Diff = Subtract(TileMap, &CameraFollowingEntity->PlayerP, &GameState->CameraP);
        if(Diff.dXY.X > (9.0f*TileMap->TileSideInMeters)) {
            GameState->CameraP.AbsTileX += 17;
        }
        if(Diff.dXY.X < -(9.0f*TileMap->TileSideInMeters)) {
            GameState->CameraP.AbsTileX -= 17;
        }
        if(Diff.dXY.Y > (5.0f*TileMap->TileSideInMeters)) {
            GameState->CameraP.AbsTileY += 9;
        }
        if(Diff.dXY.Y < -(5.0f*TileMap->TileSideInMeters)) {
            GameState->CameraP.AbsTileY -= 9;
        }
    }

    DrawBitmap(Buffer, &GameState->Backdrop, 0, 0);

    float ScreenCenterX = 0.5f*(float)Buffer->Width;
    float ScreenCenterY = 0.5f*(float)Buffer->Height;

    for(int32_t RelRow = -10; RelRow < 10; ++RelRow) {
        for(int32_t RelColumn = -20; RelColumn < 20; ++RelColumn) {
            uint32_t Column = GameState->CameraP.AbsTileX + RelColumn;
            uint32_t Row = GameState->CameraP.AbsTileY + RelRow;
            uint32_t TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);

            if(TileID > 1) {
                float Gray = 0.5f;
                if(TileID == 2) {
                    Gray = 1.0f;
                }

                if(TileID > 2) {
                    Gray = 0.25f;
                } 

                if((Column == GameState->CameraP.AbsTileX) && (Row == GameState->CameraP.AbsTileY)) {
                    Gray = 0.0f;
                }

                v2 TileSide = {0.5f*TileSideInPixels, 0.5f*TileSideInPixels};
                v2 Center = {ScreenCenterX - MetersToPixels*GameState->CameraP.Offset.X + ((float)RelColumn)*TileSideInPixels, 
                             ScreenCenterY + MetersToPixels*GameState->CameraP.Offset.Y - ((float)RelRow)*TileSideInPixels};
                v2 Min = Center - TileSide;
                v2 Max = Center + TileSide;
                DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
            }
        }
    }

    entity *Entity = GameState->Entites;
    for(uint32_t EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex, ++Entity) {
        if() {
            tile_map_difference Diff = Subtract(TileMap, &GameState->PlayerP, &GameState->CameraP);

            float PlayerR = 1.0f;
            float PlayerG = 1.0f;
            float PlayerB = 0.0f;
            float PlayerGroundPointX = ScreenCenterX + MetersToPixels*Diff.dXY.X;
            float PlayerGroundPointY = ScreenCenterY - MetersToPixels*Diff.dXY.Y; 
            v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f*MetersToPixels*PlayerWidth, PlayerGroundPointY - MetersToPixels*PlayerHeight};
            v2 PlayerWidthHeight = {PlayerWidth, PlayerHeight};
            DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MetersToPixels*PlayerWidthHeight, PlayerR, PlayerG, PlayerB);

            hero_bitmaps *HeroBitmaps = &GameState->HeroBitmaps[GameState->HeroFacingDirection];
            DrawBitmap(Buffer, &HeroBitmaps->Torso, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
            DrawBitmap(Buffer, &HeroBitmaps->Cape, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
            DrawBitmap(Buffer, &HeroBitmaps->Head, PlayerGroundPointX, PlayerGroundPointY, HeroBitmaps->AlignX, HeroBitmaps->AlignY);
        }
    }
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
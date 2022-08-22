#include "firstgame.h"

internal void GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz) {
    int16_t ToneVolume = 3000;
    // int ToneHz = 256;
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

        GameState->tSine += 2.0f*Pi32*1.0f / (float)WavePeriod;
        if(GameState->tSine > 2.0f*Pi32) {
            GameState->tSine -= 2.0f*Pi32;
        }
    } 
}

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset) {
    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for(int Y = 0; Y < Buffer->Height; ++Y) {
        uint32_t *Pixel = (uint32_t *)Row;
        for(int X = 0; X < Buffer->Width; ++X) {
            /*
              Pixel in memory: BB GG RR xx
              Little endian architecture

              When you load it into memory it becomes backwards so it is:
              0x xxBBGGRR
              However, windows did not like this so they reversed it.
              0x xxRRGGBB
            */
            uint8_t Blue = (uint8_t)(X + BlueOffset);
            uint8_t Green = (uint8_t)(Y + GreenOffset);

            /*
              Memory:   BB GG RR xx
              Register: xx RR GG BB

              Pixel (32 - bits)
            */

            *Pixel++ = ((Green << 16) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void RenderPlayer(game_offscreen_buffer *Buffer, int PlayerX, int PlayerY) {
    uint8_t *EndOfBuffer = (uint8_t *)Buffer->Memory + Buffer->Pitch*Buffer->Height;
    uint32_t Color = 0xFFFFFFFF;
    int Top = PlayerY;
    int Bottom = PlayerY + 10;

    for(int X = PlayerX; X < PlayerX+10; ++X) {
        uint8_t *Pixel = (uint8_t *)Buffer->Memory + X*Buffer->BytesPerPixel + Top*Buffer->Pitch;
        for(int Y = Top; Y < Bottom; ++Y) {
            if(Pixel >= Buffer->Memory && Pixel < EndOfBuffer) {
                *(uint32_t *)Pixel = Color;
                Pixel += Buffer->Pitch;
            }
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized) {
        char *Filename = __FILE__;

        debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Thread, Filename);
        if(File.Contents) {
            Memory->DEBUGPlatformWriteEntireFile(Thread, "test.out", File.ContentsSize, File.Contents);
            Memory->DEBUGPlatformFreeFileMemory(Thread, File.Contents);
        }

        GameState->ToneHz = 256;
        GameState->tSine = 0.0f;

        GameState->PlayerX = 100;
        GameState->PlayerY = 100;
    
        Memory->IsInitialized = true;
    }

    for(int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex) {

        game_controller_input *Controller = GetController(Input, ControllerIndex);

        if(Controller->IsAnalog) {
        GameState->BlueOffset += (int)(4.0f*Controller->StickAverageX);
        GameState->ToneHz = 256 + (int)(128.0f*Controller->StickAverageY);
        } else {
            // use digital movement tuning
            if(Controller->MoveLeft.EndedDown) {
                GameState->BlueOffset -= 1;
            }
            if(Controller->MoveRight.EndedDown) {
                GameState->BlueOffset += 1;
            }
        }

        GameState->PlayerX += (int)(1.0f*Controller->StickAverageX);
        GameState->PlayerY -= (int)(1.0f*Controller->StickAverageY);
        if(Controller->ActionDown.EndedDown) {
            GameState->PlayerY -= 10;
        }
    }

    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
    RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY);

    RenderPlayer(Buffer, Input->MouseX, Input->MouseY);
    for(int ButtonIndex = 0; ButtonIndex < ArrayCount(Input->MouseButtons); ++ButtonIndex) {
        if(Input->MouseButtons[ButtonIndex].EndedDown) {
            RenderPlayer(Buffer, 10 + 20 * ButtonIndex, 10);
        }
    }
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}
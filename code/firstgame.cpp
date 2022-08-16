#include "firstgame.h"

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz) {
    local_persist float tSine;
    int16_t ToneVolume = 3000;
    // int ToneHz = 256;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
        float SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f*Pi32*1.0f / (float)WavePeriod;
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

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer->Pitch;
    }
}

internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer) {
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if(!Memory->IsInitialized) {
        char *Filename = __FILE__;

        debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
        if(File.Contents) {
            DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
            DEBUGPlatformFreeFileMemory(File.Contents);
        }


        GameState->ToneHz = 256;
    
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

        if(Controller->ActionDown.EndedDown) {
            GameState->GreenOffset += 1;
        }
    }
    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}

internal void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer) {
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(SoundBuffer, GameState->ToneHz);
}
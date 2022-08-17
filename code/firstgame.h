#if !defined(FIRSTGAME_H)

#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int32_t bool32;




#if FIRSTGAME_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terrabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32_t SafeTruncateUInt64(uint64_t Value) {
    // TODO: Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32_t Result = (uint32_t)Value;
    return Result;
}

#if FIRSTGAME_INTERNAL
/* IMPORTANT:
    These are not for doing anything in the shipped version
    They are for blocking and the write doesn't protect against lost data.
*/
struct debug_read_file_result {
    uint32_t ContentsSize;
    void *Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(char *Filename, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

// internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
// internal void DEBUGPlatformFreeFileMemory(void *Memory);
// internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32_t MemorySize, void *Memory);

struct game_offscreen_buffer {
    // Pixels are always 32-bits wide, Little Endian 0x xx RR GG BB, Memory Order: BB GG RR xx
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer {
    int SamplesPerSecond;
    int SampleCount;
    int16_t *Samples;
};

struct game_button_state {
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input {
    bool32 IsConnected;
    bool32 IsAnalog;
    float StickAverageX;
    float StickAverageY;

    union {
        game_button_state Buttons[12];
        struct {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;

            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Start;
            game_button_state Back;

            // fake button
            game_button_state Terminator;
        };
    };
};

struct game_input {
    // TODO: Insert clock value
    game_controller_input Controllers[5];
};
inline game_controller_input *GetController(game_input *Input, int ControllerIndex) {
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    game_controller_input *Result = &Input->Controllers[ControllerIndex];
    return Result;
}
struct game_memory {
    bool32 IsInitialized;
    uint64_t PermanentStorageSize;
    void *PermanentStorage; // Required to be cleared to zero at startup

    uint64_t TransientStorageSize;
    void *TransientStorage; // Required to be cleared to zero at startup
};

// Needs - timing, input, bitmap buffer to use, sound buffer to use
internal void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer);

// Must be a very fast function
internal void GameGetSoundSamples(game_memory *Memory, game_sound_output_buffer *SoundBuffer);

struct game_state {
    int ToneHz;
    int GreenOffset;
    int BlueOffset;
};

#define FIRSTGAME_H
#endif
#if !defined(WIN32_FIRSTGAME_H)

struct win32_offscreen_buffer {
    // Pixels are always 32-bits wide, Little Endian 0x xx RR GG BB, Memory Order: BB GG RR xx
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension {
    int Width;
    int Height;
};

struct win32_sound_output {
    int SamplesPerSecond;
    uint32_t RunningSampleIndex;
    int BytesPerSample;
    int SecondaryBufferSize;
    DWORD SafetyBytes;
};

struct win32_debug_time_marker {
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;
    DWORD OutputByteCount;
    DWORD ExpectedFlipPlayCursor;

    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
};

struct win32_game_code {
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    // Both of the callbacks can be 0
    game_update_and_render *UpdateAndRender;
    game_get_sound_samples *GetSoundSamples;

    bool32 IsValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_replay_buffer {
    HANDLE FileHandle;
    HANDLE MemoryMap;
    char Filename[WIN32_STATE_FILE_NAME_COUNT];
    void *MemoryBlock;
};

struct win32_state {
    uint64_t TotalSize;
    void *GameMemoryBlock;
    win32_replay_buffer ReplayBuffers[4];

    HANDLE RecordingHandle;
    int InputRecordingIndex;
    
    HANDLE PlaybackHandle;
    int InputPlayingIndex;

    char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
    char *OnePastLastEXEFileNameSlash;
};

#define WIN32_FIRSTGAME_H
#endif
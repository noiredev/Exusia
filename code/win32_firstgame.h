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
    float tSine;
    int LatencySampleCount;
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

#define WIN32_FIRSTGAME_H
#endif
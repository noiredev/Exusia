#include "firstgame.h"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_firstgame.h"

// Some temporary global variables
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER SecondaryBuffer;
global_variable int64_t GlobalPerfCountFrequency;

// Getting around linking error in the compiler
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);

X_INPUT_SET_STATE(XInputSetStateStub) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void CatStrings(size_t SourceACount, char *SourceA, size_t SourceBCount, char *SourceB, size_t DestCount, char *Dest) {
    for(int Index = 0; Index < SourceACount; ++Index) {
        *Dest++ = *SourceA++;
    }
    for(int Index = 0; Index < SourceACount; ++Index) {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

internal void Win32GetEXEFileName(win32_state *State) {
    DWORD SizeOfFileName = GetModuleFileName(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for(char *Scan = State->EXEFileName; *Scan; ++Scan) {
        if(*Scan == '\\') {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
}

internal int StringLength(char *String) {
    int Count = 0;
    while(*String++) {
        ++Count;
    }
    return Count;
}

internal void Win32BuildEXEPathFileName(win32_state *State, char *Filename, int DestCount, char *Dest) {
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName, StringLength(Filename), Filename, DestCount, Dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory) {
    if(Memory) {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile) {
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize)) {
            uint32_t FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize.QuadPart, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents) {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead)) {
                    // File read successfully
                    Result.ContentsSize = FileSize32;
                } else {
                    DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
                    Result.Contents = 0;
                }
            } else {
                // TODO: logging
            }
        } else {
            // TODO: Logging
        }
        CloseHandle(FileHandle);
    } else {
        // TODO: logging
    }

    return Result;

}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile) {
    bool32 Result = false;

    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE) {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
            // File read successfully
            Result = (BytesWritten == MemorySize);
        } else {
            // TODO: Logging
        }
        CloseHandle(FileHandle);
    } else {
        // TODO: logging
    }

    return Result;
}

inline FILETIME Win32GetLastWriteTime(char *Filename) {
    FILETIME LastWriteTime = {};
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data)) {
        LastWriteTime = Data.ftLastWriteTime;
    }
    return LastWriteTime;
}

internal win32_game_code Win32LoadGameCode(char *SourceDLLName, char *TempDLLName) {
    win32_game_code Result = {};

    // TODO: Proper pathing
    // TODO: Automatic determination of when updates are necessary.

    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

    CopyFile(SourceDLLName, TempDLLName, FALSE);
    Result.GameCodeDLL = LoadLibraryA(TempDLLName);
    if(Result.GameCodeDLL) {
        Result.UpdateAndRender = (game_update_and_render *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
        Result.GetSoundSamples = (game_get_sound_samples *)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

        Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }

    if(!Result.IsValid) {
        Result.UpdateAndRender = 0;
        Result.GetSoundSamples = 0;
    }

    return Result;
}

internal void Win32UnloadGameCode(win32_game_code *GameCode) {
    if(GameCode->GameCodeDLL) {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
    GameCode->GetSoundSamples = 0;
}

internal void Win32LoadXInput(void) {
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary) {
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if(!XInputLibrary) {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if(XInputLibrary) {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
    } else {
        // TODO log error
    }
}

internal void Win32InitDSound(HWND Window, int32_t SamplesPerSecond, int32_t BufferSize) {
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if(DSoundLibrary) {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
                    if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
                        // Finally set the format.
                    } else {
                        // TODO log error
                    }
                } else {
                    // TODO log error
                }
            } else {
                // TODO log error
            }

            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            
            if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0))) {
                if(SUCCEEDED(SecondaryBuffer->SetFormat(&WaveFormat))) {
                    
                } else {
                    // TODO log error
                }
            }

        BufferDescription.dwBufferBytes = BufferSize;
        } else {
            // TODO log error
        }
    }
}

win32_window_dimension Win32GetWindowDimension(HWND Window) {
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

// This is only called once every time the user resizes the window so it is not really worth optimising for now.
internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {

    if(Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;
    
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*BytesPerPixel;

    // TODO: Probably need to clear this to black
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight) {
    // For prototyping it is always going to blit 1 to 1 pixels to make sure there are no artifacts.
    int OffsetX = 10;
    int OffsetY = 10;

    PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
    PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
    PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

    StretchDIBits(DeviceContext, 
                    /*
                    X, Y, Width, Height,
                    X, Y, Width, Height,
                    */
                    OffsetX, OffsetY, Buffer->Width, Buffer->Height,
                    0, 0, Buffer->Width, Buffer->Height,
                    Buffer->Memory,
                    &Buffer->Info,
                    DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Win32MainWindowCallBack(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam
) {
    LRESULT Result = 0;

    switch(Message) {
        case WM_SIZE: {

        } break;

        case WM_DESTROY: {
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_CLOSE: {
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP: {
#if 0
            if(WParam == TRUE) {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
            } else {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 64, LWA_ALPHA);
            }
#endif
        } break;

        case WM_SETCURSOR: {
            // SetCursor(0);
        } break;
        
        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default: {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal void Win32ClearBuffer(win32_sound_output *SoundOutput) {
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(SecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, 
                                        &Region1, &Region1Size, &Region2, &Region2Size, 
                                        0))) {
        uint8_t *DestSample = (uint8_t *)Region1;
        for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {
            *DestSample++ = 0;
        } 

        DestSample = (uint8_t *)Region2;
        for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex) {
            *DestSample++ = 0;
        } 
    }
    SecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, 
                                                DWORD ButtonBit, game_button_state *NewState) {
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal void Win32ProcessKeyboardEvent(game_button_state *NewState, bool32 IsDown) {
    if(NewState->EndedDown != IsDown) {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void Win32FillSoundBuffer(win32_sound_output *SoundOutput, 
                                    DWORD BytesToLock, DWORD BytesToWrite, 
                                    game_sound_output_buffer *SourceBuffer) {
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;

    if(SUCCEEDED(SecondaryBuffer->Lock(BytesToLock, BytesToWrite, 
                                        &Region1, &Region1Size, &Region2, &Region2Size, 
                                        0))) {

        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16_t *DestSample = (int16_t *)Region1;
        int16_t *SourceSample = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        } 

        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample =(int16_t *)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        SecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal float Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold) {
    float Result = 0;
    if(Value < -DeadZoneThreshold) {
        Result = (float)((Value + DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    } else if(Value > DeadZoneThreshold){
        Result = (float)((Value + DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }

    return Result;
}

internal void Win32GetInputFileLocation(win32_state *State, bool32 InputStream, int SlotIndex, int DestCount, char *Dest) {
    char Temp[64];
    wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state");
    Win32BuildEXEPathFileName(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer *Win32GetReplayBuffer(win32_state *State, unsigned Index) {
    Assert(Index < ArrayCount(State->ReplayBuffers));
    win32_replay_buffer *Result = &State->ReplayBuffers[Index];
    return Result;
}

internal void Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex) {
    win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
    if(ReplayBuffer->MemoryBlock) {
        State->InputRecordingIndex = InputRecordingIndex;

        char Filename[WIN32_STATE_FILE_NAME_COUNT];
        Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(Filename), Filename);
        State->RecordingHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
        CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
    }
}

internal void Win32EndRecordingInput(win32_state *State) {
    CloseHandle(State->RecordingHandle);
    State->InputRecordingIndex = 0;
}

internal void Win32BeginInputPlayback(win32_state *State, int InputPlayingIndex) {
    win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
    if(ReplayBuffer->MemoryBlock) {
        State->InputPlayingIndex = InputPlayingIndex;

        char Filename[WIN32_STATE_FILE_NAME_COUNT];
        Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(Filename), Filename);
        State->PlaybackHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

        CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
    }
}

internal void Win32EndInputPlayback(win32_state *State) {
    CloseHandle(State->PlaybackHandle);
    State->InputPlayingIndex = 0;
}

internal void Win32RecordInput(win32_state *State, game_input *NewInput) {
    DWORD BytesWritten;
    WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void Win32PlaybackInput(win32_state *State, game_input *NewInput) {
    DWORD BytesRead = 0;
    if(ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0)) {
        if(BytesRead == 0) {
            int PlayingIndex = State->InputPlayingIndex;
            Win32EndInputPlayback(State);
            Win32BeginInputPlayback(State, PlayingIndex);
            ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
        }
    }
}

internal void Win32ProcessPendingMessage(win32_state *State, game_controller_input *KeyboardController) {
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
        switch(Message.message) {
            case WM_QUIT: {
                GlobalRunning = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                uint32_t VKCode = (uint32_t)Message.wParam;

                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown) {
                    if(VKCode == 'W') {
                        Win32ProcessKeyboardEvent(&KeyboardController->MoveUp, IsDown);
                    } else if(VKCode == 'A') {
                        Win32ProcessKeyboardEvent(&KeyboardController->MoveLeft, IsDown);
                    } else if(VKCode == 'S') {
                        Win32ProcessKeyboardEvent(&KeyboardController->MoveDown, IsDown);
                    } else if(VKCode == 'D') {
                        Win32ProcessKeyboardEvent(&KeyboardController->MoveRight, IsDown);
                    } else if(VKCode == 'Q') {
                        Win32ProcessKeyboardEvent(&KeyboardController->LeftShoulder, IsDown);
                    } else if(VKCode == 'E') {
                        Win32ProcessKeyboardEvent(&KeyboardController->RightShoulder, IsDown);
                    } else if(VKCode == VK_UP) {
                        Win32ProcessKeyboardEvent(&KeyboardController->ActionUp, IsDown);
                    } else if(VKCode == VK_DOWN) {
                        Win32ProcessKeyboardEvent(&KeyboardController->ActionDown, IsDown);
                    } else if(VKCode == VK_LEFT) {
                        Win32ProcessKeyboardEvent(&KeyboardController->ActionLeft, IsDown);
                    } else if(VKCode == VK_RIGHT) {
                        Win32ProcessKeyboardEvent(&KeyboardController->ActionRight, IsDown);
                    } else if(VKCode == VK_ESCAPE) {
                        Win32ProcessKeyboardEvent(&KeyboardController->Start, IsDown);
                    } else if(VKCode == VK_SPACE) {
                        Win32ProcessKeyboardEvent(&KeyboardController->Back, IsDown);
                    } 
#if FIRSTGAME_INTERNAL
                    else if(VKCode == 'P') {
                        if(IsDown) {
                            GlobalPause = !GlobalPause;
                        }
                    } else if(VKCode == 'L') {
                        if(IsDown) {
                            if(State->InputPlayingIndex == 0) {
                                if(State->InputRecordingIndex == 0) {
                                    Win32BeginRecordingInput(State, 1);
                                } else {
                                    Win32EndRecordingInput(State);
                                    Win32BeginInputPlayback(State, 1);
                                }
                            } else {
                                Win32EndInputPlayback(State);
                            }
                        }
                    } 
#endif
                }

                bool32 AltKeyWasDown = Message.lParam & (1 << 29);
                if(VKCode == VK_F4 && AltKeyWasDown) {
                    GlobalRunning = false;
                }
            } break;
            
            default: {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
            } break;
        }
    }
}

inline LARGE_INTEGER Win32GetWallClock(void) {
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End) {
    float SecondsElapsedForWork = ((float)End.QuadPart - Start.QuadPart / (float)GlobalPerfCountFrequency);
    return SecondsElapsedForWork;
}

internal void Win32DebugDrawVertical(win32_offscreen_buffer *Backbuffer, int X, int Top, int Bottom, uint32_t Color) {
    if(Top <= 0) {
        Top = 0;
    }

    if(Bottom > Backbuffer->Height) {
        Bottom = Backbuffer->Height;
    }

    if((X >= 0) && (X < Backbuffer->Width)) {
        uint8_t *Pixel = (uint8_t *)Backbuffer->Memory + X*Backbuffer->BytesPerPixel + Top*Backbuffer->Pitch;
        for(int Y = Top; Y < Bottom; ++Y) {
            *(uint32_t *)Pixel = Color;
            Pixel += Backbuffer->Pitch;
        }
    }
}

inline void Win32DrawSoundBufferMarker(win32_offscreen_buffer *Backbuffer, win32_sound_output *SoundOutput, 
                                        float C, int PadX, int Top, int Bottom, DWORD Value, uint32_t Color) {
    int X = PadX + (int)(C * (float)Value);
    Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}

/*
internal void Win32DebugSyncDisplay(win32_offscreen_buffer *Backbuffer, 
                                    int MarkerCount, win32_debug_time_marker *Markers,
                                    int CurrentMarkerIndex,
                                    win32_sound_output *SoundOutput, float TargetSecondsPerFrame) {
    int PadX = 16;
    int PadY = 16;

    int LineHeight = 64;

    float C = (float)(Backbuffer->Width - 2*PadX) / (float)SoundOutput->SecondaryBufferSize;
    for(int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex) {
        // TODO: Bug fix this and make audio output better in the future
        win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor < (DWORD)SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < (DWORD)SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < (DWORD)SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < (DWORD)SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < (DWORD)SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < (DWORD)SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0xFFFFFF00;
        DWORD PlayWindowColor = 0xFFFF00FF;

        int Top = PadY;
        int Bottom = PadY + LineHeight;
        if(MarkerIndex == CurrentMarkerIndex) {
            Top += LineHeight+PadY;
            Bottom += LineHeight+PadX;

            int FirstTop = Top;

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);

            Top += LineHeight+PadY;
            Bottom += LineHeight+PadX;

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
            
            Top += LineHeight+PadY;
            Bottom += LineHeight+PadX;

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
        }
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480*SoundOutput->BytesPerSample, PlayWindowColor);
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
    }
}
*/

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CommandLine, INT ShowCode) {
    win32_state Win32State = {};
    Win32GetEXEFileName(&Win32State);

    char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "firstgame.dll", sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&Win32State, "firstgame_temp.dll", sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);
    
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    // Set the Windows scheduler granularity to 1ms, so that sleep() can be more granular
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    Win32LoadXInput();
    
    // Setting up the game window
    WNDCLASSA WindowClass = {};

    // win32_window_dimension Dimension = Win32GetWindowDimension(Window);
    Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);

    // Refer to MSDN to what the wndclass structure is to understand what these mean.
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallBack;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "FirstGameWindowClass";


    // Registering the window class
    if(RegisterClass(&WindowClass)) {
        HWND Window = 
            CreateWindowExA(
                0, // WS_EX_TOPMOST,|WS_EX_LAYERED,
                WindowClass.lpszClassName,
                "Test",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if(Window) {
            win32_sound_output SoundOutput = {};

            HDC RefreshDC = GetDC(Window);
            int MonitorRefreshHz = 60;
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(Win32RefreshRate > 1) {
                MonitorRefreshHz = Win32RefreshRate;
            }
            float GameUpdateHz =  (MonitorRefreshHz / 2.0f);
            float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16_t)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
            SoundOutput.SafetyBytes = (int)(((float)SoundOutput.SamplesPerSecond*(float)SoundOutput.BytesPerSample / GameUpdateHz)/3.0f);

            bool32 SoundIsPlaying = true;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            SecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;

#if FIRSTGAME_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terrabytes((uint64_t)2);
#else
            LPVOID BaseAddress = 0;
#endif

            int16_t *Samples = (int16_t *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes(1);
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
            GameMemory.TransientStorage = ((uint8_t *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

            for(int ReplayIndex = 0; ReplayIndex < ArrayCount(Win32State.ReplayBuffers); ++ReplayIndex) {
                win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];

                Win32GetInputFileLocation(&Win32State, false, ReplayIndex, sizeof(ReplayBuffer->Filename), ReplayBuffer->Filename);
                ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->Filename, GENERIC_WRITE|GENERIC_READ, 0, 0, CREATE_ALWAYS, 0, 0);

                LARGE_INTEGER MaxSize;
                MaxSize.QuadPart = Win32State.TotalSize;
                ReplayBuffer->MemoryMap = CreateFileMapping(ReplayBuffer->FileHandle, 0, PAGE_READWRITE, MaxSize.HighPart, MaxSize.LowPart, 0);

                ReplayBuffer->MemoryBlock = MapViewOfFile(ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, Win32State.TotalSize);
                if(ReplayBuffer->MemoryBlock) {
                } else {
                    // TODO: Diagnostic
                }
            }

            if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage) {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();

                int DebugTimeMarkerIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[30] = {};

                DWORD AudioLatencyBytes = 0;
                float AudioLatencySeconds = 0;
                bool32 SoundIsValid = false;

                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);

                uint64_t LastCycleCount = __rdtsc();
                while(GlobalRunning) {
                    NewInput->dtForFrame = TargetSecondsPerFrame;

                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0) {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                    }

                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for(int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex) {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessage(&Win32State, NewKeyboardController);

                    if(!GlobalPause) {
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = MouseP.x;
                        NewInput->MouseY = MouseP.y;
                        NewInput->MouseZ = 0;
                        Win32ProcessKeyboardEvent(&NewInput->MouseButtons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
                        Win32ProcessKeyboardEvent(&NewInput->MouseButtons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
                        Win32ProcessKeyboardEvent(&NewInput->MouseButtons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
                        Win32ProcessKeyboardEvent(&NewInput->MouseButtons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
                        Win32ProcessKeyboardEvent(&NewInput->MouseButtons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));
                    
                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1)) {
                            MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                        }
                        for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex) {
                            DWORD OurControllerIndex = ControllerIndex + 1;
                            game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                            game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

                            XINPUT_STATE ControllerState;
                            if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                                NewController->IsConnected = true;
                                NewController->IsAnalog = OldController->IsAnalog;

                                // plugged in
                                XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                                NewController->IsAnalog = true;
                                NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                if((NewController->StickAverageX != 0.0f) || (NewController->StickAverageY != 0.0f)) {
                                    NewController->IsAnalog = true;
                                }

                                if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
                                    NewController->StickAverageY = 1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if(bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)) {
                                    NewController->StickAverageY = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if(bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) {
                                    NewController->StickAverageX = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if(bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) {
                                    NewController->StickAverageX = 1.0f;
                                    NewController->IsAnalog = false;
                                }

                                float Threshold = 0.5f;
                                Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0, 
                                                                &OldController->MoveLeft, 1, &NewController->MoveLeft);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageX > Threshold) ? 1 : 0, 
                                                                &OldController->MoveRight, 1, &NewController->MoveRight);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageY > Threshold) ? 1 : 0, 
                                                                &OldController->MoveUp, 1, &NewController->MoveUp);
                                Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0, 
                                                                &OldController->MoveDown, 1, &NewController->MoveDown);

                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionDown, XINPUT_GAMEPAD_A, &NewController->ActionDown);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B, &NewController->ActionRight);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionLeft, XINPUT_GAMEPAD_X, &NewController->ActionLeft);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->ActionUp, XINPUT_GAMEPAD_Y, &NewController->ActionUp);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_START, &NewController->Start);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Back, XINPUT_GAMEPAD_BACK, &NewController->Back);
                            } else {
                                NewController->IsConnected = false;
                            }
                        }
                        // XINPUT_VIBRATION Vibration;
                        // Vibration.wLeftMotorSpeed = 60000;
                        // Vibration.wRightMotorSpeed = 60000;
                        // XInputSetState(0, &Vibration);

                        thread_context Thread = {};

                        game_offscreen_buffer Buffer = {};
                        Buffer.Memory = GlobalBackBuffer.Memory;
                        Buffer.Width = GlobalBackBuffer.Width;
                        Buffer.Height = GlobalBackBuffer.Height;
                        Buffer.Pitch = GlobalBackBuffer.Pitch;
                        Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;

                        if(Win32State.InputRecordingIndex) {
                            Win32RecordInput(&Win32State, NewInput);
                        }
                        
                        if(Win32State.InputPlayingIndex) {
                            Win32PlaybackInput(&Win32State, NewInput);
                        }
                        if(Game.UpdateAndRender) {
                            Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
                        }

                        LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                        float FromBeginToAudioSeconds = 1000.0f*Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

                        // Computes how much sound to write and where
                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK) {
                            if(!SoundIsValid) {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true;
                            } 
                            DWORD ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

                            DWORD ExpectedSoundBytesPerFrame = (int)((float)(SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameUpdateHz);
                            float SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)*(float)ExpectedSoundBytesPerFrame);
                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

                            DWORD SafeWriteCursor = WriteCursor;
                            if(SafeWriteCursor < PlayCursor) {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

                            DWORD TargetCursor = 0;
                            if(AudioCardIsLowLatency) {
                                TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
                            } else {
                                TargetCursor = WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes;
                            }
                            TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);

                            DWORD BytesToWrite = 0;
                            if(ByteToLock > TargetCursor) {
                                BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                                BytesToWrite += TargetCursor;
                            } else {
                                BytesToWrite = TargetCursor - ByteToLock;
                            }

                            game_sound_output_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;
                            if(Game.GetSoundSamples) {
                                Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
                            }
                        
#if FIRSTGAME_INTERNAL
                            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = ByteToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if(UnwrappedWriteCursor < PlayCursor) {
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            AudioLatencyBytes = WriteCursor - PlayCursor;
                            AudioLatencySeconds = (((float)AudioLatencyBytes / (float)SoundOutput.BytesPerSample) / (float)SoundOutput.SamplesPerSecond);
#if 0
                            char TextBuffer[256];
                            _snprintf_s(TextBuffer, sizeof(TextBuffer), "BTL:%u, TC:%u, BTW:%u, PC:%u, WC:%u DELA:%u, (%f)\n", 
                                        ByteToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                            OutputDebugStringA(TextBuffer);
#endif
#endif

                            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                        } else {
                            SoundIsValid = false;
                        }
                            
                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        float WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        float SecondsElapsedForFrame = WorkSecondsElapsed;
                        if(SecondsElapsedForFrame < TargetSecondsPerFrame) {
                            if(SleepIsGranular) {
                                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                                if(SleepMS > 0) {
                                    Sleep(SleepMS);
                                }
                            }

                            float TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            if(TestSecondsElapsedForFrame < TargetSecondsPerFrame) {
                                // TODO: log missed sleep
                            }
                            
                            while(SecondsElapsedForFrame < TargetSecondsPerFrame) {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            }
                        } else {
                            // TODO: Missed frame rate
                        }

                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        float MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter = EndCounter;

                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);

                        FlipWallClock = Win32GetWallClock();

#if FIRSTGAME_INTERNAL
                        {
                            if(SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK) {
                                win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];

                                Marker->FlipPlayCursor = PlayCursor;
                                Marker->FlipWriteCursor = WriteCursor;
                            }
                        }
#endif

#if 0
                        game_input *Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;

                        uint64_t EndCycleCount = __rdtsc();
                        uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        // int32_t FPS = (int32_t)(GlobalPerfCountFrequency / CounterElapsed);
                        double FPS = 0.0f;
                        double MCPF = (double)CyclesElapsed / (1000.0f * 1000.0f);

                        char FPSBuffer[256];
                        _snprintf_s(FPSBuffer, sizeof(FPSBuffer), "%.02fms/f, %.02ff/s, %.02fmc/f \n", MSPerFrame, FPS, MCPF);
                        OutputDebugStringA(FPSBuffer);
#endif

#if FIRSTGAME_INTERNAL
                        ++DebugTimeMarkerIndex;
                        if(DebugTimeMarkerIndex >= ArrayCount(DebugTimeMarkers)) {
                            DebugTimeMarkerIndex = 0;
                        }
#endif
                }

                }
            } else {
                // TODO log if this fails
            }
        } else {
            // TODO log if this fails
        }
    } else {
        // TODO log if this fails
    }

    return 0;
}
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable bool Running;

LRESULT MainWindowCallBack(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam
) {
    LRESULT Result = 0;

    switch(Message) {
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_DESTROY: {
            Running = false;
        } break;

        case WM_CLOSE: {
            Running = false;
        } break;

        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            LONG Width = Paint.rcPaint.right - Paint.rcPaint.left;
            LONG Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            PatBlt(DeviceContext, X, Y, Width, Height, WHITENESS);
            EndPaint(Window, &Paint);
        } break;

        default: {
            OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}


INT WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance,
    PSTR CommandLine, INT ShowCode) {
    // Setting up the game window
    WNDCLASSA WindowClass = {};

    // Refer to MSDN to what the wndclass structure is to understand what these mean.
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallBack;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "FirstGameWindowClass";

    // Registering the window class
    if(RegisterClass(&WindowClass)) {
        HWND WindowHandle = 
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "First game",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if(WindowHandle) {
            Running = true;
            while(Running) {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if(MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } else {
                    break;
                }
            }
        } else {
            // log if this fails
        }
    } else {
        // log if this fails
    }

    return 0;
}
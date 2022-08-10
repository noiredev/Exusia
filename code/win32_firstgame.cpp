#include <windows.h>

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{
    MessageBox(0, "This is my first game!", "First Game", MB_OK|MB_ICONINFORMATION);
    return 0;
}
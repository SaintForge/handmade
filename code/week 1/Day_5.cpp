#include <windows.h>
#include <stdint.h>


#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;


struct win32_offscreen_buffer 
{
    BITMAPINFO Info;
    void* Memory;
    int Height;  
    int Width;
    int Pitch;
};

struct win32_window_dimension
{
    int Width;
    int Height;
};

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Height = ClientRect.bottom - ClientRect.top;
    Result.Width = ClientRect.right - ClientRect.left;

    return(Result);
}

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackuffer;

internal void
RenderWeirdGradient(win32_offscreen_buffer  Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8*)Buffer.Memory;
    for(int Y = 0;
        Y < Buffer.Height;
        ++Y)
    {
        uint32 *Pixel = (uint32*)Row;
        for(int X = 0;
            X < Buffer.Width;
            ++X)
        {
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Buffer.Pitch;
    } 
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    int BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width*Buffer->Height) * BytesPerPixel;
    Buffer-> Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);    
    Buffer->Pitch = Width * BytesPerPixel;
}
internal void
Win32DisplayBufferInWindow(HDC DeviceContext, 
                  int WindowWidth, int WindowHeight, 
                  win32_offscreen_buffer Buffer)
{
    StretchDIBits(DeviceContext,
                    0, 0, WindowWidth, WindowHeight,
                    0, 0, Buffer.Width, Buffer.Height,
                    Buffer.Memory,
                    &Buffer.Info,
                    DIB_RGB_COLORS, SRCCOPY);

}
LRESULT CALLBACK
 Win32WindowProc(HWND    Window,
                    UINT    Message,
                    WPARAM  WParam,
                    LPARAM  LParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_SIZE:
        {
    
        } break;
        
        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
    
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                                        GlobalBackuffer);
            EndPaint(Window, &Paint);
        } break;
        default:
        {
            // OutputDebugStringA("Defaut");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    return(Result);
}

int CALLBACK WinMain(
   HINSTANCE Instance,
   HINSTANCE PrevInstance,
   LPSTR     CommandLine,
   int       ShowCode)
{
    WNDCLASS WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackuffer, 1366, 768);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32WindowProc;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon
    WindowClass.lpszClassName = "HandmadeHeroWindowsClass";

    if(RegisterClass(&WindowClass))
    {
        HWND Window = CreateWindowEx(
                0,
                WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,   
                0);
        if(Window)
        {
            int XOffset = 0;
            int YOffset = 0;
            
            GlobalRunning = true;
            while(GlobalRunning) 
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        GlobalRunning = false; 
                    }
                    
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);   
                }
                
                RenderWeirdGradient(GlobalBackuffer, XOffset, YOffset);

                HDC DeviceContext = GetDC(Window); 
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);    
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height,
                                    GlobalBackuffer);
                ReleaseDC(Window, DeviceContext);

                ++XOffset;
                ++YOffset;
            }
        }
        else
        {
        }
    }
    else
    {

    }
    return(0);
}
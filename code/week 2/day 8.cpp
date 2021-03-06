#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
// #include <math.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

// typedef real32 float;
// typedef real64 double;

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

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


internal void
Wind32LoadXInput(void)
{
// TODO: Check for windows8
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
	// TODO: Diagnostic
	XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if(XInputLibrary)
    {
	XInputGetState = (x_input_get_state*) GetProcAddress(XInputLibrary, "XInputGetState");
	if(!XInputGetState) {XInputGetState = XInputGetStateStub; }
	
	XInputSetState = (x_input_set_state*) GetProcAddress(XInputLibrary, "XInputSetState");
	if(!XInputSetState) {XInputSetState = XInputSetStateStub; }
    }
    else
    {
	///TODO: Diagnostic
    }
}

internal void
Win32InitDSound(HWND Window, int32 SamplePerSecond, int32 BufferSize)
{
    // TODO: Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    
    if(DSoundLibrary)
    {
	// TODO: Get a DirectSound object!
	direct_sound_create *DirectSoundCreate =
	    (direct_sound_create*) GetProcAddress(DSoundLibrary,
						  "DirectSoundCreate");
	
	//TODO: Double-check that this works on XP - DirectSound8 or 7?
	LPDIRECTSOUND DirectSound;
	if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0 )))
	{
	    WAVEFORMATEX WaveFormat =  {};
	    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
	    WaveFormat.nChannels = 2;
	    WaveFormat.nSamplesPerSec = SamplePerSecond;
	    WaveFormat.wBitsPerSample = 16;
	    WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
	    WaveFormat.nAvgBytesPerSec = WaveFormat.nBlockAlign * WaveFormat.nSamplesPerSec;
	    WaveFormat.cbSize = 0;
	    
	    if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
	    {
       		DSBUFFERDESC BufferDesciption = {   };
		BufferDesciption.dwSize = sizeof(BufferDesciption);
		BufferDesciption.dwFlags = DSBCAPS_PRIMARYBUFFER;

		// TODO: "Create" a primary buffer
		LPDIRECTSOUNDBUFFER PrimaryBuffer;
		if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesciption, &PrimaryBuffer, 0)))
		{
		    HRESULT Error =  PrimaryBuffer->SetFormat(&WaveFormat);
		    if(SUCCEEDED(Error))
		    {
			OutputDebugStringA("Primary buffer format was set.\n");
		    }
		    else
		    {
			//TODO: Diagnostic
		    }
		}
		else
		{
		    //TODO: Diagnostic
		}

	    }
	    else
	    {
		//TODO: Diagnositc
	    }
	    // TODO: "Create" a seconday buffer
	    DSBUFFERDESC BufferDesciption = {};
	    BufferDesciption.dwSize = sizeof(BufferDesciption);
	    BufferDesciption.dwFlags = 0;
	    BufferDesciption.dwBufferBytes = BufferSize;
	    BufferDesciption.lpwfxFormat = &WaveFormat;
	    
	    HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDesciption, &GlobalSecondaryBuffer, 0);
	    if(SUCCEEDED(Error))
	    {
		//TODO: Start it playing!
		OutputDebugStringA("Secondary buffer format was set.\n");
	    }
	}
	else
	{
	    // TODO: Diagnostic
	}

    }
    else
    {
	// TODO: Diagnostic
    }
}


internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Height = ClientRect.bottom - ClientRect.top;
    Result.Width = ClientRect.right - ClientRect.left;
    
    return(Result);
}



internal void
RenderWeirdGradient(win32_offscreen_buffer  *Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8*)Buffer->Memory;
    for(int Y = 0;
        Y < Buffer->Height;
        ++Y)
    {
        uint32 *Pixel = (uint32*)Row;
        for(int X = 0;
            X < Buffer->Width;
            ++X)
        {
            uint8 Blue = (X + XOffset);
            uint8 Green = (Y + YOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }
		
		
        Row += Buffer->Pitch;
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
    Buffer-> Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);    
    Buffer->Pitch = Width * BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
			   HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    StretchDIBits(DeviceContext,
                    0, 0, WindowWidth, WindowHeight,
                    0, 0, Buffer->Width, Buffer->Height,
                    Buffer->Memory,
                    &Buffer->Info,
                    DIB_RGB_COLORS, SRCCOPY);

}
internal LRESULT CALLBACK
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

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
	{
	    uint32 VKCode = WParam;
	    bool WasDown = ((LParam & (1 << 30)) != 0);
	    if (WasDown == true)
	    {
		OutputDebugStringA("WasDown - true\n");
	    }
	    bool IsDown = ((LParam & (1 << 31)) == 0);
	    
	    if(WasDown != IsDown)
	    {
		if(VKCode == 'W')
		{
		}
		else if(VKCode == 'A')
		{
		}
		else if(VKCode == 'S')
		{
		}
		else if(VKCode == 'D')
		{
		}
		else if(VKCode == 'Q')
		{
		}
		else if(VKCode == 'E')
		{
		}
		else if(VKCode == VK_UP)
		{
		}
		else if(VKCode == VK_DOWN)
		{
		}
		else if(VKCode == VK_RIGHT)
		{
		}
		else if(VKCode == VK_LEFT)
		{
		}
		else if(VKCode == VK_ESCAPE)
		{
		    OutputDebugStringA("ESCAPE: ");
		    if(IsDown)
		    {
			OutputDebugStringA("IsDown");
		    }
		    if(WasDown)
		    {
			OutputDebugStringA("WasDown");
		    }
		    OutputDebugStringA("\n");
		}
		else if(VKCode == VK_SPACE)
		{
		}
	    }


	    bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);
	    if((VKCode == VK_F4) && AltKeyWasDown)
	    {
		GlobalRunning = false;
	    }
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
            Win32DisplayBufferInWindow(&GlobalBackuffer, DeviceContext,
				       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;
        default:
        {
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
    Wind32LoadXInput();
    
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackuffer, 1280, 720);

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
	    HDC DeviceContext = GetDC(Window);

	    // NOTE: Graphics Test
            int XOffset = 0;
            int YOffset = 0;

	    // NOTE: Sound Test
	    int SamplesPerSecond = 48000;
	    int ToneHz = 256;
	    int ToneVolume = 100;
	    uint32 RunningSampleIndex  = 0;
	    int SquareWaveCounter = 0;
	    int WavePeriod = SamplesPerSecond/ToneHz;
	    int HalfWavePeriod = WavePeriod / 2;
	    int BytesPerSample = sizeof(int16)*2;
	    int SecondaryBufferSize = SamplesPerSecond*BytesPerSample;
	    

	    Win32InitDSound(Window, SamplesPerSecond, SecondaryBufferSize);

	    bool32 SoundIsPlaying = false;
		

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

		for (DWORD ControllerIndex = 0;
		     ControllerIndex < XUSER_MAX_COUNT;
		     ++ControllerIndex)
		{
		    XINPUT_STATE ControllerState;
		    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		    {
			// This conteroller is plugged in
			XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
			
			bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
			bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
			bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
			bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
			bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
			bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
			bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
			bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
			bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
			bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
			bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
			bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

			int16 StickX = Pad->sThumbLX;
			int16 StickY = Pad->sThumbLY;
			
		    }
		    else
		    {
			// The contoller is not available
		    }
		}

		
                RenderWeirdGradient(&GlobalBackuffer, XOffset, YOffset);
		XOffset++;

		// NOTE: DirectSound output test
		DWORD PlayCursor;
		DWORD WriteCursor;
		if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,
								       &WriteCursor)))
		{
		    DWORD ByteToLock = RunningSampleIndex*BytesPerSample % SecondaryBufferSize;
		    DWORD BytesToWrite;
		    //TODO: We need a more accurate check than ByteToLock == PlayCursor
		    if(ByteToLock == PlayCursor)
		    {
		    	BytesToWrite = SecondaryBufferSize;
		    }
		    if(ByteToLock > PlayCursor)
		    {
			BytesToWrite = SecondaryBufferSize - ByteToLock;
			BytesToWrite += PlayCursor;
		    }
		    else
		    {
			BytesToWrite = PlayCursor - ByteToLock;
		    }

		    //TODO: More strenuous test!
		    VOID *Region1;
		    DWORD Region1Size;
		    VOID *Region2;
		    DWORD Region2Size;
		    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
							     &Region1, &Region1Size,
							     &Region2, &Region2Size,
							     0)))
		    {
			// TODO: assert that Region1Size/Region2Size is valid
			
			DWORD Region1SampleCount = Region1Size/BytesPerSample;
			int16 *SampleOut = (int16 *)Region1;


			for(DWORD SampleIndex = 0;
			    SampleIndex < Region1SampleCount;
			    ++SampleIndex)
			{
			    // real32 SineValue = ;
			    // int16 SampleValue = ;
			    
			    int16 SampleValue = ((RunningSampleIndex++ / HalfWavePeriod) % 2) ? ToneVolume : -ToneVolume;
			    *SampleOut++ = SampleValue;
			    *SampleOut++ = SampleValue;
			}

			DWORD Region2SampleCount = Region2Size/BytesPerSample;
			SampleOut = (int16 *)Region2;

			for(DWORD SampleIndex = 0;
			    SampleIndex < Region2SampleCount;
			    ++SampleIndex)
			{
			    int16 SampleValue = ((SquareWaveCounter++ / HalfWavePeriod) % 2) ? ToneVolume : -ToneVolume;
			    *SampleOut++ = SampleValue;
			    *SampleOut++ = SampleValue;
			}
			
			GlobalSecondaryBuffer->Unlock(Region1, Region1Size,
						      Region2, Region2Size);
		    }

		}
		
		if(!SoundIsPlaying)
		{
		    GlobalSecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);
		    SoundIsPlaying = true;
		}
		
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);    
                Win32DisplayBufferInWindow(&GlobalBackuffer, DeviceContext,
					   Dimension.Width,
					   Dimension.Height);

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

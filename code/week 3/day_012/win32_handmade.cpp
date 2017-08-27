#include <malloc.h>
#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

// TODO: Implement sine ourselves
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "handmade.cpp"

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


void *
PlatformLoadFile(char* FileName)
{
     return(0);
}

internal void
Wind32LoadXInput(void)
{
// TODO: Check for windows8
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
	XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
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

struct win32_sound_ouput
{
    // NOTE: Sound Test
    int SamplesPerSecond;
    int ToneHz;
    int ToneVolume;
    uint32 RunningSampleIndex;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    real32 tSine;
    int LatencySampleCount;
};


internal void
Win32ClearBuffer(win32_sound_ouput *SoundOutput)
{
     VOID *Region1;
     DWORD Region1Size;
     VOID *Region2;
     DWORD Region2Size;
     if(SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
					      &Region1, &Region1Size,
					      &Region2, &Region2Size,
					      0)))
     {
	  uint8 *DestSample = (uint8 *)Region1;
	  for(DWORD ByteIndex = 0;
	      ByteIndex < Region1Size;
	      ++ByteIndex)
	  {
	       *DestSample++ = 0;
	  }
	  
	  DestSample = (uint8 *)Region2;
	  for(DWORD ByteIndex = 0;
	      ByteIndex < Region2Size;
	      ++ByteIndex)
	  {
	       *DestSample++ = 0;
	  }

	  GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
     }
}

internal void
Win32FillSoundBuffer(win32_sound_ouput *SoundOutput,
		     DWORD ByteToLock, DWORD BytesToWrite,
		     game_sound_ouput_buffer *SourceBuffer)
{
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
			
	DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
	int16 *DestSample = (int16 *)Region1;
	int16 *SourceSample = SourceBuffer->Samples;

	for(DWORD SampleIndex = 0;
	    SampleIndex < Region1SampleCount;
	    ++SampleIndex)
	{
	    *DestSample++ = *SourceSample++;
	    *DestSample++ = *SourceSample++;
	    ++SoundOutput->RunningSampleIndex;
	}

	DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
	DestSample = (int16 *)Region2;

	for(DWORD SampleIndex = 0;
	    SampleIndex < Region2SampleCount;
	    ++SampleIndex)
	{
	    *DestSample++ = *SourceSample++;
	    *DestSample++ = *SourceSample++;
	    ++SoundOutput->RunningSampleIndex;
	}
			
	GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }

}

int CALLBACK WinMain(
   HINSTANCE Instance,
   HINSTANCE PrevInstance,
   LPSTR     CommandLine,
   int       ShowCode)
{
    LARGE_INTEGER PerCounterFrequencyResult;
    QueryPerformanceFrequency(&PerCounterFrequencyResult);
    int64 PerCounterFrequency = PerCounterFrequencyResult.QuadPart;
    
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

	    win32_sound_ouput SoundOutput = {};

	    SoundOutput.SamplesPerSecond = 48000;
	    SoundOutput.ToneHz = 256;
	    SoundOutput.ToneVolume = 3000;
	    SoundOutput.RunningSampleIndex  = 0;
	    SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
	    SoundOutput.BytesPerSample = sizeof(int16)*2;
	    SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
	    SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
	    Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
	    Win32ClearBuffer(&SoundOutput);
	    GlobalSecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);
	    
	    GlobalRunning = true;

	    // TODO(Max): Pool with bitmap VirtualAlloc
	    int16 *Samples = (int16*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
						  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);


	    LARGE_INTEGER LastCounter;
	    QueryPerformanceCounter(&LastCounter);

	    uint64 LastCycleCount= __rdtsc(); 
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
			// The controller is not available
		    }
		}

		DWORD ByteToLock;
		DWORD TargetCursor;
		DWORD BytesToWrite;
		DWORD PlayCursor;
		DWORD WriteCursor;
		bool32 SoundIsValid = false;
		if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor,
								       &WriteCursor)))
		{
		     ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) %
					 SoundOutput.SecondaryBufferSize);
		     TargetCursor = ((PlayCursor +
		       (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) %
		      SoundOutput.SecondaryBufferSize);

		     //TODO: We need a more accurate check than ByteToLock == PlayCursor
		     if(ByteToLock > TargetCursor)
		     {
			  BytesToWrite = (SoundOutput.SecondaryBufferSize) - ByteToLock;
			  BytesToWrite += TargetCursor;
		     }
		     else
		     {
			  BytesToWrite = TargetCursor - ByteToLock;
		     }

		     SoundIsValid = true;
		}

		game_sound_ouput_buffer SoundBuffer = {};
		SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
		SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
		SoundBuffer.Samples = Samples;

		game_offscreen_buffer Buffer = {};
		Buffer.Memory = GlobalBackuffer.Memory;
		Buffer.Width = GlobalBackuffer.Width;
		Buffer.Height = GlobalBackuffer.Height;
		Buffer.Pitch = GlobalBackuffer.Pitch;
		GameUpdateAndRender(&Buffer, XOffset, YOffset, &SoundBuffer, SoundOutput.ToneHz);

		XOffset++;

		// NOTE: DirectSound output test
		if(SoundIsValid)
		{
		     Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
		}
		
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);    
                Win32DisplayBufferInWindow(&GlobalBackuffer, DeviceContext,
					   Dimension.Width,
					   Dimension.Height);

		uint64 EndCycleCount= __rdtsc(); 
				
		LARGE_INTEGER EndCounter;
		QueryPerformanceCounter(&EndCounter);

		//TODO: Display the value here
		uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
		int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
		int32 MSPerFrame = (int32)((1000* CounterElapsed) / PerCounterFrequency);
		int32 FPS = PerCounterFrequency / CounterElapsed;
		int32 MCPF = (int32)CyclesElapsed / (1000 * 1000);

		char BufferTime[256];
		wsprintf(BufferTime, "%dms/f, %df/s %dmc/f\n", MSPerFrame, FPS, MCPF);
		OutputDebugStringA(BufferTime);

		LastCounter = EndCounter;
		LastCycleCount = EndCycleCount;
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

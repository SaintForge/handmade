#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

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

struct win32_sound_ouput
{
     int SamplesPerSecond;
     uint32 RunningSampleIndex;
     int BytesPerSample;
     int SecondaryBufferSize;
     real32 tSine;
     int LatencySampleCount;
};



#endif

#if !defined(HANDMADE_H)

/*
  TODO: Services that the platform layer provides to the game
*/


/*
  NOTE: Services that the game provides to the platform layer
*/


struct game_offscreen_buffer
{
     void *Memory;
     int Width;
     int Height;
     int Pitch;
};

struct game_sound_ouput_buffer
{
     int SamplesPerSecond;
     int SampleCount;
     int16 *Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer,
				  int BlueOffset,
				  int GreenOffset,
				  game_sound_ouput_buffer *SoundBuffer);


     
#define HANDMADE_H
#endif


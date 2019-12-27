#include <i_sound.h>

void I_InitSound()
{

}

void I_UpdateSound(void)
{

}

void I_SubmitSound(void)
{

}

void I_ShutdownSound(void)
{

}


//
//  SFX I/O
//

void I_SetChannels()
{

}

int I_GetSfxLumpNum(sfxinfo_t* sfx)
{
    //char namebuf[9];
    //sprintf(namebuf, "ds%s", sfx->name);
    //return W_GetNumForName(namebuf);
    return 0;
}


// Starts a sound in a particular sound channel.
int
I_StartSound
( int		id,
  int		vol,
  int		sep,
  int		pitch,
  int		priority )
{
    return id;
}


void I_StopSound(int handle)
{

}

// Called by S_*() functions
//  to see if a channel is still playing.
// Returns 0 if no longer playing, 1 if playing.
int I_SoundIsPlaying(int handle)
{
    return 0;
}

// Updates the volume, separation,
//  and pitch of a sound channel.
void
I_UpdateSoundParams
( int		handle,
  int		vol,
  int		sep,
  int		pitch )
{
   
}


//
//  MUSIC I/O
//
void I_InitMusic(void)
{

}

void I_ShutdownMusic(void)
{

}

// Volume.
void I_SetMusicVolume(int volume)
{

}

// PAUSE game handling.
void I_PauseSong(int handle)
{

}

void I_ResumeSong(int handle)
{

}

// Registers a song handle to song data.
int I_RegisterSong(void *data)
{
    return 1;
}

// Called by anything that wishes to start music.
//  plays a song, and when the song is done,
//  starts playing it again in an endless loop.
// Horrible thing to do, considering.
void
I_PlaySong
( int		handle,
  int		looping )
{

}

// Stops a song over 3 seconds.
void I_StopSong(int handle)
{

}

// See above (register), then think backwards
void I_UnRegisterSong(int handle)
{

}


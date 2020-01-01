#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <math.h>

#include <sys/time.h>
#include <sys/types.h>

#ifndef LINUX
#include <sys/filio.h>
#endif

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Linux voxware output.
//#include <linux/soundcard.h>

// Timer stuff. Experimental.
#include <time.h>
#include <signal.h>

#include "z_zone.h"

#include "i_system.h"
#include "i_sound.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"

#include "doomdef.h"
#include "Uefi.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Include/Protocol/AudioIo.h>
EFI_AUDIO_IO_PROTOCOL *audioIo;
EFI_AUDIO_IO_PROTOCOL_PORT* outputPorts = NULL;
UINTN outputCount = 0;
UINTN outputToUse = 0;
boolean outputFound = 0;
UINTN soundFreq = 44100;
void I_InitSound()
{
    EFI_STATUS status = gBS->LocateProtocol(&gEfiAudioIoProtocolGuid,NULL,(void**) audioIo);
    if (EFI_ERROR(status))
    {
        printf("Sound driver not found.\n");
    }
    else
    {
        outputPorts = (EFI_AUDIO_IO_PROTOCOL_PORT*)calloc(sizeof(outputPorts), 256);
        status = audioIo->GetOutputs(audioIo,outputPorts,&outputCount);
        if (EFI_ERROR(status))
        {
            gST->ConOut->OutputString(gST->ConOut,L"Audio driver not found\n");
        }
        else
        {
            outputFound = 1;
            for (int i = 0; i < outputCount; i++)
            {
                if (outputPorts[i].Device == EfiAudioIoDeviceSpeaker
                    && outputPorts[i].Location == EfiAudioIoLocationRear)
                {
                    outputToUse = i;
                    if (outputPorts[i].SupportedFreqs & EfiAudioIoFreq11kHz) soundFreq = 11025;
                    else soundFreq = 44100;
                }
            }
        }
    }
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
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}
#define SAMPLECOUNT		512
void*
getsfx
( char*         sfxname,
  int*          len )
{
    unsigned char*      sfx;
    unsigned char*      paddedsfx;
    int                 i;
    int                 size;
    int                 paddedsize;
    char                name[20];
    int                 sfxlump;

    
    // Get the sound data from the WAD, allocate lump
    //  in zone memory.
    sprintf(name, "ds%s", sfxname);

    // Now, there is a severe problem with the
    //  sound handling, in it is not (yet/anymore)
    //  gamemode aware. That means, sounds from
    //  DOOM II will be requested even with DOOM
    //  shareware.
    // The sound list is wired into sounds.c,
    //  which sets the external variable.
    // I do not do runtime patches to that
    //  variable. Instead, we will use a
    //  default sound for replacement.
    if ( W_CheckNumForName(name) == -1 )
      sfxlump = W_GetNumForName("dspistol");
    else
      sfxlump = W_GetNumForName(name);
    
    size = W_LumpLength( sfxlump );

    // Debug.
    // fprintf( stderr, "." );
    //fprintf( stderr, " -loading  %s (lump %d, %d bytes)\n",
    //	     sfxname, sfxlump, size );
    //fflush( stderr );
    
    sfx = (unsigned char*)W_CacheLumpNum( sfxlump, PU_STATIC );

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size-8 + (SAMPLECOUNT-1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char*)Z_Malloc( paddedsize+8, PU_STATIC, 0 );
    // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
    // This should interfere with zone memory handling,
    //  which does not kick in in the soundserver.

    // Now copy and pad.
    memcpy(  paddedsfx, sfx, size );
    for (i=size ; i<paddedsize+8 ; i++)
        paddedsfx[i] = 128;

    // Remove the cached lump.
    Z_Free( sfx );
    
    // Preserve padded length.
    *len = paddedsize;

    // Return allocated padded data.
    return (void *) (paddedsfx + 8);
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
    int* length;
    signed char* sampdata;
    int realvol = 100 * (vol / 127);
    S_sfx[id].data = getsfx( S_sfx[id].name, length );
    if (audioIo)
    {
        if (soundFreq == 11025) audioIo->SetupPlayback(audioIo,outputToUse,realvol,EfiAudioIoFreq11kHz,EfiAudioIoBits16,2);
        else audioIo->SetupPlayback(audioIo,outputToUse,realvol,EfiAudioIoFreq44kHz,EfiAudioIoBits16,2);
        if (soundFreq == 44100)
        {
            // Expand the sample data.
            signed char* expandedSampleData = calloc(1,(*length) * 4);
            signed char* sfxdata = S_sfx[id].data;
            for (int sampleByte = 0; sampleByte < *length; sampleByte++)
            {
                expandedSampleData[sampleByte * 4] = 
                expandedSampleData[sampleByte * 4 + 1] = 
                expandedSampleData[sampleByte * 4 + 2] =
                expandedSampleData[sampleByte * 4 + 3] = sfxdata[sampleByte];
            }
            audioIo->StartPlaybackAsync(audioIo,expandedSampleData,*length * 4,0,NULL,NULL); // Blast it out processed.
        }
        else audioIo->StartPlaybackAsync(audioIo,S_sfx[id].data,*length,0,NULL,NULL); // Blast it out unprocessed.
    }
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


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
#include <wchar.h>

#include "sound/common.h"
#include "doomdef.h"
#include "Uefi.h"
#include "stdbool.h"
#include <Library/UefiBootServicesTableLib.h>
#include <Include/Protocol/AudioIo.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
EFI_AUDIO_IO_PROTOCOL *audioIo;
EFI_AUDIO_IO_PROTOCOL_PORT *outputPorts = NULL;
EFI_DEVICE_PATH_PROTOCOL *DevicePath = NULL;
UINTN outputCount = 0;
UINTN outputToUse = 0;
boolean outputFound = 0;
UINTN soundFreq = 44100;
STATIC CHAR16 *DefaultDevices[EfiAudioIoDeviceMaximum] = { L"Line", L"Speaker", L"Headphones", L"SPDIF", L"Mic", L"HDMI", L"Other" };
STATIC CHAR16 *Locations[EfiAudioIoLocationMaximum] = { L"N/A", L"rear", L"front", L"left", L"right", L"top", L"bottom", L"other" };
STATIC CHAR16 *Surfaces[EfiAudioIoSurfaceMaximum] = { L"external", L"internal", L"other" };
float* floatsounddata = NULL;
float* floatsounddataOutput = NULL;
short* shortsounddataOutput = NULL;
bool soundinit = false;
void I_InitSound()
{
    soundinit = true;
    floatsounddata = calloc(MAX_UINT16,sizeof(short));
    floatsounddataOutput = calloc(MAX_UINT16,sizeof(short) * 2);
    shortsounddataOutput = calloc(MAX_UINT16,sizeof(short));
    UINTN HandlesCount = 0;
    EFI_HANDLE* handles = malloc(sizeof(EFI_HANDLE) * 256);
    EFI_STATUS status1 = gBS->LocateHandleBuffer(ByProtocol,&gEfiAudioIoProtocolGuid,NULL,&HandlesCount,&handles);
    if (!EFI_ERROR(status1))
    {
        for (UINTN h = 0; h < HandlesCount; h++) 
        {
            // Open Audio I/O protocol.
            status1 = gBS->HandleProtocol(handles[h], &gEfiAudioIoProtocolGuid, (VOID**)&audioIo);
           // ASSERT_EFI_ERROR(status1);
            if (EFI_ERROR(status1))
                continue;

            // Get device path.
            status1 = gBS->HandleProtocol(handles[h], &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath);
           // ASSERT_EFI_ERROR(status1);
            if (EFI_ERROR(status1))
                continue;

            // Get output devices.
            status1 = audioIo->GetOutputs(audioIo, &outputPorts, &outputCount);
            //ASSERT_EFI_ERROR(status1);
            if (EFI_ERROR(status1))
                continue;

            for (int o = 0; o < outputCount; o++)
            {
                Print(L"Device Num: %d, output location: %s, output surface: %s, output device: %s\n",
                o,Locations[outputPorts[o].Location], Surfaces[outputPorts[o].Surface], DefaultDevices[outputPorts[o].Device]);
                Print(L"Supported Freqs: ");
                if (outputPorts[o].SupportedFreqs & EfiAudioIoFreq11kHz) printf("%d, ",11025);
                if (outputPorts[o].SupportedFreqs & EfiAudioIoFreq22kHz) printf("%d, ",22050);
                if (outputPorts[o].SupportedFreqs & EfiAudioIoFreq44kHz) printf("%d, ",44100);
                if (outputPorts[o].SupportedFreqs & EfiAudioIoFreq48kHz) printf("%d",48000);
                printf("\n");
                Print(L"");
                if (outputPorts[o].Device == EfiAudioIoDeviceLine
                    || outputPorts[o].Device == EfiAudioIoDeviceSpeaker)
                if (outputFound != true)
                {
                        if (outputPorts[o].SupportedFreqs & EfiAudioIoFreq11kHz) soundFreq = 11025;
                        else soundFreq = 44100;
                        getchar();
                        outputToUse = o;
                        outputFound = true;
                        printf("Selected output.\n");
                }
            }
        }
    }
    else
    {
        printf("Sound not found.\n");
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
    if (audioIo && soundinit)
    {
        soundinit = false;
        audioIo->StopPlayback(audioIo);
        free(floatsounddataOutput);
        free(floatsounddata);
        free(shortsounddataOutput);
    }
}

//
//  SFX I/O
//

void I_SetChannels()
{
}

int I_GetSfxLumpNum(sfxinfo_t *sfx)
{
    char namebuf[9];
    sprintf(namebuf, "ds%s", sfx->name);
    return W_GetNumForName(namebuf);
}
#define SAMPLECOUNT 512
void *
getsfx(char *sfxname,
       int *len)
{
    unsigned char *sfx;
    unsigned char *paddedsfx;
    int i;
    int size;
    int paddedsize;
    char name[20];
    int sfxlump;

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
    if (W_CheckNumForName(name) == -1)
        sfxlump = W_GetNumForName("dspistol");
    else
        sfxlump = W_GetNumForName(name);

    size = W_LumpLength(sfxlump);

    // Debug.
    // fprintf( stderr, "." );
    //fprintf( stderr, " -loading  %s (lump %d, %d bytes)\n",
    //	     sfxname, sfxlump, size );
    //fflush( stderr );

    sfx = (unsigned char *)W_CacheLumpNum(sfxlump, PU_STATIC);

    // Pads the sound effect out to the mixing buffer size.
    // The original realloc would interfere with zone memory.
    paddedsize = ((size - 8 + (SAMPLECOUNT - 1)) / SAMPLECOUNT) * SAMPLECOUNT;

    // Allocate from zone memory.
    paddedsfx = (unsigned char *)Z_Malloc(paddedsize + 8, PU_STATIC, 0);
    // ddt: (unsigned char *) realloc(sfx, paddedsize+8);
    // This should interfere with zone memory handling,
    //  which does not kick in in the soundserver.

    // Now copy and pad.
    memcpy(paddedsfx, sfx, size);
    for (i = size; i < paddedsize + 8; i++)
        paddedsfx[i] = 128;

    // Remove the cached lump.
    Z_Free(sfx);

    // Preserve padded length.
    if (len) *len = paddedsize;

    // Return allocated padded data.
    return (void *)(paddedsfx + 8);
}
UINTN SoundPlaybackHandles = 0;
char* calldata = {0};
void I_FinishedSound(IN EFI_AUDIO_IO_PROTOCOL *AudioIo, IN VOID *Context)
{

}
float lerp(float v0, float v1, float t) {
  return (1 - t) * v0 + t * v1;
}
// Starts a sound in a particular sound channel.
int I_StartSound(int id,
                 int vol,
                 int sep,
                 int pitch,
                 int priority)
{
    if (!outputFound) return 0;
    int *length = 0;
    char* sampdata;
    char* lumpdata;
    int sfxlumpnum = I_GetSfxLumpNum(&S_sfx[id]);
    int lumplength = W_LumpLength(sfxlumpnum);
    int sampLength = lumplength - 8;
    sampdata = malloc(lumplength - 8);
    lumpdata = malloc(lumplength);
    W_ReadLump(sfxlumpnum,lumpdata);
    memmove(sampdata,lumpdata + 8,sampLength);
    unsigned int realvol = 100 * (fabs(vol) / 127.);
    S_sfx[id].data = getsfx(S_sfx[id].name, length);
    EFI_STATUS stats;
    if (audioIo)
    {
        EFI_STATUS res = audioIo->SetupPlayback(audioIo, outputToUse, realvol, EfiAudioIoFreq44kHz, EfiAudioIoBits8, 2);
        if (EFI_ERROR(res)) audioIo->SetupPlayback(audioIo, outputToUse, realvol, EfiAudioIoFreq44kHz, EfiAudioIoBits16, 2);
        src_short_to_float_array(sampdata,floatsounddata,sampLength / sizeof(short));
        SRC_DATA data;
        memset(&data,0,sizeof(SRC_DATA));
        data.data_in = floatsounddata;
        data.data_out = floatsounddataOutput;
        data.input_frames = sampLength / sizeof(short);
        data.output_frames = data.input_frames * 4;
        data.src_ratio = 44100 / 11025;
        int convres = src_simple(&data,SRC_SINC_MEDIUM_QUALITY,1);
        if (convres != 0)
        {
            printf("Sample rate conversion failed: %s\n",src_strerror(convres));
            free(lumpdata);
            free(sampdata);
            return 0;
        }
        src_float_to_short_array(floatsounddataOutput,shortsounddataOutput,data.output_frames_gen);
        short* outputStereo = calloc(MAX_UINT16,sizeof(short));
        for (int i = 0; i < data.output_frames_gen; i++)
        {
            outputStereo[i * 2] = outputStereo[i * 2 + 1] = shortsounddataOutput[i];
        }
        stats = audioIo->StartPlaybackAsync(audioIo, outputStereo, data.output_frames_gen * 2, 0, &I_FinishedSound, NULL); // Blast it out processed.
        if (EFI_ERROR(stats)) printf("Failed to play sound.\n");
    }
    free(sampdata);
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
void I_UpdateSoundParams(int handle,
                         int vol,
                         int sep,
                         int pitch)
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
void I_PlaySong(int handle,
                int looping)
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

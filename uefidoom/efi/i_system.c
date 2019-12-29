
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"
#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/UefiBootServicesTableLib.h>

int	mb_used = 20;


void
I_Tactile
( int	on,
  int	off,
  int	total )
{
  // UNUSED.
  on = off = total = 0;
}

ticcmd_t	emptycmd;
ticcmd_t*	I_BaseTiccmd(void)
{
    return &emptycmd;
}


int  I_GetHeapSize (void)
{
    return mb_used*1024*1024;
}

byte* I_ZoneBase (int*	size)
{
    *size = mb_used*1024*1024;
    return (byte *) malloc (*size);
}

//
// I_GetTime
// returns time in 1/70th second tics
//
// [Cacodemon345] Rely upon the EFI Runtime Services GetTime function.

int  I_GetTime (void)
{
    struct timeval	tp;
    //struct timezone	tzp;
    int			newtics;
    static int		basetime=0;
    if (gST) gRT = gST->RuntimeServices;
    EFI_TIME *time = NULL;
    EFI_TIME_CAPABILITIES *timecaps = NULL;
    if (gRT) gRT->GetTime(time,timecaps);    
    if (time != NULL)
    {
        tp.tv_sec = time->Second;
        tp.tv_usec = time->Nanosecond / 1000;
    }
    else gettimeofday(&tp, NULL);
    if (!basetime)
	basetime = tp.tv_sec;
    newtics = (tp.tv_sec-basetime)*TICRATE + tp.tv_usec*TICRATE/1000000;
    return newtics;
}



//
// I_Init
//
void I_Init (void)
{
    I_InitSound();
    //  I_InitGraphics();
}

//
// I_Quit
//
void I_Quit (void)
{
    D_QuitNetGame ();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults ();
    I_ShutdownGraphics();

    exit(0);
}

void I_WaitVBL(int count)
{
#ifdef SGI
    sginap(1);                                           
#else
#ifdef SUN
    sleep(0);
#else
    usleep (count * (1000000/70) );                                
#endif
#endif
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

byte*	I_AllocLow(int length)
{
    byte*	mem;
        
    mem = (byte *)malloc (length);
    memset (mem,0,length);
    return mem;
}


//
// I_Error
//
extern boolean demorecording;

void I_Error (char *error, ...)
{
    va_list	argptr;

    // Message first.
    va_start (argptr,error);
    printf ("Error: ");
    vprintf (error,argptr);
    printf ("\n");
    va_end (argptr);

    // Shutdown. Here might be other errors.
    if (demorecording)
	G_CheckDemoStatus();

    D_QuitNetGame ();
    I_ShutdownGraphics();
    
	while(1) ;
    //exit(-1);
}

///
  
extern
INTN
ShellAppMain (
  IN UINTN Argc,
  IN CHAR16 **Argv
  );

#include <Library/UefiBootServicesTableLib.h>
#include <Library/SerialPortLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/SimplePointer.h>

EFI_STATUS UefiMain(EFI_HANDLE handle, EFI_SYSTEM_TABLE* st)
{
	const char* msg = "DOOM: testing serial port\n";
	SerialPortInitialize();
	SerialPortWrite(msg, strlen(msg));
	const CHAR16* argv[] = {L"doom.exe", NULL};
	return ShellAppMain(1, argv);
}



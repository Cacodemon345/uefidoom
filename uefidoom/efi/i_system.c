
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
#include <Library/UefiLib.h>
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
    EFI_TIME *time = malloc(sizeof(EFI_TIME));
    memset(time,0,sizeof(EFI_TIME));
    EFI_TIME_CAPABILITIES *timecaps = malloc(sizeof(EFI_TIME_CAPABILITIES));
    memset(timecaps,0,sizeof(EFI_TIME_CAPABILITIES));
    EFI_STATUS status;
    if (gST) status = gST->RuntimeServices->GetTime(time,timecaps);    
    if (!EFI_ERROR (status))
    {
        tp.tv_sec = time->Second;
        tp.tv_usec = time->Nanosecond / 1000;
    }
    else gettimeofday(&tp, NULL);
    if (!basetime)
	basetime = tp.tv_sec;
    newtics = (tp.tv_sec-basetime)*TICRATE + tp.tv_usec*TICRATE/1000000;
    free(time);
    free(timecaps);
    return newtics;
}
int nano100ticks = 0;
void EFIAPI I_TickTime(IN EFI_EVENT Event, IN VOID *Context)
{
    nano100ticks++;
    AsciiPrint("Ticks passed: %d",nano100ticks);
}
EFI_EVENT timerEvent = 0;
//
// I_Init
//
void I_Init (void)
{
    gBS->SetWatchdogTimer(0,0,0,NULL);
    I_InitSound();
    gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL | EVT_NOTIFY_WAIT,TPL_CALLBACK,(EFI_EVENT_NOTIFY)&I_TickTime,NULL,&timerEvent);
    gBS->SetTimer(&timerEvent,TimerPeriodic,1);
    //  I_InitGraphics();
}
#include "w_wad.h"
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

    int lump = W_CheckNumForName("ENDOOM");
    if (lump != -1)
    {
        char* allocChar = malloc(W_LumpLength(lump));
        W_ReadLump(lump, allocChar);
        for (int vgaline = 0; vgaline < 80; vgaline++)
        {
            for (int vgacol = 0; vgacol < 50; vgacol += 2)
            {
                unsigned char ascChar = allocChar[vgaline * 50 + vgacol];
                unsigned char attrChar = allocChar[vgaline * 50 + (vgacol + 1)];
                if (ascChar == (unsigned char)196)
                {
                    ascChar = '-';
                }
                gST->ConOut->SetAttribute(gST->ConOut,attrChar & 0x7F);
                gST->ConOut->OutputString(gST->ConOut, (wchar_t[]){ascChar,0});
            }
        }
    }

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
    // [Cacodemon345] First, shut everything down. Specially, we need to exit back to text mode.
    // Shutdown. Here might be other errors.
    if (demorecording)
	G_CheckDemoStatus();

    D_QuitNetGame ();
    I_ShutdownGraphics();
    I_ShutdownSound();

    // Message first.
    va_list	argptr;
    va_start (argptr,error);
    printf ("Error: ");
    vprintf (error,argptr);
    printf ("\n");
    va_end (argptr);

    exit(-1);
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

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE handle, EFI_SYSTEM_TABLE* st)
{
	const char* msg = "DOOM: testing serial port\n";
	SerialPortInitialize();
	SerialPortWrite(msg, strlen(msg));
	const CHAR16* argv[] = {L"doom.exe", NULL};
	return ShellAppMain(1, argv);
}



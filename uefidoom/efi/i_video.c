#include <i_video.h>
#include <v_video.h>
#include <d_event.h>
#include <d_main.h>
#include <doomdef.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION 1
#include "stb_image_resize.h"
// Efi stuff
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextIn.h>
#include <Protocol/SimpleTextInEx.h>
#include <Protocol/SimplePointer.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>

////////////////////////////////////////////////////////////////////

// Frame render func
typedef void(*FrameRenderFptr)(void);

////////////////////////////////////////////////////////////////////

static EFI_GRAPHICS_OUTPUT_PROTOCOL* gGOP = NULL;
static EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* mode = NULL;

static UINT32 frameLeft = 0;
static UINT32 frameTop = 0;
static UINT32 frameWidth = 0;
static UINT32 frameHeight = 0;

static EFI_GRAPHICS_OUTPUT_BLT_PIXEL gPalette[256] = {0};
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL* frameBuffer = NULL;
static EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* gInputEx = NULL;
EFI_SIMPLE_POINTER_PROTOCOL* gMouseProtocol = NULL;
static FrameRenderFptr gRenderFunc = NULL;
static boolean getInput = true;
static EFI_KEY_DATA keydata[4096];
static unsigned int keycounter;
#define FRAME_SCALE 2

static int xlatekey(EFI_INPUT_KEY* efiKey);

////////////////////////////////////////////////////////////////////

// Code taken from BOOM DOS source code.
unsigned char key_ascii_table[128] =
{
/* 0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F             */
   0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,   9,       /* 0 */
   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,  0,   'a', 's',     /* 1 */
   'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,  '`', 0,   92,  'z', 'x', 'c', 'v',     /* 2 */
   'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   3,   3,   3,   3,   8,       /* 3 */
   3,   3,   3,   3,   3,   0,   0,   0,   0,   0,   '-', 0,   0,   0,   '+', 0,       /* 4 */
   0,   0,   0,   127, 0,   0,   92,  3,   3,   0,   0,   0,   0,   0,   0,   0,       /* 5 */
   13,  0,   '/', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   127,     /* 6 */
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   '/', 0,   0,   0,   0,   0        /* 7 */
};
int I_ScanCode2DoomCode (int a)
{
  switch (a)
    {
    default:   return key_ascii_table[a]>8 ? key_ascii_table[a] : a+0x80;
    case 0x7b: return KEY_PAUSE;
    case 0x0e: return KEY_BACKSPACE;
    case 0x48: return KEY_UPARROW;
    case 0x4d: return KEY_RIGHTARROW;
    case 0x50: return KEY_DOWNARROW;
    case 0x4b: return KEY_LEFTARROW;
    case 0x38: return KEY_LALT;
    case 0x79: return KEY_RALT;
    case 0x1d:
    case 0x78: return KEY_RCTRL;
    case 0x36:
    case 0x2a: return KEY_RSHIFT;
  }
}

// Automatic caching inverter, so you don't need to maintain two tables.
// By Lee Killough

int I_DoomCode2ScanCode (int a)
{
  static int inverse[256], cache;
  for (;cache<256;cache++)
    inverse[I_ScanCode2DoomCode(cache)]=cache;
  return inverse[a];
}
// END BOOM code
static void DumpGraphicsProtocol(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop)
{
	printf("GOP %p\n", gop);
	printf("Starting in mode %dx%d\n", gGOP->Mode->Info->HorizontalResolution, gGOP->Mode->Info->VerticalResolution);
	printf("Frame buffer base %p, size %d\n", (void*)gGOP->Mode->FrameBufferBase, gGOP->Mode->FrameBufferSize);

	UINTN i;
	for (i = 0; i < gop->Mode->MaxMode; ++i)
	{
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Mode = NULL;
		UINTN ModeSize = gop->Mode->SizeOfInfo;

		EFI_STATUS status = gop->QueryMode(gop, i, &ModeSize, &Mode);
		if (EFI_ERROR(status))
		{
			printf("Failed mode query on %d: 0x%x\n", i, status);
		}

		printf("Mode %d: ver %d, hrez %d, vrez %d, format %d, scanline pixels %d\n", 
				i, 
				Mode->Version,
				Mode->HorizontalResolution, 
				Mode->VerticalResolution, 
				Mode->PixelFormat,
				Mode->PixelsPerScanLine);

		if (Mode->PixelFormat == PixelBitMask)
		{
			printf("Pixel bit mask: {R = 0x%x, G = 0x%x, B = 0x%x}\n",
					Mode->PixelInformation.RedMask,
					Mode->PixelInformation.GreenMask,
					Mode->PixelInformation.BlueMask);
		}
	}
}

static void EnumerateGraphics(void)
{
	EFI_HANDLE* handles;
	UINTN count;
	
	EFI_STATUS status = gBS->LocateHandleBuffer(ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &count, &handles);
	if (EFI_ERROR(status))
	{
		printf("Failed to locate gop handles\n");
		return;
	}

	printf("Total GOP handles %d\n", count);

	UINTN i;
	for(i = 0; i < count; ++i)
	{
		EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
		status = gBS->HandleProtocol(handles[i], &gEfiGraphicsOutputProtocolGuid, &gop);
		if (EFI_ERROR(status))
		{
			printf("Failed to handle GOP on handle %p\n", handles[i]);
		}

		DumpGraphicsProtocol(gop);
	}

	gBS->FreePool(handles);
}

////////////////////////////////////////////////////////////////////

// Default frame render, uses GOP blt 
void GopBltRender(void)
{
	EFI_STATUS status = gGOP->Blt(
		gGOP, 
		frameBuffer, 
		EfiBltBufferToVideo,
		0, 0,
		frameLeft, frameTop,
		frameWidth, frameHeight,
		0);

	if (EFI_ERROR(status))
	{
		printf("Blit failed\n");
	}
}

EFI_STATUS EFIAPI I_KeyNotify(EFI_KEY_DATA* keydata)
{
	event_t event;
	event.type = ev_keydown;
	event.data1 = keydata->Key.UnicodeChar;
	D_PostEvent(&event);
	return EFI_SUCCESS;
}
EFI_STATUS EFIAPI I_ScanKeyNotify(EFI_KEY_DATA* keydata)
{
	event_t event;
	event.type = ev_keydown;
	event.data1 = xlatekey(&keydata->Key);
	D_PostEvent(&event);
	return EFI_SUCCESS;
}
EFI_HANDLE *NotifHandle;
EFI_HANDLE *ScanNotifHandle;
////////////////////////////////////////////////////////////////////

// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitGraphics (void)
{
	//EnumerateGraphics();

	if (gGOP != NULL)
	{
		// Already initialized
		return;
	}
	
	EFI_STATUS status;
	
	status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void**) &gGOP);
	if (EFI_ERROR(status))
	{
		printf("I_InitGraphics: no graphics output: 0x%x\n", status);
		return;
	}
	status = gBS->HandleProtocol(gST->ConsoleInHandle,&gEfiSimplePointerProtocolGuid, (void**) &gMouseProtocol);
	if (EFI_ERROR(status))
	{
		status = gBS->LocateProtocol(&gEfiSimplePointerProtocolGuid,NULL, (void**) &gMouseProtocol);
		if (EFI_ERROR(status))
		{
			EFI_HANDLE* handles;
			UINTN count;
			status = gBS->LocateHandleBuffer(ByProtocol,&gEfiSimplePointerProtocolGuid,NULL,&count,&handles);
			if (!EFI_ERROR(status))
			{
				for (int HandleCountIndex = 0; HandleCountIndex < count; HandleCountIndex++)
				{
					status = gBS->HandleProtocol(handles[HandleCountIndex],&gEfiSimplePointerProtocolGuid,(void**)&gMouseProtocol);
					if (EFI_ERROR(status))
					{
						continue;
					}
					else break;
				}
			}
			else printf("Failed to initialize mouse pointer.\n");
		}
	}
	if (EFI_ERROR(status)) printf("Failed to initialize mouse pointer.\n");
	status = gBS->HandleProtocol(gST->ConsoleInHandle,&gEfiSimpleTextInputExProtocolGuid,(void**) &gInputEx);
	if (EFI_ERROR(status)) printf("Failed to initialize Extended Input protocol.\n");
	
	/*else
	{
		int i;
		for (i = 0; i < 256; i++)
		{
			EFI_KEY_DATA keydata;
			keydata.Key.UnicodeChar = i;
			gInputEx->RegisterKeyNotify(gInputEx,&keydata,I_KeyNotify,(void**)&NotifHandle);
		}
		for (i = 0; i < 0x17; ++i)
		{
			EFI_KEY_DATA scankeydata;
			scankeydata.Key.ScanCode = i;
			gInputEx->RegisterKeyNotify(gInputEx,&scankeydata,I_ScanKeyNotify,(void**)&ScanNotifHandle);
		}
	}*/
	mode = gGOP->Mode;
	status = gGOP->SetMode(gGOP, 0);
	if (EFI_ERROR(status))
	{
		printf("GOP mode not set; using native console mode\n");
	}
	
	// Calculate frame origin in current display mode
	frameWidth = SCREENWIDTH * FRAME_SCALE;
	frameHeight = SCREENHEIGHT * FRAME_SCALE;
	frameLeft = (gGOP->Mode->Info->HorizontalResolution - frameWidth) >> 1;
	frameTop = (gGOP->Mode->Info->VerticalResolution - frameHeight) >> 1;

	// Pick frame render func
	gRenderFunc = GopBltRender;

	// Allocate pixel screen
	screens[0] = (unsigned char *) malloc (SCREENWIDTH * SCREENHEIGHT); // byte per pixel?
	if (!screens[0])
	{
		I_Error("Failed to allocate pixel screen!\n");
		return;
	}

	frameBuffer = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)malloc(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * frameWidth * frameHeight);
	if (!frameBuffer)
	{
		free(screens[0]);
		I_Error("Failed to allocate frameBuffer!\n");
		return;
	}
	
	printf("DOOM display %dx%d\n", SCREENWIDTH, SCREENHEIGHT);
	printf("Frame buffer: top = %d, left = %d, width = %d, height = %d\n", frameTop, frameLeft, frameWidth, frameHeight);

	gST->ConOut->EnableCursor(gST->ConOut, FALSE);
	gST->ConOut->ClearScreen(gST->ConOut);
}

void MakeFrame (void)
{
	UINTN i;
	UINTN j;
	UINTN linedest;
	// Each pixel in screen buffer is a pallette index
	byte* srcLine = screens[0];
	EFI_GRAPHICS_OUTPUT_BLT_PIXEL* dstLine = frameBuffer;
	for (i = 0; i < SCREENHEIGHT; ++i)
	{
		// Convert source screen line into framebuffer pixels
		EFI_GRAPHICS_OUTPUT_BLT_PIXEL* curLine = dstLine; // Save pointer for later
		for (j = 0; j < SCREENWIDTH; ++j)
		{
			byte pixel = *srcLine++;

			for (linedest = 0; linedest < FRAME_SCALE; linedest++)
			{
			*dstLine++ = gPalette[pixel];
			}
		}
		for (linedest = 0; linedest < (FRAME_SCALE - 1); linedest++)
		{
			// Copy this dst line to the next one
			memcpy(dstLine, curLine, frameWidth * sizeof(*frameBuffer));
			// Skip next line (copied above)
			dstLine += frameWidth;
		}
	}
	
}

void I_FinishUpdate (void)
{
	MakeFrame();
	gRenderFunc();
}

void I_ShutdownGraphics(void)
{
	gGOP->SetMode(gGOP,mode->Mode);
	free(frameBuffer);
	free(screens[0]);

	gGOP = NULL;
	gRenderFunc = NULL;
	gST->ConOut->SetMode(gST->ConOut,0);
	gST->ConOut->Reset(gST->ConOut,TRUE);
	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->EnableCursor(gST->ConOut, TRUE);
}

// Takes full 8 bit values.
void I_SetPalette (byte* palette)
{
	if (!palette)
	{
		printf("I_SetPalette: No palette!\n");
	}

	size_t i = 0;
	for (i = 0; i < 256; ++i)
	{
		gPalette[i].Red = *palette++;
		gPalette[i].Green = *palette++;
		gPalette[i].Blue = *palette++;
	}
}

void I_ReadScreen (byte* scr)
{
    memcpy (scr, screens[0], SCREENWIDTH*SCREENHEIGHT);
}

////////////////////////////////////////////////////////////////////

static int	lastmousex = 0;
static int	lastmousey = 0;
boolean		mousemoved = false;

/*  
void I_GetEvent(void)
{
		
    event_t event;

    // put event-grabbing stuff in here
    XNextEvent(X_display, &X_event);
    switch (X_event.type)
    {
      case KeyPress:
	event.type = ev_keydown;
	event.data1 = xlatekey();
	D_PostEvent(&event);
	// fprintf(stderr, "k");
	break;
      case KeyRelease:
	event.type = ev_keyup;
	event.data1 = xlatekey();
	D_PostEvent(&event);
	// fprintf(stderr, "ku");
	break;
      case ButtonPress:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xbutton.state & Button1Mask)
	    | (X_event.xbutton.state & Button2Mask ? 2 : 0)
	    | (X_event.xbutton.state & Button3Mask ? 4 : 0)
	    | (X_event.xbutton.button == Button1)
	    | (X_event.xbutton.button == Button2 ? 2 : 0)
	    | (X_event.xbutton.button == Button3 ? 4 : 0);
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	// fprintf(stderr, "b");
	break;
      case ButtonRelease:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xbutton.state & Button1Mask)
	    | (X_event.xbutton.state & Button2Mask ? 2 : 0)
	    | (X_event.xbutton.state & Button3Mask ? 4 : 0);
	// suggest parentheses around arithmetic in operand of |
	event.data1 =
	    event.data1
	    ^ (X_event.xbutton.button == Button1 ? 1 : 0)
	    ^ (X_event.xbutton.button == Button2 ? 2 : 0)
	    ^ (X_event.xbutton.button == Button3 ? 4 : 0);
	event.data2 = event.data3 = 0;
	D_PostEvent(&event);
	// fprintf(stderr, "bu");
	break;
      case MotionNotify:
	event.type = ev_mouse;
	event.data1 =
	    (X_event.xmotion.state & Button1Mask)
	    | (X_event.xmotion.state & Button2Mask ? 2 : 0)
	    | (X_event.xmotion.state & Button3Mask ? 4 : 0);
	event.data2 = (X_event.xmotion.x - lastmousex) << 2;
	event.data3 = (lastmousey - X_event.xmotion.y) << 2;

	if (event.data2 || event.data3)
	{
	    lastmousex = X_event.xmotion.x;
	    lastmousey = X_event.xmotion.y;
	    if (X_event.xmotion.x != X_width/2 &&
		X_event.xmotion.y != X_height/2)
	    {
		D_PostEvent(&event);
		// fprintf(stderr, "m");
		mousemoved = false;
	    } else
	    {
		mousemoved = true;
	    }
	}
	break;
	
      case Expose:
      case ConfigureNotify:
	break;
	
      default:
	if (doShm && X_event.type == X_shmeventtype) shmFinished = true;
	break;
    }

}
*/


static int xlatekey(EFI_INPUT_KEY* efiKey)
{
	if (efiKey->ScanCode != 0)
	{
		switch(efiKey->ScanCode)
		{
		case SCAN_UP: 		return KEY_UPARROW;
		case SCAN_DOWN: 	return KEY_DOWNARROW;
		case SCAN_LEFT: 	return KEY_LEFTARROW;
		case SCAN_RIGHT: 	return KEY_RIGHTARROW;
		case SCAN_ESC:		return KEY_ESCAPE;
		case SCAN_F1:		return KEY_F1;
		case SCAN_F2:		return KEY_F2;
		case SCAN_F3:		return KEY_F3;
		case SCAN_F4:		return KEY_F4;
		case SCAN_F5:		return KEY_F5;
		case SCAN_F6:		return KEY_F6;
		case SCAN_F7:		return KEY_F7;
		case SCAN_F8:		return KEY_F8;
		case SCAN_F9:		return KEY_F9;
		case SCAN_F10:		return KEY_F10;
		case SCAN_F11:		return KEY_F11;
		case SCAN_F12:		return KEY_F12;
		default:			return (char)(efiKey->UnicodeChar & 0xFF);
		};
	}
	else
	{
		switch(efiKey->UnicodeChar)
		{
		case CHAR_BACKSPACE: 	return KEY_BACKSPACE;
		case CHAR_TAB:			return KEY_TAB;
		case CHAR_LINEFEED:		return KEY_ENTER;
		default:				return (char)(efiKey->UnicodeChar & 0xFF);
		};
	}
}
int microseconds = 0;
//
// I_StartTic
//
void I_StartTic (void)
{
	// Get all pending input events
	//IoWrite8(0x60,1 | (1 << 6));
	EFI_STATUS status = EFI_SUCCESS;
	EFI_INPUT_KEY inputKey;
	while (1)
	{
		event_t event;
		if (gMouseProtocol)
		{
			EFI_SIMPLE_POINTER_STATE state;
			status = gMouseProtocol->GetState(gMouseProtocol,&state);
			if (EFI_ERROR(status))
			{
			
			}
			else
			{
				//AsciiPrint("Mouse delta X: %d, Mouse delta Y: %d",state.RelativeMovementX,state.RelativeMovementY);
				event.type = ev_mouse;
				event.data1  = (state.LeftButton == 1);
				event.data1 |= (state.RightButton == 1 ? 2 : 0);
				event.data2 = state.RelativeMovementX * 4;
				event.data3 = -state.RelativeMovementY * 4;
				D_PostEvent(&event);
			}
		}

		status = gST->ConIn->ReadKeyStroke(gST->ConIn, &inputKey);
		if (status == EFI_NOT_READY)
		{
			// No more key strokes
			break;
		}
		else if (EFI_ERROR(status))
		{
			printf("Failed to get key stroke: 0x%x\n", status);
			break;
		}
		

		// Post pressed event (handle released event later).
		event.type = ev_keydown;
		event.data1 = xlatekey(&inputKey);
		D_PostEvent(&event);
	}
}


void I_UpdateNoBlit(void)
{
}

void I_StartFrame (void)
{

}


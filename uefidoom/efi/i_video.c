#include <i_video.h>
#include <v_video.h>
#include <d_event.h>
#include <doomdef.h>

// Efi stuff
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include <Protocol/GraphicsOutput.h>
#include <Protocol/SimpleTextIn.h>

////////////////////////////////////////////////////////////////////

// Frame render func
typedef void(*FrameRenderFptr)(void);

////////////////////////////////////////////////////////////////////

static EFI_GRAPHICS_OUTPUT_PROTOCOL* gGOP = NULL;

static UINT32 frameLeft = 0;
static UINT32 frameTop = 0;
static UINT32 frameWidth = 0;
static UINT32 frameHeight = 0;

static EFI_GRAPHICS_OUTPUT_BLT_PIXEL gPalette[256] = {0};
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL* frameBuffer = NULL;

static FrameRenderFptr gRenderFunc = NULL;

#define FRAME_SCALE 2

////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////

// Called by D_DoomMain,
// determines the hardware configuration
// and sets up the video mode
void I_InitGraphics (void)
{
	EnumerateGraphics();

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

	status = gGOP->SetMode(gGOP, 0);
	if (EFI_ERROR(status))
	{
		printf("Could not set gop mode\n");
		return;
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
		printf("!screens[0]\n");
		return;
	}

	frameBuffer = malloc(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * frameWidth * frameHeight);
	if (!frameBuffer)
	{
		free(screens[0]);
		printf("!frameBuffer\n");
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

	// For now we expect to be scaled by 2
	ASSERT (FRAME_SCALE == 2);

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

			// Double this pixel in width
			*dstLine++ = gPalette[pixel];
			*dstLine++ = gPalette[pixel];
		}
		
		// Copy this dst line to the next one
		memcpy(dstLine, curLine, frameWidth * sizeof(*frameBuffer));
		
		// Skip next line (copied above)
		dstLine += frameWidth;
	}

}

void I_FinishUpdate (void)
{
	MakeFrame();
	gRenderFunc();
}

void I_ShutdownGraphics(void)
{
	free(frameBuffer);
	free(screens[0]);

	gGOP = NULL;
	gRenderFunc = NULL;

	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->EnableCursor(gST->ConOut, TRUE);
}

// Takes full 8 bit values.
void I_SetPalette (byte* palette)
{
	if (!palette)
	{
		printf("No palette!\n");
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
		};
	}
	else
	{
		switch(efiKey->UnicodeChar)
		{
		case CHAR_BACKSPACE: 	return KEY_BACKSPACE;
		case CHAR_TAB:			return KEY_TAB;
		case CHAR_LINEFEED:		return KEY_ENTER;
		default:				return (char)efiKey->UnicodeChar;
		};
	}
}

//
// I_StartTic
//
void I_StartTic (void)
{
	// Get all pending input events
	EFI_STATUS status = EFI_SUCCESS;
	EFI_INPUT_KEY inputKey;

	while (1)
	{
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
		
		event_t event;

		// Post both pressed and released events
		event.type = ev_keydown;
		event.data1 = xlatekey(&inputKey);
		D_PostEvent(&event);
    
		event.type = ev_keyup;
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


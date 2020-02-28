// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_main.c,v 1.4 1997/02/03 22:45:10 b1 Exp $";



#include "doomdef.h"

#include "m_argv.h"
#include "d_main.h"
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>
char* UCS2toANSI(wchar_t* wstr)
{
    char* newchar = calloc(wcslen(wstr) + 1,1);
    int i = 0;
    for (i = 0; i < wcslen(wstr); i++)
    {
        newchar[i] = wstr[i];
    }
    newchar[i] = 0;
    return newchar;
}
int
main
( int		argc,
  char**	argv ) 
{ 
    myargc = argc; 
    myargv = argv; 
    mywargv = (wchar_t**)argv;
    /*for (int i = 0; i < argc; i++)
    {
        myargv[i] = UCS2toANSI(mywargv[i]);
    }*/
    D_DoomMain (); 

    return 0;
} 

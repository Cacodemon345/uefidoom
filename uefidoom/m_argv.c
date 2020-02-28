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
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: m_argv.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";


#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include <wctype.h>

int		myargc;
char**		myargv;
wchar_t** mywargv;

mbstate_t convstate;

// Taken from ReactOS.
int wcscasecmp(const wchar_t* cs,const wchar_t * ct)
 {
     while (towlower(*cs) == towlower(*ct))
     {
         if (*cs == 0)
             return 0;
         cs++;
         ct++;
     }
     return towlower(*cs) - towlower(*ct);
 }

//
// M_CheckParm
// Checks for the given parameter
// in the program's command line arguments.
// Returns the argument number (1 to argc-1)
// or 0 if not present
int M_CheckParm (char *check)
{
    int		i = 1;
    for (i = 1;i<myargc;i++)
    {
        if ( !strcasecmp(check, myargv[i]) )
        {
            printf("Found parameter: %s\n",check);
            return i;
        }
    }

    return 0;
}





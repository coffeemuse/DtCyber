/*--------------------------------------------------------------------------
**
**  Copyright (c) 2003-2011, Tom Hunter
**  Copyright (c) 2023, CoffeeMuse 
**
**
**  Name: window_sdl2.c
**
**  Description:
**      Simulate CDC 6612 or CC545 console display using SDL2.
**      This started as a port of Tom Hunter's window_X11.c
**      
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License version 3 as
**  published by the Free Software Foundation.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License version 3 for more details.
**
**  You should have received a copy of the GNU General Public License
**  version 3 along with this program in file "license-gpl-3.0.txt".
**  If not, see <http://www.gnu.org/licenses/gpl-3.0.txt>.
**
**--------------------------------------------------------------------------
*/

/*
**  -------------
**  Include Files
**  -------------
*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL_ttf.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "const.h"
#include "types.h"
#include "proto.h"

/*
**  -----------------
**  Private Constants
**  -----------------
*/
#define ListSize           5000
#define FrameTime          100000
#define FramesPerSecond    (1000000 / FrameTime)

/*
**  -----------------------
**  Private Macro Functions
**  -----------------------
*/

/*
**  -----------------------------------------
**  Private Typedef and Structure Definitions
**  -----------------------------------------
*/
typedef struct dispList
{
    u16 xPos;                       /* horizontal position */
    u16 yPos;                       /* vertical position */
    u8  fontSize;                   /* size of font */
    u8  ch;                         /* character to be displayed */
} DispList;

/*
**  ---------------------------
**  Private Function Prototypes
**  ---------------------------
*/
void *windowThread(void *param);

/*
**  ----------------
**  Public Variables
**  ----------------
*/

/*
**  -----------------
**  Private Variables
**  -----------------
*/
static volatile bool   displayActive = FALSE;
static u8              currentFont;
static i16             currentX;
static i16             currentY;
static u16             oldCurrentY;
static DispList        display[ListSize];
static u32             listEnd;
static bool            refresh = FALSE;
static pthread_mutex_t mutexDisplay;

/*
 **--------------------------------------------------------------------------
 **
 **  Public Functions
 **
 **--------------------------------------------------------------------------
 */

/*--------------------------------------------------------------------------
**  Purpose:        Create POSIX thread which will deal with all X11
**                  functions.
**
**  Parameters:     Name        Description.
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void windowInit(void)
{
    int            rc;
    pthread_t      thread;
    pthread_attr_t attr;

    /*
    **  Create display list pool.
    */
    listEnd = 0;

    /*
    **  Create a mutex to synchronise access to display list.
    */
    pthread_mutex_init(&mutexDisplay, NULL);

    /*
    **  Create POSIX thread with default attributes.
    */
    pthread_attr_init(&attr);
    rc = pthread_create(&thread, &attr, windowThread, NULL);
}

/*--------------------------------------------------------------------------
**  Purpose:        Set font size.
**                  functions.
**
**  Parameters:     Name        Description.
**                  size        font size in points.
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void windowSetFont(u8 font)
{
    currentFont = font;
}

/*--------------------------------------------------------------------------
**  Purpose:        Set X coordinate.
**
**  Parameters:     Name        Description.
**                  x           horinzontal coordinate (0 - 0777)
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void windowSetX(u16 x)
{
    currentX = x;
}

/*--------------------------------------------------------------------------
**  Purpose:        Set Y coordinate.
**
**  Parameters:     Name        Description.
**                  y           horinzontal coordinate (0 - 0777)
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void windowSetY(u16 y)
{
    currentY = 0777 - y;
    if (oldCurrentY > currentY)
        {
        refresh = TRUE;
        }

    oldCurrentY = currentY;
}

/*--------------------------------------------------------------------------
**  Purpose:        Queue characters.
**
**  Parameters:     Name        Description.
**                  ch          character to be queued.
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void windowQueue(u8 ch)
{
    DispList *elem;

    if ((listEnd >= ListSize)
        || (currentX == -1)
        || (currentY == -1))
        {
        return;
        }

    /*
    **  Protect display list.
    */
    pthread_mutex_lock(&mutexDisplay);

    if (ch != 0)
        {
        elem           = display + listEnd++;
        elem->ch       = ch;
        elem->fontSize = currentFont;
        elem->xPos     = currentX;
        elem->yPos     = currentY;
        }

    currentX += currentFont;

    /*
    **  Release display list.
    */
    pthread_mutex_unlock(&mutexDisplay);
}

/*--------------------------------------------------------------------------
**  Purpose:        Update window.
**
**  Parameters:     Name        Description.
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void windowUpdate(void)
{
    refresh = TRUE;
}

/*--------------------------------------------------------------------------
**  Purpose:        Poll the keyboard (dummy for X11)
**
**  Parameters:     Name        Description.
**
**  Returns:        Nothing
**
**------------------------------------------------------------------------*/
void windowGetChar(void)
{
}

/*--------------------------------------------------------------------------
**  Purpose:        Terminate console window.
**
**  Parameters:     Name        Description.
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void windowTerminate(void)
{
    printf("Shutting down window thread\n");
    displayActive = FALSE;
    sleep(1);
    printf("Shutting down main thread\n");
}

/*
 **--------------------------------------------------------------------------
 **
 **  Private Functions
 **
 **--------------------------------------------------------------------------
 */

/*--------------------------------------------------------------------------
**  Purpose:        Windows thread.
**
**  Parameters:     Name        Description.
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void *windowThread(void *param)
{
}
/*---------------------------  End Of File  ------------------------------*/

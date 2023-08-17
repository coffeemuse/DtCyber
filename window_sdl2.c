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
**  NOTE TO SELF - Reformat this to conform to DtCyber
**  coding standards.
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
#define ListSize 5000
#define FrameTime 100000
#define FramesPerSecond (1000000 / FrameTime)

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
    u16 xPos;    /* horizontal position */
    u16 yPos;    /* vertical position */
    u8 fontSize; /* size of font */
    u8 ch;       /* character to be displayed */
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
static volatile bool displayActive = FALSE;
static u8 currentFont;
static i16 currentX;
static i16 currentY;
static u16 oldCurrentY;
static DispList display[ListSize];
static u32 listEnd;
static int width;
static int height;
static bool refresh = FALSE;
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
    int rc;
    pthread_t thread;
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

    if ((listEnd >= ListSize) || (currentX == -1) || (currentY == -1))
    {
        return;
    }

    /*
    **  Protect display list.
    */
    pthread_mutex_lock(&mutexDisplay);

    if (ch != 0)
    {
        elem = display + listEnd++;
        elem->ch = ch;
        elem->fontSize = currentFont;
        elem->xPos = currentX;
        elem->yPos = currentY;
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
    DispList *curr;
    DispList *end;
    bool isMeta = FALSE;
    u8 oldFont = 0;
    char str[2] = " ";
    int usageDisplayCount = 0;
    SDL_Surface *sur;
    SDL_Texture *tex;
    //SDL_Rect rect;
    
    /*
    ** Initialize SDL
    */
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Failed to initialize SDL2: %s\n", SDL_GetError());
        exit(1);
    }

    /*
    ** Initialize SDL TrueType Font Support
    */
    if (TTF_Init() != 0)
    {
        fprintf(stderr, "Failed to initialize SDL2_ttf: %s\n", SDL_GetError());
        exit(1);
    }

    /*
    **  Load three Cyber fonts.
    */
    TTF_Font *hSmallFont = TTF_OpenFont("FiraMono-Regular.ttf", 14);
    if (hSmallFont == NULL)
    {
        fprintf(stderr, "Failed to load specified small font.\n");
    }
    TTF_Font *hMediumFont = TTF_OpenFont("FiraMono-Regular.ttf", 20);
    if (hMediumFont == NULL)
    {
        fprintf(stderr, "Failed to load specified small font.\n");
    }
    TTF_Font *hLargeFont = TTF_OpenFont("FiraMono-Regular.ttf", 26);
    if (hLargeFont == NULL)
    {
        fprintf(stderr, "Failed to load specified small font.\n");
    }

    /*
    **  Create a window.
    */
    width = 1100;
    height = 750;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);

    /*
    **  Set window and icon titles.
    */
    char windowTitle[132];
    windowTitle[0] = '\0';
    strcat(windowTitle, displayName);
    strcat(windowTitle, " SDL");
    strcat(windowTitle, " - " DtCyberVersion);
    strcat(windowTitle, " - " DtCyberBuildDate);
    SDL_SetWindowTitle(window, windowTitle);

    /*
    **  Setup foregrond and background colors.
    */
    SDL_Color bg = {0, 0, 0};
    SDL_Color fg = {0, 255, 0};

    /*
    **  Initialise input.
    */

    /*
     **  We like to be on top.
     */
    SDL_RaiseWindow(window);

    /*
    **  Window thread loop.
    */
    displayActive = TRUE;
    isMeta = FALSE;
    while (displayActive)
    {
        /*
        **  Process any SDL2 events.
        */
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_KEYDOWN:
                /* Detect if the Left ALT / META key is down */
                if (event.key.keysym.sym == SDLK_LALT)
                {
                    isMeta = TRUE;
                }

                /* Handle normal ASCII keys */
                if (event.key.keysym.sym >= 32 && event.key.keysym.sym <= 127)
                {
                    if (isMeta == FALSE)
                    {
                        ppKeyIn = event.key.keysym.sym;
                        sleepMsec(5);
                    }
                    else
                    {
                        switch (event.key.keysym.sym)
                        {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            traceMask ^= (1 << (event.key.keysym.sym - '0'));
                            break;

                        case 'c':
                            traceMask ^= (1 << 14);
                            break;

                        case 'e':
                            traceMask ^= (1 << 15);
                            break;

                        case 'x':
                            if (traceMask == 0)
                            {
                                traceMask = ~0;
                            }
                            else
                            {
                                traceMask = 0;
                            }
                            break;

                        case 'p':
                            break;
                        }
                        ppKeyIn = 0;
                    }
                }
                break;

            case SDL_KEYUP:
                /* Detect if the Left ALT / META key is released */
                if (event.key.keysym.sym == SDLK_LALT)
                {
                    isMeta = FALSE;
                }
                break;

            case SDL_WINDOWEVENT_SIZE_CHANGED:
                break;

            case SDL_WINDOWEVENT_CLOSE:
                break;
            }
        }

        if (opPaused)
        {
            /*
            **  Display pause message.
            */
            static char opMessage[] = "Emulation paused";
            //  XSetFont(disp, gc, hLargeFont);
            //  oldFont = FontLarge;
            //  XDrawString(disp, pixmap, gc, 20, 256, opMessage, strlen(opMessage));
        }

        /*
        **  Protect display list.
        */
        pthread_mutex_lock(&mutexDisplay);

        if (usageDisplayCount != 0)
        {
            /*
            **  Display usage note when user attempts to close window.
            */
            static char usageMessage1[] = "Please don't just close the window, but instead first cleanly halt the operating system and";
            static char usageMessage2[] = "then use the 'shutdown' command in the operator interface to terminate the emulation.";
            // XSetFont(disp, gc, hMediumFont);
            oldFont = FontMedium;
            // XDrawString(disp, pixmap, gc, 20, 256, usageMessage1, strlen(usageMessage1));
            // XDrawString(disp, pixmap, gc, 20, 275, usageMessage2, strlen(usageMessage2));
            listEnd = 0;
            usageDisplayCount -= 1;
        }

        /*
        **  Draw display list in pixmap.
        */
        curr = display;
        end = display + listEnd;

        for (curr = display; curr < end; curr++)
        {
            /*
            **  Setup new font if necessary.
            */
            if (oldFont != curr->fontSize)
            {
                oldFont = curr->fontSize
            }

            /*
            **  Draw dot or character.
            */
            if (curr->fontSize == FontDot)
            {
                SDL_SetRenderDrawColor(renderer,0,255,0,0);
                SDL_RenderDrawPoint(renderer,curr->xPos, (curr->yPos * 14) / 10 + 20);
            }
            else
            {
                str[0] = curr->ch;
                SDL_SetRenderDrawColor(renderer,0,255,0,0);
                switch (curr->fontSize)
                {
                case FontSmall:
                    sur = TTF_RenderText_Solid(hSmallFont,str,fg);
                    break;

                case FontMedium:
                   sur = TTF_RenderText_Solid(hMediumFont,str,fg);
                    break;

                case FontLarge:
                    sur = TTF_RenderText_Solid(hLargeFont,str,fg);
                    break;
                }   
            
                tex = SDL_CreateTextureFromSurface(renderer, sur);
                SDL_Rect rect = {curr->xPos, (curr->yPos * 14) / 10 + 20, sur->w, sur->h};
                SDL_RenderCopy(renderer, tex, NULL, &rect);
                SDL_DestroyTexture(tex);
                SDL_FreeSurface(sur);
                
            }
        }

        listEnd = 0;
        currentX = -1;
        currentY = -1;
        refresh = FALSE;

        /*
        **  Release display list.
        */
        pthread_mutex_unlock(&mutexDisplay);

        /*
        **  Update display from pixmap.
        */
        //XCopyArea(disp, pixmap, window, gc, 0, 0, width, height, 0, 0);

        /*
        **  Erase pixmap for next round.
        */
        //XSetForeground(disp, gc, bg);
        //XFillRectangle(disp, pixmap, gc, 0, 0, width, height);

        /*
        **  Make sure the updates make it to the X11 server.
        */
        //XSync(disp, 0);



        SDL_RenderPresent(renderer);
        SDL_SetRenderDrawColor(renderer,0,0,0,0);
        SDL_RenderClear(renderer);
        /*
        **  Give other threads a chance to run. This may require customisation.
        */
        sleepUsec(FrameTime);
    }

    /*
    **  SDL and thread cleanup
    */
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(hSmallFont);
    TTF_CloseFont(hMediumFont);
    TTF_CloseFont(hLargeFont);
    TTF_Quit();
    SDL_Quit();
    pthread_exit(NULL);
}


/*---------------------------  End Of File  ------------------------------*/

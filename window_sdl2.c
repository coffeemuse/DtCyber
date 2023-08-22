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
**  coding standards before final release.
*/

/*
**  -------------
**  Include Files
**  -------------
*/
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
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
#define coord(x,y) posX+(x*sizeX),posY+(y*sizeY)

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
void renderVectorText(SDL_Renderer*, char, int, int, int, u8, float, float);

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
**  Purpose:        Create POSIX thread which will deal with all SDL2
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
    bool isFullScreen = FALSE;


    /*
    ** Initialize SDL
    */
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Failed to initialize SDL2: %s\n", SDL_GetError());
        exit(1);
    }

    /*
    **  Create a window.
    */
    int winWidth = 1056;
    int winHeight = 512;
    float scaleX = 1.0;
    float scaleY = 1.0;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_CreateWindowAndRenderer(winWidth, winHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_SetWindowResizable(window, SDL_TRUE);
    
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
                /* 
                ** Detect if the Left ALT / META key is down 
                */
                if (event.key.keysym.sym == SDLK_LALT)
                {
                    isMeta = TRUE;
                }

                /* 
                ** Handle normal ASCII keys 
                */
                if ( (event.key.keysym.sym >= 32 && event.key.keysym.sym <= 127) 
                    || event.key.keysym.sym == SDLK_BACKSPACE 
                    || event.key.keysym.sym == SDLK_RETURN )
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
                /* 
                ** Detect if the Left ALT / META key is released. 
                */
                if (event.key.keysym.sym == SDLK_LALT)
                {
                    isMeta = FALSE;
                }
                break;

            case SDL_WINDOWEVENT:
                
                switch (event.window.event) 
                {

                    case SDL_WINDOWEVENT_RESIZED:
                    case SDL_WINDOWEVENT_SIZE_CHANGED:

                        SDL_GetWindowSize(window, &winWidth, &winHeight);
                        scaleX = winWidth/1056.0;
                        scaleY = winHeight/512.0;  

                        //DEBUG -CoffeeMuse
                        fprintf(stderr,"winH - %i\n",winHeight);
                        fprintf(stderr,"winW - %i\n",winWidth);

                        fprintf(stderr,"scaleX - %f\n",scaleX);
                        fprintf(stderr,"scaleY - %f\n",scaleY);
                        break;
                }

                break;
            }
        }

        if (opPaused)
        {
            /*
            **  Display pause message.
            */
            oldFont = FontLarge;
            static char opMessage[] = "EMULATION PAUSED";
            // loop through chars in array and draw them
            for (int i = 0; i < strlen(opMessage); i++)
            {
                renderVectorText(renderer, opMessage[i], (i*32)+128, 256, 32, 0, scaleX, scaleY);
            }

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
            char usageMessage1[] = "PLEASE DO NOT CLOSE THIS WINDOW. FIRST CLEANLY SHUTDOWN THE OPERATING SYSTEM";
            char usageMessage2[] = "THEN USE THE (SHUTDOWN) COMMAND IN THE OPEATOR INTERFACE TO SHUTDOWN EMULATOR.)";
            oldFont = FontMedium;

            for (int i = 0; i < strlen(usageMessage1); i++)
            {
                renderVectorText(renderer, usageMessage1[i], (i*16)+16, 256, 16, 0, scaleX, scaleY);
            }

            for (int i = 0; i < strlen(usageMessage2); i++)
            {
                renderVectorText(renderer, usageMessage2[i], (i*16)+16, 275, 16, 0, scaleX, scaleY);
            }
            
            listEnd = 0;
            usageDisplayCount -= 1;
        }

        /*
        **  Draw display list on renderer.
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
                oldFont = curr->fontSize;
            }

            /*
            **  Draw dot or character.
            */

            //SetPixel(hdcMem, (curr->xPos * ScaleX) / 10, (curr->yPos * ScaleY) / 10 + 30, RGB(0, 255, 0));
            //TextOut(hdcMem, (curr->xPos * ScaleX) / 10, (curr->yPos * ScaleY) / 10 + 20, str, 1);

            if (curr->fontSize == FontDot)
            {
                SDL_SetRenderDrawColor(renderer,255,255,255,255);
                SDL_RenderDrawPoint(renderer,(curr->xPos* scaleX), (curr->yPos * scaleY)+ 30);

                //SDL_RenderDrawPoint(renderer,(curr->xPos* scaleX), (curr->yPos * scaleY));

            }
            else
            {
                str[0] = curr->ch;
                SDL_SetRenderDrawColor(renderer,0,255,0,255);
                renderVectorText(renderer, str[0], curr->xPos, curr->yPos, curr->fontSize, 0, scaleX, scaleY);
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
        **  Render the Display
        */

        SDL_RenderPresent(renderer);
        /*
        **  Erase renderer for next round.
        */
        SDL_SetRenderDrawColor(renderer,0,0,0,255);
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
    SDL_Quit();
    pthread_exit(NULL);
}
/*--------------------------------------------------------------------------
**  Purpose:        Draw text to the screen
**
**  Parameters:     Name        Description.
**                  ren         SDL_Renderer
**                  c           character to be drawn
**                  x           horinzontal coordinate
**                  y           vertical coordinate
**                  size        font size
**                  color       color
**                  scaleX      scale factor for X
**                  scaleY      scale factor for Y
**
**  Returns:        Nothing.
**
**------------------------------------------------------------------------*/
void renderVectorText(SDL_Renderer* ren, char c, int x, int y, int size, u8 color, float scaleX, float scaleY)
{
    float posX = 0.0+(x*scaleX);
    float posY = 0.0+(y*scaleY);
    size = size/8;
    float sizeX = size*scaleX;
    float sizeY = size*scaleY;
    SDL_SetRenderDrawColor(ren, 255,255,255,255);
    
    switch (c)
    {
        /*
        ** Draw Letters
        */
        
        case 'A':
            SDL_FPoint points65[] = {
                {coord(0,6)},
                {coord(3,0)},
                {coord(6,6)},
                {coord(5,4)},
                {coord(1,4)}
            };
            SDL_RenderDrawLinesF(ren, points65, 5);
            break;

        case 'B':
            SDL_FPoint points66[] = {
                {coord(0,6)},
                {coord(0,0)},
                {coord(4,0)},
                {coord(6,1)},
                {coord(6,2)},
                {coord(4,3)},
                {coord(0,3)},
                {coord(4,3)},
                {coord(6,4)},
                {coord(6,5)}, 
                {coord(4,6)}, 
                {coord(0,6)}
            };
            SDL_RenderDrawLinesF(ren, points66, 12);
            break;

        case 'C':
            SDL_FPoint points67[] = {
                {coord(6,1)},
                {coord(4,0)},
                {coord(2,0)},
                {coord(0,1)},
                {coord(0,5)},
                {coord(2,6)},
                {coord(4,6)},
                {coord(6,5)}
            };

            SDL_RenderDrawLinesF(ren, points67, 8);
            break;

        case 'D':
            SDL_FPoint points68[] = {
                {coord(0,6)},
                {coord(0,0)},
                {coord(4,0)},
                {coord(6,1)},
                {coord(6,5)},
                {coord(4,6)},
                {coord(0,6)}
            };
            SDL_RenderDrawLinesF(ren, points68, 7);
            break;

        case 'E':
            SDL_FPoint points69[] = {
                {coord(6,6)},
                {coord(0,6)},
                {coord(0,0)},
                {coord(6,0)}
            };
            SDL_RenderDrawLinesF(ren, points69, 4);
            SDL_RenderDrawLineF(ren,coord(0,2),coord(4,2));
            break;

        case 'F':
            SDL_FPoint points70[] = {
                {coord(0,6)},
                {coord(0,0)},
                {coord(6,0)}
            };
            SDL_RenderDrawLinesF(ren, points70, 3);
            SDL_RenderDrawLineF(ren,coord(0,3),coord(4,3));
            break;

        case 'G':
            SDL_FPoint points71[] = {
                {coord(6,1)},
                {coord(4,0)},
                {coord(2,0)},
                {coord(0,1)},
                {coord(0,5)},
                {coord(2,6)},
                {coord(4,6)},
                {coord(6,5)},
                {coord(6,3)},
                {coord(4,3)}
            };
            SDL_RenderDrawLinesF(ren, points71, 10);
            break;

        case 'H': 
            SDL_RenderDrawLineF(ren,coord(0,0),coord(0,6));
            SDL_RenderDrawLineF(ren,coord(6,0),coord(6,6));
            SDL_RenderDrawLineF(ren,coord(0,3),coord(6,3));
            break;

        case 'I':
            SDL_RenderDrawLineF(ren,coord(0,0),coord(6,0));
            SDL_RenderDrawLineF(ren,coord(3,0),coord(3,6));
            SDL_RenderDrawLineF(ren,coord(0,6),coord(6,6));
            break;

        case 'J':
            SDL_FPoint points74[] = {
                {coord(6,0)},
                {coord(6,4)},
                {coord(4,6)},
                {coord(2,6)},
                {coord(0,4)}
            };
            SDL_RenderDrawLinesF(ren, points74, 5);
            break;
        
        case 'K':
            SDL_FPoint points75[] = {
                {coord(6,0)},
                {coord(0,3)},
                {coord(6,6)}
            };
            SDL_RenderDrawLinesF(ren, points75, 3);
            SDL_RenderDrawLineF(ren,coord(0,0),coord(0,6));
            break;
        
        case 'L':
            SDL_FPoint points76[] = {
                {coord(0,0)},
                {coord(0,6)},
                {coord(6,6)}
            };
            SDL_RenderDrawLinesF(ren, points76, 3);
            break;
        
        case 'M':
            SDL_FPoint points77[] = {
                {coord(0,6)},
                {coord(0,0)},
                {coord(3,3)},
                {coord(6,0)},
                {coord(6,6)}
            };
            SDL_RenderDrawLinesF(ren, points77, 5);
            break;

        case 'N':
            SDL_FPoint points78[] = {
                {coord(0,6)},
                {coord(0,0)},
                {coord(6,6)},
                {coord(6,0)}
            };
            SDL_RenderDrawLinesF(ren, points78, 4);
            break;

        case 'O':
            SDL_FPoint points79[] = {
                {coord(0,4)},
                {coord(0,2)},
                {coord(2,0)},
                {coord(4,0)},
                {coord(6,2)},
                {coord(6,4)},
                {coord(4,6)},
                {coord(2,6)},
                {coord(0,4)}
            };
            SDL_RenderDrawLinesF(ren, points79, 9);
            break;

        case 'P':
            SDL_FPoint points80[] = {
                {coord(0,6)},
                {coord(0,0)},
                {coord(4,0)},
                {coord(6,1)},
                {coord(6,2)},
                {coord(4,3)},
                {coord(0,3)}
            };
            SDL_RenderDrawLinesF(ren, points80, 7);
            break; 

        case 'Q':
            SDL_FPoint points81[] = {
                {coord(0,4)},
                {coord(0,2)},
                {coord(2,0)},
                {coord(4,0)},
                {coord(6,2)},
                {coord(6,4)},
                {coord(4,6)},
                {coord(2,6)},
                {coord(0,4)}
            };
            SDL_RenderDrawLinesF(ren, points81, 9);
            SDL_RenderDrawLineF(ren,coord(4,4),coord(6,6));
            break;

        case 'R':
            SDL_FPoint points82[] = {
                {coord(0,6)},
                {coord(0,0)},
                {coord(4,0)},
                {coord(6,1)},
                {coord(6,2)},
                {coord(4,3)},
                {coord(0,3)},
                {coord(6,6)}
            };
            SDL_RenderDrawLinesF(ren, points82, 8);
            break;

        case 'S':
            SDL_FPoint points83[] = {
                {coord(0,5)},
                {coord(2,6)},
                {coord(4,6)},
                {coord(6,5)},
                {coord(6,4)},
                {coord(4,3)},
                {coord(2,3)},
                {coord(0,2)},
                {coord(0,1)},
                {coord(2,0)},
                {coord(4,0)},
                {coord(6,1)}
            };
            SDL_RenderDrawLinesF(ren, points83, 12);
            break;

        case 'T':
            SDL_RenderDrawLineF(ren,coord(0,0),coord(6,0));
            SDL_RenderDrawLineF(ren,coord(3,0),coord(3,6));
            break;

        case 'U':
            SDL_FPoint points85[] = {
                {coord(0,0)},
                {coord(0,4)},
                {coord(1,6)},
                {coord(5,6)},
                {coord(6,4)},
                {coord(6,0)}
            };
            SDL_RenderDrawLinesF(ren, points85, 6);
            break;

        case 'V':
            SDL_FPoint points86[] = {
                {coord(0,0)},
                {coord(3,6)},
                {coord(6,0)}
            };
            SDL_RenderDrawLinesF(ren, points86, 3);
            break;
        
        case 'W':
            SDL_FPoint points87[] = {
                {coord(0,0)},
                {coord(0,6)},
                {coord(3,3)},
                {coord(6,6)},
                {coord(6,0)}
            };
            SDL_RenderDrawLinesF(ren, points87, 5);
            break;

        case 'X':
            SDL_RenderDrawLineF(ren,coord(0,0),coord(6,6));
            SDL_RenderDrawLineF(ren,coord(0,6),coord(6,0));
            break;

        case 'Y':
            SDL_RenderDrawLineF(ren,coord(0,0),coord(3,3));
            SDL_RenderDrawLineF(ren,coord(3,3),coord(6,0));
            SDL_RenderDrawLineF(ren,coord(3,3),coord(3,6));
            break;

        case 'Z':
            SDL_FPoint points90[] = {
                {coord(0,0)},
                {coord(6,0)},
                {coord(0,6)},
                {coord(6,6)}
            };
            SDL_RenderDrawLinesF(ren, points90, 4);
            break;

       /*
       ** Draw Numbers
       */
        case '0':
            SDL_FPoint points48[] = {
                {coord(1,0)},
                {coord(5,0)},
                {coord(6,1)},
                {coord(6,5)},
                {coord(5,6)},
                {coord(1,6)},
                {coord(0,5)},
                {coord(0,1)},
                {coord(1,0)}
            };
            SDL_RenderDrawLinesF(ren, points48, 9);
            break;

        case '1':
            SDL_FPoint points49[] = {
                {coord(1,1)},
                {coord(3,0)},
                {coord(3,6)}
            };
            SDL_RenderDrawLinesF(ren, points49, 3);
            break;

        case '2':
            SDL_FPoint points50[] = {
                {coord(0,1)},
                {coord(2,0)},
                {coord(4,0)},
                {coord(6,1)},
                {coord(6,2)},
                {coord(2,4)},
                {coord(0,6)},
                {coord(6,6)}
            };
            SDL_RenderDrawLinesF(ren, points50, 8);
            break;

        case '3':
            SDL_FPoint points51[] = {
                {coord(0,0)},
                {coord(6,0)},
                {coord(2,2)},
                {coord(4,2)},
                {coord(6,3)},
                {coord(6,5)},
                {coord(4,6)},
                {coord(3,6)},
                {coord(0,5)}
            };
            SDL_RenderDrawLinesF(ren, points51, 9);            
            break;

        case '4':           
            SDL_FPoint points52[] = {
                {coord(4,6)},
                {coord(4,0)},
                {coord(0,3)},
                {coord(6,3)}
            };
            SDL_RenderDrawLinesF(ren, points52, 4);
            break;

        case '5':
            SDL_FPoint points53[] = {
                {coord(0,5)},
                {coord(2,6)},
                {coord(4,6)},
                {coord(6,5)},
                {coord(6,3)},
                {coord(4,2)},
                {coord(0,2)},
                {coord(0,0)},
                {coord(6,0)}
            };
            SDL_RenderDrawLinesF(ren, points53, 9);
            break;

        case '6':
            SDL_FPoint points54[] = {
                {coord(6,1)},
                {coord(4,0)},
                {coord(2,0)},
                {coord(0,1)},
                {coord(0,5)},
                {coord(2,6)},
                {coord(4,6)},
                {coord(6,5)},
                {coord(6,3)},
                {coord(4,2)},
                {coord(2,2)},
                {coord(0,3)}
            };
            SDL_RenderDrawLinesF(ren, points54, 12);
            break;

        case '7':
            SDL_FPoint points55[] = {
                {coord(0,0)},
                {coord(6,0)},
                {coord(3,3)},
                {coord(2,5)},
                {coord(2,6)}
            };
            SDL_RenderDrawLinesF(ren, points55, 5);
            break;

        case '8':
            SDL_FPoint points56[] = {
                {coord(2,0)},
                {coord(4,0)},
                {coord(6,1)},
                {coord(6,2)},
                {coord(4,3)},
                {coord(2,3)},
                {coord(0,4)},
                {coord(0,5)},
                {coord(2,6)},
                {coord(4,6)},
                {coord(6,5)},
                {coord(6,4)},
                {coord(4,3)},
                {coord(2,3)},
                {coord(0,2)},
                {coord(0,1)},
                {coord(2,0)}
            };
            SDL_RenderDrawLinesF(ren, points56, 17);
            break;
      
        case '9':
            SDL_FPoint points57[] = {
                {coord(0,5)},
                {coord(2,6)},
                {coord(4,6)},
                {coord(6,5)},
                {coord(6,1)},
                {coord(4,0)},
                {coord(2,0)},
                {coord(0,1)},
                {coord(0,3)},
                {coord(2,4)},
                {coord(4,4)},
                {coord(6,3)}
            };
            SDL_RenderDrawLinesF(ren, points57, 12);
            break;

        /*
        ** Symbols
        */
        case ' ':
            /*
            ** Nothing to draw for a space.
            */
            break;

        case '=':
            SDL_RenderDrawLineF(ren,coord(1,2),coord(5,2));
            SDL_RenderDrawLineF(ren,coord(1,4),coord(5,4));
            break;
            
        case '-':
            SDL_RenderDrawLineF(ren,coord(1,3),coord(5,3));
            break;

        case '+':
            SDL_RenderDrawLineF(ren,coord(1,3),coord(5,3));
            SDL_RenderDrawLineF(ren,coord(3,1),coord(3,5));   
            break;
        
        case '*':
            SDL_RenderDrawLineF(ren,coord(1,1),coord(5,5));
            SDL_RenderDrawLineF(ren,coord(1,5),coord(5,1));
            SDL_RenderDrawLineF(ren,coord(1,3),coord(5,3));
            break;
        
        case ',':
            SDL_RenderDrawLineF(ren,coord(0,6),coord(1,5));
            break;
        
        case '.':
            /*
            **  [TODO] This is just too small!  Need to find a way to make it larger
            */ 
            SDL_RenderDrawPointF(ren,coord(0,6));         
            break;
        
        case '(':
            SDL_FPoint points40[] = {
                {coord(4,0)},
                {coord(3,1)},
                {coord(3,5)},
                {coord(4,6)}
            };
            SDL_RenderDrawLinesF(ren, points40, 4);
            break;

        case ')':
            SDL_FPoint points41[] = {
                {coord(2,0)},
                {coord(3,1)},
                {coord(3,5)},
                {coord(2,6)}
            };
            SDL_RenderDrawLinesF(ren, points41, 4);
            break;

        case '/':
            SDL_RenderDrawLineF(ren,coord(0,6),coord(6,0));
            break;

        default:
        /*
        ** Do nothing for unknown / unsupported characters.
        */
    }

}

/*---------------------------  End Of File  ------------------------------*/

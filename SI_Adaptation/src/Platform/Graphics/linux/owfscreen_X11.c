/* Copyright (c) 2009 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */


#ifdef __cplusplus
extern "C" {
#endif



#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>

#include "owfdebug.h"
#include "owfscreen.h"
#include "owfimage.h"
#include "owflinkedlist.h"
#include "owfmemory.h"
#include "owfthread.h"
#include "owfdisplaycontextgeneral.h"


typedef struct
{
    OWFint screenNumber;

    Window w;
    GC gc;
    XImage* img;
    OWFint x;
    OWFint y;
    OWFint width;
    OWFint height;
    OWFint pixelSize;
    OWF_SCREEN screenParams;
    OWF_SCREEN_CALLBACK callback;
    void* callbackData;

} OWF_SCREEN_X11;

typedef struct
{
    OWFint screenNumber;

    OWF_POOL* pool;
    Display* display;
    OWFint nbrScreens;
    OWFint displayX;
    OWFint displayY;

    OWF_THREAD eventLoop;

} OWF_SCREEN_INFO;


#define DEFAULT_SCREEN_NUMBER 1
#define MAX_SCREENS 5
#define PIXEL_PADDING 6 /* number of padding pixels around a window */


OWF_PUBLIC OWF_NODE*
OWF_Screen_GetSetScreens(OWF_NODE* s, OWFboolean set)
{
    static OWF_NODE* screens = NULL;
    static OWFboolean initialized = OWF_FALSE;

    if (!initialized) {
        initialized = OWF_TRUE;
    }

    if (set)
    {
        screens = s;
        return s;
    }
    else
    {
        return screens;
    }
}

static OWF_NODE*
OWF_Screen_GetScreens()
{
    return OWF_Screen_GetSetScreens(NULL, OWF_FALSE);
}

static void
OWF_Screen_SetScreens(OWF_NODE* s)
{
    OWF_Screen_GetSetScreens(s, OWF_TRUE);
}



static OWFint
screenCallBack(void* screenX11, void* message)
{
    OWF_SCREEN_X11* screen;

    OWF_ASSERT(screenX11);

    screen = (OWF_SCREEN_X11*)screenX11;
    if (screen->callback && message)
    {
        char c = *(char*)message;

        (screen->callback)(screen->callbackData, screen->screenNumber, c);
    }

    return 1;
}

static OWFint
findScreenByNumber(void* screenX11, void* number)
{
    OWFint screenNumber;
    OWF_SCREEN_X11* screen;

    OWF_ASSERT(number);
    OWF_ASSERT(screenX11);

    screen = (OWF_SCREEN_X11*)screenX11;
    screenNumber = *(OWFint*)number;

    return (screen->screenNumber == screenNumber) ? 1 : 0;

}

static OWFint
findScreenByWindow(void* screenX11, void* window)
{
    Window w;
    OWF_SCREEN_X11* screen;

    OWF_ASSERT(window);
    OWF_ASSERT(screenX11);

    screen = (OWF_SCREEN_X11*)screenX11;
    w = *(Window*)window;

    return (screen->w == w) ? 1 : 0;
}


static OWFint
screenCleanUp(void* screenX11, void* dummy)
{
    OWF_SCREEN_X11* screen = (OWF_SCREEN_X11*)screenX11;
    if (screen->screenNumber != OWF_INVALID_SCREEN_NUMBER)
    {
        OWF_Screen_Destroy(screen->screenNumber);
    }
    dummy = dummy; /* not used but required */
    return 1;
}

static void
handleExpose(XExposeEvent* xexpose)
{
    OWF_SCREEN_INFO* screenInfo;
    OWF_SCREEN_X11* screen;
    OWF_NODE* node;

    screenInfo = (OWF_SCREEN_INFO*)OWF_Screen_GetScreens()->data;
    OWF_ASSERT(screenInfo);

    node = OWF_List_Find(OWF_Screen_GetScreens(),
                         findScreenByWindow,
                         &xexpose->window);
    if (!node)
    {
        return;
    }

    screen = (OWF_SCREEN_X11*)node->data;
    OWF_ASSERT(screen);

    XLockDisplay(xexpose->display);
    XPutImage(xexpose->display, screen->w, screen->gc, screen->img,
            0, 0, 0, 0,
            screen->width, screen->height);
    XSync(xexpose->display, False);
    XUnlockDisplay(xexpose->display);

}

static void
handleKeyPress(XKeyEvent* xkey)
{
    OWF_SCREEN_INFO* screenInfo;
    OWF_SCREEN_X11* screen;
    OWF_NODE* node;

    char text[255]; /* a char buffer for KeyPress Events */
    KeySym key;

    if (XLookupString(xkey, text, 255, &key, 0) != 1)
    {
        return;
    }

    if (text[0] == 'q')
    {
        abort(); /* quit */
    }

    screenInfo = (OWF_SCREEN_INFO*) OWF_Screen_GetScreens()->data;
    OWF_ASSERT(screenInfo);

    node = OWF_List_Find(OWF_Screen_GetScreens(), findScreenByWindow, &xkey->window);
    if (!node)
    {
        return;
    }

    screen = (OWF_SCREEN_X11*) node->data;
    OWF_ASSERT(screen);

    if (screen->callback)
    {
        (screen->callback)(screen->callbackData, screen->screenNumber, text[0]);
    }
}

static void*
eventLoopThread(void* display)
{
    XEvent e;

    /* wait for something, e.g. keypress */
    while (1)
    {
        XNextEvent(display, &e);

        switch (e.type)
        {
        case Expose:
             handleExpose((XExposeEvent*)&e.xexpose);
             break;
        case KeyPress:
        {
            handleKeyPress((XKeyEvent*)&e.xkey);
            break;
        }
        default:
            continue;
        }
    }
}


/*
 *
 */
static OWFint
OWF_Screen_GetByteOrder()
{
    union {
        char c[sizeof(short)];
        short s;
    } order;

    order.s = 1;

    if ((1 == order.c[0])) {
        return LSBFirst;
    } else {
        return MSBFirst;
    }
}

static XImage*
OWF_Screen_CreateImage(OWF_SCREEN_INFO* screenInfo,
                       OWF_SCREEN_X11* screen)
{
    char* pixels;
    OWFint s, depth;
    Visual* v;

    s = DefaultScreen(screenInfo->display);
    depth = DefaultDepth(screenInfo->display, s);
    v = DefaultVisual(screenInfo->display, s);

    /* Create pixel storage for the image
     * Cannot use xalloc, because XDestroyImage
     * frees the area.
     */
    pixels = calloc(screen->width * screen->height, screen->pixelSize);

    screen->img = XCreateImage (screenInfo->display,
            v, depth, ZPixmap, 0, pixels,
            screen->width, screen->height, 32, 0);
    screen->img->byte_order = OWF_Screen_GetByteOrder();

    XInitImage(screen->img);

    return screen->img;
}

OWF_API_CALL void
OWF_Screen_Notify(void* data)
{
    OWF_List_ForEach(OWF_Screen_GetScreens(), screenCallBack, data);
}

OWF_API_CALL OWFboolean
OWF_Screen_Initialize()
{
    OWF_SCREEN_INFO* screenInfo = NULL;
    OWF_POOL* pool = NULL;

    DPRINT(("X11: OWF_Screen_Initialize (Enter)"));
    pool = OWF_Pool_Create(sizeof(OWF_NODE), MAX_SCREENS);
    OWF_ASSERT(pool);

    screenInfo = (OWF_SCREEN_INFO*)xalloc(1, sizeof(*screenInfo));
    OWF_ASSERT(screenInfo);

    XInitThreads();

    screenInfo->display = XOpenDisplay(NULL);;
    screenInfo->screenNumber = 0; /* head node have screen number 0 */
    screenInfo->nbrScreens = 0;
    screenInfo->displayX = 0;
    screenInfo->displayY = 0;
    screenInfo->pool = pool;

    /* info node is always the first one in the list */
    OWF_Screen_SetScreens(OWF_Node_Create(pool, screenInfo));

    screenInfo->eventLoop = OWF_Thread_Create(eventLoopThread,
            screenInfo->display);

    atexit(OWF_Screen_Terminate); /* set X exit handler */

    DPRINT(("X11: OWF_Screen_Initialize (Exit)"));
    return screenInfo->display != NULL;
}


OWF_API_CALL void
OWF_Screen_Terminate()
{
    OWF_SCREEN_INFO* screenInfo;
    OWF_POOL* pool;

    screenInfo = (OWF_SCREEN_INFO*)OWF_Screen_GetScreens()->data;
    OWF_ASSERT(screenInfo);

    pool = screenInfo->pool;
    OWF_List_ForEach(OWF_Screen_GetScreens(), screenCleanUp, NULL);
    XCloseDisplay(screenInfo->display);

    /* eventLoop should exit at first DestroyNotify event */
    OWF_Thread_Cancel(screenInfo->eventLoop);
    OWF_Thread_Join(screenInfo->eventLoop, NULL);
    OWF_Thread_Destroy(screenInfo->eventLoop);

    xfree(screenInfo); /* free list head data */
    OWF_List_Clear(OWF_Screen_GetScreens());
    OWF_Pool_Destroy(pool);
}

OWF_API_CALL OWFint
OWF_Screen_GetDefaultNumber()
{
	return DEFAULT_SCREEN_NUMBER;
}

OWF_API_CALL OWFboolean
OWF_Screen_GetHeader(OWFint screenNumber, OWF_SCREEN* header)
{
    OWF_SCREEN_INFO* screenInfo;
    OWF_SCREEN_X11* screen;
    OWF_NODE* node;
    OWF_IMAGE_FORMAT format;

    OWF_ASSERT(screenNumber != OWF_INVALID_SCREEN_NUMBER);

    screenInfo = (OWF_SCREEN_INFO*)OWF_Screen_GetScreens()->data;
    OWF_ASSERT(screenInfo);

	if (screenNumber < 1 || screenNumber > screenInfo->nbrScreens)
	{
		return OWF_FALSE;
	}

	node = OWF_List_Find(OWF_Screen_GetScreens(), findScreenByNumber, &screenNumber);
	if (!node || !node->data)
	{
	    return OWF_FALSE;
	}

	screen = (OWF_SCREEN_X11*)node->data;
    
	header->normal.width  = screen->width;
	header->normal.height = screen->height;
    header->initialRotation = header->currentRotation = OWF_ROTATION_0;
    header->supportedRotations = OWF_SUPPORT_ROTATION_ALL;
    header->pixelFormat = OWF_SURFACE_PIXEL_FORMAT;
    
    format.pixelFormat = OWF_SURFACE_PIXEL_FORMAT;
    format.linear = OWF_SURFACE_LINEAR;
    format.premultiplied = OWF_SURFACE_PREMULTIPLIED;
    format.rowPadding = OWF_SURFACE_ROWPADDING;
        
    header->normal.width  = screen->width;
    header->normal.height = screen->height;
    header->normal.stride = OWF_Image_GetStride(screen->width, &format, 0);

    header->flipped.width  = screen->height;
    header->flipped.height = screen->width;
    header->normal.stride = OWF_Image_GetStride(screen->height, &format, 0);
    
 		header->inUse = OWF_DisplayContext_IsLive(screenNumber);    
            
	return OWF_TRUE;
}

OWF_API_CALL OWFint
OWF_Screen_Create(OWFint width, OWFint height, OWF_SCREEN_CALLBACK func, void* obj)
{
    OWF_SCREEN_X11* screen = NULL;
    OWF_SCREEN_INFO* screenInfo = NULL;
    OWF_NODE* node = NULL;

    Display* d;
    Window w;
    GC gc;
    OWFint s;

    screenInfo = (OWF_SCREEN_INFO*)OWF_Screen_GetScreens()->data;
    OWF_ASSERT(screenInfo);

    d = screenInfo->display;
    s = DefaultScreen(d);

    if (screenInfo->nbrScreens > 0)
    {
        /* go to next 'row' of screens */
        if (screenInfo->displayX + PIXEL_PADDING + width > DisplayWidth(d,s))
        {
            screenInfo->displayX = 0;
        }
    }
    else
    {
        screenInfo->displayX = 0;
        screenInfo->displayY = 0;
    }

    XLockDisplay(screenInfo->display);

    w = XCreateSimpleWindow(d, RootWindow(d, s),
            0, 0,
            width, height, 2 /* border width */,
            WhitePixel(d, s), WhitePixel(d, s));

    XSelectInput(d, w, ExposureMask | KeyPressMask);

    gc = XCreateGC(d, w, 0, NULL);

    XMapWindow(d, w);

    screenInfo->displayX += PIXEL_PADDING;
    XMoveWindow(d, w, screenInfo->displayX, screenInfo->displayY);

    XUnlockDisplay(screenInfo->display);

    screen = (OWF_SCREEN_X11*)xalloc(1, sizeof(*screen));
    OWF_ASSERT(screen);
    screen->screenNumber = ++screenInfo->nbrScreens;
    screen->w = w;
    screen->gc = gc;

    screen->pixelSize = OWF_Image_GetFormatPixelSize(OWF_SURFACE_PIXEL_FORMAT);

    screen->x = screenInfo->displayX;
    screen->y = screenInfo->displayY;
    screen->width = width;
    screen->height = height;
    screen->img = OWF_Screen_CreateImage(screenInfo, screen);

    screen->callback = func;
    screen->callbackData = obj;

    node = OWF_Node_Create(screenInfo->pool, screen);
    OWF_List_Append(OWF_Screen_GetScreens(), node);

    screenInfo->displayX += width;

    return screenInfo->nbrScreens;
}

OWF_API_CALL void
OWF_Screen_Destroy(OWFint screenNumber)
{
    OWF_SCREEN_INFO* screenInfo;
    OWF_NODE* node;

    OWF_ASSERT(screenNumber != OWF_INVALID_SCREEN_NUMBER);

    screenInfo = (OWF_SCREEN_INFO*)OWF_Screen_GetScreens()->data;
    OWF_ASSERT(screenInfo);
    node = OWF_List_Find(OWF_Screen_GetScreens(), findScreenByNumber, &screenNumber);

    if (node)
    {
        OWF_SCREEN_X11* screen;

        OWF_ASSERT(node && node->data);

        OWF_List_Remove(OWF_Screen_GetScreens(), node);

        screen = (OWF_SCREEN_X11*)node->data;
        OWF_ASSERT(screenNumber == screen->screenNumber);

        XLockDisplay(screenInfo->display);

        XUnmapWindow(screenInfo->display, screen->w);
        XDestroyImage(screen->img);
        XFreeGC(screenInfo->display, screen->gc);
        XDestroyWindow(screenInfo->display, screen->w);

        XUnlockDisplay(screenInfo->display);

        --screenInfo->nbrScreens;

        xfree(screen);
        node->data = NULL;

        OWF_Node_Destroy(node);
    }
}

OWF_API_CALL OWFboolean
OWF_Screen_Resize(OWFint screenNumber, OWFint width, OWFint height)
{
    OWF_SCREEN_X11* screen;
    OWF_SCREEN_INFO* screenInfo;
    OWF_NODE* node;

    OWF_ASSERT(screenNumber != OWF_INVALID_SCREEN_NUMBER);

    screenInfo = (OWF_SCREEN_INFO*)OWF_Screen_GetScreens()->data;
    OWF_ASSERT(screenInfo);

    node = OWF_List_Find(OWF_Screen_GetScreens(), findScreenByNumber, &screenNumber);
    OWF_ASSERT(node && node->data);

    screen = (OWF_SCREEN_X11*)node->data;
    OWF_ASSERT(screen->screenNumber == screenNumber);

    XLockDisplay(screenInfo->display);

    /* destroy old image - this frees pixel data also */
    XDestroyImage(screen->img);

    XUnmapWindow(screenInfo->display, screen->w);

    /* resize window */
    XResizeWindow(screenInfo->display, screen->w, width, height);

    /* create new image */
    screen->width = width;
    screen->height = height;
    screen->img = OWF_Screen_CreateImage(screenInfo, screen);

    XMapWindow(screenInfo->display, screen->w);

    XMoveWindow(screenInfo->display, screen->w,
            screen->x, screen->y);

    XUnlockDisplay(screenInfo->display);

    return OWF_TRUE;
}


OWF_API_CALL OWFboolean
OWF_Screen_Blit(OWFint screenNumber, void* buffer, OWF_ROTATION rotation)
{
     OWF_SCREEN_X11* screen;
     OWF_SCREEN_INFO* screenInfo;
     OWF_NODE* node;
     rotation = rotation;

     DPRINT(("X11: OWF_Screen_Blit (Enter)"));
     OWF_ASSERT(screenNumber != OWF_INVALID_SCREEN_NUMBER);

     screenInfo = (OWF_SCREEN_INFO*)OWF_Screen_GetScreens()->data;
     OWF_ASSERT(screenInfo);
     node = OWF_List_Find(OWF_Screen_GetScreens(), findScreenByNumber, &screenNumber);
     OWF_ASSERT(node && node->data);

     screen = (OWF_SCREEN_X11*)node->data;
     OWF_ASSERT(screen->screenNumber == screenNumber);

     XLockDisplay(screenInfo->display);

     memcpy(screen->img->data, buffer,
             screen->width*screen->height*screen->pixelSize);
     XPutImage(screenInfo->display,
             screen->w, screen->gc, screen->img,
             0, 0, 0, 0, screen->width, screen->height);


     XSync(screenInfo->display, False);

     XUnlockDisplay(screenInfo->display);

     DPRINT(("X11: OWF_Screen_Blit (Exit)"));
	 return OWF_TRUE;
}


OWF_API_CALL OWFboolean 
OWF_Screen_Valid(OWFint screenNumber)
{
    if ((screenNumber != OWF_INVALID_SCREEN_NUMBER) && (screenNumber >= 1))
    {
        return OWF_TRUE;
    }
    else
    {
        return OWF_FALSE;
    }
}


OWF_API_CALL OWFboolean
OWF_Screen_Rotation_Supported(OWFint screenNumber)
{
    screenNumber = screenNumber;
    return OWF_FALSE;
}

#ifdef __cplusplus
}
#endif


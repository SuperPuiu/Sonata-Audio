#ifndef __PAPWINDOW__
#define __PAPWINDOW__

#include <SDL3/SDL.h>
#include <stdint.h>
#include <assert.h>

#include "render.h"

uint32_t Buffer[WINDOW_WIDTH * WINDOW_HEIGHT];

#ifndef __WINDOW_FUNC__
#define __WINDOW_FUNC__

void PAP_PutPixel(int X, int Y, uint32_t PixelData) {
  assert(Y * WINDOW_WIDTH + X <= WINDOW_WIDTH * WINDOW_HEIGHT);
  Buffer[Y * WINDOW_WIDTH + X] = PixelData;
}

uint32_t PAP_GetPixel(int X, int Y) {
  assert(Y * WINDOW_WIDTH + X <= WINDOW_WIDTH * WINDOW_HEIGHT);
  return Buffer[Y * WINDOW_WIDTH + X];
}

#ifndef WINDOWS
/*
 * X11, unlike windows, is much easier to set up for our usage. We just call SDL_CreateWindow, then get the window's Display and Window
 * which then is used for XCreateGC and XCreateImage. This method allows us to throw away the need for SDL_CreateRenderer which 
 * uses a lot of RAM for its rendering.
 */

#include <X11/Xlib.h>

Display *l_Display;
Window l_Window;
GC l_GC;
XImage *l_XImage;

void OpenWindow(void) {
  /* Let SDL carry the creation of the window for us */
  SDL_Window *MainWindow = SDL_CreateWindow("Puius Audio Player", WINDOW_WIDTH, WINDOW_HEIGHT, 0);

  if (!MainWindow)
    SDL_Log("OpenWindow: %s", SDL_GetError());
  
  SDL_SetWindowBordered(MainWindow, 0);

  /* Now populate our variables using the newly created window */
  l_Display = (Display *)SDL_GetPointerProperty(SDL_GetWindowProperties(MainWindow), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
  l_Window = (Window)SDL_GetNumberProperty(SDL_GetWindowProperties(MainWindow), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
  l_GC = XCreateGC(l_Display, l_Window, 0, 0);
  
  XWindowAttributes Attributes = {0};
  XGetWindowAttributes(l_Display, l_Window, &Attributes);
  
  l_XImage = XCreateImage(l_Display, Attributes.visual, Attributes.depth, ZPixmap, 0, (char*)Buffer, WINDOW_WIDTH, WINDOW_HEIGHT, 32, WINDOW_WIDTH * sizeof(uint32_t));
}

void RefreshWindow() {
  XPutImage(l_Display, l_Window, l_GC, l_XImage, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
}

#else
/*
 * Handling the windows part is a bit more tricky than it is with X11. We have to intercept the WM_PAINT event, which normally is not
 * exposed to the end user if we're using SDL. But since we want to have SDL events to work, we have to use SDL_CreateWindow or the
 * other function named SDL_CreateWindowFromProperties. After this, we have to save SDL's WNDPROC and then hook our own WNDPROC just
 * to intercept the required event and then we call SDL's old WNDPROC function. Not to mention that this hacky method invalidates the
 * entire window, from which performance takes a hit. So much only for RAM usage optimization..
 */

#include <windows.h>

HWND ID;
WNDPROC SDL_WNDPROC;

typedef struct {
  BITMAPINFOHEADER Header;
  RGBQUAD Colors[3];
} INFO;

/*
 * Why must windows code look so messy?
 */
LRESULT CALLBACK CustomRedrawWindow(HWND l_HWND, UINT Message, WPARAM l_WPARAM, LPARAM l_LPARAM) {
  if (Message == WM_PAINT) {
    PAINTSTRUCT PaintStruct;
    HDC l_HDC = BeginPaint(l_HWND, &PaintStruct);
    HDC MEMDC = CreateCompatibleDC(l_HDC);
    HBITMAP l_HBITMAP = CreateCompatibleBitmap(l_HDC, WINDOW_WIDTH, WINDOW_HEIGHT);
    HBITMAP OLD_BMP = SelectObject(MEMDC, l_HBITMAP);
    INFO l_INFO = {.Header = {sizeof(l_INFO), WINDOW_WIDTH, -WINDOW_HEIGHT, 1, 32, BI_BITFIELDS}};

    l_INFO.Colors[0].rgbRed = 0xff;
    l_INFO.Colors[1].rgbGreen = 0xff;
    l_INFO.Colors[2].rgbBlue = 0xff;

    SetDIBitsToDevice(MEMDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 0, WINDOW_HEIGHT, Buffer, (BITMAPINFO*)&l_INFO, DIB_RGB_COLORS);
    BitBlt(l_HDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, MEMDC, 0, 0, SRCCOPY);

    SelectObject(MEMDC, OLD_BMP);
    DeleteObject(l_HBITMAP);
    DeleteDC(MEMDC);

    EndPaint(l_HWND, &PaintStruct);
    return 0;
  } else {
    return CallWindowProc(SDL_WNDPROC, l_HWND, Message, l_WPARAM, l_LPARAM);
  }
}

void OpenWindow(void) {
  SDL_Window *Window = SDL_CreateWindow("Puius Audio Player", WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  ID = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(Window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
  
  SDL_SetWindowBordered(Window, 0);

  SDL_WNDPROC = (WNDPROC)SetWindowLongPtr(GetActiveWindow(), GWLP_WNDPROC, (LONG_PTR)&CustomRedrawWindow);

  UpdateWindow(ID);
  ShowWindow(ID, SW_NORMAL);
}

void RefreshWindow() {
  InvalidateRect(ID, NULL, FALSE); /* This will hit performance surely */
}

#endif

void ClearWindow(uint32_t BackgroundColor) {
  memset(Buffer, BackgroundColor, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(*Buffer));
}

#endif /* __WINDOW_FUNC__ */
#endif /* __PAPWINDOW__ */

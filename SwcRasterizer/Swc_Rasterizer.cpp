// Swc_Rasterizer.cpp : This file contains the 'main' function.
// Description: a simple rasterizer just using swapchain (SWC) backbuffer
//

#include "utils.hpp"
#include "Dx12_Wrapper.hpp"

//---------------------------------------------------------------------------//
// Constants
//---------------------------------------------------------------------------//
constexpr uint32_t WHITE =
  (255 << 24) |   // alpha
  (255 << 16) |   // blue
  (255 << 8)  |   // green
  (255 << 0);     // red

constexpr uint32_t BLUE =
  (255 << 24) |   // alpha
  (255 << 16) |   // blue
  (0 << 8)    |   // green
  (0 << 0);       // red


//---------------------------------------------------------------------------//
// Rendering functions
//---------------------------------------------------------------------------//
static void
clearBuffer()
{
  uint8_t* row = (uint8_t*)Dx12Wrapper::ms_BackbufferMemory;

  for (int y = 0; y < Dx12Wrapper::ms_Height; ++y) {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < Dx12Wrapper::ms_Width; ++x)
    {

      // clear to white
      *pixel = WHITE;

      ++pixel;
    }

    // move to next rowPitch:
    row += Dx12Wrapper::ms_Width * Dx12Wrapper::ms_BytePerPixel;
  }
}
//---------------------------------------------------------------------------//
static void
drawCheckerboard()
{
  uint8_t* row = (uint8_t*)Dx12Wrapper::ms_BackbufferMemory;

  for (int y = 0; y < Dx12Wrapper::ms_Height; ++y) {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < Dx12Wrapper::ms_Width; ++x)
    {
      // checkerboard
      if (x % 2 == 0 && y % 2 == 0)
        *pixel = WHITE;
      else
        *pixel = BLUE;

      ++pixel;
    }

    // move to next rowPitch:
    row += Dx12Wrapper::ms_Width * Dx12Wrapper::ms_BytePerPixel;
  }
}
//---------------------------------------------------------------------------//
static void
drawHorizonatalLine(const int p_LineY)
{
  uint8_t* row = (uint8_t*)Dx12Wrapper::ms_BackbufferMemory;

  for (int y = 0; y < Dx12Wrapper::ms_Height; ++y) {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < Dx12Wrapper::ms_Width; ++x)
    {
      if (p_LineY == y)
        *pixel = BLUE;
      else
        *pixel = WHITE;

      ++pixel;
    }

    // move to next rowPitch:
    row += Dx12Wrapper::ms_Width * Dx12Wrapper::ms_BytePerPixel;
  }
}
//---------------------------------------------------------------------------//
static void
drawVerticalLine(const int p_LineX)
{
  uint8_t* row = (uint8_t*)Dx12Wrapper::ms_BackbufferMemory;

  for (int y = 0; y < Dx12Wrapper::ms_Height; ++y) {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < Dx12Wrapper::ms_Width; ++x)
    {
      if (p_LineX == x)
        *pixel = BLUE;
      else
        *pixel = WHITE;

      ++pixel;
    }

    // move to next rowPitch:
    row += Dx12Wrapper::ms_Width * Dx12Wrapper::ms_BytePerPixel;
  }
}
//---------------------------------------------------------------------------//
static void
drawTest()
{
  uint8_t* row = (uint8_t*)Dx12Wrapper::ms_BackbufferMemory;

  for (int y = 0; y < Dx12Wrapper::ms_Height; ++y) {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < Dx12Wrapper::ms_Width; ++x)
    {
      *pixel = BLUE;

      ++pixel;
    }

    // move to next rowPitch:
    row += Dx12Wrapper::ms_Width * Dx12Wrapper::ms_BytePerPixel;
  }
}

//---------------------------------------------------------------------------//
// Message handler
//---------------------------------------------------------------------------//
LRESULT CALLBACK
windowProc(HWND p_Wnd, UINT p_Message, WPARAM p_WParam, LPARAM p_LParam)
{
  switch (p_Message)
  {
  case WM_KEYDOWN:
    // TODO
    return 0;

  case WM_KEYUP:
  {
    uint32_t virtualKeyCode = static_cast<uint32_t>(p_WParam);

#define KeyMessageWasDownBit (1 << 30)
#define KeyMessageIsDownBit (1 << 31)
    bool wasDown = ((p_LParam & KeyMessageWasDownBit) != 0);
    bool isDown = ((p_LParam & KeyMessageIsDownBit) == 0);

    if (wasDown != isDown)
    {
      if ('W' == virtualKeyCode)
      {
        clearBuffer();
      }
      else if ('H' == virtualKeyCode)
      {
        static uint8_t y = 0;
        y++;
        drawHorizonatalLine(y);
      }
      else if ('V' == virtualKeyCode)
      {
        static uint8_t x = 0;
        x++;
        drawVerticalLine(x);
      }
      else if ('C' == virtualKeyCode)
      {
        drawCheckerboard();
      }
      else if ('T' == virtualKeyCode)
      {
        drawTest();
      }
    }
  }
    return 0;

  case WM_PAINT:
    Dx12Wrapper::onRender();
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  // Handle any messages the switch statement didn't.
  return DefWindowProc(p_Wnd, p_Message, p_WParam, p_LParam);
}
//---------------------------------------------------------------------------//
_Use_decl_annotations_
int WINAPI 
WinMain(HINSTANCE p_Instance, HINSTANCE, LPSTR, int p_CmdShow)
{
  // Initialize the window class.
  WNDCLASSEX windowClass = { 0 };
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = windowProc;
  windowClass.hInstance = p_Instance;
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.lpszClassName = L"DXSampleClass";
  RegisterClassEx(&windowClass);

  int windowWidth = 512;
  int windowHeight = 512;

  RECT windowRect = { 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
  AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

  // Create the window and store a handle to it.
  g_Window = CreateWindow(
    windowClass.lpszClassName,
    L"SWC Software Rasterizer", WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    windowRect.right - windowRect.left,
    windowRect.bottom - windowRect.top,
    nullptr,        // We have no parent window.
    nullptr,        // We aren't using menus.
    p_Instance,
    0);

  Dx12Wrapper::onInit(windowWidth, windowHeight);

  ShowWindow(g_Window, p_CmdShow);

  // Main sample loop.
  MSG msg = {};
  while (msg.message != WM_QUIT)
  {
    // Process any messages in the queue.
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }


  return(0);
}
//---------------------------------------------------------------------------//

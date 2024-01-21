// Win32_Rasterizer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <stdlib.h> // rand(), srand()

#include <time.h>
#include <assert.h>

//---------------------------------------------------------------------------//
#define WIDTH 720
#define HEIGHT 1280

//---------------------------------------------------------------------------//
struct WindowDimension
{
  int width;
  int height;
};
//---------------------------------------------------------------------------//
struct BackBuffer
{
  int width;
  int height;
  int bytesPerPixel;
  void* memory;
  BITMAPINFO bitmapInfo;
  int pitch;
};
//---------------------------------------------------------------------------//
static bool g_Running;
static BackBuffer g_BackBuffer;
static HDC g_DeviceContext;

//---------------------------------------------------------------------------//
static WindowDimension
getWindowDimension (HWND p_WindowHandle)
{
  WindowDimension dimension;
  RECT rect;
  GetClientRect(p_WindowHandle, &rect);
  dimension.width = rect.right - rect.left;
  dimension.height = rect.bottom - rect.top;

  return dimension;
}
//---------------------------------------------------------------------------//
static void
copyBufforToWindow (HWND const* p_WindowHandle)
{
  WindowDimension dimension = getWindowDimension(*p_WindowHandle);
  StretchDIBits(g_DeviceContext,
    0, 0, dimension.width, dimension.height,
    0, 0, g_BackBuffer.width, g_BackBuffer.height,
    g_BackBuffer.memory, &g_BackBuffer.bitmapInfo,
    DIB_RGB_COLORS, SRCCOPY);
}
//---------------------------------------------------------------------------//
static void
clearBuffer (HWND p_WindowHandle)
{
  uint8_t* row = (uint8_t*)g_BackBuffer.memory;
  for (int y = 0; y < g_BackBuffer.height; ++y)
  {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < g_BackBuffer.width; ++x)
    {
      char unsigned blue = 255;
      char unsigned green = 255;
      char unsigned red = 255;
      *pixel++ = ((red << 16) | (green << 8) | blue);
    }
    row += g_BackBuffer.pitch;
  }

  copyBufforToWindow(&p_WindowHandle);
}
//---------------------------------------------------------------------------//
static void
renderLine (
  int p_X0, int p_Y0,
  int p_X1, int p_Y1,
  HWND p_WindowHandle
)
{
  int slope = (p_X1 == p_X0) ? 0 : (p_Y1 - p_Y0) / (p_X1 - p_X0);
  int intercept = p_Y0 - slope * p_X0;

  uint8_t* row = (uint8_t*)g_BackBuffer.memory;
  uint8_t blue = rand() % 255;
  uint8_t green = rand() % 255;
  uint8_t red = rand() % 255;

  // TODO(OM): Optimize buffer traverse
  for (int y = 0; y < g_BackBuffer.height; ++y)
  {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < g_BackBuffer.width; ++x)
    {
      if (y == (slope * x + intercept))
      {
        *pixel = ((red << 16) | (green << 8) | blue);
      }
      ++pixel;
    }
    row += g_BackBuffer.pitch;
  }

  copyBufforToWindow(&p_WindowHandle);
}
//---------------------------------------------------------------------------//
static void
resizeDIBSection (int p_Width, int p_Height)
{
  if (g_BackBuffer.memory)
  {
    VirtualFree(g_BackBuffer.memory, 0, MEM_RELEASE);
  }

  g_BackBuffer.width = p_Width;
  g_BackBuffer.height = p_Height;
  g_BackBuffer.bytesPerPixel = 4;

  g_BackBuffer.bitmapInfo.bmiHeader.biSize = sizeof(g_BackBuffer.bitmapInfo.bmiHeader);
  g_BackBuffer.bitmapInfo.bmiHeader.biWidth = g_BackBuffer.width;
  g_BackBuffer.bitmapInfo.bmiHeader.biHeight = -g_BackBuffer.height; // to have top-down bitmap (not bottom-up)
  g_BackBuffer.bitmapInfo.bmiHeader.biPlanes = 1;
  g_BackBuffer.bitmapInfo.bmiHeader.biBitCount = 32;
  g_BackBuffer.bitmapInfo.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = g_BackBuffer.width * g_BackBuffer.height * g_BackBuffer.bytesPerPixel;
  g_BackBuffer.memory = VirtualAlloc(0, BitmapMemorySize,
    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

  g_BackBuffer.pitch = p_Width * g_BackBuffer.bytesPerPixel;
}
//---------------------------------------------------------------------------//
LRESULT CALLBACK WindowProc(
  HWND   p_WindowHandle,
  UINT   p_Message,
  WPARAM p_WParam,
  LPARAM p_LParam
)
{
  LRESULT result = 0;

  switch (p_Message)
  {
  case WM_ACTIVATE:
  {
    OutputDebugStringA("Activated\n");
  }break;
  case WM_DESTROY:
  {
    g_Running = false;
  }break;
  case WM_SIZE:
  {

  }break;
  case WM_CLOSE:
  {
    g_Running = false;
  }break;
  case WM_SYSKEYDOWN:
    [[fallthrough]];
  case WM_SYSKEYUP:
    [[fallthrough]];
  case WM_KEYDOWN:
    [[fallthrough]];
  case WM_KEYUP:
  {
    uint32_t virtualKeyCode = static_cast<uint32_t>(p_WParam);

#define KeyMessageWasDownBit (1 << 30)
#define KeyMessageIsDownBit (1 << 31)
    bool wasDown = ((p_LParam & KeyMessageWasDownBit) != 0);
    bool isDown = ((p_LParam & KeyMessageIsDownBit) == 0);

    if (wasDown != isDown)
    {
      if (virtualKeyCode == 'W')
      {
        clearBuffer(p_WindowHandle);
      }
      else if (virtualKeyCode == 'L')
      {
        int x0 = rand() % WIDTH;
        int y0 = rand() % HEIGHT;
        int x1 = rand() % WIDTH;
        int y1 = rand() % HEIGHT;
        renderLine(x0, y0, x1, y1,
          p_WindowHandle);
      }
    }

  }break;
  case WM_PAINT:
  {
    PAINTSTRUCT paint;
    HDC deviceContext = BeginPaint(p_WindowHandle, &paint);
    UNREFERENCED_PARAMETER(deviceContext);

    copyBufforToWindow(&p_WindowHandle);

    EndPaint(p_WindowHandle, &paint);
  }break;
  default:
  {
    result = DefWindowProc(p_WindowHandle, p_Message, p_WParam, p_LParam);
  }break;
  }
  return result;
}
//---------------------------------------------------------------------------//
INT __stdcall
WinMain (
  _In_ HINSTANCE p_Instance,
  _In_opt_ HINSTANCE p_PrevInstance,
  _In_ PSTR p_CmdLine,
  _In_ INT p_CmdShow
)
{
  UNREFERENCED_PARAMETER(p_PrevInstance);
  UNREFERENCED_PARAMETER(p_CmdLine);
  UNREFERENCED_PARAMETER(p_CmdShow);

  WNDCLASS windowClass = {};
  windowClass.lpfnWndProc = WindowProc;
  windowClass.hInstance = p_Instance;
  windowClass.lpszClassName = L"My Window Class";

  // Initial create global buffer
  resizeDIBSection(1280, 720);

  // Seed the rng
  srand(static_cast<unsigned>(time(0)));

  if (!RegisterClass(&windowClass))
  {
    assert(false);
    return -1;
  }

  HWND windowHandle = CreateWindowEx(0, windowClass.lpszClassName,
    L"Cpu Rasterizer", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    0, 0, p_Instance, 0);

  if (windowHandle)
  {
    g_Running = true;
    MSG message;
    g_DeviceContext = GetDC(windowHandle);

    // Initial clear
    clearBuffer(windowHandle);

    while (g_Running)
    {

      while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
      {
        if (message.message == WM_QUIT)
        {
          g_Running = false;
        }

        TranslateMessage(&message);
        DispatchMessageA(&message);
      }
    }
  }

  return 0;
}
//---------------------------------------------------------------------------//

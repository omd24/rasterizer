// Win32_Rasterizer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#include <stdlib.h> // rand(), srand()
#include <assert.h>

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
// <ccomplex>, <cstdalign>, <cstdbool>, and <ctgmath> are deprecated.
#include <tinyrenderer/tgaimage.h>
#include <tinyrenderer/model.h>

// Enable to flip y for wireframe model:
// https://github.com/ssloy/tinyrenderer/wiki/Lesson-1:-Bresenham%E2%80%99s-Line-Drawing-Algorithm
#define ENABLE_FLIP_VERTICALLY 1

//---------------------------------------------------------------------------//
template <typename T> static T AlignUp(T val, T alignment)
{
  return (val + alignment - 1) & ~(alignment - 1);
}
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
// Color value [0, 1]
struct ColorRGBA
{
  float r;
  float g;
  float b;
  float a;
};
static constexpr ColorRGBA WHITE = { 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr ColorRGBA BLACK = { 0.0f, 0.0f, 0.0f, 1.0f };
static constexpr ColorRGBA RED = { 1.0f, 0.0f, 0.0f, 1.0f };
static constexpr ColorRGBA BLUE = { 0.0f, 0.0f, 1.0f, 1.0f };
//---------------------------------------------------------------------------//

static bool g_Running;
static BackBuffer g_BackBuffer;
static HDC g_DeviceContext;
static Model* g_Model = nullptr;


//---------------------------------------------------------------------------//
uint32_t
roundFloatToUInt32(float p_Value)
{
  uint32_t result = (uint32_t)roundf(p_Value);
  return(result);
}
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
      char unsigned alpha = 255;
      *pixel++ = ((alpha << 24) | (red << 16) | (green << 8) | blue);
    }
    row += g_BackBuffer.pitch;
  }

  copyBufforToWindow(&p_WindowHandle);
}
//---------------------------------------------------------------------------//
static void
renderLine(
  int p_X0, int p_Y0,
  int p_X1, int p_Y1,
  HWND p_WindowHandle,
  ColorRGBA p_Color
)
{
  // extract color
  float blue = p_Color.b;
  float green = p_Color.g;
  float red = p_Color.r;
  float alpha = p_Color.a;
  
  uint32_t Color32 =
    ((roundFloatToUInt32(alpha * 255.0f) << 24) |
      (roundFloatToUInt32(red * 255.0f) << 16) |
      (roundFloatToUInt32(green * 255.0f) << 8) |
      (roundFloatToUInt32(blue * 255.0f) << 0));


  // move from top to down
  if (p_Y0 > p_Y1) {
    std::swap(p_X0, p_X1);
    std::swap(p_Y0, p_Y1);
  }

  int minX = std::min<int>(p_X0, p_X1);
  int maxX = std::max<int>(p_X0, p_X1);
  int minY = std::min<int>(p_Y0, p_Y1);
  int maxY = std::max<int>(p_Y0, p_Y1);

  // clamp values
  if (p_X0 < 0)
  {
    p_X0 = 0;
  }

  if (p_Y0 < 0)
  {
    p_Y0 = 0;
  }

  if (p_X1 > g_BackBuffer.width)
  {
    p_X1 = g_BackBuffer.width;
  }

  if (p_Y1 > g_BackBuffer.height)
  {
    p_Y1 = g_BackBuffer.height;
  }

  if (minX < 0)
  {
    minX = 0;
  }

  if (minY < 0)
  {
    minY = 0;
  }

  if (maxX > g_BackBuffer.width)
  {
    maxX = g_BackBuffer.width;
  }

  if (maxY > g_BackBuffer.height)
  {
    maxY = g_BackBuffer.height;
  }

  // find line equation
  float deltaX = float(p_X1) - float(p_X0);
  float deltaY = float(p_Y1) - float(p_Y0);
  float slope = (p_X1 == p_X0) ? 0 : deltaY / deltaX;
  float intercept = p_Y0 - slope * p_X0;

  uint8_t* row = ((uint8_t*)g_BackBuffer.memory +
    p_X0 * g_BackBuffer.bytesPerPixel +
    p_Y0 * g_BackBuffer.pitch);

  // TODO: Make the error margin reasonable (slope dependent)
  const float margin = 0.1f;

  for (int y = minY; y < maxY; ++y) {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = minX; x < maxX; ++x)
    {
      if (std::abs(float(y) - (slope * x + intercept)) < margin)
      {
        *pixel = Color32;
      }
      ++pixel;
    }

    row += g_BackBuffer.pitch;
  }

  copyBufforToWindow(&p_WindowHandle);
}
static void
renderLineExperimental(
  int p_X0, int p_Y0,
  int p_X1, int p_Y1,
  HWND p_WindowHandle,
  ColorRGBA p_Color
)
{
  // extract color
  float blue = p_Color.b;
  float green = p_Color.g;
  float red = p_Color.r;
  float alpha = p_Color.a;

  uint32_t Color32 = 0;
    ((roundFloatToUInt32(alpha * 255.0f) << 24) |
      (roundFloatToUInt32(red * 255.0f) << 16) |
      (roundFloatToUInt32(green * 255.0f) << 8) |
      (roundFloatToUInt32(blue * 255.0f) << 0));

  bool steep = false;
  if (std::abs(p_X0 - p_X1) < std::abs(p_Y0 - p_Y1)) {
    std::swap(p_X0, p_Y0);
    std::swap(p_X1, p_Y1);
    steep = true;
  }
  if (p_X0 > p_X1) {
    std::swap(p_X0, p_X1);
    std::swap(p_Y0, p_Y1);
  }

  uint8_t* buffer = (uint8_t*)g_BackBuffer.memory;
  for (int x = p_X0; x <= p_X1; x++) {
    float t = (x - p_X0) / (float)(p_X1 - p_X0);
    int y = p_Y0 * (1. - t) + p_Y1 * t;
    if (steep) {
      buffer[(x * g_BackBuffer.height + y) * g_BackBuffer.bytesPerPixel] = Color32;
    }
    else {
      buffer[(y * g_BackBuffer.width + x )* g_BackBuffer.bytesPerPixel] = Color32;
    }
  }
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

#if (ENABLE_FLIP_VERTICALLY > 0) // bottom up
  g_BackBuffer.bitmapInfo.bmiHeader.biHeight = g_BackBuffer.height;
#else
  g_BackBuffer.bitmapInfo.bmiHeader.biHeight = -g_BackBuffer.height; // to have top-down bitmap.
#endif
  g_BackBuffer.bitmapInfo.bmiHeader.biPlanes = 1;
  g_BackBuffer.bitmapInfo.bmiHeader.biBitCount = 32;
  g_BackBuffer.bitmapInfo.bmiHeader.biCompression = BI_RGB;

  g_BackBuffer.pitch = AlignUp(p_Width * g_BackBuffer.bytesPerPixel, 4);

  int BitmapMemorySize = g_BackBuffer.pitch * g_BackBuffer.height;
  g_BackBuffer.memory = VirtualAlloc(0, BitmapMemorySize,
    MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

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
#if (ENABLE_FLIP_VERTICALLY > 0)
        // the line function is top down so early out.
        break;
#endif

        // Draw some test Lines

        renderLine(0, 0, 400, 400, p_WindowHandle, BLUE);
        renderLine(0, 200, 400, 600, p_WindowHandle, RED);
        renderLine(250, 250, 500, 100, p_WindowHandle, BLACK);

        renderLine(800, 400, 130, 200, p_WindowHandle, RED);
        renderLine(0, 0, g_BackBuffer.width, g_BackBuffer.height, p_WindowHandle, RED);
        renderLine(0, g_BackBuffer.height, g_BackBuffer.width, 0, p_WindowHandle, BLUE);
      }
      else if (virtualKeyCode == 'M')
      {

        // Draw the loaded Model
        // NOTE(OM): The y-coordinate is upside down ^^
        assert(g_Model->initialized);
        for (int i = 0; i < g_Model->nfaces(); i++) {
          std::vector<int> face = g_Model->face(i);
          for (int j = 0; j < 3; j++) {
            Vec3f v0 = g_Model->vert(face[j]);
            Vec3f v1 = g_Model->vert(face[(j + 1) % 3]);
            int x0 = (v0.x + 1.) * g_BackBuffer.width / 2.;
            int y0 = (v0.y + 1.) * g_BackBuffer.height / 2.;
            int x1 = (v1.x + 1.) * g_BackBuffer.width / 2.;
            int y1 = (v1.y + 1.) * g_BackBuffer.height / 2.;
            renderLine(x0, y0, x1, y1, p_WindowHandle, BLACK);
            //renderLineExperimental(x0, y0, x1, y1, p_WindowHandle, BLACK);
          }
        }
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

  int windowWidth = 700;
  int windowHeight = 700;
  windowWidth = AlignUp(windowWidth, 256);
  windowHeight = AlignUp(windowHeight, 256);

  assert(RegisterClass(&windowClass));

  HWND windowHandle = CreateWindowEx(0, windowClass.lpszClassName,
    L"Cpu Rasterizer", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
    CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight,
    0, 0, p_Instance, 0);
  assert(windowHandle);
  g_DeviceContext = GetDC(windowHandle);

  // Load model
  g_Model = new Model("../../Assets/obj/african_head/african_head.obj");
  assert(g_Model->initialized);

  // Initialize and clear global buffer:
  WindowDimension dimension = getWindowDimension(windowHandle);
  resizeDIBSection(dimension.width, dimension.height);
  clearBuffer(windowHandle);

  SetFocus(windowHandle);
  SetForegroundWindow(windowHandle);

  // Main loop:
  MSG message;
  g_Running = true;
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

  return 0;
}
//---------------------------------------------------------------------------//

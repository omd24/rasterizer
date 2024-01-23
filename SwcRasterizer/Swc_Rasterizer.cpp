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

constexpr uint32_t RED =
  (255 << 24) |   // alpha
  (0 << 16)   |   // blue
  (0 << 8)    |   // green
  (255 << 0);       // red

//---------------------------------------------------------------------------//
// Helper structs and functions
//---------------------------------------------------------------------------//

template <typename T> struct Vector2
{
  union {
    struct { T u, v; };
    struct { T x, y; };
    T raw[2];
  };

  Vector2() : u(0), v(0) {}
  Vector2(T p_X, T p_Y) : x(p_X), y(p_Y) {}
  inline Vector2<T> operator +(const Vector2<T>& p_V) const { return Vector2<T>(x + p_V.x, y + p_V.y); }
  inline Vector2<T> operator -(const Vector2<T>& p_V) const { return Vector2<T>(x - p_V.x, y - p_V.y); }
  inline Vector2<T> operator *(float p_Scalar)     const 
  { 
    return Vector2<T>(static_cast<T>(x * p_Scalar), static_cast<T>(y * p_Scalar)); 
  }
};
typedef Vector2<float> Vec2F;
typedef Vector2<int>   Vec2I;

//---------------------------------------------------------------------------//
template <typename T> struct Vector3 {
  union {
    struct { T x, y, z; };
    T raw[3];
  };
  Vector3() : x(0), y(0), z(0) {}
  Vector3(T p_X, T p_Y, T p_Z) : x(p_X), y(p_Y), z(p_Z) {}
  inline Vector3<T> operator +(const Vector3<T>& p_Vec) const 
  { 
    return Vector3<T>(x + p_Vec.x, y + p_Vec.y, z + p_Vec.z); 
  }
  inline Vector3<T> operator -(const Vector3<T>& p_Vec) const 
  { 
    return Vector3<T>(x - p_Vec.x, y - p_Vec.y, z - p_Vec.z); 
  }
  inline Vector3<T> operator *(float p_Val) const 
  { 
    return Vector3<T>(x * p_Val, y * p_Val, z * p_Val); 
  }
  inline T operator *(const Vector3<T>& p_Vec) const 
  { 
    return x * p_Vec.x + y * p_Vec.y + z * p_Vec.z; 
  }
  float length() const { return std::sqrt(x * x + y * y + z * z); }
  Vector3<T>& normalize() { *this = (*this) * (1 / length()); return *this; }

  static Vector3<T> cross (const Vector3<T>& p_Vec0, const Vector3<T>& p_Vec1)
  {
    return Vector3<T>(
      p_Vec0.y * p_Vec1.z - p_Vec0.z * p_Vec1.y,
      p_Vec0.z * p_Vec1.x - p_Vec0.x * p_Vec1.z,
      p_Vec0.x * p_Vec1.y - p_Vec0.y * p_Vec1.x
    );
  }

  static Vector3<T> dot (const Vector3<T>& p_Vec0, const Vector3<T>& p_Vec1)
  {
    return p_Vec0.x * p_Vec1.x + p_Vec0.y * p_Vec1.y + p_Vec0.z * p_Vec1.z;
  }
};
typedef Vector3<float> Vec3F;
typedef Vector3<int>   Vec3I;

//---------------------------------------------------------------------------//

static int
roundFloatToUInt(float p_Value)
{
  int result = (int)roundf(p_Value);
  return(result);
}

//---------------------------------------------------------------------------//
// Rendering functions
//---------------------------------------------------------------------------//
static void
colorPixel (int p_X, int p_Y, uint32_t p_Color)
{
  if (g_FlipVertically)
    p_Y = Dx12Wrapper::ms_Height - p_Y;

  if (p_X < 0 || p_X >= Dx12Wrapper::ms_Width 
    || p_Y < 0 || p_Y >= Dx12Wrapper::ms_Height) {
    return;
  }

  uint32_t* buffer = (uint32_t*)Dx12Wrapper::ms_BackbufferMemory;
  buffer[p_Y * Dx12Wrapper::ms_Width + p_X] = p_Color;
}
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
drawHorizonatalLine(const int p_LineY)
{
  for (int y = 0; y < Dx12Wrapper::ms_Height; ++y) {
    for (int x = 0; x < Dx12Wrapper::ms_Width; ++x)
    {
      if (p_LineY == y)
        colorPixel(x, y, BLUE);
      else
        colorPixel(x, y, WHITE);
    }
  }
}
//---------------------------------------------------------------------------//
static void
drawVerticalLine(const int p_LineX)
{
  for (int y = 0; y < Dx12Wrapper::ms_Height; ++y) {
    for (int x = 0; x < Dx12Wrapper::ms_Width; ++x)
    {
      if (p_LineX == x)
        colorPixel(x, y, BLUE);
      else
        colorPixel(x, y, WHITE);
    }
  }
}
//---------------------------------------------------------------------------//
static void
drawLineSimple(int p_X0, int p_Y0, int p_X1, int p_Y1, uint32_t p_Color)
{
  // if the line is steep, we transpose the image
  bool steep = false;
  if (std::abs(p_X0 - p_X1) < std::abs(p_Y0 - p_Y1))
  {
    std::swap(p_X0, p_Y0);
    std::swap(p_X1, p_Y1);
    steep = true;
  }
  // make it left−to−right
  if (p_X0 > p_X1)
  {
    std::swap(p_X0, p_X1);
    std::swap(p_Y0, p_Y1);
  }
  for (int x = p_X0; x <= p_X1; x++) {

    // interpolate y based on x ratio:
    float t = (x - p_X0) / (float)(p_X1 - p_X0);
    float yLerped = p_Y0 * (1.0f - t) + p_Y1 * t;
    int y = roundFloatToUInt(yLerped);

    if (steep) {
      colorPixel(y, x, p_Color);
    }
    else {
      colorPixel(x, y, p_Color);
    }
  }
}
//---------------------------------------------------------------------------//
// https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
Vec3F barycentric(Vec2I p_TriangleVertices[3], Vec2I p_Point) {
  Vec3F vec0 = Vec3F(
    (float)p_TriangleVertices[2].x - p_TriangleVertices[0].x,
    (float)p_TriangleVertices[1].x - p_TriangleVertices[0].x,
    (float)p_TriangleVertices[0].x - p_Point.x);
    
  Vec3F vec1 = Vec3F(
    (float)p_TriangleVertices[2].y - p_TriangleVertices[0].y, 
    (float)p_TriangleVertices[1].y - p_TriangleVertices[0].y, 
    (float)p_TriangleVertices[0].y - p_Point.y);

  Vec3F u = Vec3F::cross(vec0, vec1);

  /* `p_TriangleVertices` and `p_Point` has integer value as coordinates
     so `abs(u[2])` < 1 means `u[2]` is 0, that means
     triangle is degenerate, in this case return something with negative coordinates */
  if (std::abs(u.z) < 1) return Vec3F(-1, 1, 1);
  return Vec3F(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

void drawTriangle(Vec2I p_TriangleVertices[3], uint32_t p_Color)
{
  int width = Dx12Wrapper::ms_Width;
  int height = Dx12Wrapper::ms_Height;
  Vec2I bboxmin(width - 1, height - 1);
  Vec2I bboxmax(0, 0);
  Vec2I clamp(width - 1, height - 1);
  for (int i = 0; i < 3; i++) {
    bboxmin.x = std::max(0, std::min(bboxmin.x, p_TriangleVertices[i].x));
    bboxmin.y = std::max(0, std::min(bboxmin.y, p_TriangleVertices[i].y));

    bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, p_TriangleVertices[i].x));
    bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, p_TriangleVertices[i].y));
  }
  Vec2I pixel;
  for (pixel.x = bboxmin.x; pixel.x <= bboxmax.x; pixel.x++) {
    for (pixel.y = bboxmin.y; pixel.y <= bboxmax.y; pixel.y++) {
      Vec3F bc = barycentric(p_TriangleVertices, pixel);
      if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;
      colorPixel(pixel.x, pixel.y, p_Color);
    }
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
        // Render wireframe model:
        g_FlipVertically = true;

        for (int i = 0; i < g_Model->nfaces(); i++) {
          std::vector<int> face = g_Model->face(i);
          for (int j = 0; j < 3; j++) {
            Vec3f v0 = g_Model->vert(face[j]);
            Vec3f v1 = g_Model->vert(face[(j + 1) % 3]);
            int x0 = static_cast<int>((v0.x + 1.0f) * Dx12Wrapper::ms_Width / 2);
            int y0 = static_cast<int>((v0.y + 1.0f) * Dx12Wrapper::ms_Height / 2);
            int x1 = static_cast<int>((v1.x + 1.0f) * Dx12Wrapper::ms_Width / 2);
            int y1 = static_cast<int>((v1.y + 1.0f) * Dx12Wrapper::ms_Height / 2);
            drawLineSimple(x0, y0, x1, y1, WHITE);
          }
        }

        g_FlipVertically = false;
      }
      else if ('H' == virtualKeyCode)
      {
        static uint8_t y = 0;
        y += 20;
        drawHorizonatalLine(y);
      }
      else if ('V' == virtualKeyCode)
      {
        static uint8_t x = 0;
        x += 20;
        drawVerticalLine(x);
      }
      else if ('C' == virtualKeyCode)
      {
        clearBuffer();
      }
      else if ('T' == virtualKeyCode)
      {
        Vec2I t0[3] = { Vec2I(10, 70),   Vec2I(50, 160),  Vec2I(70, 80) };
        Vec2I t1[3] = { Vec2I(180, 50),  Vec2I(150, 1),   Vec2I(70, 180) };
        Vec2I t2[3] = { Vec2I(180, 150), Vec2I(120, 160), Vec2I(130, 180) };
        drawTriangle(t0, RED);
        drawTriangle(t1, WHITE);
        drawTriangle(t2, BLUE);
      }
      else if ('L' == virtualKeyCode)
      {
        drawLineSimple(50, 50, 100, 100, RED);
        drawLineSimple(50, 60, 100, 40, BLUE);
        drawLineSimple(50, 400, 100, 100, BLUE);

        drawLineSimple(13, 20, 80, 40, WHITE);
        drawLineSimple(20, 13, 40, 80, RED);
        drawLineSimple(80, 40, 13, 20, RED);
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
// Main function
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

  // TODO(OM): Check if it works for different window sizes (considering d3d alignment rules):
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

  // Load wireframe model:
  g_Model = new Model("../Assets/obj/african_head/african_head.obj");
  assert(g_Model->initialized);
  g_FlipVertically = false;

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

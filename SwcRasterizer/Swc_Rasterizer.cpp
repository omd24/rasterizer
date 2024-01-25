// Swc_Rasterizer.cpp : This file contains the 'main' function.
// Description: a simple rasterizer just using swapchain (SWC) backbuffer
//

#include "utils.hpp"
#include "Dx12_Wrapper.hpp"


//---------------------------------------------------------------------------//
// Helper functions
//---------------------------------------------------------------------------//
static float
rndf()
{
  return (float)rand() / (float)RAND_MAX;
}
//---------------------------------------------------------------------------//
static int
roundFloatToUInt(float p_Value)
{
  int result = (int)roundf(p_Value);
  return(result);
}

//---------------------------------------------------------------------------//
// Constants
//---------------------------------------------------------------------------//

// Uint32 colors can be used directly
constexpr uint32_t WHITE =
  (255 << 24) |   // alpha
  (255 << 16) |   // blue
  (255 << 8)  |   // green
  (255 << 0);     // red

constexpr uint32_t BLACK =
  (255 << 24) |   // alpha
  (0 << 16)   |   // blue
  (0 << 8)    |   // green
  (0 << 0);       // red

constexpr uint32_t BLUE =
  (255 << 24) |   // alpha
  (255 << 16) |   // blue
  (0 << 8)    |   // green
  (0 << 0);       // red

constexpr uint32_t RED =
  (255 << 24) |   // alpha
  (0 << 16)   |   // blue
  (0 << 8)    |   // green
  (255 << 0);     // red

//---------------------------------------------------------------------------//
// Float color values should be converted to Uint32 before using
namespace Colors
{
struct ColorRGBA
{
  float r;
  float g;
  float b;
  float a;

  uint32_t convertToUint32 () const
  {
    uint32_t color32 =
        ((roundFloatToUInt(a * 255.0f) << 24) |
        (roundFloatToUInt(b * 255.0f) << 16) |
        (roundFloatToUInt(g * 255.0f) << 8) |
        (roundFloatToUInt(r * 255.0f) << 0));

    return color32;
  }

  ColorRGBA operator *(float p_Scalar)     const
  {
    return { r * p_Scalar, g * p_Scalar, b * p_Scalar, a};
  }
};
static constexpr ColorRGBA White = { 1.0f, 1.0f, 1.0f, 1.0f };
static constexpr ColorRGBA Black = { 0.0f, 0.0f, 0.0f, 1.0f };
static constexpr ColorRGBA Red = { 1.0f, 0.0f, 0.0f, 1.0f };
static constexpr ColorRGBA Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
}

//---------------------------------------------------------------------------//
// Helper classes
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
    T raw[3]{};
  };
  Vector3() : x(0), y(0), z(0), raw{ 0, 0, 0 } {}
  Vector3(T p_Raw[3]) : x(p_Raw[0]), y(p_Raw[1]), z(p_Raw[2]), raw{ p_Raw[0], p_Raw[1], p_Raw[2] } {}

  // NOTE(OM): constexpr ctor is for allowing constant initialization. 
  // it does not mean all instances will be literal / constant expressions:
  // https://en.cppreference.com/w/cpp/language/constexpr
  constexpr Vector3(T p_X, T p_Y, T p_Z) : x(p_X), y(p_Y), z(p_Z), raw{ p_X, p_Y, p_Z } {}
  
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
  constexpr float length() const { return std::sqrt(x * x + y * y + z * z); }
  Vector3<T>& normalize() { *this = (*this) * (1 / length()); return *this; }

  // constexpr version of normalize
  template <typename T>
  constexpr Vector3<T> normalized() const {
    float len = length();
    return Vector3<T>(x / len, y / len, z / len);
  }

  static Vector3<T> cross (const Vector3<T>& p_Vec0, const Vector3<T>& p_Vec1)
  {
    return Vector3<T>(
      p_Vec0.y * p_Vec1.z - p_Vec0.z * p_Vec1.y,
      p_Vec0.z * p_Vec1.x - p_Vec0.x * p_Vec1.z,
      p_Vec0.x * p_Vec1.y - p_Vec0.y * p_Vec1.x
    );
  }

  static float dot (const Vector3<T>& p_Vec0, const Vector3<T>& p_Vec1)
  {
    return p_Vec0.x * p_Vec1.x + p_Vec0.y * p_Vec1.y + p_Vec0.z * p_Vec1.z;
  }
};
typedef Vector3<float> Vec3F;
typedef Vector3<int>   Vec3I;

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
clearBuffer(uint32_t p_Color)
{
  uint8_t* row = (uint8_t*)Dx12Wrapper::ms_BackbufferMemory;

  for (int y = 0; y < Dx12Wrapper::ms_Height; ++y) {
    uint32_t* pixel = (uint32_t*)row;
    for (int x = 0; x < Dx12Wrapper::ms_Width; ++x)
    {
      *pixel = p_Color;
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
static Vec3F 
barycentric(Vec3F p_TriangleVertices[3], Vec3F p_Point) {
  Vec3F vec0 = Vec3F(
    (float)p_TriangleVertices[2].x - p_TriangleVertices[0].x,
    (float)p_TriangleVertices[1].x - p_TriangleVertices[0].x,
    (float)p_TriangleVertices[0].x - p_Point.x);
    
  Vec3F vec1 = Vec3F(
    (float)p_TriangleVertices[2].y - p_TriangleVertices[0].y, 
    (float)p_TriangleVertices[1].y - p_TriangleVertices[0].y, 
    (float)p_TriangleVertices[0].y - p_Point.y);

  Vec3F u = Vec3F::cross(vec0, vec1);

  /* 
    `p_TriangleVertices` and `p_Point` has integer value as coordinates
    so `abs(u[2])` < 1 means `u[2]` is 0, that means
    triangle is degenerate, in this case return something with negative coordinates
  */
  if (std::abs(u.z) < 1) return Vec3F(-1, 1, 1);

  // Again dont forget that u[2] is integer.
  // If it is zero then triangle ABC is degenerate
  if (std::abs(u.z) > 1e-2) // 
    return Vec3F(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);

  // return smth with negative by default
  return Vec3F(-1, 1, 1);
}
//---------------------------------------------------------------------------//
static void
drawTriangle(Vec3F p_TriangleVertices[3], uint32_t p_Color, bool p_DepthTest)
{
  int width = Dx12Wrapper::ms_Width;
  int height = Dx12Wrapper::ms_Height;
  Vec2F bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
  Vec2F bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
  Vec2F clamp(float(width - 1), float(height - 1));
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      bboxmin.raw[j] = std::max(0.f, std::min(bboxmin.raw[j], p_TriangleVertices[i].raw[j]));
      bboxmax.raw[j] = std::min(clamp.raw[j], std::max(bboxmax.raw[j], p_TriangleVertices[i].raw[j]));
    }
  }
  Vec3F pixel;
  for (pixel.x = bboxmin.x; pixel.x <= bboxmax.x; pixel.x++)
  {
    for (pixel.y = bboxmin.y; pixel.y <= bboxmax.y; pixel.y++)
    {
      Vec3F bc = barycentric(p_TriangleVertices, pixel);
      if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;
      
      if (p_DepthTest)
      {
        pixel.z = 0;
        for (int i = 0; i < 3; i++) pixel.z += p_TriangleVertices[i].z * bc.raw[i];
        if (g_DepthBuffer[int(pixel.x + pixel.y * width)] < pixel.z) {
          g_DepthBuffer[int(pixel.x + pixel.y * width)] = pixel.z;
          colorPixel((int)pixel.x, (int)pixel.y, p_Color);
        }
      }
      else
      {
        // no depth-testing, just draw the pixel:
        colorPixel((int)pixel.x, (int)pixel.y, p_Color);
      }
    }
  }
}

static Vec3F
worldToScreen (Vec3F p_VecWS)
{
  // shift and scale x,y from WS [-1, 1] to SS [0, width or height]
  // keep z-values unchanged:
  return Vec3F(
    roundFloatToUInt((p_VecWS.x + 1.0f) * Dx12Wrapper::ms_Width / 2.0f),
    roundFloatToUInt((p_VecWS.y + 1.0f) * Dx12Wrapper::ms_Height / 2.0f),
    p_VecWS.z
  );
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
        clearBuffer(BLACK);

        // Render wireframe model:
        g_FlipVertically = true;

        for (int i = 0; i < g_Model->nfaces(); i++)
        {
          std::vector<int> face = g_Model->face(i);
          for (int j = 0; j < 3; j++) {
            Vec3F v0 = Vec3F(g_Model->vert(face[j]).raw);
            Vec3F v1 = Vec3F(g_Model->vert(face[(j + 1) % 3]).raw);
            int x0 = roundFloatToUInt((v0.x + 1.0f) * Dx12Wrapper::ms_Width / 2);
            int y0 = roundFloatToUInt((v0.y + 1.0f) * Dx12Wrapper::ms_Height / 2);
            int x1 = roundFloatToUInt((v1.x + 1.0f) * Dx12Wrapper::ms_Width / 2);
            int y1 = roundFloatToUInt((v1.y + 1.0f) * Dx12Wrapper::ms_Height / 2);
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
        clearBuffer(WHITE);
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
      else if ('S' == virtualKeyCode)
      {
        clearBuffer(BLACK);

        // shade the model with flat color and lamber cosine law
        g_FlipVertically = true;

        static constexpr Vec3F lightDir = Vec3F(0.0f, 0.0f, -1.0f);

        for (int i = 0; i < g_Model->nfaces(); i++) {
          std::vector<int> face = g_Model->face(i);
          Vec3F posSS[3];
          Vec3F posWS[3];
          for (int j = 0; j < 3; j++)
          {
            Vec3F v = Vec3F(g_Model->vert(face[j]).raw);
            posSS[j] = Vec3F(
              roundFloatToUInt((v.x + 1.0f) * Dx12Wrapper::ms_Width / 2.0f),
              roundFloatToUInt((v.y + 1.0f) * Dx12Wrapper::ms_Height / 2.0f),
              v.z
            );
            posWS[j] = v;
          }
          Vec3F n = Vec3F::cross(posWS[2] - posWS[0], posWS[1] - posWS[0]);
          n.normalize();
          float intensity = Vec3F::dot(n, lightDir); // dot product: Lambert cosine law
          if (intensity > 0)
          {
            drawTriangle(posSS, (Colors::White * intensity).convertToUint32(), false);
          }
        }

        g_FlipVertically = false;
      }
      else if ('D' == virtualKeyCode)
      {
        clearBuffer(BLACK);

        // Draw with Depth testing
        g_FlipVertically = true;

        static constexpr Vec3F lightDir = Vec3F(0.0f, 0.0f, -1.0f);

        for (int i = 0; i < g_Model->nfaces(); i++)
        {
          std::vector<int> face = g_Model->face(i);
          Vec3F posSS[3];
          Vec3F posWS[3];
          for (int j = 0; j < 3; j++)
          {
            posWS[j] = Vec3F(g_Model->vert(face[j]).x, g_Model->vert(face[j]).y, g_Model->vert(face[j]).z);
            posSS[j] = worldToScreen(posWS[j]);
          }
          
          // Apply intensity through dot product: (Lambert cosine law)
          Vec3F n = Vec3F::cross(posWS[2] - posWS[0], posWS[1] - posWS[0]);
          n.normalize();
          float intensity = Vec3F::dot(n, lightDir); 
          if (intensity > 0)
            drawTriangle(posSS, (Colors::White * intensity).convertToUint32(), true);
        }

        g_FlipVertically = false;
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

  // Init depth buffer
  g_DepthBuffer = new float[windowWidth * windowHeight];
  for (int i = windowWidth * windowHeight; i--;
    g_DepthBuffer[i] = -std::numeric_limits<float>::max());

  // random color
  Colors::ColorRGBA color = { .r = rndf(), .g = rndf(), .b = rndf(), .a = 1.0f };

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

#pragma once

#define NOMINMAX // suppress the min and max definitions in Windef.h
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>
#include <cassert>
#include <cstdlib>
#include <algorithm>

#include <tinyrenderer/model.h>

//---------------------------------------------------------------------------//
// Global variables:
//---------------------------------------------------------------------------//
HWND g_Window;
Model* g_Model;

bool g_FlipVertically;

//---------------------------------------------------------------------------//
// Helper functions:
//---------------------------------------------------------------------------//
template <typename T> inline T alignUp(T p_Val, T p_Alignment)
{
  return (p_Val + p_Alignment - (T)1) & ~(p_Alignment - (T)1);
}
//---------------------------------------------------------------------------//
template <typename T, size_t N> constexpr size_t arrayCount(T(&)[N]) { return N; }
//---------------------------------------------------------------------------//
template <typename T, uint32_t N> constexpr uint32_t arrayCount32(T(&)[N]) { return N; }
//---------------------------------------------------------------------------//
inline void traceHr(const std::string& p_Msg, HRESULT p_Hr)
{
  char hrMsg[512];
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, p_Hr, 0, hrMsg, arrayCount32(hrMsg), nullptr);
  std::string errMsg = p_Msg + ".\nError! " + hrMsg;

  MessageBoxA(g_Window, errMsg.c_str(), "Error", MB_OK);
}
//---------------------------------------------------------------------------//
inline void throwIfFailed(HRESULT hr)
{
  if (FAILED(hr))
  {
    traceHr("DX12 ERROR!", hr);
    __debugbreak();
  }
}


#pragma once

#include "utils.hpp"

#include <d3d12.h>
#include <dxgi1_6.h>

#include "d3dx12.h"

#include <wrl.h>
using Microsoft::WRL::ComPtr;


//---------------------------------------------------------------------------//
struct Dx12Wrapper{

  static constexpr int ms_FrameCount = 2;
  inline static unsigned int ms_FrameIndex;
  inline static unsigned int ms_RtvDescriptorSize;
  
  inline static ComPtr<ID3D12Device3> ms_Device;
  inline static ComPtr<ID3D12CommandQueue> ms_CmdQueue;
  inline static ComPtr<IDXGISwapChain3> ms_Swc;
  inline static ComPtr<ID3D12Resource> ms_RenderTargets[ms_FrameCount];
  inline static ComPtr<ID3D12DescriptorHeap> ms_RtvHeap;
  inline static ComPtr<ID3D12CommandAllocator> ms_CmdAllocator;
  inline static ComPtr<ID3D12GraphicsCommandList> ms_CmdList;
  


  // Synchronization objects.
  inline static HANDLE ms_FenceEvent;
  inline static ComPtr<ID3D12Fence> ms_Fence;
  inline static UINT64 ms_FenceValue;

  static constexpr DXGI_FORMAT ms_SwcFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
  inline static int ms_Width;
  inline static int ms_Height;
  static constexpr int ms_BytePerPixel = 4;

  inline static size_t ms_BackbufferSize;
  inline static void* ms_BackbufferMemory = nullptr;

  struct BackbufferWrapper
  {
    ComPtr<ID3D12Resource> resource;
    ComPtr<ID3D12Heap> heap;

  };
  inline static BackbufferWrapper ms_BackbufferWrapper;

  //---------------------------------------------------------------------------//
  static void
  onInit(int p_Width, int p_Height)
  {
    ms_Width = p_Width;
    ms_Height = p_Height;


    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
      ComPtr<ID3D12Debug> debugController;
      if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
      {
        debugController->EnableDebugLayer();

        // Enable additional debug layers.
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
      }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    throwIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    {
      ComPtr<ID3D12Device> device;
      ComPtr<IDXGIAdapter1> hardwareAdapter;
      getHardwareAdapter(factory.Get(), &hardwareAdapter);

      throwIfFailed(D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&device)
      ));

      device->QueryInterface(IID_PPV_ARGS(&ms_Device));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    throwIfFailed(ms_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&ms_CmdQueue)));

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = ms_FrameCount;
    swapChainDesc.Width = ms_Width;
    swapChainDesc.Height = ms_Height;
    swapChainDesc.Format = ms_SwcFormat;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    throwIfFailed(factory->CreateSwapChainForHwnd(
      ms_CmdQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
      g_Window,
      &swapChainDesc,
      nullptr,
      nullptr,
      &swapChain
    ));

    // This sample does not support fullscreen transitions.
    throwIfFailed(factory->MakeWindowAssociation(g_Window, DXGI_MWA_NO_ALT_ENTER));

    throwIfFailed(swapChain.As(&ms_Swc));
    ms_FrameIndex = ms_Swc->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
      // Describe and create a render target view (RTV) descriptor heap.
      D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
      rtvHeapDesc.NumDescriptors = ms_FrameCount;
      rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
      rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
      throwIfFailed(ms_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&ms_RtvHeap)));

      ms_RtvDescriptorSize = ms_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
      CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(ms_RtvHeap->GetCPUDescriptorHandleForHeapStart());

      // Create a RTV for each frame.
      for (UINT n = 0; n < ms_FrameCount; n++)
      {
        throwIfFailed(ms_Swc->GetBuffer(n, IID_PPV_ARGS(&ms_RenderTargets[n])));
        ms_Device->CreateRenderTargetView(ms_RenderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, ms_RtvDescriptorSize);
      }
    }

    throwIfFailed(ms_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&ms_CmdAllocator)));

    throwIfFailed(ms_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&ms_Fence)));
    ms_FenceValue = 1;

    // Create an event handle to use for frame synchronization.
    ms_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (ms_FenceEvent == nullptr)
    {
      throwIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }


    // Create the command list.
    throwIfFailed(ms_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, ms_CmdAllocator.Get(), nullptr, IID_PPV_ARGS(&ms_CmdList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    throwIfFailed(ms_CmdList->Close());


    ////////////////////////////////////////////////////
#pragma region Setup the cpu-side buffer and its wrapper:

    // Create a cpu-side buffer to fill with rasterized content
    ms_BackbufferSize = ms_Width * ms_Height * ms_BytePerPixel;

    // NOTE(OM): To use malloc instead of VirtualAlloc you need "OpenExistingHeapFromAddress1"
    // https://github.com/microsoft/DirectX-Headers/blob/main/include/directx/d3d12.idl#L5715
    // which is only available in AgilitySdk 1.6 or later
    // 
    // ms_BackbufferMemory = malloc(ms_BackbufferSize);

    ms_BackbufferMemory = VirtualAlloc(nullptr, ms_BackbufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    // Wrap CPU-rasterized content in a wrapper buffer to be copied to SWC backbuffer
    ms_Device->OpenExistingHeapFromAddress(ms_BackbufferMemory, IID_PPV_ARGS(&ms_BackbufferWrapper.heap));
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(ms_BackbufferSize, D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER);
    throwIfFailed(ms_Device->CreatePlacedResource(ms_BackbufferWrapper.heap.Get(), 0, &desc, D3D12_RESOURCE_STATE_COPY_SOURCE, nullptr, IID_PPV_ARGS(&ms_BackbufferWrapper.resource)));

#pragma endregion
    ////////////////////////////////////////////////////

  }
  //---------------------------------------------------------------------------//
  static void
  onRender()
  {
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    throwIfFailed(ms_CmdAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    throwIfFailed(ms_CmdList->Reset(ms_CmdAllocator.Get(), nullptr));

    ////////////////////////////////////////////////////////////////////
#pragma region Copy wrapper to swapchain backbuffer/rendertarget/texture:

    // Prepare source (wrapper buffer):
    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.pResource = ms_BackbufferWrapper.resource.Get();

#if 0 // automated way of computing src footprint from dst desc
    D3D12_RESOURCE_DESC srcDesc = ms_RenderTargets[ms_FrameIndex].Get()->GetDesc();

    ms_Device->GetCopyableFootprints(
      &srcDesc, 0, 1, 0,
      &src.PlacedFootprint,
      nullptr, nullptr, nullptr);

#else // manual calculation of src footprint from prior knowledge

    // Calculate pitch from DDS programming guide:
    // https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
    assert(DXGI_FORMAT_R8G8B8A8_UNORM == ms_SwcFormat);
    constexpr int bitsPerPixel = 32;
    const int pitch = (ms_Width * bitsPerPixel + 7) / 8;

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
    footprint.Footprint.Format = ms_SwcFormat;
    footprint.Footprint.Depth = 1;
    footprint.Footprint.Width = ms_Width;
    footprint.Footprint.Height = ms_Height;
    footprint.Footprint.RowPitch = pitch;
    src.PlacedFootprint = footprint;

#endif

    // Prepare destination (swc backbuffer):
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource = ms_RenderTargets[ms_FrameIndex].Get();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dst.SubresourceIndex = 0;

    // Transition swc backbuffer/texture/rendertarget into copy state:
    ms_CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ms_RenderTargets[ms_FrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST));

    ms_CmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // Transition back swc backbuffer/texture/rendertarget:
    ms_CmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(ms_RenderTargets[ms_FrameIndex].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));

#pragma endregion
    ////////////////////////////////////////////////////////////////////

    // Execute the command list.
    throwIfFailed(ms_CmdList->Close());
    ID3D12CommandList* ppCommandLists[] = { ms_CmdList.Get() };
    ms_CmdQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    throwIfFailed(ms_Swc->Present(1, 0));

    waitForPreviousFrame();
  }
  //---------------------------------------------------------------------------//
  static void 
  waitForPreviousFrame()
  {
    // Naiive synchronization approach:
 
    // Signal and increment the fence value.
    const UINT64 fence = ms_FenceValue;
    throwIfFailed(ms_CmdQueue->Signal(ms_Fence.Get(), fence));
    ms_FenceValue++;

    // Wait until the previous frame is finished.
    if (ms_Fence->GetCompletedValue() < fence)
    {
      throwIfFailed(ms_Fence->SetEventOnCompletion(fence, ms_FenceEvent));
      WaitForSingleObject(ms_FenceEvent, INFINITE);
    }

    ms_FrameIndex = ms_Swc->GetCurrentBackBufferIndex();
  }
  //---------------------------------------------------------------------------//
  static void
  getHardwareAdapter(
    IDXGIFactory1* p_Factory,
    IDXGIAdapter1** p_Adapter,
    bool p_RequestHighPerformanceAdapter = false)
  {
    *p_Adapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(p_Factory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
      for (
        UINT adapterIndex = 0;
        SUCCEEDED(factory6->EnumAdapterByGpuPreference(
          adapterIndex,
          p_RequestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
          IID_PPV_ARGS(&adapter)));
        ++adapterIndex)
      {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
          // Don't select the Basic Render Driver adapter.
          // If you want a software adapter, pass in "/warp" on the command line.
          continue;
        }

        // Check to see whether the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
          break;
        }
      }
    }

    if (adapter.Get() == nullptr)
    {
      for (UINT adapterIndex = 0; SUCCEEDED(p_Factory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
      {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
          // Don't select the Basic Render Driver adapter.
          // If you want a software adapter, pass in "/warp" on the command line.
          continue;
        }

        // Check to see whether the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
          break;
        }
      }
    }

    *p_Adapter = adapter.Detach();
  }
};


# rasterizer
Software rasterization

## SWC Software Rasterizer [WIP]
SWC stands for swapchain and it means instead of bitblting the raster content, I use swapchain backbuffer to present the software rasterization results. Here is how it works:
The cpu side buffer is wrapped in a ID3D12 resource placed on an existing heap through a call to [openexistingheapfromaddress](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device3-openexistingheapfromaddress) and then through a call to [CopyTextureRegion](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-copytextureregion) the raster content is copied to the current frame backbuffer rendertarget.

Keyboard bindings:
- Press H or V to draw a horizontal or vertical line (repeat to move the line)
- Press T to render triangles
- Press W to render the wireframe model (from [tinyrenderer](https://github.com/ssloy/tinyrenderer/wiki/Lesson-1:-Bresenham%E2%80%99s-Line-Drawing-Algorithm))
- Press C to clear screen with white color
- 
  
## Win32 Software Rasterizer [deprecated]
Win32_Rasterizer is the first attempt with only basic drawing functionality built on vanilla Win32/WinApi.

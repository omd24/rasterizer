# rasterizer
Software rasterization

## Win32 Software Rasterizer [deprecated]
Win32_Rasterizer is the first attempt with only basic drawing functionality built on vanilla Win32/WinApi.
- Press M to draw the wire frame model
- Press W to clear the screen

As the model is from [tinyrenderer](https://github.com/ssloy/tinyrenderer/wiki/Lesson-1:-Bresenham%E2%80%99s-Line-Drawing-Algorithm) it similarly requires a vertical flip (Note the call to flip_vertically() in the original [example](https://github.com/ssloy/tinyrenderer/blob/f6fecb7ad493264ecd15e230411bfb1cca539a12/main.cpp)).

## SWC Software Rasterizer [WIP]
SWC stands for swapchain and it means instead of bitblting the raster content, I use swapchain backbuffer to present the software rasterization results. Here is how it works:
The cpu side buffer is wrapped in a ID3D12 resource placed on an existing heap through a call to [openexistingheapfromaddress](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device3-openexistingheapfromaddress) and then through a call to [CopyTextureRegion](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-copytextureregion) the raster content is copied to the current frame backbuffer rendertarget.

Keyboard bindings:
- Press H to draw horizontal line (repeat to move down)
- Press V to draw vertical line (repeat to move right)
- Press C to draw a checkerboard
- Press W to clear screen with white color

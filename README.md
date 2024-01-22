# rasterizer
Software rasterization

## Win32 Software Rasterizer
Win32_Rasterizer is the first attempt with only basic drawing functionality built on vanilla Win32/WinApi.
- Press M to draw the wire frame model
- Press W to clear the screen

As the model is from [tinyrenderer](https://github.com/ssloy/tinyrenderer/wiki/Lesson-1:-Bresenham%E2%80%99s-Line-Drawing-Algorithm) it requires a vertical flip.

## SWC Software Rasterizer [WIP]
SWC stands for swapchain and it means instead of bitblting the raster content, I use swapchain backbuffer to present the software rasterization results.
- Press H to draw horizontal line (repeat to move down)
- Press V to draw vertical line (repeat to move right)
- Press C to draw a checkerboard
- Press W to clear screen with white color

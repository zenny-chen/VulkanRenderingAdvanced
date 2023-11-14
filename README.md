# VulkanRenderingAdvanced
Advanced rendering with Vulkan API

<br />

This project demonstrates how to use a vertex shader, a geometry shader and a fragment shader to render some graphics with Vulkan API. And it also shows the usage of timestamp query and occlusion query.

This project is built on Windows 11 with Visual Studio 2022 Community Edition and Vulkan SDK 1.3. If you want to generate the SPV files, just run glsl_builder.bat.

The official Vulkan SDK for Windows is here: https://vulkan.lunarg.com/sdk/home#windows

It is also convenient to be ported to other platforms, such as Android and other Linux operating systems. If GCC or Clang is used, just use **`-std=gnu2x`** option.

<br />

# GTX 1650 Fragment Shading Rate Combiner Operation

<br />

## Combining shading rate factors (width x height) with VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR

A/B |	1x1 |	1x2 | 1x4 |	2x1 |	2x2 |	2x4 | 4x1 |	4x2 |	4x4
---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ---- | ----
1x1 | 1x1 | 1x2 | 1x2 | 2x1 | 2x2 | 2x4 | 2x1 | 4x2 | 4x4
1x2 | 1x2 | 1x2 | 1x2 | 2x2 | 2x4 | 2x4 | 4x2 | 4x4 | 4x4
1x4 | 1x2 | 1x2 | 1x2 | 2x4 | 2x4 | 2x4 | 4x4 | 4x4 | 4x4
2x1 | 2x1 | 2x2 | 2x4 | 2x1 | 4x2 | 4x4 | 2x1 | 4x2 | 4x4
2x2 | 2x2 | 2x4 | 2x4 | 4x2 | 4x4 | 4x4 | 4x2 | 4x4 | 4x4
2x4 | 2x4 | 2x4 | 2x4 | 4x4 | 4x4 | 4x4 | 4x4 | 4x4 | 4x4
4x1 | 2x1 | 4x2 | 4x4 | 2x1 | 4x2 | 4x4 | 2x1 | 4x2 | 4x4
4x2 | 4x2 | 4x4 | 4x4 | 4x2 | 4x4 | 4x4 | 4x2 | 4x4 | 4x4
4x4 | 4x4 | 4x4 | 4x4 | 4x4 | 4x4 | 4x4 | 4x4 | 4x4 | 4x4

<br />

## Algorithm

```cpp
VkExtent2D MULcombiner(VkExtent2D a, VkExtent2D b)
{
    VkExtent2D c;
    c.width = a.width * b.width;
    c.height = a.height * b.height;

    if (c.width > 4) {
        c.width = 4;
    }
    if (c.height > 4) {
        c.height = 4;
    }

    if (maxFragmentShadingRateAttachmentTexelSizeAspectRatio == 2)
    {
        // In this case, 4x1 will be clamped to 2x1, and 1x4 will be clamped to 1x2
        if (c.width == 4 && c.height == 1) {
            c.width = 2;
        }
        if (c.width == 1 && c.height == 4) {
            c.height = 2;
        }
    }

    return c;
}
```


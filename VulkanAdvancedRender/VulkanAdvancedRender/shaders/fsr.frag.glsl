
#version 450 core

#extension GL_EXT_fragment_shading_rate : enable

precision mediump int;
precision highp float;

layout(location = 0) in flat lowp vec4 fragColor;
layout(location = 0) out lowp vec4 myOutput;

void main()
{
    const int fsrPixelSize1x1 = 0;
    const int fsrPixelSize1x2 = 1;
    const int fsrPixelSize1x4 = 2;
    const int fsrPixelSize2x1 = 4;
    const int fsrPixelSize2x2 = 5;
    const int fsrPixelSize2x4 = 6;
    const int fsrPixelSize4x1 = 8;
    const int fsrPixelSize4x2 = 9;
    const int fsrPixelSize4x4 = 10;

    lowp vec4 dstColor = fragColor;

    switch (gl_ShadingRateEXT)
    {
    case fsrPixelSize1x1:
    default:
        break;

    case fsrPixelSize1x2:
        // red
        dstColor = vec4(0.9f, 0.1f, 0.1f, 1.0f);
        break;

    case fsrPixelSize1x4:
        // black
        dstColor = vec4(0.1f, 0.1f, 0.1f, 1.0f);
        break;

    case fsrPixelSize2x1:
        // green
        dstColor = vec4(0.1f, 0.9f, 0.1f, 1.0f);
        break;

    case fsrPixelSize2x2:
        // blue
        dstColor = vec4(0.1f, 0.1f, 0.9f, 1.0f);
        break;

    case fsrPixelSize2x4:
        // yellow
        dstColor = vec4(0.9f, 0.9f, 0.1f, 1.0f);
        break;

    case fsrPixelSize4x1:
        // white
        dstColor = vec4(0.9f, 0.9f, 0.9f, 1.0f);
        break;

    case fsrPixelSize4x2:
        // cyan
        dstColor = vec4(0.1f, 0.9f, 0.9f, 1.0f);
        break;

    case fsrPixelSize4x4:
        // magenta
        dstColor = vec4(0.9f, 0.1f, 0.9f, 1.0f);
        break;
    }

    myOutput = dstColor;
}


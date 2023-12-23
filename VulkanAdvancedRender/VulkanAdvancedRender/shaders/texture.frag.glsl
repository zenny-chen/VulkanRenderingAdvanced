
#version 450 core

precision mediump int;
precision highp float;

layout(location = 0) in highp vec2 varyingTexCoords;
layout(location = 0) out lowp vec4 outColor;

// Combined Texture + Sampler
layout(set = 0, binding = 1) uniform sampler2D imageSampler;

void main()
{
    outColor = texture(imageSampler, varyingTexCoords);
}


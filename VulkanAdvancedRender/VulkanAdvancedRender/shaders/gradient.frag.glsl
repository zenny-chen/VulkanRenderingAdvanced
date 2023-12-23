
#version 450 core

precision mediump int;
precision highp float;

layout(location = 0) in smooth lowp vec4 fragColor;
layout(location = 0) out lowp vec4 myOutput;

layout(early_fragment_tests) in;

void main()
{
    myOutput = fragColor;
}


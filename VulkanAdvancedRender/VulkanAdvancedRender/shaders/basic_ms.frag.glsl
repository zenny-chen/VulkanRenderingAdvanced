
#version 450 core

precision mediump int;
precision highp float;

layout(location = 0) in MeshletVertexAttribute
{
    lowp smooth vec4 color;
} inVertAttr;

layout(location = 0) out lowp vec4 myOutput;

void main()
{
    myOutput = inVertAttr.color;
}


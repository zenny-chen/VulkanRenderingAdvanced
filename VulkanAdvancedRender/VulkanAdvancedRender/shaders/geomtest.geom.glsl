
#version 450 core

layout(constant_id = 0) const float edgeWidth = 0.4f;
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout(location = 0) in VS_OUT
{
    flat lowp vec4 fragColor;
} gsIn[];

layout(location = 0) out flat lowp vec4 outFragColor;

void main()
{
    const vec4 srcInPos = gl_in[0].gl_Position;
    const lowp vec4 dstColor = gsIn[0].fragColor;
    const float pointSize = 1.0f;
    const float halfWidth = edgeWidth * 0.5f;

    // bottom-left
    gl_Position = vec4(srcInPos.x - halfWidth, srcInPos.y + halfWidth, srcInPos.z, 1.0f);
    gl_PointSize = pointSize;
    outFragColor = dstColor;
    EmitVertex();

    // bottom-right
    gl_Position = vec4(srcInPos.x + halfWidth, srcInPos.y + halfWidth, srcInPos.z, 1.0f);
    gl_PointSize = pointSize;
    outFragColor = dstColor;
    EmitVertex();

    // top-left
    gl_Position = vec4(srcInPos.x - halfWidth, srcInPos.y - halfWidth, srcInPos.z, 1.0f);
    gl_PointSize = pointSize;
    outFragColor = dstColor;
    EmitVertex();

    // top-right
    gl_Position = vec4(srcInPos.x + halfWidth, srcInPos.y - halfWidth, srcInPos.z, 1.0f);
    gl_PointSize = pointSize;
    outFragColor = dstColor;
    EmitVertex();

    EndPrimitive();
}


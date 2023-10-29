
#version 450 core

#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_mesh_shader : enable

layout(local_size_x_id = 0) in;
layout(constant_id = 1) const uint payload_data_count = 1U;

// the primitive type (points, lines or triangles)
layout(triangles) out;

// maximum allocation size for each meshlet
layout(max_vertices = 256, max_primitives = 256) out;

// Vertex attributes out block (exclude vertex coordinates which should be written in gl_MeshVerticesEXT)
layout(location = 0) out MeshletVertexAttribute
{
    lowp smooth vec4 color;
} outVertAttr[];

struct MyPayloadType
{
    int data[payload_data_count];
    uint meshID;
    uint meshGroupSize;
};

taskPayloadSharedEXT MyPayloadType sharedPayload;

layout(std430, set = 0, binding = 0, scalar) uniform transform_block {
    vec2 u_factor;
    float u_angle;
} trans_consts;

/** Model view translation matrix *
 * [ 1  0  0  0
     0  1  0  0
     0  0  1  0
     x  y  z  1
 * ]
*/

/** Ortho projection matrix *
 * [ 2/(r-l)       0             0             0
     0             2/(t-b)       0             0
     0             0             -2/(f-n)      0
     -(r+l)/(r-l)  -(t+b)/(t-b)  -(f+n)/(f-n)  1
 * ]
*/

/** rotate matrix *
 * [x^2*(1-c)+c  xy*(1-c)+zs  xz(1-c)-ys  0
    xy(1-c)-zs   y^2*(1-c)+c  yz(1-c)+xs  0
    xz(1-c)+ys   yz(1-c)-xs   z^2(1-c)+c  0
    0            0            0           1
 * ]
 * |(x, y, z)| must be 1.0
*/

void main()
{
    // We're going to generate 256 vertices and 254 triangles (127 differential rectangles)
    SetMeshOutputsEXT(256, 254);

    const float edgeLength = 0.3f;
    const float baseCoord = edgeLength * 0.5f;
    const float dx = edgeLength / 127.0f;

    // podPattern == 0
    const float x = -baseCoord + dx * float(gl_LocalInvocationID.x);

    // top and bottom will be upside downed after projection
    const vec4 vert0 = vec4(x, -baseCoord, 0.0f, 1.0f);     // top vertex
    const vec4 vert1 = vec4(x, baseCoord, 0.0f, 1.0f);      // bottom vertex

    const float xOffsetList[4] = { 0.15f, 0.15f, 0.65f, 0.65f };
    const float yOffsetList[4] = { 0.15f, 0.65f, 0.15f, 0.65f };

    // glTranslate(offset, offset, -2.3, 1.0)
    mat4 translateMatrix = mat4(1.0f, 0.0f, 0.0f, xOffsetList[gl_WorkGroupID.x],    // column 0
                                0.0f, 1.0f, 0.0f, yOffsetList[gl_WorkGroupID.x],    // column 1
                                0.0f, 0.0f, 1.0f, -2.3f,                            // column 2
                                0.0f, 0.0f, 0.0f, 1.0f                              // column 3
                               );

    const uint flagBit = gl_WorkGroupID.x & 1U;
    const float radianCoeff = 1.0f - float(flagBit << 1);
    const float radian = radians(trans_consts.u_angle) * radianCoeff;

    // glRotate(radian, 0.0, 0.0, 1.0)
    mat4 rotateMatrix = mat4(cos(radian), -sin(radian), 0.0f, 0.0f,     // column 0
                             sin(radian), cos(radian), 0.0f, 0.0f,      // column 1
                             0.0f, 0.0f, 1.0f, 0.0f,                    // column 2
                             0.0f, 0.0f, 0.0f, 1.0f                     // column 3
                            );

    // glOrtho(left, right, bottom, top, near, far) and here uses:
    // glOrtho(-u_factor.x, u_factor.x, -u_factor.y, u_factor.y, 1.0, 3.0)
    // Here upside down the bottom and top to make the front face as counter-clockwise
    mat4 projectionMatrix = mat4(1.0f / trans_consts.u_factor.x, 0.0f, 0.0f, 0.0f,  // column 0
                                 0.0f, 1.0f / trans_consts.u_factor.y, 0.0f, 0.0f,  // column 1
                                 0.0f, 0.0f, -1.0f, -2.0f,                          // column 2
                                 0.0f, 0.0f, 0.0f, 1.0f                             // colimn 3
                                );

    mat4 mvpMatrix = rotateMatrix * (translateMatrix * projectionMatrix);

    // Each work item generates 2 vertices
    gl_MeshVerticesEXT[gl_LocalInvocationID.x * 2U + 0U].gl_Position = vert0 * mvpMatrix;
    gl_MeshVerticesEXT[gl_LocalInvocationID.x * 2U + 1U].gl_Position = vert1 * mvpMatrix;

    // Process colors
    float c0 = float(sharedPayload.data[gl_WorkGroupID.x * 256 + gl_LocalInvocationID.x]) / 256.0f;
    float c1 = float(sharedPayload.data[gl_WorkGroupID.x * 256 + 128 + gl_LocalInvocationID.x]) / 256.0f;

    c0 = fract(c0);
    float rc0 = 1.0f - c0;
    c1 = fract(c1);
    float rc1 = 1.0f - c1;

    c0 = clamp(c0, 0.1f, 0.9f);
    rc0 = clamp(rc0, 0.1f, 0.9f);
    c1 = clamp(c1, 0.1f, 0.9f);
    rc1 = clamp(rc1, 0.1f, 0.9f);

    outVertAttr[gl_LocalInvocationID.x * 2U + 0U].color = vec4(c0, c0, rc0, 1.0f);
    outVertAttr[gl_LocalInvocationID.x * 2U + 1U].color = vec4(c1, c1, rc1, 1.0f);

    // Assemble the primitive
    if (gl_LocalInvocationID.x >= 127) return;

    // Each work item assembles 2 primitives (2 triangles compose 1 rectangle)
    const uint v0 = gl_LocalInvocationID.x * 2U;
    const uint v1 = v0 + 1U;
    const uint v2 = v0 + 2U;
    const uint v3 = v0 + 3U;

    gl_PrimitiveTriangleIndicesEXT[gl_LocalInvocationID.x * 2 + 0U] = uvec3(v0, v1, v2);
    gl_PrimitiveTriangleIndicesEXT[gl_LocalInvocationID.x * 2 + 1U] = uvec3(v2, v1, v3);
}



#version 450 core

#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_mesh_shader : enable

layout(local_size_x_id = 0) in;
layout(constant_id = 1) const uint payload_data_count = 1U;

struct MyPayloadType
{
    int data[payload_data_count];
    uint meshID;
    uint meshGroupSize;
};

taskPayloadSharedEXT MyPayloadType sharedPayload;

void main()
{
    const uint baseData = gl_WorkGroupID.x * gl_WorkGroupID.y * gl_WorkGroupID.z;

    for (uint i = 0; i < payload_data_count; ++i) {
        sharedPayload.data[i] = int(baseData + i);
    }

    sharedPayload.meshID = gl_WorkGroupID.x;
    sharedPayload.meshGroupSize = 4U;     // launch 4 mesh shader groups

    EmitMeshTasksEXT(sharedPayload.meshGroupSize, 1U, 1U);
}


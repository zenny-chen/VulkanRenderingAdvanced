%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o flatten.vert.spv  flatten.vert.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o flatten.frag.spv  flatten.frag.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o gradient.vert.spv  gradient.vert.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o gradient.frag.spv  gradient.frag.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o fsr.vert.spv  fsr.vert.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o fsr.frag.spv  fsr.frag.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o geomtest.vert.spv  geomtest.vert.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o geomtest.geom.spv  geomtest.geom.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o geomtest.frag.spv  geomtest.frag.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.2  -Os  -o basic_ms.task.spv  basic_ms.task.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.2  -Os  -o basic_ms.mesh.spv  basic_ms.mesh.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -Os  -o basic_ms.frag.spv  basic_ms.frag.glsl


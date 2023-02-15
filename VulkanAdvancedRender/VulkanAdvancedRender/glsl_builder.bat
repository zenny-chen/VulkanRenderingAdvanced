%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -o flatten.vert.spv  flatten.vert.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -o flatten.frag.spv  flatten.frag.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -o gradient.vert.spv  gradient.vert.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -o gradient.frag.spv  gradient.frag.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -o geomtest.vert.spv  geomtest.vert.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -o geomtest.geom.spv  geomtest.geom.glsl
%VK_SDK_PATH%/Bin/glslangValidator  --target-env vulkan1.1  -o geomtest.frag.spv  geomtest.frag.glsl
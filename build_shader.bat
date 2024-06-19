pushd W:\Warpunk\shader
W:\VulkanSDK\1.3.246.1/Bin/glslc.exe .\shader.vert -o ..\shader\vert.spv
W:\VulkanSDK\1.3.246.1/Bin/glslc.exe .\shader.frag -o ..\shader\frag.spv
pause
popd

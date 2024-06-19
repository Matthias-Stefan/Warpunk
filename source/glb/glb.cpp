#include "source/glb/glb.h"

glb LoadGLB(platform_file_handle *File, memory_block *Memory, work_queue *Queue)
{
	u8 *Data = File->Content;
	
	glb Result = {};
	
	u32 Magic = ExtractData<u32>(Data);
	u32 Version = ExtractData<u32>(Data);
	u32 Length = ExtractData<u32>(Data);

	return Result;
}

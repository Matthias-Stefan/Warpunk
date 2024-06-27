#version 450

layout(set = 0, binding = 0) uniform view_projection_matrices
{
	mat4 View;
	mat4 Projection;
} vpm;

layout(set = 1, binding = 0) uniform model_matrix {
    mat4 Model;
} mm;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main()
{
	gl_Position = vpm.Projection * vpm.View * mm.Model * vec4(inPosition, 1.0);  

	fragColor = inColor;
	fragTexCoord = inTexCoord;
}

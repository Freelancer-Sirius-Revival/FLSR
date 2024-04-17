//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//
//Simple color shader for FLAR by Schmackbolzen

#version 330

uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform vec3 position;
uniform vec3 color;
uniform float size;
uniform bool isInstanced;

layout (location = 0) in vec3 inVertex;
layout (location = 1) in vec3 inPositionInstanced;
layout (location = 2) in float inSizeInstanced;
layout (location = 3) in vec3 inColorInstanced;
out vec3 currentColor;

void main()
{	
	vec3 worldSpacePos;
	if(!isInstanced)
	{
		currentColor=color;
		worldSpacePos=size * inVertex + position;
	}
	else
	{
		currentColor=inColorInstanced;
		worldSpacePos=inSizeInstanced * inVertex + inPositionInstanced;
	}
	gl_Position = projectionMat * modelViewMat * vec4(worldSpacePos,1);
}

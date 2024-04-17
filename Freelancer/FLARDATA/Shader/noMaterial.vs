//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//No material shader for FLAR by Schmackbolzen

#version 330

uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform bool isInstanced;

layout (location = 0) in vec4 inPosition;
layout (location = 6) in mat4 inModelViewMatInstanced;

void main()
{		
	mat4 mv;
	if(!isInstanced)
		mv=modelViewMat;
	else
		mv=inModelViewMatInstanced;
	gl_Position = projectionMat * mv * inPosition; 		
}
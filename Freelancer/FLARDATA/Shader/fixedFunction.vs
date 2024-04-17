//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Fixed function shader for FLAR by Schmackbolzen

#version 330
out vec2 texCoords[2];
out vec3 N;
out vec3 v;
out float RHWdepth;
out vec4 vertexColor;
out vec4 vertexColorSecondary;
out float fogEnd;
out float fogScale;

struct glFogParameters
{
	vec3 color;
	float density;
	float start;
	float end;
};

uniform glFogParameters glFog;
uniform bool enableRHWCoordinates;
uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform mat3 normalMat;
uniform vec2 projCorrection;
uniform mat4 textureMatrix[2];
uniform bool isInstanced;
uniform bool isInstancedTransparent;
uniform bool isInstancedFog;
uniform bool enableTextureTransform0;
uniform bool enableTextureTransform1;
uniform int texture1Index;

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec4 inSecondColor;
layout (location = 4) in vec4 inTexCoord0;
layout (location = 6) in mat4 inModelViewMatInstanced;
layout (location = 10) in float inAlphaValue;
layout (location = 11) in vec2 inFogStartEndInstanced;

void main()
{	
	mat4 mv;
	if(!isInstanced)
		mv=modelViewMat;
	else
		mv=inModelViewMatInstanced;
		
	vertexColor.rgba = (inColor).bgra;
	vertexColorSecondary.rgba=inSecondColor.bgra;

	if(isInstancedTransparent)
	{
		vertexColor.a = inAlphaValue;
	}
	
	if(!isInstancedFog)
	{		
		fogEnd = glFog.end;
		fogScale = 1.0f / (glFog.end - glFog.start);
	}
	else
	{		
		fogEnd = inFogStartEndInstanced.y;
		fogScale = 1.0f / (inFogStartEndInstanced.y - inFogStartEndInstanced.x);
	}

	v = vec3(mv * inPosition);
	N = normalize(mat3(mv) * inNormal);

	if (!enableRHWCoordinates)
		gl_Position = projectionMat * mv * inPosition; 	
	else
	{
		gl_Position = vec4((inPosition.x+63.0f / 128.0f)*projCorrection[0]-1,-(inPosition.y+63.0f / 128.0f)*projCorrection[1]+1,inPosition.z*2-1,1);
		RHWdepth = inPosition.z/inPosition.w;
	}
	
	if(enableTextureTransform0)
	{	
		vec4 texCoords0=vec4(inTexCoord0.xy,0,1);
		texCoords[0]  = (textureMatrix[0]*texCoords0).xy;
	}
	else
		texCoords[0]  = inTexCoord0.xy;
		
	if (texture1Index == 0)
		if(enableTextureTransform1)
		{
			vec4 texCoords0=vec4(inTexCoord0.xy,0,1);
			texCoords[1]  = (textureMatrix[1]*texCoords0).xy;
		}
		else
			texCoords[1]  = inTexCoord0.xy;
	else if (texture1Index == 1)
		if(enableTextureTransform1)
		{
			vec4 texCoords1=vec4(inTexCoord0.zw,0,1);
			texCoords[1]  = (textureMatrix[1]*texCoords1).xy;
		}
		else
			texCoords[1]  = inTexCoord0.zw;
}
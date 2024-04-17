//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Code based on "Parallel-Split Shadow Maps on Programmable GPUs" GPU Gems 3 (2005)
#version 330

layout (location = 0) in vec4 inPosition;
layout (location = 4) in vec2 inTexCoord0;
layout (location = 6) in mat4 inMVPMatInstanced;
layout (location = 10) in int inSplitIndexInstanced;

out vec2 inTexCoords;
flat out int splitIndex;

const int g_iNumSplits = $NUM_SPLITS;

uniform mat4 worldMat;
uniform bool isInstanced;
uniform bool isInstancedSplits;
uniform float alphaTestValue;
uniform int firstSplit;

void main(){
	mat4 mvp;
	
	if(!isInstanced)
		mvp=worldMat;
	else
	{
		mvp=inMVPMatInstanced;			
	}
	
	if(!isInstancedSplits)
	{
		splitIndex = firstSplit + gl_InstanceID;		
	}
	else
	{
		splitIndex = inSplitIndexInstanced;
	}	
	
	gl_Position =  mvp * inPosition;
	if (alphaTestValue > 0)
		inTexCoords = inTexCoord0.st;
}


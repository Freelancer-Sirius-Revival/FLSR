//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//PBR lighting shader for FLAR by Schmackbolzen

#version 330

#define MAX_SHADOWS $MAX_SHADOWS
#define NUM_SPLITS $NUM_SPLITS

out vec3 eyeVec;
out vec3 v,vWorld,worldNormal;
out mat3 tbnMatrix,tbnMatrixObj;
out vec3 N;
out vec4 shadowCoord[MAX_SHADOWS*NUM_SPLITS];
out vec4 texCoords;
out mat3 worldRotMat;
out float alphaValue;
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
uniform mat4 depthBiasMVP[MAX_SHADOWS*NUM_SPLITS];
uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform mat4 invViewMat;
uniform mat4 textureMatrix[2];
uniform bool enableNormalMaps;
uniform int shadowCount;
uniform bool enableEnvMap;
uniform bool isInstanced;
uniform bool isInstancedTransparent;
uniform bool isInstancedFog;
uniform float matDiffuseAlpha;
uniform int tex2Mode;
uniform int texture1Index;

layout (location = 0) in vec4 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec4 inSecondColor;
layout (location = 4) in vec4 inTexCoord0;
layout (location = 5) in vec4 inTexCoord2;
layout (location = 6) in mat4 inModelViewMatInstanced;
layout (location = 10) in float inAlphaValueInstanced;
layout (location = 11) in vec2 inFogStartEndInstanced;

const mat4 biasMatrix = mat4(0.5f, 0.0f, 0.0f, 0.0f,
							 0.0f, 0.5f, 0.0f, 0.0f,
							 0.0f, 0.0f, 0.5f, 0.0f,
							 0.5f, 0.5f, 0.5f, 1.0f);


void main(void)
{
	

	mat4 mv;
	if(!isInstanced)
		mv=modelViewMat;
	else
		mv=inModelViewMatInstanced;
		
	if(!isInstancedTransparent)
	{
		alphaValue = matDiffuseAlpha;	
	}
	else
	{
		alphaValue = inAlphaValueInstanced;	
	}
	
	if(!isInstancedFog)
	{		
		fogEnd = glFog.end;
		fogScale = 1 / (glFog.end - glFog.start);
	}
	else
	{		
		fogEnd = inFogStartEndInstanced.y;
		fogScale = 1 / (inFogStartEndInstanced.y - inFogStartEndInstanced.x);
	}
	
	mat3 normalMat = mat3(mv);
	v = vec3(mv * inPosition);
	mat4 worldMat = invViewMat*mv;
	if(enableEnvMap)
	{		
		vWorld = vec3(worldMat * inPosition);
		worldRotMat = mat3(worldMat);
		worldNormal = worldRotMat * inNormal;
	}
	if (shadowCount > 0)
	{
	for(int i=0;i<NUM_SPLITS;i++)
		shadowCoord[i] = biasMatrix*depthBiasMVP[i] * worldMat * inPosition;	
	if (shadowCount > 1)
		for(int i=0;i<NUM_SPLITS;i++)
			shadowCoord[NUM_SPLITS+i] = biasMatrix*depthBiasMVP[NUM_SPLITS+i] * worldMat * inPosition;	
	}
	N = normalize(normalMat*inNormal);
	
	if (enableNormalMaps)
	{
		vec3 TO = normalize(vec3(inTexCoord2.s,inTexCoord2.t,inTexCoord2.p));
		//TO = normalize(TO - dot(TO, inNormal) * inNormal);
		vec3 BO = normalize(cross(inNormal,TO)*(inTexCoord2.q));
		vec3 TA=normalMat*TO;
		vec3 BA=normalMat*BO;		
		vec3 T=normalize(TA);
		vec3 B=normalize(BA);	
		tbnMatrix = mat3(T.x, B.x, N.x,
						  T.y, B.y, N.y,
						  T.z, B.z, N.z);
						  
		tbnMatrixObj = mat3(TO,BO,inNormal.xyz);
		

		eyeVec		  = tbnMatrix*(-v);  
	}
	else			
		eyeVec		  = -v; 

	gl_Position     = projectionMat * mv * inPosition; 
	
	vec4 texCoords0=vec4(inTexCoord0.xy,0,1);
	vec4 texCoords1=vec4(inTexCoord0.zw,0,1);

	texCoords.xy  = (textureMatrix[0]*texCoords0).xy;
	
	//texCoords.zw  = (textureMatrix[1]*vec4(inTexCoord0.zw,0,1)).zw;
		
	if (texture1Index == 0)
		texCoords.zw   = (textureMatrix[1]*texCoords0).xy;
	else if (texture1Index == 1)
		texCoords.zw   = (textureMatrix[1]*texCoords1).zw;
} 
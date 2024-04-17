//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//
//PBR bloom downsample shader for FLAR by Schmackbolzen based on code from https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom
//The code has been modified to be more like the original presentation and also blog article from the original
//authors of the technique (referenced in the link above).

#version 330
#include "ColorConversion.inc"

uniform sampler2D srcTexture;
uniform vec2 srcTexelSize[$NUM_MIPS];


uniform int mipLevel;

in vec2 texCoord;
layout (location = 0) out vec3 downsample;

float KarisAverage(vec3 col)
{
	// Formula is 1 / (1 + luma)
	float luma = ToLuma(col);
	return 1.0f / (1.0f + luma);
}

void main()
{
	vec2 texelSize = srcTexelSize[mipLevel];
	float x = texelSize.x;
	float y = texelSize.y;

	// Take 13 samples around current texel:
	// a - b - c
	// - j - k -
	// d - e - f
	// - l - m -
	// g - h - i
	// === ('e' is the current texel) ===
	vec3 a = texture(srcTexture, vec2(texCoord.x - 2*x, texCoord.y + 2*y)).rgb;
	vec3 b = texture(srcTexture, vec2(texCoord.x,       texCoord.y + 2*y)).rgb;
	vec3 c = texture(srcTexture, vec2(texCoord.x + 2*x, texCoord.y + 2*y)).rgb;

	vec3 d = texture(srcTexture, vec2(texCoord.x - 2*x, texCoord.y)).rgb;
	vec3 e = texture(srcTexture, vec2(texCoord.x,       texCoord.y)).rgb;
	vec3 f = texture(srcTexture, vec2(texCoord.x + 2*x, texCoord.y)).rgb;

	vec3 g = texture(srcTexture, vec2(texCoord.x - 2*x, texCoord.y - 2*y)).rgb;
	vec3 h = texture(srcTexture, vec2(texCoord.x,       texCoord.y - 2*y)).rgb;
	vec3 i = texture(srcTexture, vec2(texCoord.x + 2*x, texCoord.y - 2*y)).rgb;

	vec3 j = texture(srcTexture, vec2(texCoord.x - x, texCoord.y + y)).rgb;
	vec3 k = texture(srcTexture, vec2(texCoord.x + x, texCoord.y + y)).rgb;
	vec3 l = texture(srcTexture, vec2(texCoord.x - x, texCoord.y - y)).rgb;
	vec3 m = texture(srcTexture, vec2(texCoord.x + x, texCoord.y - y)).rgb;	
	
	vec3 groups[5];
	const float factor1=0.125f/4.f;
	const float factor2=0.5f/4.f;
	groups[0] = (a+b+d+e) * factor1;
	groups[1] = (b+c+e+f) * factor1;
	groups[2] = (d+e+g+h) * factor1;
	groups[3] = (e+f+h+i) * factor1;
	groups[4] = (j+k+l+m) * factor2;
	if (mipLevel == 0)
	{		 
	  float avg1=KarisAverage(groups[0]);
	  float avg2=KarisAverage(groups[1]);
	  float avg3=KarisAverage(groups[2]);
	  float avg4=KarisAverage(groups[3]);
	  float avg5=KarisAverage(groups[4]);	  
	  float normalization=avg1+avg2+avg3+avg4+avg5+0.0001;
	  downsample = (groups[0]*avg1+groups[1]*avg2+groups[2]*avg3+groups[3]*avg4+groups[4]*avg5)/normalization;
	  downsample = max(downsample, 0.0001f);
	}
	else
	{
	  downsample = (groups[0]+groups[1]+groups[2]+groups[3]+groups[4]);	  
	}
}

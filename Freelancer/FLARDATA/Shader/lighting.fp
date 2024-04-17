//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//PBR lighting shader for FLAR by Schmackbolzen
//Sources are either mentioned in the comments below,
//from https://learnopengl.com/PBR/Lighting or https://learnopengl.com/PBR/IBL/Specular-IBL
//or "Parallel-Split Shadow Maps on Programmable GPUs" GPU Gems 3 (2005)

#version 330
#include "ColorConversion.inc"
#define MAX_SHADOWS $MAX_SHADOWS
#define NUM_SPLITS $NUM_SPLITS
#define USE_PCF $USE_PCF
#define USE_HQ_DIFFUSE $USE_HQ_DIFFUSE
#define USE_PARALLAX_MAPS $USE_PARALLAX_MAPS
#define USE_CLUSTERED_SHADING $USE_CLUSTERED_SHADING


//#define SIMPLE_KD
#define IBL_MULTIPLE_SCATTERING

#if USE_CLUSTERED_SHADING == 1
#define NUM_POSSIBLE_LIGHTS $NUM_POSSIBLE_LIGHTS
#define LIGHT_GRID_TILE_DIM_X $LIGHT_GRID_TILE_DIM_X
#define LIGHT_GRID_TILE_DIM_Y $LIGHT_GRID_TILE_DIM_Y
#define LIGHT_GRID_MAX_DIM_X $LIGHT_GRID_MAX_DIM_X
#define LIGHT_GRID_MAX_DIM_Y $LIGHT_GRID_MAX_DIM_Y
#endif

const float PBR_FACTOR = 2;
const float PBR_FACTOR_ENV = 1.5;

in vec3 eyeVec;
in vec3 v,vWorld,worldNormal;
in vec3 N;
in mat3 tbnMatrix,tbnMatrixObj;
in vec4 shadowCoord[MAX_SHADOWS*NUM_SPLITS];
in vec4 texCoords;
in mat3 worldRotMat;
in float alphaValue;

in float fogEnd;
in float fogScale;

struct Light{
	vec3 ambient; 
	vec3 diffuse;
	vec4 position;
	vec3 spotDirection; 
	float spotExponent;
	float spotCosPhi; 
	float spotCosTheta;       
	float constantAttenuation; 
	float linearAttenuation;  
	float quadraticAttenuation;
};

struct glFogParameters
{
	vec3 color;
	float density;
	float start;
	float end;
};

#if USE_CLUSTERED_SHADING == 1
//Has to be 16 byte aligned (otherwise it will be padded)
struct ClusteredLight{ 
	vec4 position;
	vec4 diffuse;		
};

layout (std140) uniform LightSourceClustered
{    
	ClusteredLight clusteredLights[NUM_POSSIBLE_LIGHTS];
};

uniform isamplerBuffer clusterLightIndexListsTex;
uniform usamplerBuffer clusterGridTex;
#endif

uniform glFogParameters glFog;
uniform Light glLightSource[8];
uniform sampler2D tex;
uniform sampler2D tex2;
uniform sampler2D heightMap;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D metalnessMap;
uniform sampler2D reflectionMap;
uniform sampler2D brdfLUT;
uniform sampler2DArrayShadow shadowMap[MAX_SHADOWS];
uniform float splitPlanes[NUM_SPLITS];

uniform samplerCube envMap;
uniform samplerCube irradianceMap;
uniform vec2 texture1Size;
uniform vec2 textureShadowSize;
uniform int lightCount;
uniform int shadowCount;
uniform int tex2Mode;
uniform int fogMode;
uniform float metalnessConst;
uniform float roughnessConst;
uniform float roughnessTextureBias;

uniform mat4 invWorldView;
uniform vec3 camPos;
uniform vec3 ambientColor;
uniform vec3 matAmbientColor;
uniform vec3 matDiffuseColor;
uniform vec3 matEmissiveColor;
uniform int alphaTestCompareMethod;
uniform float alphaTestValue;

uniform bool enableNormalMaps;
uniform bool enableParallaxMaps;
uniform bool enableFirstTexture;
uniform bool enableSecondTexture;
uniform bool enableFog;
uniform bool enableEnvMap;
uniform bool enableTex2Modulate2X;
uniform bool enableRoughnessMap;
uniform bool enableMetalnessMap;
uniform bool enableAlphaTest;
#if USE_CLUSTERED_SHADING == 1
uniform bool enableClusteredShading;
uniform float recNear; /**< 1.0f / g_near */
uniform	float recLogSD1; /**< 1.0f / logf(sD + 1.0f), used to compute cluster index. */
#endif



float HEIGHT_SCALE = 0.01;

const float PI = 3.141592654;

out vec4 diffuseColorOut;


float ShadowCalculation(const float bias, const int shadowMapIndex, const int lightIndex, const sampler2DArrayShadow shadowSampler)
{
	vec4 fragPosLightSpace = shadowCoord[lightIndex*NUM_SPLITS+shadowMapIndex];
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	#if USE_PCF == 1
		float currentDepth = clamp(projCoords.z,0,1);
		
		
		// Check whether current frag pos is in shadow
		//if(projCoords.z <= 1.0)
		{
			float shadow = 0.0;
			vec2 texelSize = 1.0 / vec2(textureShadowSize);
			for(int x = -1; x <= 1; ++x)
			{
				for(int y = -1; y <= 1; ++y)
				{
					float pcfDepth = clamp(texture(shadowSampler, vec4(projCoords.xy + vec2(x, y) * texelSize, shadowMapIndex, projCoords.z - bias)),0,1);
					shadow += pcfDepth;					
				}    
			}
			shadow *= 1.0/9.0;	
				return shadow;
		}
		//else
			//return 1;	
	#else
		return texture(shadowSampler, vec4(projCoords.xy, shadowMapIndex, projCoords.z - bias));
	#endif	
}  

float ShadowCalculationForAllSplitPlanes(const int lightIndex, const vec3 lightDirViewSpace, const vec3 normal, float lightIntensity, const sampler2DArrayShadow shadowSampler)
{			 
		float distance = v.z;		
		float bias = max(0.001 * (1.0 - dot(normal, lightDirViewSpace)), 0.0003);

		bool applied = false;
		
		for (int i = 0; i < NUM_SPLITS; i++)			
			if(!applied && (distance < splitPlanes[i]))
			{
				//bias = ((i+1)*0.1)*0.001;
				lightIntensity *= ShadowCalculation(bias, i, lightIndex, shadowSampler);
				applied=true;	
				//Graphics cards hate break (SLOW), hence boolean value above
				//break;
			}		
		return lightIntensity;
	
}

//Parallax mapping with offset limiting
//You can try different parallax mapping techniques, but they all have problems with steep angles so I decided this is something for later
vec2 ParallaxMapping(vec3 V, vec2 T)
{
   // get depth for this fragment
   float initialHeight = texture2D(heightMap, T).r-0.5;
   
   // calculate amount of offset for Parallax Mapping With Offset Limiting
   vec2 texCoordOffset = HEIGHT_SCALE * V.xy * initialHeight;

   // return modified texture coordinates
   return T + texCoordOffset;
} 

float DistributionGGX(float NdotH, float roughnessSquared)
{
	float a = roughnessSquared;
	float a2 = a*a;
	float NdotH2 = NdotH*NdotH;

	float nom   = a2;
	float denom = NdotH2 * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom + 0.0001;

	return nom / denom;
}

//"PBR Diffuse Lighting for GGX+Smith Microsurfaces", Earl Hammon, Jr.
float GeometrySmithG2HammonDenom(float NdotL, float NdotV, float roughnessSquared)
{
	float denom = mix(2*NdotL*NdotV,NdotL+NdotV,roughnessSquared);
	return denom;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta,0), 5.0);
}


vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}   

struct LightValues
{
	vec3 lightVec;	
	vec3 lightDir;
	vec3 lightDirViewSpace;
	float  attenFactor;
	float lightDist;	
};

LightValues CalculateLightValues(const int index)
{
	LightValues result;
	if(glLightSource[index].spotCosTheta != -1.)
	{
		result.lightVec	= glLightSource[index].position.xyz - v;
		result.lightDirViewSpace=normalize(result.lightVec);
		result.lightDist	= length(result.lightVec);
		result.lightDir	= normalize(result.lightVec);		
		
		float angle = max(dot(normalize(glLightSource[index].spotDirection), -result.lightDir),0);
		
		if (angle > glLightSource[index].spotCosPhi)
			{
			float spotAtten;
			if (angle < glLightSource[index].spotCosTheta) 
				spotAtten = (angle-glLightSource[index].spotCosPhi)/(glLightSource[index].spotCosTheta-glLightSource[index].spotCosPhi);
			else
				spotAtten = 1;
			
			
			if(glLightSource[index].spotExponent != 1.0)
				spotAtten = pow(spotAtten, glLightSource[index].spotExponent);
			
			result.attenFactor = spotAtten * min(1.0/ (glLightSource[index].constantAttenuation +
					glLightSource[index].linearAttenuation * result.lightDist +
					glLightSource[index].quadraticAttenuation * result.lightDist * result.lightDist),1);				
		}
		else
			result.attenFactor = 0.;		
	}
	else if (glLightSource[index].position.w != 0.0)
	{
		result.lightVec = glLightSource[index].position.xyz - v;
		result.lightDirViewSpace=normalize(result.lightVec);	
		result.lightDist	= length(result.lightVec);		
					
		if (enableNormalMaps)
			result.lightVec	= tbnMatrix*result.lightVec;
		result.lightDir	= normalize(result.lightVec);
		
		
		// positional light source		
		result.attenFactor	= clamp(1.0/(	glLightSource[index].constantAttenuation + 
					glLightSource[index].linearAttenuation * result.lightDist +
					glLightSource[index].quadraticAttenuation * result.lightDist * result.lightDist ),0,1);		
	}		
	else 
	{			
		// directional light source			
		result.attenFactor	= 1.0;			
		
		if (enableNormalMaps)
		{
			result.lightDirViewSpace=normalize(glLightSource[index].position.xyz);
			result.lightVec	= tbnMatrix*glLightSource[index].position.xyz;
			result.lightDir	= normalize(result.lightVec);
		}
		else
		{
			result.lightVec	= glLightSource[index].position.xyz;
			result.lightDir	= normalize(result.lightVec);
			result.lightDirViewSpace = result.lightDir;
		}
		result.lightDist	= length(result.lightVec);
	} 
	return result;
}

#if USE_CLUSTERED_SHADING == 1
LightValues CalculateLightValuesClustered(const int index)
{
	LightValues result;
	
	result.lightVec = clusteredLights[index].position.xyz - v;
	result.lightDirViewSpace=normalize(result.lightVec);	
	result.lightDist	= length(result.lightVec);		
				
	if (enableNormalMaps)
		result.lightVec	= tbnMatrix*result.lightVec;
	result.lightDir	= normalize(result.lightVec);		
		
	// positional light source
	float inner = 0.0;
	float att = max(1.0 - max(0.0, (result.lightDist - inner) / (clusteredLights[index].position.w - inner)), 0.0);
	result.attenFactor	= att;
	return result;
}
#endif

vec3 ApplyAmbientLight(const int index,const vec3 firstTexture, const float intensity, vec3 lightSum)
{
	vec3 IAmbient        = (matAmbientColor)*(PBR_FACTOR*glLightSource[index].ambient)*firstTexture;
	lightSum.rgb += IAmbient*intensity;
	return lightSum;
}

vec3 ApplyPBRLight(vec3 lightDiffuseColor, const LightValues lightValues, const vec3 mapNormal, const vec3 eyeV, const vec3 F0, const float roughness, const vec3 IDiffuse, const vec3 firstTexture, vec3 lightSum, const float metalness)
{	
	
	float visible = 1.0;	
	float intensity = lightValues.attenFactor;	
	
	//vec3 IAmbient        = (matAmbientColor.rgb)*(PBR_FACTOR*glLightSource[index].ambient)*firstTexture;
	//lightSum.rgb += IAmbient*intensity; 

	vec3 L = lightValues.lightDir;
	vec3 V = normalize(eyeV);
	vec3 H = normalize(L + V);

	vec3 reflected;	
	vec3 N = mapNormal;	

	float HdotV = max(dot(H, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0001);	
	float NdotH = max(dot(N, H), 0.0001);

	 vec3 F    = fresnelSchlick(HdotV, F0);	

	// Cook-Torrance BRDF
	float roughnessSquared = roughness*roughness;
	float NDF = DistributionGGX(NdotH, roughnessSquared);	
   
	vec3 nominator    = NDF * F;
	float denominator = 2.0 * GeometrySmithG2HammonDenom(NdotV, NdotL, roughnessSquared) + 0.00001; // 0.00001 to prevent divide by zero.
	vec3 specular = nominator/denominator;
	
	#ifndef SIMPLE_KD
		vec3 kD=mix(vec3(1.0)-F,vec3(0.0),metalness);
	#else
		float kD = 1.0 - metalness;
	#endif	
	
	#if USE_HQ_DIFFUSE == 1
		//GGX Diffuse Approximation from
		//"PBR Diffuse Lighting for GGX+Smith Microsurfaces, Earl Hammon, Jr."
		float LdotV = dot(L, V);			
		float facing = 0.5+0.5*LdotV;		
		float rough = facing*(0.9-0.4*facing)*(.5/(NdotH+0.05) + 1); //+0.05 to prevent too large values
		float smooth_ = 1.05*(1-pow(1-NdotL,5))*(1-pow(1-NdotV,5));
		float single = 1./PI*mix(smooth_, rough, roughnessSquared);
		float multi = 0.1159*roughnessSquared;
		vec3 diffuse = kD*IDiffuse.rgb*(single+IDiffuse.rgb*multi);
	#else
		vec3 diffuse = kD*IDiffuse.rgb/PI;
	#endif	
	
	lightSum+= PBR_FACTOR*lightDiffuseColor*(diffuse + specular)*intensity*NdotL;
	

		
return lightSum;		
}

vec3 ApplyLight(const int index, const LightValues lightValues, const vec3 mapNormal, const vec3 eyeV, const vec3 F0, const float roughness, const vec3 IDiffuse, const vec3 firstTexture, vec3 lightSum, const float metalness)
{
	lightSum=ApplyAmbientLight(index, firstTexture, lightValues.attenFactor, lightSum);
	lightSum=ApplyPBRLight(glLightSource[index].diffuse.rgb, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse, firstTexture, lightSum, metalness);
	return lightSum;
}

#if USE_CLUSTERED_SHADING == 1
//Clustered shading code partially from 
//"Demo code implementing Clustered Forward Shading, accompanying the talk 
//'Tiled and Clustered Forward Shading' presented at Siggraph 2012."

/**
 * Computes the {i,j,k} integer index into the cluster grid. For details see our paper:
 * 'Clustered Deferred and Forward Shading'
 * http://www.cse.chalmers.se/~olaolss/main_frame.php?contents=publication&id=clustered_shading
 */
ivec3 calcClusterLoc(vec2 fragPos, float viewSpaceZ)
{
	// i and j coordinates are just the same as tiled shading, and based on screen space position.
  ivec2 l = ivec2(int(fragPos.x) / LIGHT_GRID_TILE_DIM_X, int(fragPos.y) / LIGHT_GRID_TILE_DIM_Y);

	// k is based on the log of the view space Z coordinate.
	float gridLocZ = log(viewSpaceZ * recNear) * recLogSD1;

	return ivec3(l, int(gridLocZ));
}

/**
 * Linearlizes the 3D cluster location into an offset, which can be used as an
 * address into a linear buffer.
 */
int calcClusterOffset(ivec3 clusterLoc)
{
	return (clusterLoc.z * LIGHT_GRID_MAX_DIM_Y + clusterLoc.y) * LIGHT_GRID_MAX_DIM_X + clusterLoc.x;
}

/**
 * Convenience shorthand function, see above...
 */
int calcClusterOffset(vec2 fragPos, float viewSpaceZ)
{
	return calcClusterOffset(calcClusterLoc(fragPos, viewSpaceZ));
}

vec3 ApplyClusteredLights(const vec3 mapNormal, const vec3 eyeV, const vec3 F0, const float roughness, const vec3 IDiffuse, const vec3 firstTexture, vec3 lightSum, const float metalness)
{
  // fetch cluster data (i.e. offset to light indices, and numer of lights) from grid buffer.
  uvec2 offsetCount = texelFetch(clusterGridTex, calcClusterOffset(gl_FragCoord.xy, v.z)).xy; 

  int lightOffset = int(offsetCount.x);
  int lightCount = int(offsetCount.y);
	
  for (int i = 0; i < lightCount; ++i)
  {
		// fetch light index from list of lights for the cluster.
		int lightIndex = texelFetch(clusterLightIndexListsTex, lightOffset + i).x; 
		LightValues lightValues = CalculateLightValuesClustered(lightIndex);
		// compute and accumulate shading.		
		lightSum = ApplyPBRLight(clusteredLights[lightIndex].diffuse.rgb*2, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse, firstTexture, lightSum, metalness);
  }

  return lightSum;
}
#endif

void main(void)
{ 	
	float fogFactor = 0;
	if (enableFog && fogMode > 0)
	{		
		//Radial fog
		float z = length(v);
		if (fogMode == 3)
		{
			fogFactor = (fogEnd - z) * fogScale;			
		}
		else if (fogMode == 1)
		{		
			fogFactor = 1.0/exp(glFog.density * z);				
		} 
		else if (fogMode == 2)
		{					
			fogFactor = 1.0 /exp( (z * glFog.density)* (z * glFog.density));
		} 
		
		fogFactor = clamp(fogFactor, 0.0, 1.0);
	}
	
	vec3 eyeV=normalize(eyeVec);
	vec3 normal=normalize(N);	
	
	vec2 texCoordsProcessed;
	
#if USE_PARALLAX_MAPS == 1
	if (enableParallaxMaps)
		texCoordsProcessed = ParallaxMapping(eyeV, texCoords.st);
	else
#endif
		texCoordsProcessed = texCoords.st;	
		
	vec4 firstTexture;	
	vec4 lightSum = vec4(0);
	if (enableFirstTexture)
	{	
		firstTexture = texture2D(tex, texCoordsProcessed);		
		if (enableSecondTexture)
		{		  
		  if(tex2Mode == 0)
		  {
			firstTexture *= texture2D(tex2, texCoords.pq);			
			if (enableTex2Modulate2X)
				firstTexture.rgb*=4.6; //2^2.2 since rendering is gamma correct			
				
		  }
		  else
		  {		
			lightSum.rgb += texture2D(tex2, texCoords.pq).rgb;				
		  }   
		}	
		
	}
	else{
		firstTexture = vec4(1,1,1,1);
	}
	
	float alpha = alphaValue*firstTexture.a;		
	if (enableAlphaTest)
		if(alphaTestCompareMethod==5)
		{
			if( alpha <= alphaTestValue)
				discard;
		}
		else if(alphaTestCompareMethod==2)
		{
			if( alpha > alphaTestValue)
				discard;
		}
		else
			firstTexture =vec4(1,0,0,1);
	
	vec3 mapNormal;
	if (enableNormalMaps)
	{	
			vec3 textureNormal = texture2D(normalMap, texCoordsProcessed).rgb;
			textureNormal.xy	= (textureNormal.rg * 2.0 - 1.0);
			textureNormal.z = sqrt(1.0-dot(textureNormal.xy,textureNormal.xy));			
			mapNormal = normalize(textureNormal.xyz);
	}
	else
		mapNormal = normal;
	
	lightSum+=vec4(((matEmissiveColor.rgb))* firstTexture.rgb, alpha); 
	
	float metalness;	
	if (enableMetalnessMap)
	{
		metalness=texture2D(metalnessMap, texCoordsProcessed.st).r;	
	}
	else
	{
		metalness=metalnessConst;
		
	}	
	
	vec3 IDiffuse = (matDiffuseColor)*firstTexture.rgb;
	
	float roughness;
	if (enableRoughnessMap)
	{		
		roughness=clamp(roughnessTextureBias+texture2D(roughnessMap, texCoordsProcessed.st).r,0.0,1.0);
	}		
	else
	{
		roughness=roughnessConst;		
	}   
	
	#define DIELECTRIC_SPECULAR 0.04
	vec3 F0 = mix(vec3(DIELECTRIC_SPECULAR), IDiffuse, metalness);
	if (enableEnvMap)
	{	
		vec3 n;
		if (enableNormalMaps)
			n=normalize( worldRotMat * (tbnMatrixObj * mapNormal ));
		else
			n=normalize(worldNormal);
		vec3 v = normalize( camPos - vWorld);
		vec3 reflectDir = reflect(-v, n);
		const float MAX_REFLECTION_LOD = 7;
		float lod = roughness * (2.0 - roughness) * MAX_REFLECTION_LOD;	
		vec3 irradiance;
		vec3 radiance;
		if (enableFog)
		{	
			vec3 ambientTint=PBR_FACTOR_ENV*glFog.color.rgb;
			irradiance = vec3(texture(irradianceMap, n).r)*ambientTint;
			radiance = vec3(textureLod(envMap, reflectDir, lod).r)*ambientTint;
		}
		else
		{
			irradiance = texture(irradianceMap, n).rgb*PBR_FACTOR_ENV;
			radiance = textureLod(envMap, reflectDir, lod).rgb*PBR_FACTOR_ENV;
		}	
		
		float NdotV = max(dot(n,v),0.0);		
		vec2 brdf  = texture(brdfLUT, vec2(NdotV, roughness)).rg;
		
	#ifdef IBL_MULTIPLE_SCATTERING
		vec3 diffuseColor = mix(IDiffuse.rgb, vec3(0), metalness);
		// Multiple scattering, from:
		//"A Multiple-Scattering Microfacet Model for Real-Time Image-based Lighting", Carmelo J. Fdez-Agüera
		vec3 kS = F0;

		vec3 FssEss = kS * brdf.x + brdf.y;
		
		float Ess = brdf.x + brdf.y;
		float Ems = 1.0-Ess;
		vec3 Favg = F0 + (1.0-F0)/21.0;
		vec3 Fms = FssEss*Favg/(1.0-Ems*Favg);
		vec3 FmsEms = Fms * Ems;
		// Dielectrics
		vec3 Edss = 1.0 - (FssEss + FmsEms);
		vec3 kD = diffuseColor * Edss;
		// Composition
		lightSum.rgb += FssEss * radiance + (FmsEms+kD) * irradiance;
	#else
		vec3 diffuse    = irradiance * IDiffuse;
		vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
	#ifndef SIMPLE_KD		
		vec3 kD=mix(vec3(1.0)-F,vec3(0.0),metalness);
	#else
		float kD = 1.0 - metalness;
	#endif			

		vec3 specular = radiance * (F0 * brdf.x + brdf.y);
		lightSum.rgb += kD*diffuse + specular;
	#endif
	}	
	else
	{			
		if (enableNormalMaps)
		{
			vec3 dir = vec3(0,0,1);
			float ambientIntensity = dot(dir,mapNormal);
			lightSum.rgb+=matAmbientColor.rgb * ambientColor.rgb* ambientIntensity*IDiffuse.rgb;
		}
		else
			lightSum.rgb+=matAmbientColor.rgb * ambientColor.rgb* IDiffuse.rgb;
	}	

	//Unrolled loop and a separated shadow pass to help some glsl compilers. Also some glsl compilers need a constant value (not variable!) for sampler arrays.
	if(lightCount > 0)
	{
		const int lightIndex = 0;
		LightValues lightValues = CalculateLightValues(lightIndex);
		#if MAX_SHADOWS > 0
		 if (shadowCount > 0)
			lightValues.attenFactor = ShadowCalculationForAllSplitPlanes(lightIndex, lightValues.lightDirViewSpace, normal, lightValues.attenFactor, shadowMap[0]);
		#endif		
		lightSum.rgb = ApplyLight(lightIndex, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);
	}
	if(lightCount > 1)
	{
		const int lightIndex = 1;
		LightValues lightValues = CalculateLightValues(lightIndex);
		#if MAX_SHADOWS > 1
		 if (shadowCount > 1)
			lightValues.attenFactor = ShadowCalculationForAllSplitPlanes(lightIndex, lightValues.lightDirViewSpace, normal, lightValues.attenFactor, shadowMap[1]);
		#endif		
		lightSum.rgb = ApplyLight(lightIndex, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);
	}
	if (lightCount > 2)
	{
		const int lightIndex = 2;
		LightValues lightValues = CalculateLightValues(lightIndex);
		lightSum.rgb = ApplyLight(lightIndex, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);
	}
	if (lightCount > 3)
	{
		const int lightIndex = 3;
		LightValues lightValues = CalculateLightValues(lightIndex);
		lightSum.rgb = ApplyLight(lightIndex, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);
	}	
	if (lightCount > 4)
	{
		const int lightIndex = 4;
		LightValues lightValues = CalculateLightValues(lightIndex);
		lightSum.rgb = ApplyLight(lightIndex, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);
	}
	if (lightCount > 5)
	{
		const int lightIndex = 5;
		LightValues lightValues = CalculateLightValues(lightIndex);
		lightSum.rgb = ApplyLight(lightIndex, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);
	}
	if (lightCount > 6)
	{
		const int lightIndex = 6;
		LightValues lightValues = CalculateLightValues(lightIndex);
		lightSum.rgb = ApplyLight(lightIndex, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);
	}
	if (lightCount > 7)
	{
		const int lightIndex = 7;
		LightValues lightValues = CalculateLightValues(lightIndex);
		lightSum.rgb = ApplyLight(lightIndex, lightValues, mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);	
	}
	#if USE_CLUSTERED_SHADING == 1
	if(enableClusteredShading)
		lightSum.rgb = ApplyClusteredLights(mapNormal, eyeV, F0, roughness, IDiffuse.rgb, firstTexture.rgb, lightSum.rgb, metalness);	
	#endif
		
	vec4 finalColor = lightSum;

	
	if (enableFog && fogMode > 0)
	{		
		finalColor = mix(vec4(glFog.color,1.0),finalColor, fogFactor);
	}

	diffuseColorOut=vec4(ToGammaCorrected(finalColor.rgb),finalColor.a);
}
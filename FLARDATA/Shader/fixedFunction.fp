//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Fixed function shader for FLAR by Schmackbolzen

#version 330
#define TEXTURESTAGE_COUNT 2

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
	float range;
};

struct glFogParameters
{
	vec3 color;
	float density;
	float start;
	float end;
};


uniform Light glLightSource[8];
uniform glFogParameters glFog;
uniform sampler2D textures[TEXTURESTAGE_COUNT];
uniform samplerCube cubemap; 
uniform int colorOp[TEXTURESTAGE_COUNT];
uniform int alphaOp[TEXTURESTAGE_COUNT];
uniform int colorArg1[TEXTURESTAGE_COUNT];
uniform int colorArg2[TEXTURESTAGE_COUNT];
uniform int alphaArg1[TEXTURESTAGE_COUNT];
uniform int alphaArg2[TEXTURESTAGE_COUNT];
uniform bool textureBound[TEXTURESTAGE_COUNT];
uniform vec3 ambientColor;
uniform vec3 matAmbientColor;
uniform vec4 matDiffuseColor;
uniform vec3 matEmissiveColor;
uniform vec4 matSpecularColor;
uniform float alphaTestValue;
uniform int alphaTestCompareMethod;
uniform int lightCount;
uniform int texture1Index;
uniform int fogMode;

uniform bool enableVertexColor;
uniform bool vertexColorAvailable;
uniform bool specularVertexColorAvailable;
uniform bool enableLighting;
uniform bool enableAlphaTest;
uniform bool enableCubemap;
uniform bool enableRHWCoordinates;

in vec3 N;
in vec3 v;
in float RHWdepth;
in vec2 texCoords[2];
in vec4 vertexColor;
in vec4 vertexColorSecondary;
in float fogEnd;
in float fogScale;

#define D3DTA_DIFFUSE           0x00000000  // select diffuse color (read only)
#define D3DTA_CURRENT           0x00000001  // select stage destination register (read/write)
#define D3DTA_TEXTURE           0x00000002  // select texture color (read only)
#define D3DTA_TFACTOR           0x00000003  // select D3DRS_TEXTUREFACTOR (read only)
#define D3DTA_SPECULAR          0x00000004  // select specular color (read only)
#define D3DTA_TEMP              0x00000005  // select temporary register color (read/write)
#define D3DTA_COMPLEMENT        0x00000010  // take 1.0  x (read modifier)
#define D3DTA_ALPHAREPLICATE    0x00000020  // replicate alpha to color components (read modifier)

#define D3DTOP_DISABLE               1      // disables stage
#define     D3DTOP_SELECTARG1        2      // the default
#define     D3DTOP_SELECTARG2        3

    // Modulate
#define     D3DTOP_MODULATE          4      // multiply args together
#define     D3DTOP_MODULATE2X        5      // multiply and << 1 bit
#define     D3DTOP_MODULATE4X        6      // multiply and << 2 bits

    // Add
#define    D3DTOP_ADD                 7   // add arguments together
#define    D3DTOP_ADDSIGNED           8  // add with -0.5 bias
#define     D3DTOP_ADDSIGNED2X        9  // as above but left << 1 bit


vec3 ToLinear(vec3 inColor){	
	return pow(inColor,vec3(2.2));
}

vec3 ToGammaCorrected(vec3 inColor){
	return pow(inColor.rgb,1./vec3(2.2));
}
	
vec4 GetColorForStage(const int stage, int colorArg, int alphaArg, vec4 previousColor, vec4 gouradDiffuseColor, vec4 specularColor, vec2 texCoords, vec3 cubeMapTexCoords)
{
	vec4 color;
	vec4 textureColor;
	if(colorArg == D3DTA_TEXTURE || alphaArg == D3DTA_TEXTURE)
		if(textureBound[stage])
			//Use if because some glsl compilers can not propagate the constant stage even though it has a constant value
			//when the function gets called (0 or 1) and complain about missing glsl compliance for texture lookup (no constant index)
			if (stage==0)
				textureColor=texture2D(textures[0],texCoords);
			else
				if (!enableCubemap)
					textureColor=texture2D(textures[1],texCoords);
				else
					textureColor=texture(cubemap,cubeMapTexCoords);
		else
			textureColor=vec4(1);
	
	if(colorArg == D3DTA_DIFFUSE)
		color.rgb=gouradDiffuseColor.rgb;
	else if(colorArg == D3DTA_CURRENT)
		color.rgb=previousColor.rgb;
	else if(colorArg == D3DTA_TEXTURE)			
		color.rgb=textureColor.rgb;
	else if(colorArg == D3DTA_SPECULAR)
		color.rgb =specularColor.rgb;
	else
		color=vec4(1,0,0,1);
		
	if(alphaArg == D3DTA_DIFFUSE)
		color.a=gouradDiffuseColor.a;
	else if(alphaArg == D3DTA_CURRENT)
		color.a=previousColor.a;
	else if(alphaArg == D3DTA_TEXTURE)
		color.a=textureColor.a;
	else if(alphaArg == D3DTA_SPECULAR)
		color.a =specularColor.a;	
	else
		color=vec4(1,0,0,1);
	return color;
}

vec4 ApplyFunctionForStage(const int stage, vec4 previousColor, vec4 gouradDiffuseColor, vec4 specularColor, vec2 texCoords, vec3 cubeMapTexCoords)
{
	vec4 color1=GetColorForStage(stage,colorArg1[stage],alphaArg1[stage], previousColor, gouradDiffuseColor, specularColor, texCoords, cubeMapTexCoords);
	vec4 color2=GetColorForStage(stage,colorArg2[stage],alphaArg2[stage], previousColor, gouradDiffuseColor, specularColor, texCoords, cubeMapTexCoords);
	
	if (colorOp[stage]==D3DTOP_SELECTARG2)
	{
		color1.rgb = color2.rgb;
	}
	else if (colorOp[stage]==D3DTOP_MODULATE)
	{
		color1.rgb*=color2.rgb;		
	}
	else if (colorOp[stage]==D3DTOP_MODULATE2X)
	{		
		color1.rgb=color1.rgb*color2.rgb*4.6; //2^2.2 since rendering is gamma correct
	}
	else if (colorOp[stage]==D3DTOP_MODULATE4X)
	{
		color1.rgb=color1.rgb*color2.rgb*21.11; //4^2.2 since rendering is gamma correct
	}
	else if (colorOp[stage]==D3DTOP_ADD)
	{	
		color1.rgb=color1.rgb+color2.rgb;		
	}
	else if (colorOp[stage]==D3DTOP_ADDSIGNED)
	{
		color1.rgb=color1.rgb+color2.rgb-0.5;
	}
	else if (colorOp[stage]==D3DTOP_ADDSIGNED2X)
	{
		color1.rgb=(color1.rgb+color2.rgb-0.5)*4.6; //2^2.2 since rendering is gamma correct
	}
	
	if (alphaOp[stage]==D3DTOP_SELECTARG2)
	{
		color1.a = color2.a;
	}
	else if (alphaOp[stage]==D3DTOP_MODULATE)
	{	
		color1.a*=color2.a;
	}
	else if (alphaOp[stage]==D3DTOP_MODULATE2X)
	{
		color1.a=(color1.a*color2.a)*4.6; //2^2.2 since rendering is gamma correct
	}
	else if (alphaOp[stage]==D3DTOP_MODULATE4X)
	{
		color1.a=(color1.a*color2.a)*21.11; //4^2.2 since rendering is gamma correct
	}
	else if (alphaOp[stage]==D3DTOP_ADD)
	{
		color1.a=(color1.a+color2.a);
	}
	else if (alphaOp[stage]==D3DTOP_ADDSIGNED)
	{
		color1.a=color1.a+color2.a-0.5;
	}
	else if (alphaOp[stage]==D3DTOP_ADDSIGNED2X)
	{
		color1.a=(color1.a+color2.a-0.5)*4.6; //2^2.2 since rendering is gamma correct
	}
	
	return color1;
}

void main()
{	
	vec4 gouradDiffuseColor;
	vec4 specularColor;
	vec3 vertexAmbientColor;
	
	if (enableLighting)
	{
		if(enableVertexColor)
		{
			
			float alpha = vertexColorAvailable ? vertexColor.a : matDiffuseColor.a;
			if (specularVertexColorAvailable)
			{
				vertexAmbientColor=ToLinear(vertexColorSecondary.rgb);
				gouradDiffuseColor=vec4(ambientColor*vertexAmbientColor,alpha);
			}
			else
			{
				vertexAmbientColor=matAmbientColor.rgb;
				gouradDiffuseColor=vec4(ambientColor*vertexAmbientColor,alpha);
			}
		}
		else
		{
			vertexAmbientColor=matAmbientColor.rgb;
			gouradDiffuseColor=vec4(ambientColor*vertexAmbientColor,matDiffuseColor.a);
		}
		//FL never has specular lighting enabled
		specularColor=vec4(0,0,0,0);
	}
	else
	{
		if(vertexColorAvailable)
			gouradDiffuseColor=vec4(ToLinear(vertexColor.rgb),vertexColor.a);
		else
			gouradDiffuseColor=vec4(1,1,1,1);
		
		if(specularVertexColorAvailable)
			specularColor=vec4(ToLinear(vertexColorSecondary.rgb),vertexColor.a);
		else
			specularColor=vec4(0,0,0,0);
	}	
	
	vec3 normal=normalize(N);
	if (enableLighting)
	{
		gouradDiffuseColor.rgb+=matEmissiveColor;
		vec3 vertexDiffuseColor=vertexColorAvailable&&enableVertexColor ? ToLinear(vertexColor.rgb): matDiffuseColor.rgb;
		
		for (int index = 0; index < lightCount; index++)
		{			
			
			vec3 lightDir;
			float  attenFactor;
			// attenuation and light direction
			if(glLightSource[index].spotCosTheta != -1.)
			{
				lightDir	= normalize(glLightSource[index].position.xyz - v);
				
				float angle = max(dot(normalize(glLightSource[index].spotDirection), -lightDir),0);
				
				if (angle > glLightSource[index].spotCosPhi)
				{
					float spotAtten;
					if (angle < glLightSource[index].spotCosTheta) 
						spotAtten = (angle-glLightSource[index].spotCosPhi)/(glLightSource[index].spotCosTheta-glLightSource[index].spotCosPhi);
					else
						spotAtten = 1;					
					
					if(glLightSource[index].spotExponent != 1.0)
						spotAtten = pow(spotAtten, glLightSource[index].spotExponent);
						
					float dist	= distance(glLightSource[index].position.xyz, v);			
					attenFactor = spotAtten * min(1.0/ (glLightSource[index].constantAttenuation +
							glLightSource[index].linearAttenuation * dist +
							glLightSource[index].quadraticAttenuation * dist * dist),1);				
				}
				else
					attenFactor = 0.;		
			}
			else if (glLightSource[index].position.w != 0.0)
			{
				// positional light source
				float dist	= distance(glLightSource[index].position.xyz, v);
				if (dist <= glLightSource[index].range )					
				{
					attenFactor	= min(1.0/(	glLightSource[index].constantAttenuation +
								glLightSource[index].linearAttenuation * dist +
								glLightSource[index].quadraticAttenuation * dist * dist ),1);		
					lightDir	= normalize(glLightSource[index].position.xyz - v);	
				}
				else
					attenFactor = 0;
			}				
			else
			{			
				attenFactor = 1;
				lightDir	= normalize(glLightSource[index].position.xyz);
			}
			if(attenFactor > 0)
			{
				float intensity      = max(dot(normal,lightDir),0.0);	
				
				vec3 lightColorAmbient = glLightSource[index].ambient.rgb*vertexAmbientColor;	
				vec3 lightColorDiffuse = glLightSource[index].diffuse.rgb*vertexDiffuseColor*intensity;
				gouradDiffuseColor.rgb  += (lightColorAmbient + lightColorDiffuse)*attenFactor;				
			}
		}		
	}
	
	vec4 finalColor=gouradDiffuseColor;
	if (colorOp[0]!=D3DTOP_DISABLE)
	{
		vec3 cubeMapTexCoords;
		finalColor=ApplyFunctionForStage(0,finalColor,gouradDiffuseColor,specularColor,texCoords[0], cubeMapTexCoords);
		
		if (colorOp[1]!=D3DTOP_DISABLE)
		{
			
			if(enableCubemap)
				cubeMapTexCoords=reflect(v,normal);
			finalColor=ApplyFunctionForStage(1,finalColor,gouradDiffuseColor,specularColor,texCoords[1], cubeMapTexCoords);
		}
	}
	else
		finalColor=gouradDiffuseColor;
	
	if (enableAlphaTest)
		if(alphaTestCompareMethod==5)
		{
			if( finalColor.a <= alphaTestValue)
				discard;
		}
		else if(alphaTestCompareMethod==2)
		{
			if( finalColor.a > alphaTestValue)
				discard;
		}
		else
			finalColor =vec4(1,0,0,1);
		
	if (fogMode > 0)
	{
		float z;
		if(enableRHWCoordinates)
			z = RHWdepth;
		else
			z = gl_FragCoord.z / gl_FragCoord.w;

		const float LOG2E = 1.442695;
		float fogFactor;
		if (fogMode == 3)
		{
			fogFactor = (fogEnd - z) * fogScale;
		}
		else if (fogMode == 1)
		{		
			fogFactor =exp(-glFog.density * z);				
		} 
		else if (fogMode == 2)
		{		
			fogFactor = exp2(-glFog.density * glFog.density * z * z * LOG2E);	
		} 		
		else
			fogFactor = 1;		
		
		fogFactor = clamp(fogFactor, 0.0, 1.0);
		finalColor.rgb = mix(glFog.color.rgb,finalColor.rgb, fogFactor);
	}
	gl_FragColor = vec4(ToGammaCorrected(finalColor.rgb), finalColor.a);
}
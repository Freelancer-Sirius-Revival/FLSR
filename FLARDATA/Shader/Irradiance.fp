//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Code based on https://learnopengl.com/PBR/IBL/Specular-IBL

#version 330
in vec3 WorldPos;

uniform samplerCube environmentMap;
uniform bool convertToLinear;

const float PI = 3.14159265359;

vec3 ToGammaCorrected(vec3 inColor){
	return pow(inColor.rgb,1./vec3(2.2));
}

vec3 ToLinear(vec3 inColor){	
	return pow(inColor,vec3(2.2));
}

void main()
{		
    vec3 N = normalize(WorldPos);

    vec3 irradiance = vec3(0.0);   
    
    // tangent space calculation from origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples = 0.0f;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

			vec3 texColor=texture(environmentMap, sampleVec).rgb;	
            if(convertToLinear)
				texColor=ToLinear(texColor);
            irradiance += texColor * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    
    gl_FragColor = vec4(ToGammaCorrected(irradiance), 1.0);
}
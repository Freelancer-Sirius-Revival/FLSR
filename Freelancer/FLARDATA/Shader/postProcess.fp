//This shader file is part of FLAR - Advanced Renderer for Freelancer by Schmackbolzen
//If you use the supplied shader files you may not modify them unless you state in them what you changed
//and also mention the source or who the author is.
//Post processing shader for FLAR by Schmackbolzen
//Sources are mentioned in the comments

#version 330
#include "ColorConversion.inc"
#define USE_ACES_CURVE $USE_ACES_CURVE
#define USE_PBR_BLOOM $USE_PBR_BLOOM

in  vec2  TexCoords;
  
uniform sampler2D scene;
#if USE_PBR_BLOOM == 1
uniform sampler2D bloomtex;
#endif
uniform float exposure;
uniform float exposureLowerLimit;

//https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
//ACES fit by Steven Hill

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat = mat3(
	0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3(
     1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = ACESInputMat * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = ACESOutputMat * color;

    // Clamp to [0, 1]
    color = clamp(color,0.,1.);

    return color;
}

// Uchimura 2017, "HDR theory and practice"
// Math: https://www.desmos.com/calculator/gslcdxvipg
// Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
// Used in "Gran Turismo Sport" game
vec3 uchimura(vec3 x, float P, float a, float m, float l, float c, float b) {
  float l0 = ((P - m) * l) / a;
  float L0 = m - m / a;
  float L1 = m + (1.0 - m) / a;
  float S0 = m + l0;
  float S1 = m + a * l0;
  float C2 = (a * P) / (P - S1);
  float CP = -C2 / P;

  vec3 w0 = vec3(1.0 - smoothstep(0.0, m, x));
  vec3 w2 = vec3(step(m + l0, x));
  vec3 w1 = vec3(1.0 - w0 - w2);

  vec3 T = vec3(m * pow(x / m, vec3(c)) + b);
  vec3 S = vec3(P - (P - S1) * exp(CP * (x - S0)));
  vec3 L = vec3(m + a * (x - m));

  return T * w0 + L * w1 + S * w2;
}

vec3 uchimura(vec3 x) {
  const float P = 1.0;  // max display brightness
  const float a = 1.0;  // contrast
  const float m = 0.22; // linear section start
  const float l = 0.4;  // linear section length
  const float c = 1.33; // black
  const float b = 0.0;  // pedestal

  return uchimura(x, P, a, m, l, c, b);
}

#if USE_PBR_BLOOM == 1
const float bloomStrength = 0.2f;
vec4 bloom_new()
{
    vec4 hdrColor = texture(scene, TexCoords);	
	//hdrColor.rgb=vec3(0);
    vec3 bloomColor = texture(bloomtex, TexCoords).rgb;
	//Blend in linear space
	/*hdrColor.rgb=ToLinear(hdrColor.rgb);
	bloomColor=ToLinear(bloomColor);*/
	vec4 blended=vec4(mix(hdrColor.rgb, bloomColor, bloomStrength),hdrColor.a); // linear interpolation
	//Blend in gamma corrected space
	blended.rgb=ToLinear(blended.rgb);
    return blended;
}
#endif

void main()
{  
#if USE_PBR_BLOOM == 0
  vec4 color = texture2D(scene, TexCoords);
  //Not a nice solution to convert two times (ideally there would be none), but not easily solvable currently
  color.rgb = ToLinear(color.rgb);
#else
  vec4 color = bloom_new();
#endif
  //Exposure is chosen so that both curves have about the same brightness and the look is fine with very bright starspheres like in Tau-37
#if USE_ACES_CURVE == 1  
  color.rgb=ACESFitted((exposure)*color.rgb);
#else 
  color.rgb=uchimura((exposure)*color.rgb);
#endif
  
  color.rgb=ToGammaCorrected(color.rgb);  
  gl_FragColor =  color;  
 
}
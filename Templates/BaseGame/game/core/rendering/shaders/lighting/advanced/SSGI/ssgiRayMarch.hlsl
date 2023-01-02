#include "../../../postFX/postFx.hlsl"
#include "../../../shaderModel.hlsl"
#include "../../../shaderModelAutoGen.hlsl"
#include "../../../lighting.hlsl"
#include "../../../brdf.hlsl"


TORQUE_UNIFORM_SAMPLER2D(deferredBuffer, 0);
TORQUE_UNIFORM_SAMPLER2D(colorBuffer, 1);
TORQUE_UNIFORM_SAMPLER2D(matInfoBuffer, 2);
TORQUE_UNIFORM_SAMPLER2D(stochNorm, 3);
uniform float4 rtParams0;
uniform float4 vsFarPlane;
uniform float4x4 cameraToWorld;
uniform float4x4 invCameraMat;
uniform float2 oneOverTargetSize;

float screenEdgeFade(float2 screenPos)
{
  const float cb_fadeEnd =10.0;
  const float cb_fadeStart = 9.0;
  const float fadeDiffRcp = 1.0f / (cb_fadeEnd  - cb_fadeStart);
  float2 boundary = abs(screenPos*10.0 - float2(0.5f, 0.5f)/10.0 );
  float fadeOnBorder = 1.0f - saturate((boundary.x - cb_fadeStart) * fadeDiffRcp);
  fadeOnBorder *= 1.0f - saturate((boundary.y - cb_fadeStart) * fadeDiffRcp);
  fadeOnBorder = smoothstep(0.0f, 1.0f, fadeOnBorder);
  return fadeOnBorder;
}

float4 RayMarch(float3 dir, float3 screenPos, float stepSize, int stepCount)
{
	float mask = 0.0; 
	float3 samplePos = float3(screenPos.xy * 0.5 + 0.5, screenPos.z);
	float Depth = samplePos.z;
    float DepthDiff = 0;
	float curStep = stepSize;
	float prevDepth = samplePos.z;
	float prevDepthDiff = 0.0;
	float3 prevSamplePos = samplePos;
    float3 curDir = dir;
    float3 curColor = float3(0,0,0);
    float hitCount = 0;
    [unroll]
	for(int i = 0; i < stepCount; i++)
	{
		float4 normDepth = TORQUE_DEFERRED_UNCONDITION( deferredBuffer, samplePos.xy);
        float3 norm = normalize(mul(float4(normDepth.xyz,1),invCameraMat).xyz);
        Depth = normDepth.a;
		DepthDiff = Depth-prevDepth;
		if (samplePos.z <= 0.0001 || screenEdgeFade(screenPos.xy)<=0.0)
        {
            break;
        }
        else
        {
            if (DepthDiff>0.0) //we only care if we're going further into th screen
            {
                mask += 1.0-(DepthDiff*samplePos.z); 
                curColor += TORQUE_TEX2D(colorBuffer, samplePos.xy).rgb;
                curStep *= samplePos.z;
                hitCount++;
            }
            else
            {
                //bounce
                curDir = reflect(-curDir,norm.xyz);
            }
        }
		prevDepth = Depth;
		samplePos += curDir * curStep;
	}
    hitCount = max(hitCount,1.0);
	curColor /= hitCount;
    mask/=hitCount;
	return float4(curColor, mask)*screenEdgeFade(screenPos.xy);
}

float3 getView(float3 screenPos)
{
	float4 viewPos = mul(cameraToWorld, float4(screenPos, 1));
	return viewPos.xyz / viewPos.w;
}

float random (float2 uv) {
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453123); //simple random function
}

float4 main(PFXVertToPix IN) : SV_TARGET
{
   //unpack normal and linear depth 
   float4 normDepth = TORQUE_DEFERRED_UNCONDITION(deferredBuffer, IN.uv0.xy);

   //create surface
   float3 viewDir = TORQUE_TEX2D(stochNorm,IN.uv0.xy).rgb;
   Surface surface = createSurface(normDepth, TORQUE_SAMPLER2D_MAKEARG(colorBuffer),TORQUE_SAMPLER2D_MAKEARG(matInfoBuffer),
      IN.uv0.xy, eyePosWorld, IN.wsEyeRay, cameraToWorld);
   
   // SSGI raymarch - need this to feed into the uv coord for cubemaps
   float3 screenPos = float3(IN.uv0.xy * 2 - 1, normDepth.a);
      
   float stepSize = 1024.0*length(oneOverTargetSize);
   float jit = stepSize*(random(IN.uv0.xy)*2-1);
   stepSize = stepSize * jit;
   float3 dir = normalize(mul(float4(reflect(IN.wsEyeRay, viewDir),1), cameraToWorld)).xyz;

   float4 rayTrace = RayMarch(dir, screenPos, stepSize, 16);
   return float4(rayTrace.rgb,rayTrace.a);
}

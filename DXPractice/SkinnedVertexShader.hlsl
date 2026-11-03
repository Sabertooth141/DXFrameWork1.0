#define MAX_BONES 128 // must match maxBones in Skeleton.h

cbuffer Transform : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
}

cbuffer BonePalette : register(b1)
{
	matrix bones[MAX_BONES];
}

struct VSInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXTCOORD;
	uint4 boneIndices : BLENDINDICES;
	float4 boneWeights : BONEWEIGHT;
};

struct VSOutput
{
	float3 worldPos : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD;
	float4 pos : SV_POSITION;
};

VSOutput main(VSInput input)
{
	VSOutput output;

	float4x4 skin =
		bones[input.boneIndices.x] * input.boneWeights.x +
		bones[input.boneIndices.y] * input.boneWeights.y +
		bones[input.boneIndices.z] * input.boneWeights.z +
		bones[input.boneIndices.w] * input.boneWeights.w;

	float4 localPos = mul(float4(input.pos, 1), skin);
	float3 localNormal = mul(input.normal, (float3x3) skin);

	float4 worldPos = mul(localPos, model);
	output.worldPos = worldPos.xyz;

	output.normal = normalize(mul(localNormal, (float3x3) model));

	output.texcoord = input.texcoord;

	float4 clipPos = mul(worldPos, view);
	clipPos = mul(clipPos, projection);
	output.pos = clipPos;


	return output;
}

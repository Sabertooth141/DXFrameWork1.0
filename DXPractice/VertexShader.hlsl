cbuffer Transform : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
}

struct VSInput
{
	float3 pos : POSITION;
	float3 color : COLOR;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR;
};

VSOutput main(VSInput input)
{
	VSOutput output;

	float4 worldPos = mul(float4(input.pos, 1), model);
	worldPos = mul(worldPos, view);
	worldPos = mul(worldPos, projection);

	output.pos = worldPos;
	output.color = input.color;

	return output;
}

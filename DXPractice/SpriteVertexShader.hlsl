cbuffer CameraBuffer : register(b0)
{
    matrix view;
    matrix projection;
}

cbuffer ModelBuffer : register(b1)
{
    matrix world;
}

struct VSOut
{
    float2 uv : TEXCOORD;
    float4 pos : SV_POSITION;
};

VSOut main(float3 pos : POSITION, float3 normal : NORMAL, float2 uv : TEXCOORD)
{
    VSOut output;
    float4 posOut = float4(pos, 1);
	posOut = mul(pos, world);
    posOut = mul(pos, view);
    posOut = mul(pos, projection);
    output.pos = posOut;
    output.uv = uv;

    return output;
}
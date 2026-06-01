Texture2D tex : register(t0);
SamplerState sam : register(s0);

cbuffer SpriteBuffer : register(b2)
{
    float2 uvOffset;
    float2 uvScale;
};

float4 main(float2 uv : TEXCOORD) : SV_TARGET
{
    float2 finalUV = uvOffset + uv * uvScale;
    float4 color = tex.Sample(sam, finalUV);
    clip(color.a - 0.1f);
    return color;
}
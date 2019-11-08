struct PixelInput
{
	float4 position_out: SV_POSITION;
    float2 texcoord_out: TEXCOORD;
};

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
    float update_speed;
    float color_update_speed;
    float zoom_ratio;
    int iterations_per_zoom;
    int show_color;
};

cbuffer ConfigBuffer : register(b1)
{
    float4 colors[8];
}

Texture2D tex : register(t0);
Texture2D tex_m : register(t1);
Texture2D tex_old : register(t2);
Texture2D tex_new : register(t3);
SamplerState tex_sampler : register(s0);

float4 main(PixelInput input) : SV_TARGET
{
    float v = 0.0;
	float4 color = tex.Sample(tex_sampler, input.texcoord_out);
    if(show_color) {
        float f = tex_m.Sample(tex_sampler, input.texcoord_out).x;
        int idx1 = floor(f * 7);
        int idx2 = ceil(f * 7);
        //color.xyz = colors[idx2] * frac(f * 7) + colors[idx1] * (1 - frac(f * 7));
        color.xyz *= tex_m.Sample(tex_sampler, input.texcoord_out).x;
    } else {
        color.xyz = tex_m.Sample(tex_sampler, input.texcoord_out).x;
    }
    color.w = 1.0f;
    return color;
}
RWTexture2D<float4> tex_in: register(u0);
RWTexture2D<float4> tex_out: register(u1);
SamplerState tex_sampler : register(s0);

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
    float update_speed;
    float color_update_speed;
    float zoom_ratio;
};

[numthreads(32, 32, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID, uint gidx : SV_GroupIndex){
    int2 idx = dispatchThreadId.xy; 
    float2 relative_pos = idx - float2(world_width, world_height) / 2.0f;
    relative_pos *= zoom_ratio;

    int2 new_pos_0 = int2(floor(relative_pos.x), floor(relative_pos.y)) + float2(world_width, world_height) / 2.0f;
    int2 new_pos_1 = int2(ceil(relative_pos.x), floor(relative_pos.y)) + float2(world_width, world_height) / 2.0f;
    int2 new_pos_2 = int2(floor(relative_pos.x), ceil(relative_pos.y)) + float2(world_width, world_height) / 2.0f;
    int2 new_pos_3 = int2(ceil(relative_pos.x), ceil(relative_pos.y)) + float2(world_width, world_height) / 2.0f;
    float4 val_0 = tex_in[new_pos_0];
    float4 val_1 = tex_in[new_pos_1];
    float4 val_2 = tex_in[new_pos_2];
    float4 val_3 = tex_in[new_pos_3];
    float x_frac = frac(relative_pos.x);
    float y_frac = frac(relative_pos.y);
    float4 val01 = (1 - x_frac) * val_0 + x_frac * val_1;
    float4 val23 = (1 - x_frac) * val_2 + x_frac * val_3;
    float4 val = (1 - y_frac) * val01 + y_frac * val23;
    tex_out[idx] = val;
}
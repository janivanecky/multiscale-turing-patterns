RWTexture2D<float4> tex_in: register(u0);
RWTexture2D<float4> tex_out: register(u1);

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
};

cbuffer KernelSizeBuffer : register(b3) {
    int4 kernel_sizes;
}

[numthreads(32, 32, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID, uint3 dispatchThreadId : SV_DispatchThreadID) {
    int2 idx = dispatchThreadId.xy;
    float4 result = float4(0,0,0,0);
    float max_size = max(kernel_sizes[0], max(kernel_sizes[1], max(kernel_sizes[2], kernel_sizes[3])));
    if (max_size == 0){
        tex_out[idx] = tex_in[idx];
    } else {
        float4 weights = 1.0 / float4(kernel_sizes * 2.0f + 1.0f);
        for(int y = -max_size; y <= max_size; ++y) {
            int2 pos = idx + int2(0, y);
            if (pos.y < 0) {
                pos.y = world_height - 1;
            }
            pos = pos % int2(world_width, world_height);
            float4 v = tex_in[pos];
            float4 mask = clamp(sign(kernel_sizes - abs(y) + 1), 0, 1);
            result += v * weights * mask;
        }    
        tex_out[idx] = result;
    }
    
}
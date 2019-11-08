RWTexture2D<float1> tex_in: register(u0);
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
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID, uint3 dispatchThreadId : SV_DispatchThreadID){
    int2 idx = dispatchThreadId.xy;
    float4 result = float4(0,0,0,0);
    float max_size = max(kernel_sizes[0], max(kernel_sizes[1], max(kernel_sizes[2], kernel_sizes[3])));
    if (max_size == 0){
        tex_out[idx] = tex_in[idx];
    } else {
        float4 weights = 1.0 / float4(kernel_sizes * 2.0f + 1.0f);
        for(int x = -max_size; x <= max_size; ++x) {
            int2 pos = idx + int2(x, 0);
            if (pos.x < 0) {
                pos.x = world_width - 1;
            }
            pos = pos % int2(world_width, world_height);

            float v_ = tex_in[pos];
            float4 v = float4(v_,v_,v_,v_);
            float4 mask = clamp(sign(kernel_sizes - abs(x) + 1), 0, 1);
            result += v * weights * mask;
        }    
        tex_out[idx] = result;
    }
}
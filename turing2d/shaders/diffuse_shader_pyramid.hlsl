RWTexture2D<float1> tex_in: register(u0);
RWTexture2D<float4> tex_out: register(u1);

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
    float update_speed;
    float color_update_speed;
    float zoom_ratio;
    int iterations_per_zoom;
    int show_color;
    int wrap_bounds;
};

cbuffer KernelSizeBuffer : register(b3) {
    int4 kernel_sizes;
};

int2 wrap(int2 pos, int2 minb, int2 maxb) {
    if(pos.x < minb.x) {
        if(wrap_bounds) pos.x += maxb.x - minb.x; else pos.x = minb.x;
    }
    if(pos.y < minb.y) {
        if(wrap_bounds) pos.y += maxb.y - minb.y; else pos.y = minb.y;
    }
    if(pos.x >= maxb.x) {
        if(wrap_bounds) pos.x -= maxb.x - minb.x; else pos.x = maxb.x;
    }
    if(pos.y >= maxb.y) {
        if(wrap_bounds) pos.y -= maxb.y - minb.y; else pos.y = maxb.y;
    }
    return pos;
}

[numthreads(32, 32, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID, uint3 dispatchThreadId : SV_DispatchThreadID){
    int2 idx = dispatchThreadId.xy;
    float4 result = 0;

    for (int k = 0; k < 4; ++k) {
        float kernel_size = kernel_sizes[k];
        float level = clamp(log2(kernel_size), 0, log2(world_width));
        
        int level1 = floor(level);
        int level2 = ceil(level);
        int2 size1 = int2(world_width, world_height);
        int2 start1 = 0;
        for(int i = 0; i < level1; ++i) {
            start1.x += size1.x;
            size1 /= 2;
        }
        int2 size2 = size1 / 2;
        int2 start2 = start1 + int2(size1.x, 0);
        
        float2 ideal_size = float2(world_width, world_height) / pow(2, level);
        float2 pos = idx / pow(2, level);

        float2 pos1 = pos / ideal_size * float2(size1);
        float2 pos2 = pos / ideal_size * float2(size2);

        int2 min_boundary1 = start1;
        int2 max_boundary1 = start1 + size1;

        int2 min_boundary2 = start2;
        int2 max_boundary2 = start2 + size2;

        int2 pos11 = wrap(start1 + int2(floor(pos1.x), floor(pos1.y)), min_boundary1, max_boundary1);
        int2 pos12 = wrap(start1 + int2(floor(pos1.x), ceil(pos1.y)), min_boundary1, max_boundary1);
        int2 pos13 = wrap(start1 + int2(ceil(pos1.x), floor(pos1.y)), min_boundary1, max_boundary1);
        int2 pos14 = wrap(start1 + int2(ceil(pos1.x), ceil(pos1.y)), min_boundary1, max_boundary1);

        int2 pos21 = wrap(start2 + int2(floor(pos2.x), floor(pos2.y)), min_boundary2, max_boundary2);
        int2 pos22 = wrap(start2 + int2(floor(pos2.x), ceil(pos2.y)), min_boundary2, max_boundary2);
        int2 pos23 = wrap(start2 + int2(ceil(pos2.x), floor(pos2.y)), min_boundary2, max_boundary2);
        int2 pos24 = wrap(start2 + int2(ceil(pos2.x), ceil(pos2.y)), min_boundary2, max_boundary2);
        
        float val11 = tex_in[pos11];
        float val12 = tex_in[pos12];
        float val13 = tex_in[pos13];
        float val14 = tex_in[pos14];

        float val21 = tex_in[pos21];
        float val22 = tex_in[pos22];
        float val23 = tex_in[pos23];
        float val24 = tex_in[pos24];

        float val112 = val11 * (1 - frac(pos1.y)) + val12 * frac(pos1.y);
        float val134 = val13 * (1 - frac(pos1.y)) + val14 * frac(pos1.y);
        float val11234 = val112 * (1 - frac(pos1.x)) + val134 * frac(pos1.x);

        float val212 = val21 * (1 - frac(pos2.y)) + val22 * frac(pos2.y);
        float val234 = val23 * (1 - frac(pos2.y)) + val24 * frac(pos2.y);
        float val21234 = val212 * (1 - frac(pos2.x)) + val234 * frac(pos2.x);

        float val = val11234 * (1 - frac(level)) + val21234 * frac(level);
        result[k] = val;
    }

    tex_out[idx] = result;
}
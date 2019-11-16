RWTexture2D<float1> tex_in: register(u0);
RWTexture2D<float1> tex_out: register(u1);

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

cbuffer PyramidBuffer : register(b3) {
    int pyramid_level;
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
    if (pyramid_level == 0) {
        int2 idx = dispatchThreadId.xy;
        tex_out[idx] = tex_in[idx];
    } else {
        int2 out_idx = dispatchThreadId.xy;
        int2 in_idx = dispatchThreadId.xy * 2;
        float4 result = float4(0,0,0,0);
        int2 start_pos = 0;
        int2 pyramid_size = int2(world_width, world_height);
        for (int i = 0; i < pyramid_level; ++i) {
            start_pos.x += pyramid_size.x;
            pyramid_size /= 2;
        }
        int2 min_bounds = int2(start_pos.x - pyramid_size.x * 2, 0);
        int2 max_bounds = min_bounds + pyramid_size * 2;
        out_idx += start_pos;

        float val = 0.0f;
        
        // Gaussian weights
        float weights[4] = {0.13,0.37,0.37,0.13};
        // Box blur weights
        //float weights[4] = {0.25,0.25,0.25,0.25};
        for (int y = -1; y <= 2; ++y) {
            for (int x = -1; x <= 2; ++x) {
                int2 pos = min_bounds + in_idx + int2(x, y);
                pos = wrap(pos, min_bounds, max_bounds);
                val += tex_out[pos] * weights[x + 1] * weights[y + 1];
            }
        }
        tex_out[out_idx] = val;
    }
}
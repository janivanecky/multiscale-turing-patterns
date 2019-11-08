RWTexture2D<float4> tex_in: register(u0);
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

cbuffer SymmetryBuffer : register(b2) {
    int4 symmetries;
}

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
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID){
    uint2 idx = dispatchThreadId.xy;
    float2 center = float2(world_width / 2.0f, world_height / 2.0f);
    float2 relative_pos = float2(idx) - center;
    float pi = 3.14159265359;
    float angle = atan2(relative_pos.y, relative_pos.x);
    float4 val = 0.0f;
    for(int s = 0; s < 4; ++s) {
        int symmetry_num = symmetries[s];
        if(symmetry_num > 1) {
            for(int i = 0; i < symmetry_num; ++i) {
                float angle2 = angle + pi * 2.0f / symmetry_num * i;
                int2 other_pos = round(float2(sin(angle2), cos(angle2)) * length(relative_pos)) + center;
                other_pos = wrap(other_pos, int2(0,0), int2(world_width, world_height));
                float4 v = tex_in[other_pos];
                val[s] += v[s] / float(symmetry_num);
            }
        } else {
            float4 v = tex_in[idx];
            val[s] = v[s];
        }
    }
    tex_out[idx] = val;
}
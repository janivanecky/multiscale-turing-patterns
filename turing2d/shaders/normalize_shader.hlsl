RWTexture2D<float> tex_in: register(u0);

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
};

groupshared half vals_max[1024];
groupshared half vals_min[1024];

[numthreads(32, 32, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID, uint gidx : SV_GroupIndex){
    int2 size = int2(world_width, world_height) / 32;
    int2 idx = dispatchThreadId.xy * size; 
    float max_val = 0.0f;
    float min_val = 10000.0f;
    for (int y = 0; y < size.y; y++) {
        for (int x = 0; x < size.x; x++) {
            int2 pos = idx + int2(x, y);
            max_val = max(max_val, tex_in[pos]);
            min_val = min(min_val, tex_in[pos]);
        }
    }
    vals_min[gidx] = min_val;
    vals_max[gidx] = max_val;
    GroupMemoryBarrierWithGroupSync();
    
    uint max_idx = 512;
    for (; max_idx > 0; max_idx /= 2) {
        if (gidx < max_idx) {
            half v1 = vals_max[gidx];
            half v2 = vals_max[gidx + max_idx];
            half v_max = max(v1, v2);

            v1 = vals_min[gidx];
            v2 = vals_min[gidx + max_idx];
            half v_min = min(v1, v2);

            vals_max[gidx] = v_max;
            vals_min[gidx] = v_min;
        }
        GroupMemoryBarrierWithGroupSync();
    }
    
    for (int y = 0; y < size.y; y++) {
        for (int x = 0; x < size.x; x++) {
            int2 pos = idx + int2(x, y);
            tex_in[pos] = (tex_in[pos] - vals_min[0]) / (vals_max[0] - vals_min[0]);
        }
    }
}
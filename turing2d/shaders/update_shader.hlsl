RWTexture2D<float4> activators1: register(u0);
RWTexture2D<float4> activators2: register(u1);
RWTexture2D<float4> inhibitors1: register(u2);
RWTexture2D<float4> inhibitors2: register(u3);
RWTexture2D<float> grid: register(u4);
RWTexture2D<float4> grid_colors: register(u5);

cbuffer ConfigBuffer : register(b0)
{
    int world_width;
    int world_height;
    float update_speed;
    float color_update_speed;
};

cbuffer ConfigBuffer : register(b1)
{
    float4 colors[8];
}

cbuffer UpdateValsBuffer : register(b4)
{
    float4 update_vals[2];
}

[numthreads(32, 32, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID){
    uint2 idx = dispatchThreadId.xy;
    float4 ac1 = activators1[idx];
    float4 ac2 = activators2[idx];
    float4 in1 = inhibitors1[idx];
    float4 in2 = inhibitors2[idx];
    float4 variation1 = abs(ac1 - in1);
    float4 variation2 = abs(ac2 - in2);

    float min_variation1 = min(variation1.x, min(variation1.y, min(variation1.z, variation1.w)));
    float min_variation2 = min(variation2.x, min(variation2.y, min(variation2.z, variation2.w)));
    float min_variation = min(min_variation1, min_variation2);

    float as[8] = {
        ac1.x, ac1.y, ac1.z, ac1.w, ac2.x, ac2.y, ac2.z, ac2.w
    };
    float is[8] = {
        in1.x, in1.y, in1.z, in1.w, in2.x, in2.y, in2.z, in2.w
    };
    float vals[8] = {
        update_vals[0][0], update_vals[0][1], update_vals[0][2], update_vals[0][3],
        update_vals[1][0], update_vals[1][1], update_vals[1][2], update_vals[1][3],
    };
    float variations[8] = {
        variation1.x, variation1.y, variation1.z, variation1.w, variation2.x, variation2.y, variation2.z, variation2.w
    };
    float a, i, val;
    float3 col;
    for(int c = 0; c < 8; ++c) {
        if (min_variation == variations[c]) {
            a = as[c];
            i = is[c];
            val = vals[c];
            col = colors[c];
            break;
        }
    }
    float m = sign(a - i);
    grid[idx] += m * val * update_speed;
    float4 color = lerp(grid_colors[idx], float4(col, 1.0f), val * color_update_speed);
    grid_colors[idx] = color;
}
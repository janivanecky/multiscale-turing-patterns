RWTexture2D<float> activators: register(u0);
RWTexture2D<float> inhibitors: register(u1);
RWTexture2D<float> variations: register(u2);


[numthreads(1, 1, 1)]
void main(uint3 threadIDInGroup : SV_GroupThreadID, uint3 groupID : SV_GroupID,
          uint3 dispatchThreadId : SV_DispatchThreadID){
    uint2 idx = dispatchThreadId.xy;
    variations[idx] = variations[idx] + abs(activators[idx] - inhibitors[idx])
}
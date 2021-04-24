
StructuredBuffer<float4> InputPos : register(t0);
RWTexture2D<float4> OutputColors : register(u1);

[numthreads(1, 1, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    OutputColors[DTid.xy] = InputPos[DTid.x];
}


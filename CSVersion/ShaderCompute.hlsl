float rand(inout float2 uv)
{
    float2 noise = (frac(sin(dot(uv ,float2(12.9898,78.233)*2.0)) * 43758.5453));
    uv = noise;

    return abs(noise.x + noise.y) * 0.5;
}

RWTexture2D<float4> OutputColors : register(t0);

[numthreads(32, 32, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    //float2 rs = DTid.xy;

    //float r = rand(rs);
    //float g = rand(rs);
    //float b = rand(rs);

    float r = DTid.x / (float)256;
    float g = DTid.y / (float)256;
    float b = 0.25;

    OutputColors[DTid.xy] = float4(r,g,b,1.0);
}


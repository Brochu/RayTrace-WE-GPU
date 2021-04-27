cbuffer PerFrame : register(b0)
{
	float4x4 viewProj;
	float4x4 invViewProj;

	float4 time;
}

////////////////////////////////////////
// Ray
struct Ray
{
    float3 orig;
    float3 dir;
};

float3 ray_at(Ray r, float t)
{
    return r.orig + t * r.dir;
}

Ray get_ray(float s, float t)
{
    // Build the ray and move it based on view and projection
    Ray r;
    r.orig = mul(float4(0, 0, 0, 1), invViewProj);
    float4 at = mul(float4((s * 2) - 1, (t * 2) - 1, -1.0, 1), invViewProj);
    r.dir = normalize(at.xyz - r.orig);

    return r;
}
////////////////////////////////////////

////////////////////////////////////////
// Hit
struct Hit
{
    float3 p;
    float3 normal;
    float t;
    bool front_face;
};

void set_face_normal(Ray r, float3 outward_normal, inout Hit h)
{
    h.front_face = dot(r.dir, outward_normal) < 0;
    h.normal = (h.front_face) ? outward_normal : -outward_normal;
}
////////////////////////////////////////

////////////////////////////////////////
// Random funcs
float rand(inout float2 uv)
{
    float2 noise = (frac(sin(dot(uv ,float2(12.9898,78.233)*2.0)) * 43758.5453));
    uv = noise;

    return abs(noise.x + noise.y) * 0.5;
}
////////////////////////////////////////

////////////////////////////////////////
// Geometry funcs
bool hit_sphere(float4 sphere, Ray r, float t_min, float t_max, out Hit h)
{
    float3 oc = r.orig - sphere.xyz;
    float a = length(r.dir) * length(r.dir);
    float half_b = dot(oc, r.dir);
    float c = length(oc) * length(oc) - (sphere.w * sphere.w);

    float d = half_b*half_b - a*c;

    if (d < 0) return false;
    float sqrtd = sqrt(d);

    // Find nearest root in range
    float root = (-half_b - sqrtd) / a;
    if (root < t_min || t_max < root)
    {
        root = (-half_b + sqrtd) / a;
        if (root < t_min || t_max < root)
        {
            return false;
        }
    }

    h.t = root;
    h.p = ray_at(r, h.t);
    float3 outward_normal = (h.p - sphere.xyz) / sphere.w;
    set_face_normal(r, outward_normal, h);

    return true;
}
////////////////////////////////////////

float4 sample_color(Ray r)
{
    Hit h;
    float4 sphere = float4(0,0,-1,0.5);
    float4 ground = float4(0,-100,0,100);

    if (hit_sphere(sphere, r, 0.0001, 1.#INF, h))
    {
        //return float4(h.normal, 1.0);
        return float4(0.8, 0.2, 0.5, 1.0);
    }

    if (hit_sphere(ground, r, 0.0001, 1.#INF, h))
    {
        //return float4(h.normal, 1.0);
        return float4(0.2, 0.2, 0.8, 1.0);
    }

    return float4(0.1, 0.8, 0.4, 1.0);
}

RWTexture2D<float4> OutputColors : register(t0);

[numthreads(32, 32, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
    float u = DTid.x / (float)256;
    float v = DTid.y / (float)256;
    Ray r = get_ray(u, v);
    OutputColors[DTid.xy] = sample_color(r);

    //float2 seed = DTid.xy * frac(time.z);
    //float rr = rand(seed);
    //float rg = rand(seed);
    //float rb = rand(seed);
    //OutputColors[DTid.xy] = float4(rr, rg, rb, 1);
    //OutputColors[DTid.xy] = float4(u * abs(sin(time.x+1)), 0.2, v * abs(cos(time.x)), 1);
}


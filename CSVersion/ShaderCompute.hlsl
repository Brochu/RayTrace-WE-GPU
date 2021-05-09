#define PI 3.1415926535897932385;

cbuffer PerFrame : register(b0)
{
    float4 time;
    float4 perspectiveVals;
    float4 currSamples;

    float4x4 viewVals;
}

cbuffer WorldDef : register(b1)
{
    float4 sceneValues; // Values: Object Count, Max Ray Depth , Ray Per Pixels, unused... 
    float4 spheres[512];

    float4 matTypes[128];
    float4 matValues[512];
};

////////////////////////////////////////
// Random funcs
uint baseHash(uint2 p)
{
    p = 1103515245U*((p >> 1U)^(p.yx));
    uint h32 = 1103515245U*((p.x)^(p.y>>3U));
    return h32^(h32 >> 16);
}

float hash1(inout float seed)
{
    uint n = baseHash(asuint(float2(seed += 0.1,seed += 0.1)));
    return float(n) / float(0xffffffffU);
}

float2 hash2(inout float seed)
{
    uint n = baseHash(asuint(float2(seed += 0.1,seed += 0.1)));
    uint2 rz = uint2(n, n * 48271U);
    return float2(rz.xy & uint2(0x7fffffffU, 0x7fffffffU)) / float(0x7fffffff);
}

float3 hash3(inout float seed)
{
    uint n = baseHash(asuint(float2(seed += 0.1,seed += 0.1)));
    uint3 rz = uint3(n, n * 16807U, n * 48271U);
    return float3(rz & uint3(0x7fffffffU, 0x7fffffffU, 0x7fffffffU)) / float(0x7fffffff);
}

float2 random_in_unit_disk(inout float seed)
{
    float2 hash = hash2(seed) * float2(1.0, 6.28318530718);
    float phi = hash.y;
    float r = sqrt(hash.x);

    return r * float2(sin(phi), cos(phi));
}

float3 random_in_unit_sphere(inout float seed)
{
    float3 hash = hash3(seed) * float3(2.0, 6.28318530718, 1.0) - float3(1.0, 0.0, 0.0);
    float phi = hash.y;
    float r = pow(hash.z, 1.0 / 3.0);

    return r * float3(sqrt(1.0 - hash.x * hash.x) * float2(sin(phi), cos(phi)), hash.x);
}

////////////////////////////////////////
// Vector
bool near_zero(float3 e)
{
    float s = 0.000000001;
    return (abs(e.x) < s) && (abs(e.y) < s) && (abs(e.z) < s);
}

float3 reflect(float3 v, float3 n)
{
    return v - 2 * dot(v, n) * n;
}

float3 refract(float3 uv, float3 n, float ratio)
{
    float cos_theta = min(dot(-uv, n), 1.0);
    float3 r_out_perp =  ratio * (uv + cos_theta*n);
    float3 r_out_parallel = -sqrt(abs(1.0 - length(r_out_perp) * length(r_out_perp))) * n;

    return r_out_perp + r_out_parallel;
}

float reflectance(float cos, float ref_idx)
{
    // Schlick approx for reflectance
    float r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;

    return r0 + (1 - r0)*pow((1 - cos), 5);
}

float3 toGamma(float3 i)
{
    float ratio = 1.0 / 2.2;
    return pow(i, float3(ratio, ratio, ratio));
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

Ray get_ray(float s, float t, inout float p)
{
    Ray r =
    {
        viewVals[0].xyz,
        viewVals[3].xyz + s*viewVals[1].xyz + t*viewVals[2].xyz - viewVals[0].xyz
    };

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

    float mat_type;
    int mat_vals_idx;
};

void set_face_normal(Ray r, float3 outward_normal, uint sphere_idx, inout Hit h)
{
    h.front_face = dot(r.dir, outward_normal) < 0;
    h.normal = (h.front_face) ? outward_normal : -outward_normal;

    h.mat_type = matTypes[sphere_idx / 4][sphere_idx % 4];
    h.mat_vals_idx = sphere_idx;
}
////////////////////////////////////////

////////////////////////////////////////
// Geometry funcs
bool hit_sphere(int sphere_idx, Ray r, float t_min, float t_max, out Hit h)
{
    float4 sphere = spheres[sphere_idx];

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
    set_face_normal(r, outward_normal, sphere_idx, h);

    return true;
}

bool hit_world(Ray r, float t_min, float t_max, out Hit h)
{
    Hit temp_hit;
    bool hit_anything = false;
    float closest_so_far = t_max;

    for (int i = 0; i < sceneValues.x; ++i)
    {
        if (hit_sphere(i, r, t_min, closest_so_far, temp_hit))
        {
            hit_anything = true;
            closest_so_far = temp_hit.t;
            h = temp_hit;
        }
    }

    return hit_anything;
}

bool scatter(Ray rIn, Hit rec, inout float p, out float3 atten, out Ray scattered)
{
    if (rec.mat_type == 0) // DIFFUSE
    {
        float3 target = rec.p + rec.normal + random_in_unit_sphere(p);
        Ray r = { rec.p, normalize(target - rec.p) };
        scattered = r;

        atten = matValues[rec.mat_vals_idx].xyz;
        return true;
    }

    if (rec.mat_type == 1) // METAL
    {
        float3 refl = reflect(rIn.dir, rec.normal);
        Ray r = { rec.p, normalize(refl + matValues[rec.mat_vals_idx].w * random_in_unit_sphere(p)) };
        scattered = r;

        atten = matValues[rec.mat_vals_idx].xyz;
        return true;
    }

    if (rec.mat_type == 2) // DIELECTRIC
    {
        atten = float3(1, 1, 1);
        float refrac_ratio = rec.front_face ? (1.0 / matValues[rec.mat_vals_idx].w) : matValues[rec.mat_vals_idx].w;

        float3 unit_dir = normalize(rIn.dir);
        float cosine = min(dot(-unit_dir, rec.normal), 1.0);
        float sine = sqrt(1.0 - cosine * cosine);

        bool cant = refrac_ratio * sine > 1.0;
        float3 direction;

        if (cant || reflectance(cosine, refrac_ratio) > hash1(p))
            direction = reflect(unit_dir, rec.normal);
        else
            direction = refract(unit_dir, rec.normal, refrac_ratio);

        Ray r = { rec.p, direction };
        scattered = r;
        return true;
    }

    return false;
}
////////////////////////////////////////

float3 sample_color(Ray r, inout float p)
{
    Ray cur_ray = r;
    Hit h;
    float3 col = float3(1,1,1);
    for (int i = 0; i < sceneValues.y; ++i)
    {
        if (hit_world(cur_ray, 0.001, 1.#INF, h))
        {
            Ray scattered;
            float3 attenuation;

            if (scatter(cur_ray, h, p, attenuation, scattered))
            {
                col *= attenuation;
                cur_ray = scattered;
            }
            else
            {
                return float3(0, 0, 0);
            }
        }
        else
        {
            float3 unit_dir = normalize(cur_ray.dir);
            float t = 0.5 * (unit_dir.y + 1);
            float3 sky = (1.0-t)*float3(1.0, 1.0, 1.0) + t*float3(0.5, 0.7, 1.00);

            return col * sky;
        }
    }
    return float3(0, 0, 0);
}

RWTexture2D<float4> OutputColors : register(t0);

[numthreads(32, 32, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    float2 img_dim = float2(perspectiveVals.w, perspectiveVals.w / perspectiveVals.y);
    float p = float(baseHash(asint(DTid.xy)))/float(0xffffffffU);
    //p += time.x;

    //float rr = rand(p);
    //float rg = rand(p);
    //float rb = rand(p);
    //OutputColors[DTid.xy] = float4(rr, rg, rb, 1);

    float3 accColor = float3(0,0,0);
    for (int i = 0; i < sceneValues.z; ++i)
    {
        float u = (DTid.x + hash2(p).x*1.1) / (img_dim.x-1);
        float v = (DTid.y + hash2(p).y*1.1) / (img_dim.y-1);

        accColor += sample_color(get_ray(u, v, p), p);
    }

    accColor /= (sceneValues.z);
    accColor = toGamma(accColor);
    OutputColors[DTid.xy] = float4(accColor, 1.0);
}

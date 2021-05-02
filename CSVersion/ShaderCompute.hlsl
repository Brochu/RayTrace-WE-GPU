#define PI 3.1415926535897932385;

cbuffer PerFrame : register(b0)
{
    float4x4 viewMat;
    float4x4 invViewMat;

    float4 time;
}

cbuffer WorldDef : register(b1)
{
    float4 sceneValues; // Values: Object Count, Max Ray Depth , Ray Per Pixels, unused... 
    float4 spheres[64];

    float4 matTypes[16];
    float4 matValues[64];
};

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

float3 refract(float3 v, float3 n, float ratio)
{
    float cos_theta = min(dot(-v, n), 1.0);
    float3 r_out_perp = ratio * (v + cos_theta * n);
    float3 r_out_para = -sqrt(abs(1.0 - length(r_out_perp) * length(r_out_perp))) * n;

    return r_out_perp + r_out_para;
}

float reflectance(float cos, float ref_idx)
{
    // Schlick approx for reflectance
    float r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;

    return r0 + (1 - r0)*pow((1 - cos), 5);
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
    r.orig = mul(float4(0, 0, 0, 1), invViewMat).xyz;
    float4 at = mul(float4((s * 2) - 1, (t * 2) - 1, -1.0, 1), invViewMat);
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

    float mat_type;
    int mat_vals_idx;
};

void set_face_normal(Ray r, float3 outward_normal, int sphere_idx, inout Hit h)
{
    h.front_face = dot(r.dir, outward_normal) < 0;
    h.normal = (h.front_face) ? outward_normal : -outward_normal;

    h.mat_type = matTypes[sphere_idx / 4][sphere_idx % 4];
    h.mat_vals_idx = sphere_idx;
}
////////////////////////////////////////

////////////////////////////////////////
// Random funcs
float rand(inout float2 p)
{
    float r = frac(cos(dot(p, float2(23.14069263277926,2.665144142690225)))*12345.6789);
    p = float2(r, r*r);

    return r;
}

float3 random_in_unit_sphere(float2 randState)
{
    float phi = 2.0 * rand(randState) * (float) PI;
    float cosTheta = 2.0 * rand(randState) - 1.0;
    float u = rand(randState);

    float theta = acos(cosTheta);
    float r = pow(u, 1.0 / 3.0);

    float x = r * sin(theta) * cos(phi);
    float y = r * sin(theta) * sin(phi);
    float z = r * cos(theta);

    return float3(x, y, z);
}

float3 random_unit_vector(float2 randState)
{
    return normalize(random_in_unit_sphere(randState));
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

bool lambert(Ray r, Hit h, float2 randState, out float4 color, out Ray scatter)
{
    float3 scatter_dir = h.normal + random_unit_vector(randState);
    if (near_zero(scatter_dir))
    {
        scatter_dir = h.normal;
    }

    Ray s = { h.p, scatter_dir };
    scatter = s;
    color = float4(matValues[h.mat_vals_idx].xyz, 1.0);

    return true;
}

bool metal(Ray r, Hit h, float2 randState, out float4 color, out Ray scatter)
{
    float3 reflected = reflect(normalize(r.dir), h.normal);
    Ray s = { h.p, reflected + matValues[h.mat_vals_idx].w*random_in_unit_sphere(randState) };
    scatter = s;
    color = float4(matValues[h.mat_vals_idx].xyz, 1.0);

    return (dot(scatter.dir, h.normal) > 0);
}

bool dielectric(Ray r, Hit h, float2 randState, out float4 color, out Ray scatter)
{
    color = float4(1, 1, 1, 1);
    float ir = matValues[h.mat_vals_idx].w;
    float refrac_ratio = h.front_face ? (1.0 / ir) : ir;

    float3 unit_dir = normalize(r.dir);

    float cos_theta = min(dot(-unit_dir, h.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    bool cant_refract = refrac_ratio * sin_theta > 1.0;
    float3 dir;

    if (cant_refract)
        dir = reflect(unit_dir, h.normal);
    else
        dir = refract(unit_dir, h.normal, refrac_ratio);

    Ray s = { h.p, dir };
    scatter = s;

    return true;
}
////////////////////////////////////////

float4 sample_color(Ray r, float2 randState)
{
    Ray cur_ray = r;
    float4 cur_atten = float4(1.0, 1.0, 1.0, 1.0);
    for (int i = 0; i < sceneValues.y; ++i)
    {
        Hit h;
        if (hit_world(cur_ray, 0.001, 1.#INF, h))
        {
            Ray scattered;
            float4 color;
            bool success = false;

            if (h.mat_type == 0)
            {
                success = lambert(cur_ray, h, randState, color, scattered);
            }
            else if (h.mat_type == 1)
            {
                success = metal(cur_ray, h, randState, color, scattered);
            }
            else if (h.mat_type == 2)
            {
                success = dielectric(cur_ray, h, randState, color, scattered);
            }

            if (success)
            {
                cur_ray = scattered;
                cur_atten.x *= color.x;
                cur_atten.y *= color.y;
                cur_atten.z *= color.z;
            }
        }
        else
        {
            float3 unit_dir = normalize(cur_ray.dir);
            float t = 0.5 * (unit_dir.y + 1);
            float4 color = (1.0-t)*float4(1.0, 1.0, 1.0, 1.0) + t*float4(0.5, 0.7, 1.0, 1.0);

            return cur_atten * color;
        }
    }

    return float4(0, 0, 0, 1);
}

RWTexture2D<float4> OutputColors : register(t0);

[numthreads(32, 32, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    float2 randState = DTid.xy; // *frac(time.z);

    float u;
    float v;
    Ray r;

    float ratio = 1.0 / sceneValues.z;
    float4 color;
    for (int i = 0; i < sceneValues.z; ++i)
    {
        u = (DTid.x + rand(randState)) / (float)256;
        v = (DTid.y + rand(randState)) / (float)256;
        r = get_ray(u, v);

        color += ratio * sample_color(r, randState);
    }
    OutputColors[DTid.xy] = color;


    //float rr = rand(randState);
    //float rg = rand(randState);
    //float rb = rand(randState);
    //OutputColors[DTid.xy] = float4(rr, rg, rb, 1);
    //OutputColors[DTid.xy] = float4(u * abs(sin(time.x+1)), 0.2, v * abs(cos(time.x)), 1);
}


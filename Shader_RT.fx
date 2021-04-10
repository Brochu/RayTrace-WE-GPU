cbuffer CamSettings : register(b0)
{
	float4 img_vp;

    float4 origin;
    float4 horizontal;
    float4 vertical;

    float4 lower_left_corner;
}

struct VS_Input
{
    float4 pos  : POSITION;
    float2 tex0 : TEXCOORD0;
};


struct PS_Input
{
    float4 pos  : SV_POSITION;
    float2 tex0 : TEXCOORD0;
};

struct Ray
{
    float3 orig;
    float3 dir;
};

struct Material
{
    bool is_metal;
    float4 albedo; // color
    float fuzz; // Fuzzy reflections
};

struct Hit
{
    float3 p;
    float3 normal;
    float t;
    bool front_face;

    Material mat;
};

struct World
{
    int count;
    float4 world[64];

    Material world_mat[64]; // 1:1 rel between world && world_mat
};

#define PI 3.1415926535

PS_Input VS_Main(VS_Input vertex)
{
    PS_Input vsOut = (PS_Input)0;

    vsOut.pos = vertex.pos;
    vsOut.tex0 = vertex.tex0;

    return vsOut;
}

// I LOST THE RUN TO RNG DUDE??!
float rand2d(inout float2 randState)
{
    randState.x = frac(sin(dot(randState.xy, float2(12.9898, 78.233))) * 43758.5453);
    randState.y = frac(sin(dot(randState.xy, float2(12.9898, 78.233))) * 43758.5453);

    return randState.x;
}

float rand2d(inout float2 randState, float min, float max)
{
    return min + (max - min) * rand2d(randState);
}

float3 random_in_unit_sphere(float2 randState)
{
    float phi = 2.0 * PI * rand2d(randState);
    float cosTheta = 2.0 * rand2d(randState) - 1.0;
    float u = rand2d(randState);

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

float3 random_in_hemisphere(float2 randState, float3 normal)
{
    float3 in_unit_sphere = random_in_unit_sphere(randState);
    if (dot(in_unit_sphere, normal) > 0.0)
    {
        return in_unit_sphere;
    }
    else
    {
        return -in_unit_sphere;
    }
}

float degrees_to_radians(float degrees) {
    return degrees * PI / 180.0;
}

float3 ray_at(Ray r, float t)
{
    return r.orig + t * r.dir;
}

bool vec_near_zero(float3 vec)
{
    const float s = 1e-8;
    return (abs(vec.x) < s) && (abs(vec.y) < s) && (abs(vec.z) < s);
}

float3 reflect(float3 v, float3 normal)
{
    return v - 2 * dot(v, normal) * normal;
}

void set_face_normal(Ray r, float3 outward_normal, inout Hit h)
{
    h.front_face = dot(r.dir, outward_normal) < 0;

    if (h.front_face)
    {
        h.normal = outward_normal;
    }
    else
    {
        h.normal = -outward_normal;
    }
}

bool lambert(Material m, Ray r, Hit h, float2 randState, out float4 attenuation, out Ray scattered)
{
    float3 scatter_dir = h.normal + random_in_hemisphere(randState, h.normal);
    if (vec_near_zero(scatter_dir))
    {
        scatter_dir = h.normal;
    }

    Ray s = { h.p, scatter_dir };
    scattered = s;
    attenuation = m.albedo;

    return true;
}

bool metal(Material m, Ray r, Hit h, float2 randState, out float4 attenuation, out Ray scattered)
{
    float3 reflected = reflect(normalize(r.dir), h.normal);

    Ray s = { h.p, reflected + saturate(m.fuzz) * random_in_hemisphere(randState, h.normal) };
    scattered = s;
    attenuation = m.albedo;

    return true;
}

bool mat_scatter(Material m, Ray r, Hit h, float2 randState, out float4 attenuation, out Ray scattered)
{
    if (m.is_metal)
    {
        return metal(m, r, h, randState, attenuation, scattered);
    }
    else
    {
        return lambert(m, r, h, randState, attenuation, scattered);
    }
}

bool hit_sphere(float4 sphere, Material m, Ray r, float t_min, float t_max, out Hit h)
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
    h.mat = m;
    float3 outward_normal = (h.p - sphere.xyz) / sphere.w;
    set_face_normal(r, outward_normal, h);

    return true;
}

bool hit_world(World w, Ray r, float t_min, float t_max, out Hit h)
{
    Hit temp_hit;
    bool hit_anything = false;
    float closest = t_max;

    for (int i = 0; i < w.count; ++i)
    {
        if (hit_sphere(w.world[i], w.world_mat[i], r, t_min, closest, temp_hit))
        {
            hit_anything = true;
            closest = temp_hit.t;
            h = temp_hit;
        }
    }

    return hit_anything;
}

float4 color(Ray r, World w, float2 randState)
{
    Ray cur_ray = r;
    float4 cur_atten = float4(1.0, 1.0, 1.0, 1.0);
    for (int i = 0; i < 50; ++i)
    {
        Hit h;
        if (hit_world(w, cur_ray, 0.001, 1.#INF, h))
        {
            Ray scattered;
            float4 attenuation;

            if (mat_scatter(h.mat, cur_ray, h, randState, attenuation, scattered))
            {
                cur_atten *= attenuation;
                cur_ray = scattered;
            }
        }
        else
        {
            float3 unit_dir = normalize(cur_ray.dir);
            float t = 0.5 * (unit_dir.y + 1.0);
            float3 color = (1.0 - t) * float3(1, 1, 1) + t * float3(0.5, 0.7, 1.0);

            return float4((cur_atten.xyz * color), 1.0);
        }
    }

    return float4(0.0, 0.0, 0.0, 1.0);
}

float4 PS_Main(PS_Input frag) : SV_TARGET
{
    // World
    World w;
    w.count = 4;

    w.world[0] = float4(0.0, -100.5, -1.0, 100.0);
    Material m0 = { false, float4(0.8, 0.8, 0.0, 1.0), -1.0 };
    w.world_mat[0] = m0;

	w.world[1] = float4(0.0, 0.0, -1.0, 0.5);
    Material m1 = { false, float4(0.7, 0.3, 0.3, 1.0), -1.0 };
    w.world_mat[1] = m1;

	w.world[2] = float4(-1.0, 0.0, -1.0, 0.5);
    Material m2 = { true, float4(0.8, 0.8, 0.8, 1.0), 0.3 };
    w.world_mat[2] = m2;

	w.world[3] = float4(1.0, 0.0, -1.0, 0.5);
    Material m3 = { true, float4(0.8, 0.6, 0.2, 1.0), 1.0 };
    w.world_mat[3] = m3;

    // RNG
    float2 randState = frag.tex0;

    // Anti Aliasing setup
    float3 color_acc = float3(0, 0, 0);

    int pixel_samples = 10;
    for (int i = 0; i < pixel_samples; ++i)
    {
        float u = ((frag.tex0.x * img_vp.x) + rand2d(randState)) / img_vp.x;
        float v = ((frag.tex0.y * img_vp.y) + rand2d(randState)) / img_vp.y;

        Ray r = { float3(0, 0, 0), float3(lower_left_corner.xyz + u * horizontal.xyz + v * vertical.xyz - origin.xyz) };
        color_acc += color(r, w, randState);
    }

    // Separate the colors accumulated
    float r = color_acc.x;
    float g = color_acc.y;
    float b = color_acc.z;

    // Average out the values
    float scale = 1.0 / pixel_samples;
    r = sqrt(scale * r);
    g = sqrt(scale * g);
    b = sqrt(scale * b);

    // Output final color
    return float4(r, g, b, 1.0);

}

cbuffer CamSettings : register(b0)
{
	float4 img_vp;
	float4 f4u;
	float4 f4v;
	float4 f4w;

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
    // TYPES
    // 0: Lambert
    // 1: Metal
    // 2: Dielectric
    int type;
    float4 albedo; // color
    float fuzz; // Fuzzy reflections
    float ir;
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
    float4 world[128];

    Material world_mat[128]; // 1:1 rel between world && world_mat
};

#define PI 3.1415926535

PS_Input VS_Main(VS_Input vertex)
{
    PS_Input vsOut = (PS_Input)0;

    vsOut.pos = vertex.pos;
    vsOut.tex0 = vertex.tex0;

    return vsOut;
}

// World utilities
float4 AddLambert(out Material m, float3 pos, float r, float3 color)
{
    m.albedo = float4(color, 1.0);
    m.type = 0;
    m.fuzz = -1.0;
    m.ir = -1.0;

    return float4(pos, r);
}

float4 AddMetal(out Material m, float3 pos, float r, float3 color, float f)
{
    m.albedo = float4(color, 1.0);
    m.type = 1;
    m.fuzz = f;
    m.ir = -1.0;

    return float4(pos, r);
}

float4 AddDielectric(out Material m, float3 pos, float r, float ir)
{
    m.albedo = float4(1,1,1,1);
    m.type = 2;
    m.fuzz = -1.0;
    m.ir = ir;

    return float4(pos, r);
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

float3 random_in_unit_disk(float2 randState)
{
    float sinTheta = 2.0 * rand2d(randState) - 1.0;
    float cosTheta = 2.0 * rand2d(randState) - 1.0;

    float x = cos(cosTheta);
    float y = cos(sinTheta);

    return float3(x, y, 0);
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

float3 refract(float3 v, float3 normal, float etai_on_etat)
{
    float costheta = min(dot(-v, normal), 1.0);
    float3 r_out_perp = etai_on_etat * (v + costheta * normal);

    float perp_length_sq = length(r_out_perp) * length(r_out_perp);
    float3 r_out_par = -sqrt(abs(1.0 - perp_length_sq)) * normal;

    return r_out_perp + r_out_par;
}

float reflectance(float cosine, float ref_index)
{
    // Schlick approx
    float r0 = (1 - ref_index) / (1 + ref_index);
    r0 = r0 * r0;

    return r0 + (1 - r0) * pow((1 - cosine), 5);
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

bool dielectric(Material m, Ray r, Hit h, float2 randState, out float4 attenuation, out Ray scattered)
{
    attenuation = float4(1.0, 1.0, 1.0, 1.0);
    float refraction_ratio = h.front_face ? (1.0/m.ir) : m.ir;
    float3 unit_direction = normalize(r.dir);

    float cos_theta = min(dot(-unit_direction, h.normal), 1.0);
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    bool cantrefract = refraction_ratio * sin_theta > 1.0;

    float3 direction = float3(0, 0, 0);
    if (cantrefract || reflectance(cos_theta, refraction_ratio) > rand2d(randState))
    {
        direction = reflect(unit_direction, h.normal);
    }
    else
    {
        direction = refract(unit_direction, h.normal, refraction_ratio);
    }

    Ray s = { h.p, direction };
    scattered = s;

    return true;
}

bool mat_scatter(Material m, Ray r, Hit h, float2 randState, out float4 attenuation, out Ray scattered)
{
    if (m.type == 0)
    {
        return lambert(m, r, h, randState, attenuation, scattered);
    }
    else if (m.type == 1)
    {
        return metal(m, r, h, randState, attenuation, scattered);
    }
    else if (m.type == 2)
    {
        return dielectric(m, r, h, randState, attenuation, scattered);
    }
    else return false;
}

Ray get_ray(float s, float t, float2 randState)
{
    float3 rd = f4w.w * random_in_unit_disk(randState);
    float4 offset = f4u * rd.x + f4v * rd.y;

    Ray r = {
        origin.xyz + offset.xyz,
        float3(lower_left_corner.xyz + s * horizontal.xyz + t * vertical.xyz) - origin.xyz - offset
    };
    return r;
}

World random_world(float2 randState)
{
    World w;
    Material m;
    int i = 0;

    w.world[i] = AddLambert(m, float3(0, -1000, 0), 1000, float3(0.5, 0.5, 0.5));
    w.world_mat[i] = m;
    ++i;

    //TODO: debug why loops + randoms don't work
    w.world[i] = AddLambert(m, float3(3, 0.2, 1.5), 0.2, float3(0.2, 0.2, 0.8));
    w.world_mat[i] = m;
    ++i;

    w.world[i] = AddLambert(m, float3(4.5, 0.2, 1), 0.2, float3(0.2, 0.8, 0.2));
    w.world_mat[i] = m;
    ++i;

    w.world[i] = AddLambert(m, float3(4.5, 0.2, 2), 0.2, float3(0.8, 0.3, 0.2));
    w.world_mat[i] = m;
    ++i;

    w.world[i] = AddDielectric(m, float3(0, 1, 0), 1.0, 1.5);
    w.world_mat[i] = m;
    ++i;
    w.world[i] = AddLambert(m, float3(-4, 1, 0), 1.0, float3(0.4, 0.2, 0.1));
    w.world_mat[i] = m;
    ++i;
    w.world[i] = AddMetal(m, float3(4, 1, 0), 1.0, float3(0.7, 0.6, 0.5), 0.0);
    w.world_mat[i] = m;
    ++i;

    w.count = i;
    return w;
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
    for (int i = 0; i < 25; ++i)
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
    // RNG
    float2 randState = frag.tex0;

    // World
    World w = random_world(randState);

    // Anti Aliasing setup
    float3 color_acc = float3(0, 0, 0);

    int pixel_samples = 1;
    for (int i = 0; i < pixel_samples; ++i)
    {
        float u = ((frag.tex0.x * img_vp.x) + rand2d(randState)) / img_vp.x;
        float v = ((frag.tex0.y * img_vp.y) + rand2d(randState)) / img_vp.y;

        //TODO: Find how I can optimize this
        Ray r = get_ray(u, v, randState);
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

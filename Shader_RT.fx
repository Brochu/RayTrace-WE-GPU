Texture2D colorMap : register(t0);
SamplerState colorSampler : register(s0);


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


PS_Input VS_Main(VS_Input vertex)
{
    PS_Input vsOut = (PS_Input)0;

    vsOut.pos = vertex.pos;
    vsOut.tex0 = vertex.tex0;

    return vsOut;
}

float rand(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float nrand(float2 uv)
{
	return (2.0 * rand(uv)) - 1;
}

float3 random_in_unit_sphere(float2 uv)
{
    while (true)
	{
        float3 p = float3(nrand(uv), nrand(uv), nrand(uv));
        if (length(p) * length(p) >= 1) continue;
        return p;
    }
	
	// We should not be here
	return float3(0,0,0);
}

float hit_sphere(float3 center, float radius, float3 origin, float3 direction)
{
    float3 oc = origin - center;
    float a = dot(direction, direction);
    float half_b = dot(oc, direction);
    float c = dot(oc, oc) - radius*radius;

    float discriminant = half_b*half_b - a*c;

	if (discriminant < 0)
		return -1.0;
	else
		return (-half_b - sqrt(discriminant)) / a;
}

float3 find_normal(float3 center, float radius, float3 origin, float3 direction, float t)
{
	float3 N = (t*direction + origin - center) / radius;
	if (dot(direction, N) > 0)
	{
		N = -N;
	}

	return N;
}

float3 find_target(float3 P, float3 N, float2 uv)
{
	return float3(P + N + random_in_unit_sphere(uv));
}

float4 color(float3 origin, float3 direction, float2 uv, float depth)
{
	// Make sure we don't explode the callstack
	if (depth <= 0)
	{
		return float4(0, 0, 0, 1);
	}

	const int SPHERE_COUNT = 2;

	// Sheres gen, please move this to constant buffer
	float4 spheres[SPHERE_COUNT];
	spheres[0] = float4(0, 0, -1, 0.5);
	spheres[1] = float4(0, -100.5, -1, 100.0);

	float t = 0.0;
	for (int i = 0; i < SPHERE_COUNT; ++i)
	{
		t = hit_sphere(spheres[i].xyz, spheres[i].w, origin, direction);
		if (t > 0.0)
		{
			float3 N = find_normal(spheres[i].xyz, spheres[i].w, origin, direction, t);
			float3 P = (t * direction) + origin;
			float3 target = find_target(P, N, uv);

			return 0.5 * color(P, (target - P), uv, depth-1);
			//return 0.5 * float4(N.x + 1, N.y + 1, N.z + 1, 1.0);
		}
	}

	float3 unit_dir = normalize(direction);

	t = 0.5 * (unit_dir.y + 1.0);
	float3 lerped = lerp(float3(1.0, 1.0, 1.0), float3(0.5, 0.7, 1.0), t);

	return float4(lerped, 1.0);
}

float4 PS_Main(PS_Input frag) : SV_TARGET
{
	float u = frag.tex0.x;
	float v = frag.tex0.y;

	// Image
    float aspect_ratio = 4.0 / 3.0;
    int image_width = 640;
    int image_height = (int)(image_width / aspect_ratio);

    // Camera
    float viewport_height = 2.0;
    float viewport_width = aspect_ratio * viewport_height;
    float focal_length = 1.0;

    float3 origin = float3(0, 0, 0);
    float3 horizontal = float3(viewport_width, 0, 0);
    float3 vertical = float3(0, viewport_height, 0);
    float3 lower_left_corner = origin - horizontal/2.0 - vertical/2.0 - float3(0, 0, focal_length);

	float3 orig = float3(0,0,0);
	float3 dir = float3(lower_left_corner + u*horizontal + v*vertical - origin);

	const int MAX_DEPTH = 50;
	return color(orig, dir, frag.tex0, MAX_DEPTH);
}

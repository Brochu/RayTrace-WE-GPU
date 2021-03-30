#include "DXRayTrace.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>

// USINGS
using std::shared_ptr;
using std::make_shared;
using std::sqrt;

// CONSTS
const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

//UTILS
inline double deg_2_rad(double degrees)
{
	return degrees * pi / 180.0;
}

inline double random_double()
{
	// random between [0,1)
	return rand() / (RAND_MAX + 1);
}

inline double random_double(double min, double max)
{
	// random between [min,max)
	return min + (max - min)*random_double();
}

inline double clamp(double x, double min, double max)
{
	if (x < min) return min;
	if (x > max) return max;

	return x;
}

inline static Vec3 random()
{
	return Vec3(random_double(), random_double(), random_double());
}

inline static Vec3 random(double min, double max)
{
	return Vec3(random_double(min, max), random_double(min, max), random_double(min, max));
}

inline Vec3 random_in_unit_sphere()
{
	int max = 100;
	Vec3 p(0,0,0);

	while (max >= 0)
	{
		p = random(-1, 1);
		if (p.length_squared() < 1) break;
		--max;

	}
	return p;
}

// Vertex struct
struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT2 tex0;
};


//////////////////////////////////////////////////////////////////////
// Constructors

DXRayTrace::DXRayTrace()
{
    pVS = NULL;
    pPS = NULL;
    pInputLayout = NULL;
    pVertexBuffer = NULL;
    pColorMap = NULL;
    pColorMapSampler = NULL;
}

DXRayTrace::~DXRayTrace()
{
}


//////////////////////////////////////////////////////////////////////
// Overrides

bool DXRayTrace::LoadContent()
{
    // Compile vertex shader
    ID3DBlob* pVSBuffer = NULL;
    bool res = CompileShader("Shader_RT.fx", "VS_Main", "vs_4_0", &pVSBuffer);
    if (res == false) {
        ::MessageBox(hWnd, "Unable to load vertex shader", "ERROR", MB_OK);
        return false;
    }

    // Create vertex shader
    HRESULT hr;
    hr = pD3DDevice->CreateVertexShader(
        pVSBuffer->GetBufferPointer(),
        pVSBuffer->GetBufferSize(),
        0, &pVS
	);
    if (FAILED(hr)) {
        if (pVSBuffer) pVSBuffer->Release();
        return false;
    }

    // Define input layout
    D3D11_INPUT_ELEMENT_DESC shaderInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numLayoutElements = ARRAYSIZE(shaderInputLayout);

    // Create input layout
    hr = pD3DDevice->CreateInputLayout(
        shaderInputLayout, numLayoutElements,
        pVSBuffer->GetBufferPointer(), 
        pVSBuffer->GetBufferSize(), 
        &pInputLayout
	);
    if (FAILED(hr)) {
        return false;
    }

    // Release VS buffer
    pVSBuffer->Release();
    pVSBuffer = NULL;

    // Compile pixel shader
    ID3DBlob* pPSBuffer = NULL;
    res = CompileShader("Shader_RT.fx", "PS_Main", "ps_4_0", &pPSBuffer);
    if (res == false) {
        ::MessageBox(hWnd, "Unable to load pixel shader", "ERROR", MB_OK);
        return false;
    }

    // Create pixel shader
    hr = pD3DDevice->CreatePixelShader(
        pPSBuffer->GetBufferPointer(),
        pPSBuffer->GetBufferSize(), 
        0, &pPS
	);
    if (FAILED(hr)) {
        return false;
    }

    // Cleanup PS buffer
    pPSBuffer->Release();
    pPSBuffer = NULL;

    // Define triangle
    Vertex vertices[] =
    {
        { XMFLOAT3(  1.0f,  1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
        { XMFLOAT3(  1.0f, -1.0f, 1.0f ), XMFLOAT2( 1.0f, 0.0f ) },
        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },

        { XMFLOAT3( -1.0f, -1.0f, 1.0f ), XMFLOAT2( 0.0f, 0.0f ) },
        { XMFLOAT3( -1.0f,  1.0f, 1.0f ), XMFLOAT2( 0.0f, 1.0f ) },
        { XMFLOAT3(  1.0f,  1.0f, 1.0f ), XMFLOAT2( 1.0f, 1.0f ) },
    };

    // Vertex description
    D3D11_BUFFER_DESC vertexDesc;
    ::ZeroMemory(&vertexDesc, sizeof(vertexDesc));
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.ByteWidth = sizeof(Vertex) * 6;

    // Resource data
    D3D11_SUBRESOURCE_DATA resourceData;
    ZeroMemory(&resourceData, sizeof(resourceData));
    resourceData.pSysMem = vertices;

    // Create vertex buffer
    hr = pD3DDevice->CreateBuffer(&vertexDesc, &resourceData, &pVertexBuffer);
    if (FAILED(hr)) {
        return false;
    }

    // Load texture
    hr = ::D3DX11CreateShaderResourceViewFromFile(
        pD3DDevice, "borg.dds", 0, 0, &pColorMap, 0);
    if (FAILED(hr)) {
        ::MessageBox(hWnd, "Unable to load texture", "ERROR", MB_OK);
        return false;
    }

    // Texture sampler
    D3D11_SAMPLER_DESC textureDesc;
    ::ZeroMemory(&textureDesc, sizeof(textureDesc));
    textureDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    textureDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    textureDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    textureDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    textureDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    textureDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = pD3DDevice->CreateSamplerState(&textureDesc, &pColorMapSampler);
    if (FAILED(hr)) {
        ::MessageBox(hWnd, "Unable to create texture sampler state", "ERROR", MB_OK);
        return false;
    }

    return true;
}

void DXRayTrace::UnloadContent()
{
    // Cleanup
	if (pRayTraceTex)
		pRayTraceTex->Release();
	pRayTraceTex = NULL;
	if (pRayTraceMap)
		pRayTraceMap->Release();
	pRayTraceMap = NULL;

    if (pColorMap)
        pColorMap->Release();
    pColorMap = NULL;
    if (pColorMapSampler)
        pColorMapSampler->Release();
    pColorMapSampler = NULL;
    if (pVS)
        pVS->Release();
    pVS = NULL;
    if (pPS)
        pPS->Release();
    pPS = NULL;
    if (pInputLayout)
        pInputLayout->Release();
    pInputLayout = NULL;
    if (pVertexBuffer)
        pVertexBuffer->Release();
    pVertexBuffer = NULL;
}

void DXRayTrace::TraceTexture()
{
	// Image
	const auto aspect_ratio = (double)Mode.Width / Mode.Height;
	const auto pixel_samples = 1;
	const auto max_depth = 50;

	// World
	Hittable_list world;
	world.add(make_shared<Sphere>(Point3(0.0, 0.0, -1.0), 0.5));
	world.add(make_shared<Sphere>(Point3(0.0, -100.5, -1.0), 100));

	// Camera
	Camera cam(Mode.Width, Mode.Height);

	// Render
	// Update texture buffer with results of raytrace

	for (int j = Mode.Height - 1; j >= 0; --j)
	{
		std::cout << "Starting new line ... \n";

		for (int i = 0; i < Mode.Width; ++i)
		{
			Color c(0.0, 0.0, 0.0);
			for (int s = 0; s < pixel_samples; ++s)
			{
				auto u = (i+random_double()) / (Mode.Width - 1);
				auto v = (j+random_double()) / (Mode.Height - 1);

				c += ray_color(cam.get_ray(u, v), world, max_depth);
			}

			auto r = c.x();
			auto g = c.y();
			auto b = c.z();

			auto scale = 1.0 / pixel_samples;
			r *= scale;
			g *= scale;
			b *= scale;

			// Place color in buffer
			RayTraceData.emplace_back(1.0);
			RayTraceData.emplace_back(256*clamp(b, 0.0, 0.999));
			RayTraceData.emplace_back(256*clamp(g, 0.0, 0.999));
			RayTraceData.emplace_back(256*clamp(r, 0.0, 0.999));
		}
	}
	std::reverse(RayTraceData.begin(), RayTraceData.end());

	std::cout << "Render done !";

	// Move render results to texture
	D3D11_TEXTURE2D_DESC tex_desc;
	tex_desc.ArraySize = 1;

	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.MiscFlags = 0;

	tex_desc.Width = Mode.Width;
	tex_desc.Height = Mode.Height;

	tex_desc.MipLevels = 1;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;

	D3D11_SUBRESOURCE_DATA res_data;
	res_data.SysMemPitch = sizeof(BYTE) * 4 * Mode.Width;
	res_data.pSysMem = RayTraceData.data();

	HRESULT hr = pD3DDevice->CreateTexture2D(&tex_desc, &res_data, &pRayTraceTex);
	if (FAILED(hr))
	{
        ::MessageBox(hWnd, "Could not create ray trace texture", "ERROR", MB_OK);
	}

	pD3DDevice->CreateShaderResourceView(pRayTraceTex, 0, &pRayTraceMap);
}

Color DXRayTrace::ray_color(const Ray& r, const Hittable& world, int depth) const
{
	hit_record rec;

	if (depth <= 0) return Color(0, 0, 0);

	if (world.hit(r, 0, infinity, rec))
	{
		Point3 target = rec.p + rec.normal + random_in_unit_sphere();
		return 0.5 * ray_color(Ray(rec.p, target - rec.p), world, depth-1);
	}

	Vec3 unit_direction = unit_vector(r.direction());
	auto t = 0.5*(unit_direction.y() + 1.0);

	return (1.0 - t)*Color(1.0, 1.0, 1.0) + t * Color(0.5, 0.7, 1.0);
}

void DXRayTrace::Update()
{
}

void DXRayTrace::Render()
{
    // Check if D3D is ready
    if (pD3DContext == NULL)
        return;

    // Clear back buffer
    float color[4] = { 0.0f, 0.0f, 0.5f, 1.0f };
    pD3DContext->ClearRenderTargetView(pD3DRenderTargetView, color);


    // Stride and offset
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    // Set vertex buffer
    pD3DContext->IASetInputLayout(pInputLayout);
    pD3DContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
    pD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set shaders
    pD3DContext->VSSetShader(pVS, 0, 0);
    pD3DContext->PSSetShader(pPS, 0, 0);
    pD3DContext->PSSetShaderResources(0, 1, &pRayTraceMap);
    pD3DContext->PSSetSamplers(0, 1, &pColorMapSampler);

    // Draw triangles
    pD3DContext->Draw(6, 0);


    // Present back buffer to display
    pSwapChain->Present(0, 0);
}

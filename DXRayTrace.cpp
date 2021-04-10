#include "DXRayTrace.h"

// Vertex struct
struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT2 tex0;
};

struct CameraSettings
{
	XMFLOAT4 img_vp;

    XMFLOAT4 origin;
    XMFLOAT4 horizontal;
    XMFLOAT4 vertical;

    XMFLOAT4 lower_left_corner;

	CameraSettings(float aspect, int img_width, float vp_height, float f_length, XMFLOAT3 orig)
	{
		// Image && Viewport
		img_vp.x = (FLOAT)img_width;
		img_vp.y = (FLOAT)(img_width / aspect);

		img_vp.z = vp_height;
		img_vp.w = aspect* vp_height;

		// Camera
		origin = XMFLOAT4(orig.x, orig.y, orig.z, 1.0);
		horizontal = XMFLOAT4(img_vp.w, 0, 0, 0);
		vertical = XMFLOAT4(0, img_vp.z, 0, 0);
		lower_left_corner = origin;

		lower_left_corner.x -= horizontal.x / 2.0;
		lower_left_corner.y -= horizontal.y / 2.0;
		lower_left_corner.z -= horizontal.z / 2.0;

		lower_left_corner.x -= vertical.x / 2.0;
		lower_left_corner.y -= vertical.y / 2.0;
		lower_left_corner.z -= vertical.z / 2.0;

		lower_left_corner.z -= f_length;
	}
};

struct World
{
	float count;
	XMFLOAT4 spheres[64];
};

//////////////////////////////////////////////////////////////////////
// Constructors

DXRayTrace::DXRayTrace()
{
    pVS = NULL;
    pPS = NULL;
    pInputLayout = NULL;
    pVertexBuffer = NULL;
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

	// CREATE CAM SETTINGS
	CameraSettings cam(4.0 / 3.0, 640, 2.0, 1.0, XMFLOAT3(0, 0, 0));

	// CAM SETTINGS BUFFER
	D3D11_BUFFER_DESC camSettingsDesc;
	::ZeroMemory(&camSettingsDesc, sizeof(camSettingsDesc));
	camSettingsDesc.Usage = D3D11_USAGE_DEFAULT;
	camSettingsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	camSettingsDesc.ByteWidth = sizeof(CameraSettings);

	D3D11_SUBRESOURCE_DATA camSettingsData;
	::ZeroMemory(&camSettingsData, sizeof(camSettingsData));
	camSettingsData.pSysMem = &cam;

	// Create cam settings cbuffer
    hr = pD3DDevice->CreateBuffer(&camSettingsDesc, &camSettingsData, &pCamSettingsBuffer);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void DXRayTrace::UnloadContent()
{
    // Cleanup
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
    //pD3DContext->PSSetShaderResources(0, 1, &pColorMap);
    //pD3DContext->PSSetSamplers(0, 1, &pColorMapSampler);

	// Set CBuffers for PS
	pD3DContext->PSSetConstantBuffers(0, 1, &pCamSettingsBuffer);
	//pD3DContext->PSSetConstantBuffers(1, 1, &pWorldBuffer);

    // Draw triangles
    pD3DContext->Draw(6, 0);


    // Present back buffer to display
    pSwapChain->Present(0, 0);
}

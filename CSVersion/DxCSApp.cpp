
#include "DxCSApp.h"

struct Vertex
{
	XMFLOAT3 pos;
	XMFLOAT2 tex0;
};

struct PerFrame
{
	XMMATRIX viewProj;
	XMMATRIX invViewProj;

	// To change some things based on time...?
	XMFLOAT4 time;
};

DxCSApp::DxCSApp()
{
	pCShader = NULL;
    pVShader = NULL;
    pPShader = NULL;

    pInputLayout  = NULL;
    pVertexBuffer = NULL;

	pOutputTex = NULL;
	pOutputSRV = NULL;
	pOutputUAV = NULL;

    pOutputSampler = NULL;

	pPerFrameCBuf = NULL;
	camPos = XMVECTOR { 0.0, 0.0, -1.0, 1.0 };
	camLookAt = XMVECTOR { 0.0, 0.0, 0.0, 1.0 };
	upDir = XMVECTOR { 0.0, 1.0, 0.0, 0.0 };
	perspectiveVals = { 80.0, 1, 0.001, 10000 }; // Values: FoV angle Y, aspect ratio, near Z, far Z

	t0 = Clock::now().time_since_epoch().count();
	tlast = Clock::now().time_since_epoch().count();
	tnow = Clock::now().time_since_epoch().count();
	timeVals = XMFLOAT4{ 0.0, 0.0, 0.0, 0.0 }; // Values: Time, DeltaTime, Time*2, DeltaTime*2
}

DxCSApp::~DxCSApp()
{
}


//////////////////////////////////////////////////////////////////////
// Overrides

bool DxCSApp::LoadContent()
{
    // Compile vertex shader
    ID3DBlob* pVSBuffer = NULL;
    bool res = CompileShader(DisplayShaderName, "VS_Main", "vs_5_0", &pVSBuffer);
    if (res == false) {
        ::MessageBox(m_hWnd, L"Unable to load vertex shader", L"ERROR", MB_OK);
        return false;
    }

    // Create vertex shader
    HRESULT hr;
    hr = m_pD3DDevice->CreateVertexShader(
        pVSBuffer->GetBufferPointer(),
        pVSBuffer->GetBufferSize(),
        0, &pVShader
	);
    if (FAILED(hr)) {
        if (pVSBuffer)
            pVSBuffer->Release();
        return false;
    }

	////////////////////////////////////////
    // Define input layout
    D3D11_INPUT_ELEMENT_DESC shaderInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numLayoutElements = ARRAYSIZE( shaderInputLayout );

    // Create input layout
    hr = m_pD3DDevice->CreateInputLayout(
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

	////////////////////////////////////////
    // Compile pixel shader
    ID3DBlob* pPSBuffer = NULL;
    res = CompileShader(DisplayShaderName, "PS_Main", "ps_5_0", &pPSBuffer);
    if (res == false) {
        ::MessageBox(m_hWnd, L"Unable to load pixel shader", L"ERROR", MB_OK);
        return false;
    }

    // Create pixel shader
    hr = m_pD3DDevice->CreatePixelShader(
        pPSBuffer->GetBufferPointer(),
        pPSBuffer->GetBufferSize(), 
        0, &pPShader
	);
    if (FAILED(hr)) {
        return false;
    }

    // Cleanup PS buffer
    pPSBuffer->Release();
    pPSBuffer = NULL;

	////////////////////////////////////////
    // Compile Compute Shader
    ID3DBlob* pCSBuffer = NULL;
    res = CompileShader(ComputeShaderName, "CSMain", "cs_5_0", &pCSBuffer);
    if (res == false) {
        ::MessageBox(m_hWnd, L"Unable to load compute shader", L"ERROR", MB_OK);
        return false;
    }

    // Create Compute shader
	hr = m_pD3DDevice->CreateComputeShader(
		pCSBuffer->GetBufferPointer(),
		pCSBuffer->GetBufferSize(),
		0, &pCShader
	);
    if (FAILED(hr)) {
        return false;
    }

    // Cleanup PS buffer
    pCSBuffer->Release();
    pCSBuffer = NULL;


	////////////////////////////////////////
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
    hr = m_pD3DDevice->CreateBuffer(&vertexDesc, &resourceData, &pVertexBuffer);
    if (FAILED(hr)) {
        return false;
    }

	////////////////////////////////////////
	// Define OutputColors buffer
	D3D11_TEXTURE2D_DESC outputTexDesc = {};
    outputTexDesc.ArraySize = 1;
    outputTexDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    outputTexDesc.CPUAccessFlags = 0;
    outputTexDesc.Height = 256;
    outputTexDesc.Width = 256;
    outputTexDesc.MipLevels = 1;
    outputTexDesc.MiscFlags = 0;
    outputTexDesc.SampleDesc.Count = 1;
    outputTexDesc.SampleDesc.Quality = 0;
    outputTexDesc.Usage = D3D11_USAGE_DEFAULT;
    outputTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	hr = m_pD3DDevice->CreateTexture2D(&outputTexDesc, 0, &pOutputTex);
    if (FAILED(hr)) {
        return false;
    }

	// Define Shader Resource View for output texture
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = outputTexDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	hr = m_pD3DDevice->CreateShaderResourceView(pOutputTex, &srvDesc, &pOutputSRV);

	CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = outputTexDesc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	hr = m_pD3DDevice->CreateUnorderedAccessView(pOutputTex, &uavDesc, &pOutputUAV);

	////////////////////////////////////////
    // Texture sampler
    D3D11_SAMPLER_DESC textureDesc;
    ::ZeroMemory(&textureDesc, sizeof(textureDesc));
    textureDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    textureDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    textureDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    textureDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    textureDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    textureDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pD3DDevice->CreateSamplerState(&textureDesc, &pOutputSampler);
    if (FAILED(hr)) {
        ::MessageBox(m_hWnd, L"Unable to create texture sampler state", L"ERROR", MB_OK);
        return false;
    }

	////////////////////////////////////////
    // Per Frame CBuffer
	PerFrame frameCBuf;
	// Set matrices in the struct OR Set everything in ctor???
	// Also find a way to update this every frame
	frameCBuf.time = timeVals;
	XMMATRIX viewMat = XMMatrixLookAtLH(camPos, camLookAt, upDir);
	XMMATRIX projMat = XMMatrixPerspectiveFovLH(perspectiveVals.x, perspectiveVals.y, perspectiveVals.z, perspectiveVals.w);

	D3D11_BUFFER_DESC frameCBufDesc;
	::ZeroMemory(&frameCBufDesc, sizeof(frameCBufDesc));
	frameCBufDesc.Usage = D3D11_USAGE_DEFAULT;
	frameCBufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	frameCBufDesc.ByteWidth = sizeof(PerFrame);

	D3D11_SUBRESOURCE_DATA frameCBufData;
	::ZeroMemory(&frameCBufData, sizeof(frameCBufData));
	frameCBufData.pSysMem = &frameCBuf;

	// Create cam settings cbuffer
	hr = m_pD3DDevice->CreateBuffer(&frameCBufDesc, &frameCBufData, &pPerFrameCBuf);
    if (FAILED(hr)) {
        return false;
    }

	debugLog = std::ofstream("debug.log");

    return true;
}

void DxCSApp::UnloadContent()
{
    // Cleanup
	// Clear Shader
    if (pCShader) pCShader->Release();
    pCShader = NULL;
    if (pVShader) pVShader->Release();
    pVShader = NULL;
    if (pPShader) pPShader->Release();
    pPShader = NULL;

	// Clear VBuffer
    if (pInputLayout) pInputLayout->Release();
    pInputLayout = NULL;
    if (pVertexBuffer) pVertexBuffer->Release();
    pVertexBuffer = NULL;

	// Clear Textures
	if (pOutputTex) pOutputTex->Release();
	pOutputTex = NULL;
	if (pOutputSRV) pOutputSRV->Release();
	pOutputSRV = NULL;
	if (pOutputUAV) pOutputUAV->Release();
	pOutputUAV = NULL;

	// Clear Sampler
	if (pOutputSampler) pOutputSampler->Release();
	pOutputSampler = NULL;

	if (pPerFrameCBuf) pPerFrameCBuf->Release();
	pPerFrameCBuf = NULL;

	debugLog.close();
}

void DxCSApp::Update()
{
	// Time updates
	tnow = Clock::now().time_since_epoch().count();

	long long delta = tnow - tlast;
	long long elapsed = tnow - t0;

	timeVals.x = elapsed / 10000000.0;
	timeVals.y = delta / 10000000.0;
	timeVals.z = timeVals.x * 2;
	timeVals.w = timeVals.y * 2;

	tlast = tnow;
}

void DxCSApp::Render()
{
    // Check if D3D is ready
    if (m_pD3DContext == NULL)
        return;

	////////////////////////////////////////
    // Clear back buffer
    float color[4] = { 0.5f, 0.0f, 0.5f, 1.0f };
    m_pD3DContext->ClearRenderTargetView(m_pD3DRenderTargetView, color);

	////////////////////////////////////////
	// Compute Shader pass, texture generation
	UINT UAVInitialCounts = 0;

	m_pD3DContext->CSSetShader(pCShader, 0, 0);

	ID3D11ShaderResourceView* NullSRV = nullptr;
	m_pD3DContext->PSSetShaderResources(0, 1, &NullSRV);

	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &pOutputUAV, &UAVInitialCounts);

	// Compute Shader Run
	m_pD3DContext->Dispatch(32, 32, 1);

	////////////////////////////////////////
	// Render texture on screen
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set Vertex Buffer
	m_pD3DContext->IASetInputLayout(pInputLayout);
	m_pD3DContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
	m_pD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Shaders
	m_pD3DContext->VSSetShader(pVShader, 0, 0);
	m_pD3DContext->PSSetShader(pPShader, 0, 0);

	ID3D11UnorderedAccessView* NullUAV = nullptr;
	m_pD3DContext->CSSetUnorderedAccessViews(0, 1, &NullUAV, &UAVInitialCounts);

	m_pD3DContext->PSSetShaderResources(0, 1, &pOutputSRV);
	m_pD3DContext->PSSetSamplers(0, 1, &pOutputSampler);


	// Draw Triangle
	m_pD3DContext->Draw(6, 0);

    // Present back buffer to display
    m_pSwapChain->Present(0, 0);
}

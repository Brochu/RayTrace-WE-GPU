
#include "DxCSApp.h"

struct Vertex
{
	XMFLOAT3 pos;
};

DxCSApp::DxCSApp()
{
	pCShader = NULL;
    pVShader = NULL;
    pPShader = NULL;

    pInputLayout  = NULL;
    pVertexBuffer = NULL;
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
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
    // Define triangle
    Vertex vertices[] =
    {
        XMFLOAT3(  0.0f,  0.5f, 0.5f ),
        XMFLOAT3(  0.5f, -0.5f, 0.5f ),
        XMFLOAT3( -0.5f, -0.5f, 0.5f )
    };

    // Vertex description
    D3D11_BUFFER_DESC vertexDesc;
    ::ZeroMemory(&vertexDesc, sizeof(vertexDesc));
    vertexDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexDesc.ByteWidth = sizeof(Vertex) * 3;

    // Resource data
    D3D11_SUBRESOURCE_DATA resourceData;
    ZeroMemory(&resourceData, sizeof(resourceData));
    resourceData.pSysMem = vertices;

    // Create vertex buffer
    hr = m_pD3DDevice->CreateBuffer(&vertexDesc, &resourceData, &pVertexBuffer);
    if (FAILED(hr)) {
        return false;
    }

    return true;
}

void DxCSApp::UnloadContent()
{
    // Cleanup
    if (pCShader) pCShader->Release();
    pCShader = NULL;
    if (pVShader) pVShader->Release();
    pVShader = NULL;
    if (pPShader) pPShader->Release();
    pPShader = NULL;

    if (pInputLayout) pInputLayout->Release();
    pInputLayout = NULL;
    if (pVertexBuffer) pVertexBuffer->Release();
    pVertexBuffer = NULL;
}

void DxCSApp::Update()
{
}

void DxCSApp::Render()
{
    // Check if D3D is ready
    if (m_pD3DContext == NULL)
        return;

    // Clear back buffer
    float color[4] = { 0.5f, 0.0f, 0.5f, 1.0f };
    m_pD3DContext->ClearRenderTargetView(m_pD3DRenderTargetView, color);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	// Set Vertex Buffer
	m_pD3DContext->IASetInputLayout(pInputLayout);
	m_pD3DContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);
	m_pD3DContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set Shaders
	m_pD3DContext->VSSetShader(pVShader, 0, 0);
	m_pD3DContext->PSSetShader(pPShader, 0, 0);

	// Draw Triangle
	m_pD3DContext->Draw(3, 0);

    // Present back buffer to display
    m_pSwapChain->Present(0, 0);
}

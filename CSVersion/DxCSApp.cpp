
#include "DxCSApp.h"

const double pi = 3.1415926535897932385;

inline float random()
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

void SetFloat4Cmpt(XMFLOAT4& vec, int pos, float val)
{
    if (pos == 0) vec.x = val;
    else if (pos == 1) vec.y = val;
    else if (pos == 2) vec.z = val;
    else if (pos == 3) vec.w = val;
}

inline float deg2rad(float deg)
{
    return deg * pi / 180.0;
}

struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT2 tex0;
};

struct PerFrame
{
    // To change some things based on time...?
    XMFLOAT4 time;
    XMFLOAT4 perspectiveVals;
    XMFLOAT4 currSamples;

    XMMATRIX viewVals;

    void ComputeViewVals(FXMVECTOR lookFrom, FXMVECTOR lookAt, FXMVECTOR vup,
                        float vfov, float aspect_ratio, float aperture, float focus_dist)
    {
        float theta = deg2rad(vfov);
        float h = tan(theta / 2);
        float view_h = 2.0 * h;
        float view_w = aspect_ratio * view_h;

        XMVECTOR w = XMVector3Normalize(lookFrom - lookAt);
        XMVECTOR u = XMVector3Normalize(XMVector3Cross(vup, w));
        XMVECTOR v = XMVector3Cross(w, u);

        XMVECTOR horizontal = focus_dist * view_w  * u;
        XMVECTOR vertical = focus_dist * view_h  * v;
        XMVECTOR lowerLeft = lookFrom - horizontal / 2 - vertical / 2 - focus_dist * w;

        viewVals.r[0] = { lookFrom.m128_f32[0], lookFrom.m128_f32[1], lookFrom.m128_f32[2], 1.0 };
        viewVals.r[1] = { horizontal.m128_f32[0], horizontal.m128_f32[1], horizontal.m128_f32[2], 0.0 };
        viewVals.r[2] = { vertical.m128_f32[0], vertical.m128_f32[1], vertical.m128_f32[2], 0.0 };
        viewVals.r[3] = { lowerLeft.m128_f32[0], lowerLeft.m128_f32[1], lowerLeft.m128_f32[2], 1.0 };

        viewVals = XMMatrixTranspose(viewVals);
    }
};

struct WorldDef
{
    XMFLOAT4 sceneValues; // Values: Object Count, Max Ray Depth , Ray Per Pixels, unused... 
    XMFLOAT4 spheres[512];

    XMFLOAT4 matTypes[128];
    XMFLOAT4 matValues[512];

    static void random_world(WorldDef& w)
    {
        int count = 0;
        SetFloat4Cmpt(w.matTypes[count / 4], count % 4, 0.0);
        w.spheres[count] = { 0.0, -1000.0, 0.0, 1000.0 };
        w.matValues[count] = { 0.5, 0.5, 0.5, 1.0 };
        ++count;

        SetFloat4Cmpt(w.matTypes[count / 4], count % 4, 2.0);
        w.spheres[count] = { 0.0, 1.0, 0.0, 1.0 };
        w.matValues[count] = { 0.0, 0.0, 0.0, 1.5 };
        ++count;

        SetFloat4Cmpt(w.matTypes[count / 4], count % 4, 0.0);
        w.spheres[count] = { -4.0, 1.0, 0.0, 1.0 };
        w.matValues[count] = { 0.4, 0.2, 0.1, 1.0 };
        ++count;

        SetFloat4Cmpt(w.matTypes[count / 4], count % 4, 1.0);
        w.spheres[count] = { 4.0, 1.0, 0.0, 1.0 };
        w.matValues[count] = { 0.7, 0.6, 0.5, 0.0 };
        ++count;

        for (int a = -9; a < 9; ++a)
        {
            for (int b = -9; b < 9; ++b)
            {
                float mat_choice = random();
                XMVECTOR center = { a + 0.9*random(), 0.2, b + 0.9*random() };
                XMVECTOR ref = { 4.0, 0.2, 0.0 };

                if (XMVector3Length(center - ref).m128_f32[0] > 0.9)
                {
                    if (mat_choice < 0.8)
                    {
                        // Diffuse
                        SetFloat4Cmpt(w.matTypes[count / 4], count % 4, 0.0);
                        w.spheres[count] = { center.m128_f32[0], center.m128_f32[1], center.m128_f32[2], 0.2 };
                        w.matValues[count] = { random() * random(), random() * random(), random() * random(), 0.0 };
                        ++count;
                    }
                    else if (mat_choice < 0.95)
                    {
                        // Metal
                        SetFloat4Cmpt(w.matTypes[count / 4], count % 4, 1.0);
                        w.spheres[count] = { center.m128_f32[0], center.m128_f32[1], center.m128_f32[2], 0.2 };
                        w.matValues[count] = { random()/2 + 1, random()/2 + 1, random()/2 + 1, 0.0 };
                        ++count;
                    }
                    else
                    {
                        // Glass
                        SetFloat4Cmpt(w.matTypes[count / 4], count % 4, 2.0);
                        w.spheres[count] = { center.m128_f32[0], center.m128_f32[1], center.m128_f32[2], 0.2 };
                        w.matValues[count] = { 0.0, 0.0, 0.0, 1.5 };
                        ++count;
                    }
                }
            }
        }

        w.sceneValues = { (float)count, 50, 60, -1 };
    }

    static void test_world(WorldDef& w)
    {
        w.matTypes[0] = { 0, 0, 1, 2 };

        // Ground
        w.spheres[0] = { 0.0, -1000.5, -1.0, 1000.0 };
        w.matValues[0] = { 0.5, 0.5, 0.5, 1.0 };

        // Middle Lambert
        w.spheres[1] = { 0.0, 0.0, -1.0, 0.5 };
        w.matValues[1] = { 0.2, 0.4, 0.8, 1.0 };

        // Right metal
        w.spheres[2] = { 1.0, 0.0, -1.0, 0.5 };
        w.matValues[2] = { 0.8, 0.4, 0.2, 0.0 };

        // Left hollow glass
        w.spheres[3] = { -1.0, 0.0, -1.0, 0.5 };
        w.matValues[3] = { 0.5, 0.5, 0.5, 1.5 };

        w.sceneValues = { 4, 50, 50, -1 };
    }
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
    camPos = FXMVECTOR { 13.0, 2.0, 3.0 };
    camLookAt = FXMVECTOR { 0.0, 0.0, 0.0 };
    upDir = FXMVECTOR { 0.0, 1.0, 0.0 };
    perspectiveVals = { 20.0, 16.0/9.0, 2.0, 1024.0 }; // Values: FoV angle Y, aspect ratio, aperture, img width

    t0 = Clock::now().time_since_epoch().count();
    tlast = Clock::now().time_since_epoch().count();
    tnow = Clock::now().time_since_epoch().count();
    timeVals = XMFLOAT4{ 0.0, 0.0, 0.0, 0.0 }; // Values: Time, DeltaTime, Time*2, DeltaTime*2

    sampleCount = 0;

    pWorldCBuf = NULL;
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
    outputTexDesc.Height = 576;
    outputTexDesc.Width = 1024;
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
    D3D11_BUFFER_DESC frameCBufDesc;
    ::ZeroMemory(&frameCBufDesc, sizeof(frameCBufDesc));
    frameCBufDesc.ByteWidth = sizeof(PerFrame);
    frameCBufDesc.Usage = D3D11_USAGE_DYNAMIC;
    frameCBufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    frameCBufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    frameCBufDesc.MiscFlags = 0;
    frameCBufDesc.StructureByteStride = 0;

    // Create per frame cbuffer
    hr = m_pD3DDevice->CreateBuffer(&frameCBufDesc, 0, &pPerFrameCBuf);
    if (FAILED(hr)) {
        return false;
    }

    ////////////////////////////////////////
    // World Definition CBuffer
    D3D11_BUFFER_DESC worldBufDesc;
    ::ZeroMemory(&worldBufDesc, sizeof(worldBufDesc));
    worldBufDesc.ByteWidth = sizeof(WorldDef);
    worldBufDesc.Usage = D3D11_USAGE_IMMUTABLE;
    worldBufDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    worldBufDesc.MiscFlags = 0;
    worldBufDesc.StructureByteStride = 0;

    WorldDef w;
    WorldDef::random_world(w);

    // World data
    D3D11_SUBRESOURCE_DATA worldData;
    ZeroMemory(&worldData, sizeof(worldData));
    worldData.pSysMem = &w;

    // Create World cbuffer
    hr = m_pD3DDevice->CreateBuffer(&worldBufDesc, &worldData, &pWorldCBuf);
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

    if (pWorldCBuf) pWorldCBuf->Release();
    pWorldCBuf = NULL;

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

    // Transformation matrices updates
    // TEMP START
    //camPos = { 13.0, 5*abs(sin((float)timeVals.x)), 3.0 };
    //camLookAt = { sin((float)timeVals.x), 0.0, 0.0 };
    //upDir = { sin((float)timeVals.x), 1.0, 0.0 };
    //perspectiveVals = { 60 * abs(cos((float)timeVals.x)) + 20, 1, 2.0, 400.0 }; // Values: FoV angle Y, aspect ratio, aperture, img width
    // TEMP END

    D3D11_MAPPED_SUBRESOURCE mappedRes;
    ZeroMemory(&mappedRes, sizeof(D3D11_MAPPED_SUBRESOURCE));

    PerFrame frameCBuf;
    frameCBuf.time = timeVals;
    frameCBuf.perspectiveVals = perspectiveVals;

    FLOAT focus_dist = XMVector4Length(camPos - camLookAt).m128_f32[0];
    frameCBuf.ComputeViewVals(camPos, camLookAt, upDir, perspectiveVals.x, perspectiveVals.y, perspectiveVals.z, focus_dist);

    sampleCount++;
    frameCBuf.currSamples = { sampleCount, sampleCount, sampleCount, sampleCount };

    m_pD3DContext->Map(pPerFrameCBuf, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedRes);
    memcpy(mappedRes.pData, &frameCBuf, sizeof(frameCBuf));
    m_pD3DContext->Unmap(pPerFrameCBuf, 0);
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
    m_pD3DContext->CSSetConstantBuffers(0, 1, &pPerFrameCBuf);
    m_pD3DContext->CSSetConstantBuffers(1, 1, &pWorldCBuf);

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

// Dx11Base.cpp : Implements the CDx11Base class.
// By Geelix School of Serious Games and Edutainment.

#include "Dx11Base.h"

CDx11Base::CDx11Base()
{
    hWnd = NULL;
    hInst = NULL;
    pD3DDevice = NULL;
    pD3DContext = NULL;
    pD3DRenderTargetView = NULL;
    pSwapChain = NULL;
}

CDx11Base::~CDx11Base()
{
}


//////////////////////////////////////////////////////////////////////
// Overrides
bool CDx11Base::Initialize(HWND hWnd, HINSTANCE hInst)
{
    // Set attributes
    hWnd = hWnd;
    hInst = hInst;

	HRESULT hr;
	IDXGIFactory* factory;
	hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(hr)) return false;

	IDXGIAdapter* adapter;
	hr = factory->EnumAdapters(0, &adapter);
	if (FAILED(hr)) return false;

	IDXGIOutput* output;
	hr = adapter->EnumOutputs(0, &output);
	if (FAILED(hr)) return false;

	UINT modeCount = 0;
	hr = output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modeCount, NULL);
	if (FAILED(hr)) return false;

	DXGI_MODE_DESC* modeList = new DXGI_MODE_DESC[modeCount];
	output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &modeCount, modeList);
	if (FAILED(hr)) return false;

	for (UINT i = 0; i < modeCount; ++i)
	{
		if (modeList[i].Width == 640 && modeList[i].Height == 480)
		{
			Mode = modeList[i];
		}
	}

	delete[] modeList;
	modeList = 0;
	output->Release();
	output = 0;
	adapter->Release();
	adapter = 0;
	factory->Release();
	factory = 0;

    // Swap chain structure
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ::ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = Mode.Width;
    swapChainDesc.BufferDesc.Height = Mode.Height;
    swapChainDesc.BufferDesc.Format = Mode.Format;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = Mode.RefreshRate.Numerator;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = Mode.RefreshRate.Denominator;
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = (HWND)hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = true;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDesc.Flags = 0;

    // Supported feature levels
    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    // Supported driver levels
    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE, D3D_DRIVER_TYPE_SOFTWARE
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    // Flags
    UINT flags = 0;
#ifdef _DEBUG
    flags = D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Create the D3D device and the swap chain
    hr = ::D3D11CreateDeviceAndSwapChain(
        NULL, 
        D3D_DRIVER_TYPE_HARDWARE, 
        NULL, 
        flags, 
        featureLevels,
        numFeatureLevels,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &pSwapChain, 
        &pD3DDevice, 
        &featureLevel,
        &pD3DContext
        );

    // Check device
    if (FAILED(hr))	{
        ::MessageBox(hWnd, TEXT("A DX11 Video Card is Required"), TEXT("ERROR"), MB_OK);
        return false;
    }

    // Get the back buffer from the swapchain
    ID3D11Texture2D *pBackBuffer;
    hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) {
        ::MessageBox(hWnd, TEXT("Unable to get back buffer"), TEXT("ERROR"), MB_OK);
        return false;
    }

    // Create the render target view
    hr = pD3DDevice->CreateRenderTargetView(pBackBuffer, NULL, &pD3DRenderTargetView);

    // Release the back buffer
    if (pBackBuffer != NULL)
        pBackBuffer->Release();

    // Check render target view
    if (FAILED(hr)) {
        ::MessageBox(hWnd, TEXT("Unable to create render target view"), TEXT("ERROR"), MB_OK);
        return false;
    }

    // Set the render target
    pD3DContext->OMSetRenderTargets(1, &pD3DRenderTargetView, NULL);

	// Set the viewport
    D3D11_VIEWPORT viewPort;
    viewPort.Width = (float)Mode.Width;
    viewPort.Height = (float)Mode.Height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    viewPort.TopLeftX = 0;
    viewPort.TopLeftY = 0;
    pD3DContext->RSSetViewports(1, &viewPort);

    // Load content
    return LoadContent();
}

void CDx11Base::Terminate()
{
    // Unload content
    UnloadContent();

    // Clean up
    if (pD3DRenderTargetView != NULL)
        pD3DRenderTargetView->Release();
    pD3DRenderTargetView= NULL;
    if (pSwapChain != NULL)
        pSwapChain->Release();
    pSwapChain= NULL;
    if (pD3DContext != NULL)
        pD3DContext->Release();
    pD3DContext= NULL;
    if (pD3DDevice != NULL)
        pD3DDevice->Release();
    pD3DDevice= NULL;
}

bool CDx11Base::CompileShader(LPCSTR szFilePath, LPCSTR szFunc, LPCSTR szShaderModel, ID3DBlob** buffer)
{
    // Set flags
    DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif
    
    // Compile shader
    HRESULT hr;
    ID3DBlob* errBuffer = 0;
    hr = ::D3DX11CompileFromFile(
        szFilePath, 0, 0, szFunc, szShaderModel,
        flags, 0, 0, buffer, &errBuffer, 0);

    // Check for errors
    if (FAILED(hr)) {
        if (errBuffer != NULL) {
            ::OutputDebugStringA((char*)errBuffer->GetBufferPointer());
			::MessageBox(hWnd, (char*)errBuffer->GetBufferPointer(), "ERROR", MB_OK);
            errBuffer->Release();
        }
        return false;
    }
    
    // Cleanup
    if (errBuffer != NULL)
        errBuffer->Release( );
    return true;
}

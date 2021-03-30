// Dx11Base.h : Defines the CDx11Base class.
// By Geelix School of Serious Games and Edutainment.

#ifndef _DX11BASE_H_
#define _DX11BASE_H_

#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcompiler.h>
#include <dxerr.h>
#include <xnamath.h>

#include <vector>

class CDx11Base
{
// Constructors
public:
    CDx11Base();
    virtual ~CDx11Base();

// Methods
public:
    bool Initialize(HWND hWnd, HINSTANCE hInst);
    void Terminate();
    bool CompileShader(LPCSTR szFilePath, LPCSTR szFunc, LPCSTR szShaderModel, ID3DBlob** buffer);

// Overrides
public:
    virtual bool LoadContent() = 0;
    virtual void UnloadContent() = 0;

    virtual void Update() = 0;
    virtual void Render() = 0;

// Attributes
public:
    HWND hWnd;
    HINSTANCE hInst;

    ID3D11Device* pD3DDevice;
    IDXGISwapChain* pSwapChain;
    ID3D11DeviceContext* pD3DContext;
    ID3D11RenderTargetView*	pD3DRenderTargetView;

	DXGI_MODE_DESC Mode;
};


#endif // _DX11BASE_H_

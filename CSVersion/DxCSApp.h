
#ifndef _DXCSAPP_H_
#define _DXCSAPP_H_

#include "Dx11Base.h"

class DxCSApp : public CDx11Base
{
// Constructors
public:
    DxCSApp();
    virtual ~DxCSApp();

	// Start Overrides
    virtual bool LoadContent();
    virtual void UnloadContent();

    virtual void Update();
    virtual void Render();
	// End Overrides

protected:
    ID3D11VertexShader* pVShader;
    ID3D11PixelShader* pPShader;
	ID3D11ComputeShader* pCShader;

    ID3D11InputLayout* pInputLayout;
    ID3D11Buffer* pVertexBuffer;

	LPCWSTR DisplayShaderName = L"ShaderDisplay.hlsl";
	LPCWSTR ComputeShaderName = L"ShaderCompute.hlsl";
};


#endif // _DXCSAPP_H_

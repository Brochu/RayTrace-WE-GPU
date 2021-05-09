
#ifndef _DXCSAPP_H_
#define _DXCSAPP_H_

#include "Dx11Base.h"
#include <chrono>
#include <fstream>

// Time typedefs
using Clock = std::chrono::system_clock;

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
    std::ofstream debugLog;

    LPCWSTR DisplayShaderName = L"ShaderDisplay.hlsl";
    LPCWSTR ComputeShaderName = L"ShaderCompute.hlsl";

    ID3D11VertexShader* pVShader;
    ID3D11PixelShader* pPShader;
    ID3D11ComputeShader* pCShader;

    ID3D11InputLayout* pInputLayout;
    ID3D11Buffer* pVertexBuffer;

    ID3D11Texture2D* pOutputTex;
    ID3D11ShaderResourceView* pOutputSRV;
    ID3D11UnorderedAccessView* pOutputUAV;

    ID3D11SamplerState* pOutputSampler;

    ID3D11Buffer* pPerFrameCBuf;
    XMVECTOR camPos;
    XMVECTOR camLookAt;
    XMVECTOR upDir;
    XMFLOAT4 perspectiveVals; // Packed transformation matrix values

    long long t0;
    long long tlast;
    long long tnow;
    XMFLOAT4 timeVals; // Packed time values

    FLOAT sampleCount;

    ID3D11Buffer* pWorldCBuf;
};


#endif // _DXCSAPP_H_

#ifndef _DXRAYTRACE_H_
#define _DXRAYTRACE_H_

#include "Dx11Base.h"

class DXRayTrace : public CDx11Base
{
// Constructors
public:
    DXRayTrace();
    virtual ~DXRayTrace();

// Overrides
public:
    virtual bool LoadContent();
    virtual void UnloadContent();

    virtual void Update();
    virtual void Render();

// Members
protected:
    ID3D11VertexShader* pVS;
    ID3D11PixelShader* pPS;
    ID3D11InputLayout* pInputLayout;
    ID3D11Buffer* pVertexBuffer;

    ID3D11Buffer* pCamSettingsBuffer;
    //ID3D11Buffer* pWorldBuffer;
};


#endif // _DXRAYTRACE_H_

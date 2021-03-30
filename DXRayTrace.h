#ifndef _DXRAYTRACE_H_
#define _DXRAYTRACE_H_

#include "Dx11Base.h"

#include "Vec3.h"
#include "Color.h"
#include "Ray.h"
#include "Hittable.h"
#include "Sphere.h"
#include "Hittable_list.h"
#include "Camera.h"

#include <vector>

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

	void TraceTexture();
	Color ray_color(const Ray& r, const Hittable& world, int depth) const;

// Members
protected:
    ID3D11VertexShader* pVS;
    ID3D11PixelShader* pPS;
    ID3D11InputLayout* pInputLayout;
    ID3D11Buffer* pVertexBuffer;
    ID3D11ShaderResourceView* pColorMap;
    ID3D11SamplerState* pColorMapSampler;

    ID3D11Texture2D* pRayTraceTex;
    ID3D11ShaderResourceView* pRayTraceMap;

	std::vector<BYTE> RayTraceData;
};


#endif // _DXRAYTRACE_H_


#ifndef _DXCSAPP_H_
#define _DXCSAPP_H_

#include "Dx11Base.h"

class DxCSApp : public CDx11Base
{
// Constructors
public:
    DxCSApp();
    virtual ~DxCSApp();

// Overrides
public:
    virtual bool LoadContent();
    virtual void UnloadContent();

    virtual void Update();
    virtual void Render();
};


#endif // _DXCSAPP_H_

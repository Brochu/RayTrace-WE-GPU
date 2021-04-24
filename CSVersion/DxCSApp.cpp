
#include "DxCSApp.h"

DxCSApp::DxCSApp()
{
}

DxCSApp::~DxCSApp()
{
}


//////////////////////////////////////////////////////////////////////
// Overrides

bool DxCSApp::LoadContent()
{
    return true;
}

void DxCSApp::UnloadContent()
{
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

    // Present back buffer to display
    m_pSwapChain->Present(0, 0);
}

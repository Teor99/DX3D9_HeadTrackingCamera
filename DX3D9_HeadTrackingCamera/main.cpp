#include <d3d9.h>
#include <d3dx9.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include "upd_server.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

extern struct CameraCoordsPacket cc;

HWND g_hWnd;
LPDIRECT3D9 d3d = NULL;
LPDIRECT3DDEVICE9 d3ddev = NULL;
LPDIRECT3DVERTEXBUFFER9 v_buffer = NULL;
ID3DXFont* g_pFont;
D3DXMATRIX startViewMatrix;

struct CUSTOMVERTEX {
    FLOAT x, y, z;
    DWORD color;
};

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)

bool InitFont(const char* pszFont, int ptSize, LPD3DXFONT& pFont)
{
    static DWORD dwQuality = 0;

    int logPixelsY = 0;

    // Convert from font point size to pixel size.

    if (HDC hDC = GetDC((0)))
    {
        logPixelsY = GetDeviceCaps(hDC, LOGPIXELSY);
        ReleaseDC(0, hDC);
    }

    int fontCharHeight = -ptSize * logPixelsY / 72;

    // Now create the font. Prefer anti-aliased text.

    HRESULT hr = D3DXCreateFont(
        d3ddev,
        fontCharHeight,                 // height
        0,                              // width
        FW_BOLD,                        // weight
        1,                              // mipmap levels
        FALSE,                          // italic
        DEFAULT_CHARSET,                // char set
        OUT_DEFAULT_PRECIS,             // output precision
        ANTIALIASED_QUALITY,            // quality
        DEFAULT_PITCH | FF_DONTCARE,    // pitch and family
        pszFont,                        // font name
        &pFont);

    return SUCCEEDED(hr) ? true : false;
}

void initD3D(HWND hWnd) {
    d3d = Direct3DCreate9(D3D_SDK_VERSION);

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;

    d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &d3dpp, &d3ddev);
    InitFont("Consolas", 10, g_pFont);
}

void init_graphics() {
    d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);
    d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

    d3ddev->SetFVF(D3DFVF_CUSTOMVERTEX);

    d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);
    d3ddev->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

    d3ddev->SetRenderState(D3DRS_AMBIENT, 0xffffffff);
}

void init_geometry() {
    CUSTOMVERTEX vertices[] = {
        { -1.0f, 1.0f, -1.0f, 0xff0000ff, },
        { 1.0f, 1.0f, -1.0f, 0xff0000ff, },
        { -1.0f, -1.0f, -1.0f, 0xff0000ff, },
        { 1.0f, -1.0f, -1.0f, 0xff0000ff, },
        { -1.0f, 1.0f, 1.0f, 0xff0000ff, },
        { 1.0f, 1.0f, 1.0f, 0xff0000ff, },
        { -1.0f, -1.0f, 1.0f, 0xff0000ff, },
        { 1.0f, -1.0f, 1.0f, 0xff0000ff, },
    };

    d3ddev->CreateVertexBuffer(8 * sizeof(CUSTOMVERTEX),
        0, D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &v_buffer, NULL);

    VOID* pVoid;
    v_buffer->Lock(0, 0, (void**)&pVoid, 0);
    memcpy(pVoid, vertices, sizeof(vertices));
    v_buffer->Unlock();
}

void init_camera() {
    D3DXVECTOR3 eye(0.0f, 0.0f, -4.0f);
    D3DXVECTOR3 at(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);

    D3DXMatrixLookAtLH(&startViewMatrix, &eye, &at, &up);

    D3DXMATRIX projectionMatrix;
    D3DXMatrixPerspectiveFovLH(&projectionMatrix, D3DX_PI / 4, 1.0f, 0.1f, 1000.0f);
    d3ddev->SetTransform(D3DTS_PROJECTION, &projectionMatrix);
}

void render_frame() {

    D3DXMATRIX viewMatrix = startViewMatrix;

    D3DXMATRIX rotationMatrix;
    D3DXMatrixRotationYawPitchRoll(&rotationMatrix, D3DXToRadian(-cc.yaw), D3DXToRadian(cc.pitch), D3DXToRadian(cc.roll));
    D3DXMatrixMultiply(&viewMatrix, &startViewMatrix, &rotationMatrix);
    //viewMatrix._41 += cc.x;
    //viewMatrix._42 += cc.y;
    //viewMatrix._43 += cc.z;
    d3ddev->SetTransform(D3DTS_VIEW, &viewMatrix);

    d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);

    d3ddev->BeginScene();

    d3ddev->SetStreamSource(0, v_buffer, 0, sizeof(CUSTOMVERTEX));
    d3ddev->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

    // text rendering
    std::ostringstream output;
    RECT rcClient;

    output.setf(std::ios::fixed, std::ios::floatfield);
    output << std::setprecision(2);

    output
        << "Camera view matrix:" << std::endl
        << "  " << viewMatrix._11 << "\t" << viewMatrix._12 << "\t" << viewMatrix._13 << "\t" << viewMatrix._14 << std::endl
        << "  " << viewMatrix._21 << "\t" << viewMatrix._22 << "\t" << viewMatrix._23 << "\t" << viewMatrix._24 << std::endl
        << "  " << viewMatrix._31 << "\t" << viewMatrix._32 << "\t" << viewMatrix._33 << "\t" << viewMatrix._34 << std::endl
        << "  " << viewMatrix._41 << "\t" << viewMatrix._42 << "\t" << viewMatrix._43 << "\t" << viewMatrix._44 << std::endl
        << "OpenTrack data:" << std::endl
        << "    x:\t" << cc.x << std::endl
        << "    y:\t" << cc.y << std::endl
        << "    z:\t" << cc.z << std::endl
        << "  yaw:\t" << cc.yaw << std::endl
        << "pitch:\t" << cc.pitch << std::endl
        << " roll:\t" << cc.roll << std::endl;

    GetClientRect(g_hWnd, &rcClient);
    rcClient.left += 4;
    rcClient.top += 2;

    g_pFont->DrawText(0, output.str().c_str(), -1, &rcClient,
        DT_EXPANDTABS | DT_LEFT, D3DCOLOR_XRGB(255, 255, 0));
    //

    d3ddev->EndScene();

    d3ddev->Present(NULL, NULL, NULL, NULL);
}

void cleanD3D() {
    stopUdpServer();
    g_pFont->Release();
    v_buffer->Release();
    d3ddev->Release();
    d3d->Release();
}

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow) {
    HWND hWnd;

    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        MsgProc,
        0L,
        0L,
        GetModuleHandle(NULL),
        NULL,
        NULL,
        NULL,
        NULL,
        "D3D Tutorial",
        NULL
    };

    std::thread t1(udpServer);

    RegisterClassEx(&wc);

    g_hWnd = CreateWindow("D3D Tutorial", "DX3D9_HeadTrackingCamera",
        WS_OVERLAPPEDWINDOW, 100, 100, 800, 800,
        NULL, NULL, wc.hInstance, NULL);

    initD3D(g_hWnd);
    init_graphics();
    init_geometry();
    init_camera();

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            render_frame();
        }
    }

    cleanD3D();
    return msg.wParam;
}
//**********************************************************************
//
// ShaderFramework.cpp
// 
// Super simple C-style framework for Shader Demo
// (NEVER ever write framework like this when you are making real games.)

//
// Author: Pope Kim
//
//**********************************************************************

#include "ShaderFramework.h"
#include <stdio.h>

#define PI           3.14159265f
#define FOV          (PI/4.0f)							// Field of View
#define ASPECT_RATIO (WIN_WIDTH/(float)WIN_HEIGHT)		// aspect ratio of screen
#define NEAR_PLANE   1									
#define FAR_PLANE    10000								


//----------------------------------------------------------------------
// Global variables
//----------------------------------------------------------------------

// D3D-related
LPDIRECT3D9             gpD3D = NULL;					// D3D
LPDIRECT3DDEVICE9       gpD3DDevice = NULL;				// D3D device

// Fonts
ID3DXFont*              gpFont = NULL;

// Models
LPD3DXMESH				gpTeapot = NULL;

// Shaders
LPD3DXEFFECT			gpEnvironmentMappingShader = NULL;

// Textures
LPDIRECT3DTEXTURE9		gpStoneDM = NULL;
LPDIRECT3DTEXTURE9		gpStoneSM = NULL;
LPDIRECT3DTEXTURE9		gpStoneNM = NULL;
LPDIRECT3DCUBETEXTURE9	gpSnowENV = NULL;

// Application Name
const char*				gAppName = "Super Simple Shader Demo Framework";

// Rotation around UP vector
float					gRotationY = 0.0f;

// world position of the light
D3DXVECTOR4				gWorldLightPosition(500.0f, 500.0f, -500.0f, 1.0f);

// ���� ����
D3DXVECTOR4				gLightColor(0.7f, 0.7f, 1.0f, 1.0f);

// world position of the camera
D3DXVECTOR4				gWorldCameraPosition(0.0f, 0.0f, -200.0f, 1.0f);

//-----------------------------------------------------------------------
// Program entry point/message loop
//-----------------------------------------------------------------------

// entry point
INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, INT)
{
	// register windows class
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
		gAppName, NULL };
	RegisterClassEx(&wc);

	// Create the program window
	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	HWND hWnd = CreateWindow(gAppName, gAppName,
		style, CW_USEDEFAULT, 0, WIN_WIDTH, WIN_HEIGHT,
		GetDesktopWindow(), NULL, wc.hInstance, NULL);

	// Client Rect size will be same as WIN_WIDTH and WIN_HEIGHT
	POINT ptDiff;
	RECT rcClient, rcWindow;

	GetClientRect(hWnd, &rcClient);
	GetWindowRect(hWnd, &rcWindow);
	ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
	ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
	MoveWindow(hWnd, rcWindow.left, rcWindow.top, WIN_WIDTH + ptDiff.x, WIN_HEIGHT + ptDiff.y, TRUE);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	// Initialize everything including D3D
	if (!InitEverything(hWnd))
		PostQuitMessage(1);

	// Message Loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else // If there is no message to process, update and render the scene
		{
			PlayDemo();
		}
	}

	UnregisterClass(gAppName, wc.hInstance);
	return 0;
}

// Message procedure
LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		ProcessInput(hWnd, wParam);
		break;

	case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Keyboard input handling
void ProcessInput(HWND hWnd, WPARAM keyPress)
{
	switch (keyPress)
	{
		// When ESC key is pressed, exit the program
	case VK_ESCAPE:
		PostMessage(hWnd, WM_DESTROY, 0L, 0L);
		break;
	}
}

//------------------------------------------------------------
// Game loop
//------------------------------------------------------------
void PlayDemo()
{
	Update();
	RenderFrame();
}

// Game logic update
void Update()
{
}

//------------------------------------------------------------
// rendering
//------------------------------------------------------------

void RenderFrame()
{
	D3DCOLOR bgColor = 0xFF0000FF;	// background color - blue

	gpD3DDevice->Clear(0, NULL, (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER), bgColor, 1.0f, 0);

	gpD3DDevice->BeginScene();
	{
		RenderScene();				// draw 3D objects and so on
		RenderInfo();				// display debug info
	}
	gpD3DDevice->EndScene();

	gpD3DDevice->Present(NULL, NULL, NULL, NULL);
}


// draw 3D objects and so on
void RenderScene()
{
	// make the view matrix
	D3DXMATRIXA16 matView;
	D3DXVECTOR3 vEyePt(gWorldCameraPosition.x, gWorldCameraPosition.y, gWorldCameraPosition.z);
	D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);

	// projection matrix
	D3DXMATRIXA16			matProjection;
	D3DXMatrixPerspectiveFovLH(&matProjection, FOV, ASPECT_RATIO, NEAR_PLANE, FAR_PLANE);

	// for each frame, we roate 0.4 degree
	gRotationY += 0.4f * PI / 180.0f;
	if (gRotationY > 2 * PI)
	{
		gRotationY -= 2 * PI;
	}

	// world matrix
	D3DXMATRIXA16			matWorld;
	D3DXMatrixRotationY(&matWorld, gRotationY);

	// concatenate world/view/projection matrices
	D3DXMATRIXA16 matWorldView;
	D3DXMATRIXA16 matWorldViewProjection;
	D3DXMatrixMultiply(&matWorldView, &matWorld, &matView);
	D3DXMatrixMultiply(&matWorldViewProjection, &matWorldView, &matProjection);

	// set shader global variables
	gpEnvironmentMappingShader->SetMatrix("gWorldMatrix", &matWorld);
	gpEnvironmentMappingShader->SetMatrix("gWorldViewProjectionMatrix", &matWorldViewProjection);

	gpEnvironmentMappingShader->SetVector("gWorldLightPosition", &gWorldLightPosition);
	gpEnvironmentMappingShader->SetVector("gWorldCameraPosition", &gWorldCameraPosition);

	gpEnvironmentMappingShader->SetVector("gLightColor", &gLightColor);
	gpEnvironmentMappingShader->SetTexture("DiffuseMap_Tex", gpStoneDM);
	gpEnvironmentMappingShader->SetTexture("SpecularMap_Tex", gpStoneSM);
	gpEnvironmentMappingShader->SetTexture("NormalMap_Tex", gpStoneNM);
	gpEnvironmentMappingShader->SetTexture("EnvironmentMap_Tex", gpSnowENV);

	// start a shader
	UINT numPasses = 0;
	gpEnvironmentMappingShader->Begin(&numPasses, NULL);
	{
		for (UINT i = 0; i < numPasses; ++i)
		{
			gpEnvironmentMappingShader->BeginPass(i);
			{
				// draw a sphere
				gpTeapot->DrawSubset(0);
			}
			gpEnvironmentMappingShader->EndPass();
		}
	}
	gpEnvironmentMappingShader->End();
}

// display debug info
void RenderInfo()
{
	// text color
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 255, 255, 255);

	// location to display the text
	RECT rct;
	rct.left = 5;
	rct.right = WIN_WIDTH / 3;
	rct.top = 5;
	rct.bottom = WIN_HEIGHT / 3;

	// display debug key info
	gpFont->DrawText(NULL, "Demo Framework\n\nESC: Exit", -1, &rct, 0, fontColor);
}

//------------------------------------------------------------
// Initialization code
//------------------------------------------------------------
bool InitEverything(HWND hWnd)
{
	// init D3D
	if (!InitD3D(hWnd))
	{
		return false;
	}

	// loading models, shaders and textures
	if (!LoadAssets())
	{
		return false;
	}

	// load fonts
	if (FAILED(D3DXCreateFont(gpD3DDevice, 20, 10, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, (DEFAULT_PITCH | FF_DONTCARE),
		"Arial", &gpFont)))
	{
		return false;
	}

	return true;
}

// init D3D object and device
bool InitD3D(HWND hWnd)
{
	// D3D object
	gpD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!gpD3D)
	{
		return false;
	}

	// fill in the structure needed to create a D3D device
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferWidth = WIN_WIDTH;
	d3dpp.BackBufferHeight = WIN_HEIGHT;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality = 0;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.Windowed = TRUE;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24X8;
	d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	d3dpp.FullScreen_RefreshRateInHz = 0;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	// create a D3D device
	if (FAILED(gpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &gpD3DDevice)))
	{
		return false;
	}

	return true;
}

bool LoadAssets()
{
	// loading textures
	gpStoneDM = LoadTexture("Fieldstone_DM.tga");
	if (!gpStoneDM)
	{
		return false;
	}

	gpStoneSM = LoadTexture("Fieldstone_SM.tga");
	if (!gpStoneSM)
	{
		return false;
	}

	gpStoneNM = LoadTexture("Fieldstone_NM.tga");
	if (!gpStoneNM)
	{
		return false;
	}

	D3DXCreateCubeTextureFromFile(gpD3DDevice, "Snow_ENV.dds", &gpSnowENV);
	if (!gpSnowENV)
	{
		return false;
	}

	// loading shaders
	gpEnvironmentMappingShader = LoadShader("EnvironmentMapping.fx");
	if (!gpEnvironmentMappingShader)
	{
		return false;
	}

	// loading models
	gpTeapot = LoadModel("TeapotWithTangent.x");
	if (!gpTeapot)
	{
		return false;
	}

	return true;
}

// loading shaders
LPD3DXEFFECT LoadShader(const char * filename)
{
	LPD3DXEFFECT ret = NULL;

	LPD3DXBUFFER pError = NULL;
	DWORD dwShaderFlags = 0;

#if _DEBUG
	dwShaderFlags |= D3DXSHADER_DEBUG;
#endif

	D3DXCreateEffectFromFile(gpD3DDevice, filename,
		NULL, NULL, dwShaderFlags, NULL, &ret, &pError);

	// if failed at loading shaders, display compile error
	// to output window
	if (!ret && pError)
	{
		int size = pError->GetBufferSize();
		void *ack = pError->GetBufferPointer();

		if (ack)
		{
			char* str = new char[size];
			sprintf(str, (const char*)ack, size);
			OutputDebugString(str);
			delete[] str;
		}
	}

	return ret;
}

// loading models
LPD3DXMESH LoadModel(const char * filename)
{
	LPD3DXMESH ret = NULL;
	if (FAILED(D3DXLoadMeshFromX(filename, D3DXMESH_SYSTEMMEM, gpD3DDevice, NULL, NULL, NULL, NULL, &ret)))
	{
		OutputDebugString("failed at loading a model: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	};

	return ret;
}

// loading textures
LPDIRECT3DTEXTURE9 LoadTexture(const char * filename)
{
	LPDIRECT3DTEXTURE9 ret = NULL;
	if (FAILED(D3DXCreateTextureFromFile(gpD3DDevice, filename, &ret)))
	{
		OutputDebugString("failed at loading a texture: ");
		OutputDebugString(filename);
		OutputDebugString("\n");
	}

	return ret;
}
//------------------------------------------------------------
// cleanup code
//------------------------------------------------------------

void Cleanup()
{
	// release fonts
	if (gpFont)
	{
		gpFont->Release();
		gpFont = NULL;
	}

	// release models
	if (gpTeapot)
	{
		gpTeapot->Release();
		gpTeapot = NULL;
	}

	// release shaders
	if (gpEnvironmentMappingShader)
	{
		gpEnvironmentMappingShader->Release();
		gpEnvironmentMappingShader = NULL;
	}

	// release textures
	if (gpStoneDM)
	{
		gpStoneDM->Release();
		gpStoneDM = NULL;
	}

	if (gpStoneSM)
	{
		gpStoneSM->Release();
		gpStoneSM = NULL;
	}

	if (gpStoneNM)
	{
		gpStoneNM->Release();
		gpStoneNM = NULL;
	}

	if (gpSnowENV)
	{
		gpSnowENV->Release();
		gpSnowENV = NULL;
	}

	// release D3D
	if (gpD3DDevice)
	{
		gpD3DDevice->Release();
		gpD3DDevice = NULL;
	}

	if (gpD3D)
	{
		gpD3D->Release();
		gpD3D = NULL;
	}
}


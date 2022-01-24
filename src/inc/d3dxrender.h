
#pragma once


#ifdef __cplusplus
extern "C" {
#endif
#include <windows.h>

#ifdef D3DXRENDER_EXPORTS
#define D3DXRENDER_API     __declspec(dllexport)
#else
#define D3DXRENDER_API     __declspec(dllimport)
#endif


D3DXRENDER_API void* D3DXRenderCreate(HWND hWnd, int nVideoWidth, int nVideoHeight, BOOL bIsLock);
D3DXRENDER_API void D3DXRenderClose(void* pHandle, BOOL bIsLock);
D3DXRENDER_API bool D3DXRenderDraw(void* pHandle, HWND hWnd, void* pData, int nSize, int nWidth, int nHeight, BOOL bIsLock);


#ifdef __cplusplus
}
#endif



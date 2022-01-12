
#include "d3dxrender.h"
#include "D3DRenderer9.h"

void* D3DXRenderCreate(HWND hWnd, int nVideoWidth, int nVideoHeight, BOOL bIsLock)
{
    CD3DRenderer9* pD3DObj;
    pD3DObj = new CD3DRenderer9();
	pD3DObj->Create(hWnd, nVideoWidth, nVideoHeight, bIsLock);
	return pD3DObj;
}


void D3DXRenderClose(void* pHandle, BOOL bIsLock)
{
	CD3DRenderer9* pD3DObj = (CD3DRenderer9*)pHandle;
	if (pHandle) {
		pD3DObj->Close(bIsLock);
	}
}



bool D3DXRenderDraw(void* pHandle, HWND hWnd, BOOL bIsLock)
{
    CD3DRenderer9* pD3DObj = (CD3DRenderer9*)pHandle;
    HANDLE fh;
    fh = CreateFile(_T("frame-63.yuv"),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (fh == INVALID_HANDLE_VALUE) {
        return 0;
    }
    char* pBuffer = (char*)malloc(704 * 480 * 3 / 2);
    DWORD read_bytes;
    if (!ReadFile(fh, (void*)pBuffer, 704 * 480 * 3 / 2, &read_bytes, NULL)) {
        return 0;

    }
    pD3DObj->DrawVideo(hWnd, pBuffer, 704 * 480 * 3 / 2, 704, 480, NULL);

    if (pBuffer) free(pBuffer);

    return 1;
}



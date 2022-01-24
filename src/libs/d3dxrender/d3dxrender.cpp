
#include "d3dxrender.h"
#include "D3DRenderer9.h"

// 마샬링으로 앤해서 포인터 테스트 용도
//#define __POINTER_TEST__
#ifdef __POINTER_TEST__
static void* pTestRender;
#endif
void* D3DXRenderCreate(HWND hWnd, int nVideoWidth, int nVideoHeight, BOOL bIsLock)
{
    CD3DRenderer9* pD3DObj;
    pD3DObj = new CD3DRenderer9();
	pD3DObj->Create(hWnd, nVideoWidth, nVideoHeight, bIsLock);
#ifdef __POINTER_TEST__
    pTestRender = pD3DObj;
#endif
    printf("<D3DXRenderCreate> pD3DObj=%X\n", pD3DObj);
	return pD3DObj;
}


void D3DXRenderClose(void* pHandle, BOOL bIsLock)
{
	CD3DRenderer9* pD3DObj = (CD3DRenderer9*)pHandle;
	if (pHandle) {
		pD3DObj->Close(bIsLock);
	}
}



bool D3DXRenderDraw(void* pHandle, HWND hWnd, void* pBuffer, int nSize, int nWidth, int nHeight, BOOL bIsLock)
{
    CD3DRenderer9* pD3DObj = (CD3DRenderer9*)pHandle;

    //printf("<D3DXRenderDraw> pHandle=%X\n", pHandle);

#ifdef __POINTER_TEST__
    pD3DObj = (CD3DRenderer9*)pTestRender;
#endif
//#define __USE_RENDER_TEST__
#ifdef __USE_RENDER_TEST__
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
    char* pBuf = (char*)malloc(704 * 480 * 3 / 2);
    DWORD read_bytes;
    if (!ReadFile(fh, (void*)pBuf, 704 * 480 * 3 / 2, &read_bytes, NULL)) {
        return 0;

    }

    pD3DObj->DrawVideo(hWnd, pBuf, 704 * 480 * 3 / 2, 704, 480, NULL);

    if (pBuf) free(pBuf);
#else
    pD3DObj->DrawVideo(hWnd, pBuffer, nSize, nWidth, nHeight, NULL);
#endif
    return 1;
}



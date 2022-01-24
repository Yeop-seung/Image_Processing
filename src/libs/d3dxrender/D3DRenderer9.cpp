//#include "stdafx.h"
#include "pch.h"
#include "D3DRenderer9.h"
//#include "motion.h"

// ftp://210.93.53.175/2011

//struct Vertex
//{
//	float	 x, y, z;		// �簢�� ��ǥ
//	D3DCOLOR color;			// ����
//	float	 tu, tv;		// �ؽ��� ��ǥ
//};

#define ICON_SIZE					16
#define MAX_TITLE_HEIGHT			30
//#define RECT_OFFSET					3

#define LINE_THICKNESS_NORMAL		2
#define LINE_THICKNESS_BOLD			5

//#define D3DFVF_CUSTOMVERTEX			(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

#define IMAGE_TEST_FILE				0
#define USE_RGB24_DRAWING			0

#define	ERROR_COLOR					D3D_RGB_BLUE

#if IMAGE_TEST_FILE
static LPTSTR pTestFile[] = { _T("1.jpg"), _T("2.jpg"), _T("3.jpg"), _T("4.jpg"), _T("5.jpg"), _T("6.jpg") };
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(x)				{ if (x) x->Release(); x = NULL; }
#endif

#ifndef SAFE_ARRAY_DELETE
#define SAFE_ARRAY_DELETE(p)		{ if(p) delete[] (p); (p) = NULL; }
#endif

#ifndef __WFUNCTION__
#ifdef UNICODE
#define __W(x)					L##x
#define _W(x)					__W(x)
#define __WFUNCTION__			_W(__FUNCTION__)
#define __WFILE__				_W(__FILE__)
#else
#define __WFUNCTION__			__FUNCTION__
#define __WFILE__				__FILE__
#endif
#endif

#define IDD_ABOUTBOX			103
Vertex	g_stVertexQuad[4] = { { -1.0f,  1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0.0f, 0.0f },
							  {  1.0f,  1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 0.0f },
							  { -1.0f, -1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0.0f, 1.0f },
							  {  1.0f, -1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 1.0f } };


extern "C" void yv12_to_uyvy_mmx(BYTE * dst, int dst_stride, BYTE * y_src, BYTE * u_src, BYTE * v_src, int y_stride, int uv_stride, int width, int height);
extern "C" void yv12_to_yuyv_mmx(BYTE * dst, int dst_stride, BYTE * y_src, BYTE * u_src, BYTE * v_src, int y_stride, int uv_stride, int width, int height);
extern "C" void yv12_to_rgb32_mmx(BYTE * dst, int dst_stride, BYTE * y_src, BYTE * u_src, BYTE * v_src, int y_stride, int uv_stride, int width, int height);
extern "C" void yv12_to_rgb32_c(BYTE * dst, int dst_stride, BYTE * y_src, BYTE * u_src, BYTE * v_src, int y_stride, int uv_stride, int width, int height);


// ������
CD3DRenderer9::CD3DRenderer9()
{
	m_pID3D = NULL;
	m_pID3DDevice = NULL;

	m_pID3DTextureSurface = NULL;
	m_pID3DTextureVideo = NULL;
	m_pID3DTextureIcon = NULL;
	// 	m_pID3DVertexVideo		= NULL;

	m_pID3DFont = NULL;
	m_pID3DSprite = NULL;
	m_plD3DBoxLine = NULL;

	ZeroMemory(&m_stD3DPresentParam, sizeof(D3DPRESENT_PARAMETERS));
	ZeroMemory(&m_stVideoSize, sizeof(SIZE));

	m_bIsDeviceChanging = FALSE;

	m_fPosX = 0.0f;
	m_fPosY = 0.0f;
	m_fPosZ = 1.0f;

	m_dwBackgroundColor = D3D_RGB_BLUE;
	m_dwTitleTextColor = D3D_RGB_WHITE;
	m_strTitle = _T("Title");

	m_lStatusFlag = 0;
	m_nStatusIconID = -1;
	m_dwStatusTextColor = D3D_RGB_WHITE;
	m_strStatusText = _T("Status");

	m_strTextString = _T("");
	m_dwTextColor = D3D_RGB_WHITE;

	m_szIconOrginal.cx = m_szIconOrginal.cy = 0;
	m_szIconResized.cx = m_szIconResized.cy = 0;

	ZeroMemory(&m_stFontInfo, sizeof(D3DXFONT_DESC));
	InitializeCriticalSection(&m_stLock);
}

// �Ҹ���
CD3DRenderer9::~CD3DRenderer9()
{
	Close();
	DeleteCriticalSection(&m_stLock);
}

// ��
void CD3DRenderer9::Lock()
{
	EnterCriticalSection(&m_stLock);
}

// ���
void CD3DRenderer9::Unlock()
{
	LeaveCriticalSection(&m_stLock);
}

// Ÿ��Ʋ�� ���� ����
void CD3DRenderer9::SetTitleInfo(CString strText, D3DCOLOR dwTextColor)
{
	m_strTitle = strText;
	m_dwTitleTextColor = dwTextColor;
}

// �ϴ� ���� ���� ����
void CD3DRenderer9::SetStatusInfo(CString strText, D3DCOLOR dwTextColor)
{
	m_strStatusText = strText;
	m_dwStatusTextColor = dwTextColor;
}

// �ؽ�Ʈ ������ ����
void CD3DRenderer9::SetTextDataInfo(CString strText, D3DCOLOR dwTextColor)
{
	m_strTextString = strText;
	m_dwTextColor = dwTextColor;
}

// �α׸� ����Ѵ�.
void CD3DRenderer9::WriteD3DError(LPCTSTR pFormat, ...)
{
	SYSTEMTIME	st;
	FILE* fp;
	va_list		argptr;
	int			cnt;
	TCHAR		szLogFileName[MAX_PATH] = _T("");
	TCHAR		szAppDataPath[MAX_PATH] = _T("");

	GetModuleFileName(NULL, szAppDataPath, _countof(szAppDataPath));
	PathRemoveFileSpec(szAppDataPath);
	PathAppend(szAppDataPath, _T("Log"));

	if (!PathFileExists(szAppDataPath))
	{
		if (!CreateDirectory(szAppDataPath, NULL))
			return;
	}

	GetLocalTime(&st);

	if (szAppDataPath[_tcslen(szAppDataPath) - 1] == _T('\\'))
		_stprintf_s(szLogFileName, _T("%s%04d%02d%02d.log"), szAppDataPath, st.wYear, st.wMonth, st.wDay);
	else
		_stprintf_s(szLogFileName, _T("%s\\%04d%02d%02d.log"), szAppDataPath, st.wYear, st.wMonth, st.wDay);

	if ((fp = _tfopen(szLogFileName, _T("a+"))) == NULL)
		return;

	_ftprintf(fp, _T("%02d:%02d:%02d.%03d "), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	va_start(argptr, pFormat);
	cnt = _vftprintf(fp, pFormat, argptr);
	va_end(argptr);

	fclose(fp);
	return;
}

// D3D ����
HRESULT CD3DRenderer9::Create(HWND hWnd, int nVideoWidth, int nVideoHeight, BOOL bIsLock)
{
	// �θ� �����찡 ���� ���
	if (!::IsWindow(hWnd))
	{
		WriteD3DError(_T("[Error] %s : Invalid parameter.\n"), __WFUNCTION__);
		return E_INVALIDARG;
	}

	if (nVideoWidth <= 0 || nVideoHeight <= 0)
	{
		WriteD3DError(_T("[Error] %s : Invalid video size.\n"), __WFUNCTION__);
		return E_INVALIDARG;
	}

	HRESULT	hr = S_OK;

	if (bIsLock)
		Lock();

	//  	if(m_pID3DDevice)
	//  		m_pID3DDevice->EvictManagedResources();

	DestroyD3D9();
	DestroyResource(FALSE);

	// D3D ����
	if (SUCCEEDED(hr = CreateD3D(hWnd)))
	{
		// ���� ���ҽ� ����
		if (FAILED(hr = CreateResource(hWnd, nVideoWidth, nVideoHeight)))
			WriteD3DError(_T("[Error] %s : Failed to create a resource. error=0x%x\n"), __WFUNCTION__, hr);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to create a D3D. error=0x%x\n"), __WFUNCTION__, hr);

	// ������ ������ ��� ����
	if (FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("[Error] Failed to create a D3D. error=0x%x"), hr);
		Close(FALSE);
	}

	if (bIsLock)
		Unlock();

	return hr;
}

// D3D �缳��
HRESULT CD3DRenderer9::Reset(HWND hWnd, BOOL bIsLock)
{
	if (!::IsWindow(hWnd))
	{
		WriteD3DError(_T("[Error] %s : Invalid parameter.\n"), __WFUNCTION__);
		return E_INVALIDARG;
	}

	// D3D ����̽��� ��ȿ�� ���
	if (!IsValid())
	{
		WriteD3DError(_T("[Error] %s : Invalid device.\n"), __WFUNCTION__);
		return E_POINTER;
	}

	HRESULT hr = E_FAIL;
	CRect	rcWindow;

	// ����̽� ���� ��
	m_bIsDeviceChanging = TRUE;

	GetClientRect(hWnd, &rcWindow);

	if (bIsLock)
		Lock();

	// ����̽� ��� ó��
	OnLostDevice();

	if (rcWindow.Width() <= 0 || rcWindow.Height() <= 0)
		WriteD3DError(_T("[Warning] %s : Invalid windows size. w=%d h=%d\n"), __WFUNCTION__, rcWindow.Width(), rcWindow.Height());

	// ����� ũ�� ����
	m_stD3DPresentParam.BackBufferWidth = rcWindow.Width() <= 0 ? 1 : rcWindow.Width();
	m_stD3DPresentParam.BackBufferHeight = rcWindow.Height() <= 0 ? 1 : rcWindow.Height();

	// D3D �ʱ�ȭ
	if (SUCCEEDED(hr = m_pID3DDevice->Reset(&m_stD3DPresentParam)))
	{
		// ���ҽ� �ʱ�ȭ
		if (FAILED(hr = OnResetDevice(hWnd)))
			WriteD3DError(_T("[Error] %s : Failed to reset resource. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, rcWindow.Width(), rcWindow.Height());
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to reset device. error=0x%x\n"), __WFUNCTION__, hr);

	// �ʱ�ȭ�� ������ ���
	if (FAILED(hr))
	{
		// D3D �����
		if (FAILED(hr = Create(hWnd, m_stVideoSize.cx, m_stVideoSize.cy, !bIsLock)))
		{
			WriteD3DError(_T("[Error] %s : Failed to recreate a D3D. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, m_stVideoSize.cx, m_stVideoSize.cy);
			Close(FALSE);
		}
	}

	// D3D �ʱ�ȭ �� ȭ���� �ʱ�ȭ�Ѵ�.
	if (SUCCEEDED(hr))
	{
		if (FAILED(hr = ClearScreen(hWnd,  !bIsLock)))
			WriteD3DError(_T("[Error] %s : Failed to initialize a screen. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, m_stVideoSize.cx, m_stVideoSize.cy);
	}
	else
		DXTRACE_ERR_MSGBOX(_T("[Error] Failed to reset a D3D. error=0x%x"), hr);

	if (bIsLock)
		Unlock();

	// ����̽� ���� �Ϸ�
	m_bIsDeviceChanging = FALSE;

	return hr;
}

// ��ġ�� �ҽ��� ���
void CD3DRenderer9::OnLostDevice()
{
	if (m_pID3DSprite)	m_pID3DSprite->OnLostDevice();
	if (m_pID3DFont)		m_pID3DFont->OnLostDevice();
	if (m_plD3DBoxLine)	m_plD3DBoxLine->OnLostDevice();

	// �缳���� �Ұ����� ���ҽ� ����
	DestroyResource(TRUE);
}

// ��ġ �缳��
HRESULT	CD3DRenderer9::OnResetDevice(HWND hWnd)
{
	if (m_pID3DSprite)	m_pID3DSprite->OnResetDevice();
	if (m_pID3DFont)		m_pID3DFont->OnResetDevice();
	if (m_plD3DBoxLine)	m_plD3DBoxLine->OnResetDevice();

	// �缳���� �Ұ����� ���ҽ� ����
	return CreateResource(hWnd, m_stVideoSize.cx, m_stVideoSize.cy, TRUE);
}

// ��ġ�� ����� ���
BOOL CD3DRenderer9::IsDeviceLost(HWND hWnd, BOOL bIsLock)
{
	if (!IsValid())
		return TRUE;

	HRESULT hr;

	// ���� ����̽� ���� üũ
	if (SUCCEEDED(hr = m_pID3DDevice->TestCooperativeLevel()))
		return FALSE;

	// ��ġ ��� : ���� �� �� �ٽ� �׽�Ʈ�Ѵ�.
	if (hr == D3DERR_DEVICELOST)
	{
		WriteD3DError(_T("[Warning] %s : Lost device. error=0x%x\n"), __WFUNCTION__, hr);
		Sleep(100);
		return TRUE;
	}
	// ��ġ�� ����̹� ����
	else if (hr == D3DERR_DRIVERINTERNALERROR)
	{
		WriteD3DError(_T("[Fatal] %s : Internal driver error. error=0x%x\n"), __WFUNCTION__, hr);
		PostQuitMessage(0);
		return TRUE;
	}
	// ��ġ �缳�� ��û
	else if (hr == D3DERR_DEVICENOTRESET)
	{
		// ��ġ�� �缳���Ѵ�.
		if (FAILED(hr = Reset(hWnd, bIsLock)))
		{
			WriteD3DError(_T("[Error] %s : Failed to reset a device in D3DERR_DEVICENOTRESET. error=0x%x\n"), __WFUNCTION__, hr);
			return FALSE;
		}

		return TRUE;
	}
	// ����� ���� ��Ʈ�� â�� �ߴ� ��� ������ ������ �� �߻��Ǵ� ����
	else if (hr == 141953144 || hr == 141953143)
	{
		if (FAILED(hr = Reset(hWnd, bIsLock)))
		{
			WriteD3DError(_T("[Error] %s : Failed to reset a device in D3DERR_DEVICENOTRESET. error=0x%x\n"), __WFUNCTION__, hr);
			return FALSE;
		}

		return TRUE;
	}

	WriteD3DError(_T("[Error] %s : unknown device check error. error=0x%x\n"), __WFUNCTION__, hr);
	return TRUE;
}

// DXVA2 ����
void CD3DRenderer9::Close(BOOL bIsLock)
{
	if (bIsLock)
		Lock();

	DestroyResource();
	DestroyD3D9();

	if (bIsLock)
		Unlock();
}

// D3D �ʱ�ȭ
HRESULT CD3DRenderer9::CreateD3D(HWND hWnd)
{
	HRESULT  hr = E_FAIL;

	// D3D ����
	if ((m_pID3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
		WriteD3DError(_T("[Error] %s : Failed to create a D3D. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	D3DDISPLAYMODE	stDisplayMode;
	D3DCAPS9		stDisplayInfo;
	D3DFORMAT		eFormat = D3DFMT_UNKNOWN;
	DWORD			dwVertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;

	// ���÷��� ��� ���
	if (FAILED(hr = m_pID3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &stDisplayMode)))
	{
		WriteD3DError(_T("[Error] %s : Failed to get a display information. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// ����Ϸ��� ���÷��� ������ ������ ��忡�� �����Ǵ� �� �˻�
	if (FAILED(hr = m_pID3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, stDisplayMode.Format, stDisplayMode.Format, TRUE)))
	{
		WriteD3DError(_T("[Error] %s : Unable to use a display format (%d) in H/W. error=0x%x\n"), __WFUNCTION__, stDisplayMode.Format, hr);
		return hr;
	}

	eFormat = stDisplayMode.Format;

	// ����̽� ���� ���
	if (FAILED(hr = m_pID3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &stDisplayInfo)))
	{
		WriteD3DError(_T("[Error] %s : Failed to get a graphic card information. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// ��Ƽ �ؽ��� �������� �������� �ʴ� ��� ���� ó��
	if (stDisplayInfo.NumSimultaneousRTs <= 1)
	{
		WriteD3DError(_T("[Error] %s : Unable to use multi-texture in this system. error=0x%x\n"), __WFUNCTION__, hr);
		return E_FAIL;
	}

	if ((stDisplayInfo.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		dwVertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// 	// �׷��� ī�尡 �ϵ���� �ؼ����̼��� �����ϴ� ���
	// 	if(stDisplayInfo.DevCaps & D3DDEVCAPS_PUREDEVICE && dwVertexProcessing & D3DCREATE_HARDWARE_VERTEXPROCESSING)
	// 		dwVertexProcessing |= D3DCREATE_PUREDEVICE;

	CRect	rcWindow;

	GetClientRect(hWnd, &rcWindow);

	// D3D ���� �ɼ� ����
	m_stD3DPresentParam.BackBufferWidth = rcWindow.Width();
	m_stD3DPresentParam.BackBufferHeight = rcWindow.Height();
	m_stD3DPresentParam.BackBufferFormat = eFormat;							// �ĸ� ���� ���� ����
	m_stD3DPresentParam.BackBufferCount = 1;								// �ĸ� ���� ��
	m_stD3DPresentParam.SwapEffect = D3DSWAPEFFECT_DISCARD;			// ���� ���� ȿ��
	m_stD3DPresentParam.hDeviceWindow = hWnd;								// ����̽� ������
	m_stD3DPresentParam.Windowed = TRUE;								// ������ ��� �Ǵ� ��ü ���
	m_stD3DPresentParam.EnableAutoDepthStencil = TRUE;
	m_stD3DPresentParam.AutoDepthStencilFormat = D3DFMT_D16;

	m_stD3DPresentParam.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_stD3DPresentParam.MultiSampleQuality = 0;
	m_stD3DPresentParam.Flags = D3DPRESENTFLAG_VIDEO;
	m_stD3DPresentParam.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	m_stD3DPresentParam.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;	// ��� ǥ��

	// D3D ����̽� ����
	if (SUCCEEDED(hr = m_pID3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, dwVertexProcessing | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED,
		&m_stD3DPresentParam, &m_pID3DDevice)))
	{
		BOOL	bUseAntiAlias = TRUE;

		// ��Ƽ ���ø�(��Ƽ�ٸ����) ���� ���� �˻�
		if (SUCCEEDED(hr = CheckAntialias(m_stD3DPresentParam, m_pID3D, 0, D3DDEVTYPE_HAL, D3DMULTISAMPLE_4_SAMPLES)))
			bUseAntiAlias = TRUE;

		if (bUseAntiAlias)
			hr = m_pID3DDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
		else
			hr = m_pID3DDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to create a D3D device. error=0x%x\n"), __WFUNCTION__, hr);

	return hr;
}

// D3D �Ҹ�
void CD3DRenderer9::DestroyD3D9()
{
	// �ݵ�� ���� ������ �������� �����ؾ� ��!!
	SAFE_RELEASE(m_pID3DDevice);
	SAFE_RELEASE(m_pID3D);
}

// ��Ƽ ���ø�(Anti-Alias) ���� ���� �˻�
HRESULT CD3DRenderer9::CheckAntialias(D3DPRESENT_PARAMETERS& stParam, IDirect3D9* pD3D, UINT uAdapter, D3DDEVTYPE eDevType, D3DMULTISAMPLE_TYPE eAntiAliasType)
{
	HRESULT hr = E_FAIL;
	DWORD	QualityBackBuffer = 0;
	DWORD	QualityZBuffer = 0;
	DWORD	dwAliasValue = (DWORD)eAntiAliasType;

	while (dwAliasValue)
	{
		// �ĸ� ���� ��Ƽ�ٸ���� ��� ���� ���� üũ
		if (SUCCEEDED(hr = pD3D->CheckDeviceMultiSampleType(uAdapter, eDevType, stParam.BackBufferFormat, stParam.Windowed,
			(D3DMULTISAMPLE_TYPE)dwAliasValue, &QualityBackBuffer)))
		{
			// ���Ľ� ���� ��Ƽ�ٸ���� ��� ���� ���� üũ
			if (SUCCEEDED(hr = pD3D->CheckDeviceMultiSampleType(uAdapter, eDevType, stParam.AutoDepthStencilFormat, stParam.Windowed,
				(D3DMULTISAMPLE_TYPE)dwAliasValue, &QualityZBuffer)))
			{
				// ��Ƽ�ٸ������ �����ϹǷ� Ÿ���� �����Ѵ�.
				stParam.MultiSampleType = (D3DMULTISAMPLE_TYPE)dwAliasValue;

				// QualityBackBuffer�� QualityZBuffer �� ���� �� ����
				if (QualityBackBuffer < QualityZBuffer)
					stParam.MultiSampleQuality = QualityBackBuffer - 1;
				else
					stParam.MultiSampleQuality = QualityZBuffer - 1;

				break;
			}
		}

		// ��Ƽ�ٸ���� �ܰ踦 �Ѵܰ� ����
		dwAliasValue--;
	}

	// ������ ���
	if (dwAliasValue == 0 && FAILED(hr))
	{
		if (SUCCEEDED(hr = pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, eDevType, stParam.BackBufferFormat,
			stParam.Windowed, D3DMULTISAMPLE_NONMASKABLE, &QualityBackBuffer)))
		{
			// enable non maskable multi sampling
			stParam.MultiSampleType = D3DMULTISAMPLE_NONMASKABLE;
			stParam.MultiSampleQuality = QualityBackBuffer - 1;
		}
	}

	return hr;
}

// ���� ���ҽ� ����
HRESULT CD3DRenderer9::CreateResource(HWND hWnd, int nVideoWidth, int nVideoHeight, BOOL bIsReset)
{
	HRESULT hr;
	CRect	rcWindow;

	// D3D�� �������� ���� ���
	if (!IsValid())
	{
		WriteD3DError(_T("[Error] %s : Invalid device pointer. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	// ������ ���� ���
	GetClientRect(hWnd, &rcWindow);

	// ��ġ�� Reset�ϴ� ���� �������� �ʴ´�.
	if (!bIsReset)
	{
		// ��Ʈ ����
		if (FAILED(hr = CreateFont()))
		{
			WriteD3DError(_T("[Error] %s : Failed to create a font. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}

		// ��������Ʈ ����
		if (FAILED(hr = CreateSprite()))
		{
			WriteD3DError(_T("[Error] %s : Failed to create a sprite. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}

		// ���� ����
		if (FAILED(hr = CreateLine()))
		{
			WriteD3DError(_T("[Error] %s : Failed to create a line object. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}
	}

	// ������ �׸� �ؽ��� ����
	if (FAILED(hr = CreateVideoTexture(hWnd, nVideoWidth, nVideoHeight)))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a texture for video. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// ���� ������ ����
	if (FAILED(hr = CreateStatusIcon()))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a status icon. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// �������� ��ǥ ����
	if (SUCCEEDED(hr))
	{
		D3DXMATRIX	matProj;
		CRect		rc;

		GetClientRect(hWnd, &rc);
		D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI * 0.5f, 1.0f, 0.1f, 1000.0f);

		if (SUCCEEDED(hr = m_pID3DDevice->SetTransform(D3DTS_PROJECTION, &matProj)))
		{
			if (FAILED(hr = m_pID3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE)))
				WriteD3DError(_T("[Error] %s : Failed to set off the light. error=0x%x\n"), __WFUNCTION__, hr);
		}
		else
			WriteD3DError(_T("[Error] %s : Failed to set a projection view position. error=0x%x\n"), __WFUNCTION__, hr);
	}

	return hr;
}

// ���ҽ� ����
void CD3DRenderer9::DestroyResource(BOOL bIsReset)
{
	// ��ġ�� �缳���ϴ� ���� �������� �ʴ´�.
	if (!bIsReset)
	{
		SAFE_RELEASE(m_pID3DFont);
		SAFE_RELEASE(m_pID3DSprite);
		SAFE_RELEASE(m_plD3DBoxLine);
	}

	SAFE_RELEASE(m_pID3DTextureIcon);
	// 	SAFE_RELEASE(m_pID3DVertexVideo);
	SAFE_RELEASE(m_pID3DTextureVideo);
	SAFE_RELEASE(m_pID3DTextureSurface);
}

// ��������Ʈ ����
HRESULT	CD3DRenderer9::CreateSprite()
{
	SAFE_RELEASE(m_pID3DSprite);
	return D3DXCreateSprite(m_pID3DDevice, &m_pID3DSprite);
}

// ������ �׸� �ؽ��� ����
HRESULT CD3DRenderer9::CreateVideoTexture(HWND hWnd, int nVideoWidth, int nVideoHeight)
{
	HRESULT	hr = S_OK;

	SAFE_RELEASE(m_pID3DTextureVideo);
	SAFE_RELEASE(m_pID3DTextureSurface);
	// 	SAFE_RELEASE(m_pID3DVertexVideo);

#if IMAGE_TEST_FILE

	D3DXIMAGE_INFO	stImageInfo;
	TCHAR			szFileName[MAX_PATH] = _T("");

	GetModuleFileName(NULL, szFileName, _countof(szFileName));
	PathRemoveFileSpec(szFileName);
	PathAppend(szFileName, pTestFile[0]);

	// �׽�Ʈ�� �̹��� ���� �б�
	if (SUCCEEDED(hr = D3DXGetImageInfoFromFile(szFileName, &stImageInfo)))
	{
		if (FAILED(hr = D3DXCreateTextureFromFileEx(m_pID3DDevice, szFileName, stImageInfo.Width, stImageInfo.Height, 1, 0,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL,
			&m_pID3DTextureVideo)))
		{
			WriteD3DError(_T("[Error] %s : Failed to load a test image file (%s). error=0x%x\n"), __WFUNCTION__, szFileName, hr);
			return hr;
		}
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to get a test image file information (%s). error=0x%x\n"), __WFUNCTION__, szFileName, hr);

	m_stVideoSize.cx = stImageInfo.Width;
	m_stVideoSize.cy = stImageInfo.Height;

#else

	// ���� ũ�⿡ ������ �ִ� ��� �⺻������ ����
	if (nVideoWidth == 0 || nVideoHeight == 0)
	{
		WriteD3DError(_T("[Warning] %s : Inputted an invalid video size. w=%d h=%d\n"), __WFUNCTION__, nVideoWidth, nVideoHeight);

		nVideoWidth = 704;
		nVideoHeight = 480;
	}

#if USE_RGB24_DRAWING

	// ������ �׸� �ؽ��� ����
	if (FAILED(hr = D3DXCreateTexture(m_pID3DDevice, nVideoWidth, nVideoHeight, 1, D3DUSAGE_DYNAMIC,
		D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pID3DTextureVideo)))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a texture for video. error=0x%x w=%d h=%d mem=%d\n"),
			__WFUNCTION__, hr, nVideoWidth, nVideoHeight, m_pID3DDevice->GetAvailableTextureMem());
		return hr;
	}

#else

	// ������ �׸� �ؽ��� ����
	if (FAILED(hr = D3DXCreateTexture(m_pID3DDevice, nVideoWidth, nVideoHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pID3DTextureVideo)))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a render target texture for video. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, nVideoWidth, nVideoHeight);
		return hr;
	}

	// ������ �׸� ���ǽ� ����
	if (FAILED(hr = m_pID3DDevice->CreateOffscreenPlainSurface(nVideoWidth, nVideoHeight, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pID3DTextureSurface, NULL)))
	{
		WriteD3DError(_T("[Warning] %s : Failed to create a render target texture for video. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, nVideoWidth, nVideoHeight);
		return hr;
	}

#endif

	m_stVideoSize.cx = nVideoWidth;
	m_stVideoSize.cy = nVideoHeight;

	// �ؽ��ĸ� �ε巴�� �׸����� ����
	hr = m_pID3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	hr = m_pID3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	hr = m_pID3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	// ȭ�鿡 ǥ�õ� ���� �ʱ�ȭ
	if (FAILED(hr = InitializeVideo(D3D_RGB_BLUE)))
	{
		WriteD3DError(_T("[Error] %s : Failed to initialize a texture. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, nVideoWidth, nVideoHeight);
		return hr;
	}

#endif

	//	if(SUCCEEDED(hr))
	//	{
	// 		// ���� �ؽ��ĸ� �׸� ��ǥ ����
	// 		if(SUCCEEDED(hr = m_pID3DDevice->CreateVertexBuffer(4 * sizeof(Vertex), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, 
	// 															D3DPOOL_DEFAULT, &m_pID3DVertexVideo, NULL)))
	// 		{
	// 			LPVOID	pVertices = NULL;
	// 			Vertex	stVertexQuad[4] = { { -1.0f,  1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0.0f, 0.0f },
	// 									    {  1.0f,  1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 0.0f },
	// 									    { -1.0f, -1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 0.0f, 1.0f },
	// 									    {  1.0f, -1.0f, 0.0f, D3DCOLOR_ARGB(255, 255, 255, 255), 1.0f, 1.0f } };
	// 
	// 			if(SUCCEEDED(hr = m_pID3DVertexVideo->Lock(0, sizeof(stVertexQuad), (LPVOID *)&pVertices, D3DLOCK_DISCARD)))
	// 			{
	// 				memcpy(pVertices, stVertexQuad, sizeof(stVertexQuad));
	// 				m_pID3DVertexVideo->Unlock();
	// 			}
	// 			else
	// 				WriteD3DError(_T("[Error] %s : Failed to lock a vertex buffer. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, nVideoWidth, nVideoHeight);
	// 		}
	// 		else
	// 			WriteD3DError(_T("[Error] %s : Failed to create a vertex buffer. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, nVideoWidth, nVideoHeight);
	//	}

	return hr;
}

// ������ �ʱ�ȭ�Ѵ�.
HRESULT CD3DRenderer9::InitializeVideo(D3DCOLOR dwColor)
{
#if IMAGE_TEST_FILE
	return S_OK;
#endif

	if (!m_pID3DTextureVideo)
	{
		WriteD3DError(_T("[Error] %s : Invalid texture. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	LPDIRECT3DSURFACE9	pITempTextureSurface = NULL;
	HRESULT				hr;

	if (SUCCEEDED(hr = m_pID3DTextureVideo->GetSurfaceLevel(0, &pITempTextureSurface)))
	{
		// ���� �ʱ� ���� ����
		if (FAILED(hr = m_pID3DDevice->ColorFill(pITempTextureSurface, NULL, dwColor)))
			WriteD3DError(_T("[Error] %s : Failed to fill color. error=0x%x\n"), __WFUNCTION__, hr);

		SAFE_RELEASE(pITempTextureSurface);
	}

	return hr;
}

// ���� ��Ʈ ����
HRESULT CD3DRenderer9::CreateFont()
{
	HRESULT hr = S_OK;
	HDC		hDC = NULL;

	SAFE_RELEASE(m_pID3DFont);

	// ��Ʈ�� ���� ��� �⺻ �ý��� ��Ʈ�� ����
	if (_tcslen(m_stFontInfo.FaceName) <= 0)
	{
		NONCLIENTMETRICS	nc;

		ZeroMemory(&nc, sizeof(NONCLIENTMETRICS));
		nc.cbSize = sizeof(NONCLIENTMETRICS);

		// �ý��� �⺻ ��Ʈ ���� ���
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &nc, 0);

		m_stFontInfo.Width = nc.lfMenuFont.lfWidth;
		m_stFontInfo.Height = nc.lfMenuFont.lfHeight;

		m_stFontInfo.Weight = FW_BOLD;
		m_stFontInfo.MipLevels = 0;
		m_stFontInfo.Italic = FALSE;
		m_stFontInfo.CharSet = DEFAULT_CHARSET;
		m_stFontInfo.OutputPrecision = OUT_DEFAULT_PRECIS;
		m_stFontInfo.Quality = DEFAULT_QUALITY;
		m_stFontInfo.PitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;

		_tcscpy_s(m_stFontInfo.FaceName, _countof(m_stFontInfo.FaceName), nc.lfMenuFont.lfFaceName);
	}

	// ��Ʈ ����
	if (FAILED(hr = D3DXCreateFontIndirect(m_pID3DDevice, &m_stFontInfo, &m_pID3DFont)))
		WriteD3DError(_T("[Error] %s : Failed to create a font. error=0x%x\n"), __WFUNCTION__, hr);

	return hr;
}

// ��Ʈ ����
BOOL CD3DRenderer9::ChangeFont(LOGFONT* pFontInfo, COLORREF clrFontColor)
{
	if (!IsValid() || !pFontInfo)
		return FALSE;

	HRESULT	hr;
	BOOL	bSuccess = TRUE;

	ZeroMemory(&m_stFontInfo, sizeof(D3DXFONT_DESC));

	m_stFontInfo.Height = pFontInfo->lfHeight;
	m_stFontInfo.Width = pFontInfo->lfWidth;
	m_stFontInfo.Weight = pFontInfo->lfWeight;
	m_stFontInfo.MipLevels = 0;
	m_stFontInfo.Italic = pFontInfo->lfItalic;
	m_stFontInfo.CharSet = pFontInfo->lfCharSet;
	m_stFontInfo.OutputPrecision = pFontInfo->lfOutPrecision;
	m_stFontInfo.Quality = pFontInfo->lfQuality;
	m_stFontInfo.PitchAndFamily = pFontInfo->lfPitchAndFamily;
	_tcscpy_s(m_stFontInfo.FaceName, _countof(m_stFontInfo.FaceName), pFontInfo->lfFaceName);

	Lock();

	// ��Ʈ�� �����Ѵ�.
	if (FAILED(hr = CreateFont()))
	{
		bSuccess = FALSE;
		WriteD3DError(_T("[Error] %s : Failed to create a font. error=0x%x\n"), __WFUNCTION__, hr);
	}

	Unlock();

	return bSuccess;
}

// ���� ������/��Ʈ���� �����Ѵ�.
HRESULT CD3DRenderer9::CreateStatusIcon()
{
	SAFE_RELEASE(m_pID3DTextureIcon);

	if (m_nStatusIconID <= 0)
		return S_OK;

	D3DXIMAGE_INFO	stImageInfo;
	HMODULE			hInstance = NULL;
	HRESULT			hr;

	ZeroMemory(&stImageInfo, sizeof(D3DXIMAGE_INFO));

#ifdef _USRDLL
	hInstance = GetModuleHandle(NULL);
#else
	hInstance = AfxGetInstanceHandle();
#endif

	// �̹����� ũ�⸦ ��´�.
	if (FAILED(hr = D3DXGetImageInfoFromResource(hInstance, MAKEINTRESOURCE(m_nStatusIconID), &stImageInfo)))
	{
		WriteD3DError(_T("[Error] %s : Failed to get a image for icon. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	m_szIconOrginal.cx = stImageInfo.Width;
	m_szIconOrginal.cy = stImageInfo.Height;

	// 2�� ����� �������� ũ�⸦ �ٲٴ� ��� ��ǥ������ scale�Ǽ� ����� �׷����� �ʴ� ���� �߻���.
	// ���� �̹��� ����� 2�� ����� ó��
	m_szIconResized.cx = GetIconSize(m_szIconOrginal.cx);
	m_szIconResized.cy = GetIconSize(m_szIconOrginal.cy);

	// ���ҽ����� ��Ʈ���� �о���δ�.
	if (FAILED(hr = D3DXCreateTextureFromResource(m_pID3DDevice, hInstance, MAKEINTRESOURCE(m_nStatusIconID), &m_pID3DTextureIcon)))
		WriteD3DError(_T("[Error] %s : Failed to create a texture for icon. error=0x%x\n"), __WFUNCTION__, hr);

	return hr;
}

// �������� ũ�⸦ ��´� => Sprite�� ���� ����� 2�� ����� ������.
int	CD3DRenderer9::GetIconSize(int nSize)
{
	int		i = 1;
	int		nResizeSize = 0;

	for (;;)
	{
		nResizeSize = (int)pow((double)2, (double)i);

		if (nResizeSize >= nSize)
			break;

		i++;
	}

	return nResizeSize;
}

// ��� ���� ����
HRESULT CD3DRenderer9::CreateLine()
{
	HRESULT hr;

	SAFE_RELEASE(m_plD3DBoxLine);

	// �ڽ��� �׸� �� ���Ǵ� ���� ����
	if (FAILED(hr = D3DXCreateLine(m_pID3DDevice, &m_plD3DBoxLine)))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a line. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// ������ �β� ����
	if (SUCCEEDED(hr = m_plD3DBoxLine->SetWidth(LINE_THICKNESS_NORMAL)))
	{
		if (FAILED(hr = m_plD3DBoxLine->SetAntialias(TRUE)))
			WriteD3DError(_T("[Error] %s : Failed to set anti-alias mode of the line. error=0x%x\n"), __WFUNCTION__, hr);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to assign a width of the line. error=0x%x\n"), __WFUNCTION__, hr);

	return hr;
}

// ȭ���� �ʱ�ȭ�Ѵ�.
HRESULT	CD3DRenderer9::ClearScreen(HWND hWnd, BOOL bIsLock)
{
	D3DCOLOR dwColor = D3D_RGB_BLUE;
	if (!IsValid())
	{
		WriteD3DError(_T("[Warning] %s : Failed to get a D3D. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	if (!::IsWindow(hWnd))
	{
		WriteD3DError(_T("[Warning] %s : Invalid window. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return S_FALSE;
	}

	HRESULT		hr;
	D3DXMATRIX	matWorld;
	CRect		rcWindow;

	// ����̽� ���� ���� ���� �׸��� �ʴ´�.
	if (m_bIsDeviceChanging)
		return S_OK;

	if (bIsLock)
		Lock();

	// ��ġ�� �Ҿ���� ���
	if (IsDeviceLost(hWnd, !bIsLock))
	{
		hr = S_FALSE;
		WriteD3DError(_T("[Warning] %s : Lost device. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		goto ClearScreen_Complete;
	}

	// ȭ���� ���� �������� �ʱ�ȭ�Ѵ�.
	if (FAILED(hr = InitializeVideo(dwColor)))
	{
		WriteD3DError(_T("[Error] %s : Failed to initialize a screen. error=0x%x\n"), __WFUNCTION__, hr);
		goto ClearScreen_Complete;
	}

	GetClientRect(hWnd, &rcWindow);

	// ������ ȭ�� ��ġ ����
	D3DXMatrixIdentity(&matWorld);
	D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 1.0f);

	if (FAILED(hr = m_pID3DDevice->SetTransform(D3DTS_WORLD, &matWorld)))
	{
		WriteD3DError(_T("[Error] %s : Failed to set a world view position. error=0x%x\n"), __WFUNCTION__, hr);
		goto ClearScreen_Complete;
	}

	// �׸��� ����
	if (SUCCEEDED(hr = m_pID3DDevice->BeginScene()))
	{
		if ((hr = m_pID3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, dwColor, 1.0f, 0)))
			WriteD3DError(_T("[Error] %s : Failed to clear scene. error=0x%x\n"), __WFUNCTION__, hr);

		// ���ڿ� �׸���
		if (FAILED(hr = DrawAllString(rcWindow, dwColor)))
			WriteD3DError(_T("[Error] %s : Failed to draw a strings. error=0x%x\n"), __WFUNCTION__, hr);

		if (FAILED(hr = DrawChannelBox(FALSE, rcWindow)))
			WriteD3DError(_T("[Error] %s : Failed to draw a box. error=0x%x\n"), __WFUNCTION__, hr);

		if (FAILED(hr = m_pID3DDevice->EndScene()))
			WriteD3DError(_T("[Error] %s : Failed to end scene. error=0x%x\n"), __WFUNCTION__, hr);

		// ȭ�� ���
		hr = m_pID3DDevice->Present(NULL, NULL, NULL, NULL);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to begin scene. error=0x%x\n"), __WFUNCTION__, hr);

ClearScreen_Complete:

	if (bIsLock)
		Unlock();

	return hr;
}

// ������ �׸���.
HRESULT	CD3DRenderer9::DrawVideo(HWND hWnd, LPVOID pFrame, int nSize, int nVideoWidth, int nVideoHeight,
	BOOL bMotionDraw)
{
	LPVOID pMotion = NULL;
	D3DCOLOR dwColor = D3D_RGB_YELLOW;
	
	if (!IsValid())
	{
		WriteD3DError(_T("[Warning] %s : Failed to get a D3D. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}
	
	if (!::IsWindow(hWnd))
	{
		WriteD3DError(_T("[Warning] %s : Invalid window. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return S_FALSE;
	}

	// ����̽� ���� ���� ���� �׸��� �ʴ´�.
	if (m_bIsDeviceChanging)
		return S_OK;

	D3DXMATRIX	matWorld;
	CRect		rcWindow;
	HRESULT		hr;

	Lock();

	// ��ġ�� �Ҿ���� ���
	if (IsDeviceLost(hWnd, FALSE))
	{
		hr = S_FALSE;
		goto DrawVideo_Complete;
	}

	// ���� ũ�Ⱑ �ٸ� ���� �ؽ��ĸ� ������Ѵ�.
	if (m_stVideoSize.cx != nVideoWidth || m_stVideoSize.cy != nVideoHeight)
	{
		if (FAILED(hr = CreateVideoTexture(hWnd, nVideoWidth, nVideoHeight)))
		{
			WriteD3DError(_T("[Error] %s : Failed to create a texture for video. error=0x%x\n"), __WFUNCTION__, hr);
			goto DrawVideo_Complete;
		}
	}

	// �����Ͱ� �ִ� ���
	if (pFrame && nSize > 0)
	{
		// ������ �׸���.
		if (FAILED(hr = FillVideo(pFrame, nSize)))
		{
			WriteD3DError(_T("[Error] %s : Failed to fill a video. error=0x%x video size=%d w=%d h=%d\n"), __WFUNCTION__, hr, nSize, nVideoWidth, nVideoHeight);
			goto DrawVideo_Complete;
		}
	}
	else
	{
		// ȭ���� ���� �������� �ʱ�ȭ�Ѵ�.
		if (FAILED(InitializeVideo(D3D_RGB_GRAY)))
			WriteD3DError(_T("[Error] %s : Failed to initialize a video. error=0x%x video size=%d w=%d h=%d\n"), __WFUNCTION__, hr, nSize, nVideoWidth, nVideoHeight);
	}

	GetClientRect(hWnd, &rcWindow);

	// ������ ȭ�� ��ġ ����
	D3DXMatrixIdentity(&matWorld);
	D3DXMatrixTranslation(&matWorld, m_fPosX, m_fPosY, m_fPosZ);

	if (FAILED(hr = m_pID3DDevice->SetTransform(D3DTS_WORLD, &matWorld)))
	{
		WriteD3DError(_T("[Error] %s : Failed to set a world view position. error=0x%x\n"), __WFUNCTION__, hr);
		goto DrawVideo_Complete;
	}

	// �׸��� ����
	if (SUCCEEDED(hr = m_pID3DDevice->BeginScene()))
	{
		if (FAILED(hr = m_pID3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 255), 1.0f, 0)))
			WriteD3DError(_T("[Error] %s : Failed to clear a screen. error=0x%x\n"), __WFUNCTION__, hr);

		if (m_pID3DTextureVideo != NULL)
		{
			if (FAILED(hr = m_pID3DDevice->SetFVF(D3DFVF_CUSTOMVERTEX)))
				WriteD3DError(_T("[Error] %s : Failed to set a vertex stream source format. error=0x%x\n"), __WFUNCTION__, hr);

			if (FAILED(hr = m_pID3DDevice->SetTexture(0, m_pID3DTextureVideo)))
				WriteD3DError(_T("[Error] %s : Failed to draw a video into the texture. error=0x%x\n"), __WFUNCTION__, hr);

			if (FAILED(hr = m_pID3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, (LPVOID)&g_stVertexQuad, sizeof(Vertex))))
				WriteD3DError(_T("[Error] %s : Failed to draw a video on the vertex stream source. error=0x%x\n"), __WFUNCTION__, hr);

			if (FAILED(hr = m_pID3DDevice->SetTexture(0, NULL)))
				WriteD3DError(_T("[Error] %s : Failed to clear a video in the device. error=0x%x\n"), __WFUNCTION__, hr);
		}
		else
			WriteD3DError(_T("[Error] %s : Invalid texture for video. error=0x%x\n"), __WFUNCTION__, hr);

		// ���ڿ� �׸���
		if (FAILED(hr = DrawAllString(rcWindow, dwColor)))
			WriteD3DError(_T("[Error] %s : Failed to draw a string. error=0x%x\n"), __WFUNCTION__, hr);

		// ������ �׸���
		DrawIcon(rcWindow);

		// ä�� �ڽ� �׸���
		if (FAILED(hr = DrawChannelBox(TRUE, rcWindow)))
			WriteD3DError(_T("[Error] %s : Failed to draw a box. error=0x%x\n"), __WFUNCTION__, hr);


		if (FAILED(hr = m_pID3DDevice->EndScene()))
			WriteD3DError(_T("[Error] %s : Failed to end scene. error=0x%x\n"), __WFUNCTION__, hr);

		// ȭ�� ���
		if (FAILED(hr = m_pID3DDevice->Present(NULL, NULL, hWnd, NULL)))
			WriteD3DError(_T("[Error] %s : Failed to present a scene. error=0x%x\n"), __WFUNCTION__, hr);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to begin scene. error=0x%x\n"), __WFUNCTION__, hr);

DrawVideo_Complete:
	Unlock();

	return hr;
}

// ������ ä���.
HRESULT CD3DRenderer9::FillVideo(LPVOID pFrame, int nSize)
{
#if IMAGE_TEST_FILE
	return TRUE;
#endif

	if (!m_pID3DTextureSurface || !m_pID3DTextureVideo)
	{
		WriteD3DError(_T("[Error] %s : Invalid texture. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	D3DLOCKED_RECT	stLockRect;
	HRESULT			hr = E_POINTER;
	char* pY = NULL, * pU = NULL, * pV = NULL;
	int				nError = 0;

	ZeroMemory(&stLockRect, sizeof(D3DLOCKED_RECT));

#if USE_RGB24_DRAWING

	// ���� ��Ʈ���� �׸���.
	if (FAILED(hr = m_pID3DTextureVideo->LockRect(0, &stLockRect, NULL, D3DLOCK_DISCARD)))
	{
		WriteD3DError(_T("[Error] %s : Failed to lock a texture. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	pY = (char*)pFrame;
	pU = pY + (m_stVideoSize.cx * m_stVideoSize.cy);
	pV = pU + (m_stVideoSize.cx * m_stVideoSize.cy / 4);

	yv12_to_rgb32_mmx((BYTE*)stLockRect.pBits, stLockRect.Pitch, (BYTE*)pY, (BYTE*)pU, (BYTE*)pV, m_stVideoSize.cx, m_stVideoSize.cx / 2, m_stVideoSize.cx, m_stVideoSize.cy);

	// �� ����
	if (FAILED(hr = m_pID3DTextureVideo->UnlockRect(0)))
	{
		WriteD3DError(_T("[Error] %s : Failed to unlock a texture. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

#else

	// ���� ��Ʈ���� �׸���.
	if (SUCCEEDED(hr = m_pID3DTextureSurface->LockRect(&stLockRect, NULL, D3DLOCK_DISCARD)))
	{
		pY = (char*)pFrame;
		pU = pY + (m_stVideoSize.cx * m_stVideoSize.cy);
		pV = pU + (m_stVideoSize.cx * m_stVideoSize.cy / 4);

		yv12_to_rgb32_c((BYTE*)stLockRect.pBits, stLockRect.Pitch, (BYTE*)pY, (BYTE*)pU, (BYTE*)pV, m_stVideoSize.cx, m_stVideoSize.cx / 2, m_stVideoSize.cx, m_stVideoSize.cy);
		//yv12_to_rgb32_mmx((BYTE*)stLockRect.pBits, stLockRect.Pitch, (BYTE*)pY, (BYTE*)pU, (BYTE*)pV, m_stVideoSize.cx, m_stVideoSize.cx / 2, m_stVideoSize.cx, m_stVideoSize.cy);
		//YV12toYUY2((BYTE *)pY, (BYTE *)pU, (BYTE *)pV, (BYTE *)stLockRect.pBits, m_stVideoSize.cx, m_stVideoSize.cy, m_stVideoSize.cx, stLockRect.Pitch);
		//yv12_to_yuyv_mmx((BYTE *)stLockRect.pBits, stLockRect.Pitch, (BYTE *)pY, (BYTE *)pU, (BYTE *)pV, m_stVideoSize.cx, m_stVideoSize.cx / 2, m_stVideoSize.cx, m_stVideoSize.cy); 

		// �� ����
		if (FAILED(hr = m_pID3DTextureSurface->UnlockRect()))
		{
			WriteD3DError(_T("[Error] %s : Failed to unlock a texture. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}

		LPDIRECT3DSURFACE9 pTempTextureSurface = NULL;

		// �׷��� ���ǽ��� �ؽ��Ŀ� �����Ѵ�.
		if (SUCCEEDED(hr = m_pID3DTextureVideo->GetSurfaceLevel(0, &pTempTextureSurface)))
		{
			if (FAILED(hr = m_pID3DDevice->StretchRect(m_pID3DTextureSurface, NULL, pTempTextureSurface, NULL, D3DTEXF_LINEAR)))
				WriteD3DError(_T("[Error] %s : Failed to stretch a texture surface. error=0x%x\n"), __WFUNCTION__, hr);

			SAFE_RELEASE(pTempTextureSurface);
		}
		else
			WriteD3DError(_T("[Error] %s : Failed to get a texture surface. error=0x%x\n"), __WFUNCTION__, hr);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to lock a texture. error=0x%x\n"), __WFUNCTION__, hr);

#endif

	return hr;
}


void CD3DRenderer9::YV12toYUY2(BYTE* pY, BYTE* pU, BYTE* pV, BYTE* pDest, int nWidth, int nHeight, int nSourcePitch, int nDestPitch)
{
	int		nRow, nCol;
	int		nDestPadBytes = nDestPitch - 2 * nWidth;
	int		nSourcePadBytes = nSourcePitch - nWidth;
	char* pDestTemp = (char*)pDest;

	for (nRow = 0; nRow < (int)nHeight; nRow += 2)
	{
		// Original, partial writes
		for (nCol = 0; nCol < (int)nWidth; nCol += 2)
		{
			// first row, YUYV
			*pDestTemp = *pY;
			*(pDestTemp + 1) = *pU;
			*(pDestTemp + 2) = *(pY + 1);
			*(pDestTemp + 3) = *pV;

			// second row, YUYV
			*(pDestTemp + nDestPitch) = *(pY + nSourcePitch);
			*(pDestTemp + nDestPitch + 1) = *pU;
			*(pDestTemp + nDestPitch + 2) = *(pY + nSourcePitch + 1);
			*(pDestTemp + nDestPitch + 3) = *pV;

			pDestTemp += 4;
			pY += 2;
			pU++;
			pV++;
		}

		// jump to start of third row
		pDestTemp += nDestPadBytes + nDestPitch;
		pY += nSourcePadBytes + nSourcePitch;
		pU += nSourcePadBytes >> 1;
		pV += nSourcePadBytes >> 1;
	}
}

// ������ ���ڿ�(����, ����, �ؽ�Ʈ ���)�� �׸���.
HRESULT	CD3DRenderer9::DrawAllString(CRect& rcWindow, D3DCOLOR dwColor)
{
	int nOffset = 5, nHeight = 20;

	if (!IsValid())
	{
		WriteD3DError(_T("[Error] %s : Invalid D3D. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	HRESULT	hr = S_OK;
	CRect	rcTitle, rcText;

	if (m_pID3DSprite != NULL)
	{
		if (FAILED(hr = m_pID3DSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE | D3DXSPRITE_DO_NOT_ADDREF_TEXTURE)))
		{
			WriteD3DError(_T("[Error] %s : Failed to start a sprite drawing. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}
	}

	// ���� ��� ����
	if (m_strTitle.GetLength() > 0 && m_pID3DFont != NULL)
	{
		rcTitle.SetRect(rcWindow.left + nOffset, rcWindow.top + nOffset, rcWindow.Width(), rcWindow.top + nHeight);
		m_pID3DFont->DrawText(m_pID3DSprite, m_strTitle, -1, &rcTitle, DT_LEFT | DT_TOP, m_dwTitleTextColor);
	}

	// �ؽ�Ʈ ������ : ���� ��� ���� �Ʒ� ������
	if (m_strTextString.GetLength() > 0 && m_pID3DFont != NULL)
	{
		rcText.SetRect(rcTitle.left, rcTitle.bottom + nOffset, rcWindow.Width(), rcTitle.bottom + nHeight);
		m_pID3DFont->DrawText(m_pID3DSprite, m_strTextString, -1, &rcText, DT_LEFT | DT_VCENTER, m_dwTitleTextColor);
	}

	// �ϴ� ���¹�(���� �����ΰ�� ��� ǥ��. �ƴѰ�� �ϴ�ǥ��)
	if (m_strStatusText.GetLength() > 0 && m_pID3DFont != NULL)
	{
		rcText = rcWindow;

		rcText.DeflateRect(nOffset, nOffset, nOffset, nOffset);

		if (dwColor == ERROR_COLOR)
			m_pID3DFont->DrawText(m_pID3DSprite, m_strStatusText, -1, &rcText, DT_CENTER | DT_VCENTER, m_dwStatusTextColor);
		else
			m_pID3DFont->DrawText(m_pID3DSprite, m_strStatusText, -1, &rcText, DT_LEFT | DT_BOTTOM, m_dwStatusTextColor);
	}

	if (m_pID3DSprite != NULL)
		m_pID3DSprite->End();

	return hr;
}

// ���� �������� �׸���.
HRESULT	CD3DRenderer9::DrawIcon(CRect& rcWindow)
{
	int nOffset = 5, nHeight = 20;

	if (!IsValid() || m_pID3DTextureIcon == NULL)
		return E_POINTER;

	HRESULT	hr = S_OK;

	if (m_pID3DSprite != NULL)
	{
		if (FAILED(hr = m_pID3DSprite->Begin(D3DXSPRITE_ALPHABLEND)))
			return hr;
	}

	CRect		rcIconPos;
	D3DXVECTOR3	stScreenPos;
	D3DXMATRIX	mScale;
	float		fXScale = 1.0f, fYScale = 1.0f;
	int			nSingleIconWidth = m_szIconResized.cy;
	float		fStartPos = (float)rcWindow.Width();
	int			nDrawCount = 1;

	fXScale = (float)((float)m_szIconOrginal.cx / (float)m_szIconResized.cx);
	fYScale = (float)((float)m_szIconOrginal.cy / (float)m_szIconResized.cy);

	// DX���� Ȯ���� �̹����� ���� ������ �����ش�.
	D3DXMatrixScaling(&mScale, fXScale, fYScale, 1.0f);
	m_pID3DSprite->SetTransform(&mScale);

	// �������� �׸���.
	stScreenPos.x = fStartPos;

	if ((m_lStatusFlag & SHOW_ICON_PTZ) == SHOW_ICON_PTZ)
	{
		// ��Ʈ�� ���� ������ �ε���
		rcIconPos.SetRect(0, 0, nSingleIconWidth, m_szIconResized.cy);

		stScreenPos.x = fStartPos - (nSingleIconWidth + RECT_OFFSET) * nDrawCount;
		stScreenPos.y = 5.0f;
		stScreenPos.z = 0.0f;

		m_pID3DSprite->Draw(m_pID3DTextureIcon, &rcIconPos, NULL, &stScreenPos, D3DCOLOR_RGBA(255, 255, 255, 255));
		nDrawCount++;
	}

	if ((m_lStatusFlag & SHOW_ICON_AUDIO) == SHOW_ICON_AUDIO)
	{
		rcIconPos.SetRect(nSingleIconWidth, 0, nSingleIconWidth * 2, m_szIconResized.cy);

		stScreenPos.x = fStartPos - (nSingleIconWidth + RECT_OFFSET) * nDrawCount;
		stScreenPos.y = 5.0f;
		stScreenPos.z = 0.0f;

		m_pID3DSprite->Draw(m_pID3DTextureIcon, &rcIconPos, NULL, &stScreenPos, D3DCOLOR_RGBA(255, 255, 255, 255));
		nDrawCount++;
	}

	if ((m_lStatusFlag & SHOW_ICON_MOTION) == SHOW_ICON_MOTION)
	{
		rcIconPos.SetRect(nSingleIconWidth * 2, 0, nSingleIconWidth * 3, m_szIconResized.cy);

		stScreenPos.x = fStartPos - (nSingleIconWidth + RECT_OFFSET) * nDrawCount;
		stScreenPos.y = 5.0f;
		stScreenPos.z = 0.0f;

		m_pID3DSprite->Draw(m_pID3DTextureIcon, &rcIconPos, NULL, &stScreenPos, D3DCOLOR_RGBA(255, 255, 255, 255));
		nDrawCount++;
	}

	if ((m_lStatusFlag & SHOW_ICON_SENSOR) == SHOW_ICON_SENSOR)
	{
		rcIconPos.SetRect(nSingleIconWidth * 3, 0, nSingleIconWidth * 4, m_szIconResized.cy);

		stScreenPos.x = fStartPos - (nSingleIconWidth + RECT_OFFSET) * nDrawCount;
		stScreenPos.y = 5.0f;
		stScreenPos.z = 0.0f;

		m_pID3DSprite->Draw(m_pID3DTextureIcon, &rcIconPos, NULL, &stScreenPos, D3DCOLOR_RGBA(255, 255, 255, 255));
		nDrawCount++;
	}

	if ((m_lStatusFlag & SHOW_ICON_RECORDING) == SHOW_ICON_RECORDING)
	{
		rcIconPos.SetRect(nSingleIconWidth * 4, 0, nSingleIconWidth * 5, m_szIconResized.cy);

		stScreenPos.x = fStartPos - (nSingleIconWidth + RECT_OFFSET) * nDrawCount;
		stScreenPos.y = 5.0f;
		stScreenPos.z = 0.0f;

		m_pID3DSprite->Draw(m_pID3DTextureIcon, &rcIconPos, NULL, &stScreenPos, D3DCOLOR_RGBA(255, 255, 255, 255));
		nDrawCount++;
	}

	if ((m_lStatusFlag & SHOW_ICON_EMERGENCY) == SHOW_ICON_EMERGENCY)
	{
		rcIconPos.SetRect(nSingleIconWidth * 5, 0, nSingleIconWidth * 6, m_szIconResized.cy);

		stScreenPos.x = fStartPos - (nSingleIconWidth + RECT_OFFSET) * nDrawCount;
		stScreenPos.y = 5.0f;
		stScreenPos.z = 0.0f;

		m_pID3DSprite->Draw(m_pID3DTextureIcon, &rcIconPos, NULL, &stScreenPos, D3DCOLOR_RGBA(255, 255, 255, 255));
	}

	if (m_pID3DSprite != NULL)
	{
		if (FAILED(hr = m_pID3DSprite->End()))
			WriteD3DError(_T("[Error] %s : Failed to end a sprite drawing. error=0x%x\n"), __WFUNCTION__, hr);
	}

	return hr;
}

// ��Ʈ���� �׵θ��� �׸���.
HRESULT	CD3DRenderer9::DrawChannelBox(BOOL bMotionDraw, CRect& rcWindow)
{
	if (!IsValid())
	{
		WriteD3DError(_T("[Error] %s : Invalid D3D. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	if (!m_plD3DBoxLine)
	{
		WriteD3DError(_T("[Error] %s : Invalid line object. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	// �׸� ������ ���� ���
	if ((rcWindow.Width() <= 0) || (rcWindow.Height() <= 0))
		return S_FALSE;

	D3DXVECTOR2 vList[5];
	D3DCOLOR	dwMotionLineColor = D3D_RGB_WHITE;
	CRect		rcBox;
	BOOL		bMotion = FALSE;
	HRESULT		hr;

	CopyRect(&rcBox, &rcWindow);

	vList[0].x = (FLOAT)rcBox.left;
	vList[0].y = (FLOAT)rcBox.top;
	vList[1].x = (FLOAT)rcBox.right;
	vList[1].y = (FLOAT)rcBox.top;
	vList[2].x = (FLOAT)rcBox.right;
	vList[2].y = (FLOAT)rcBox.bottom;
	vList[3].x = (FLOAT)rcBox.left;
	vList[3].y = (FLOAT)rcBox.bottom;
	vList[4] = vList[0];

	// ��� �����ΰ�� �׵θ� ������ ǥ��.
	if ((m_lStatusFlag & SHOW_ICON_EMERGENCY) == SHOW_ICON_EMERGENCY)
	{
		dwMotionLineColor = D3D_RGB_RED;
		bMotion = TRUE;
	}
	// ����� ��� ������ ǥ��
	else if (bMotionDraw && (m_lStatusFlag & SHOW_ICON_MOTION) == SHOW_ICON_MOTION)
	{
		dwMotionLineColor = D3D_RGB_RED;
		bMotion = TRUE;
	}

	// �Ϲ� ���� �׸���
	if (m_dwBackgroundColor == D3D_RGB_GREEN)
	{
		if (bMotion)
			m_plD3DBoxLine->SetWidth(LINE_THICKNESS_BOLD + LINE_THICKNESS_NORMAL);
		else
			m_plD3DBoxLine->SetWidth(LINE_THICKNESS_BOLD);

		if (SUCCEEDED(hr = m_plD3DBoxLine->Begin()))
		{
			m_plD3DBoxLine->Draw(vList, _countof(vList), m_dwBackgroundColor);
			m_plD3DBoxLine->End();
		}
	}
	// �˻�ä�� ����
	else if (m_dwBackgroundColor == D3D_RGB_BLUE)
	{
		m_plD3DBoxLine->SetWidth(LINE_THICKNESS_BOLD + LINE_THICKNESS_NORMAL);
		if (SUCCEEDED(hr = m_plD3DBoxLine->Begin()))
		{
			m_plD3DBoxLine->Draw(vList, _countof(vList), m_dwBackgroundColor);
			m_plD3DBoxLine->End();
		}
	}


	// ��� / ����������
	if (bMotion || m_dwBackgroundColor != D3D_RGB_GREEN)
	{
		m_plD3DBoxLine->SetWidth(LINE_THICKNESS_NORMAL);

		if (SUCCEEDED(hr = m_plD3DBoxLine->Begin()))
		{
			m_plD3DBoxLine->Draw(vList, _countof(vList), dwMotionLineColor);
			m_plD3DBoxLine->End();
		}
	}

	return S_OK;
}



// ȭ���� �̹��� ���� ����
HRESULT	CD3DRenderer9::SaveImageToFile(D3DXIMAGE_FILEFORMAT eImageType, LPTSTR pFileName)
{
#if IMAGE_TEST_FILE
	return S_OK;
#endif

	if (!m_pID3DTextureVideo)
	{
		WriteD3DError(_T("[Error] %s : Invalid texture. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	HRESULT hr = D3DXSaveTextureToFile(pFileName, eImageType, m_pID3DTextureVideo, NULL);

	if (FAILED(hr))
		WriteD3DError(_T("[Error] %s : Failed to save a screen capture to file (%s). error=0x%x\n"), __WFUNCTION__, hr, pFileName);

	return hr;
}

// �������� ȭ�� �̵�
void CD3DRenderer9::ScreenMoveLeft()
{
	m_fPosX -= 0.02f;
}

// ���������� ȭ�� �̵�
void CD3DRenderer9::ScreenMoveRight()
{
	m_fPosX += 0.02f;
}

// ���� ȭ�� �̵�
void CD3DRenderer9::ScreenMoveUp()
{
	m_fPosY -= 0.02f;
}

// �Ʒ��� ȭ�� �̵�
void CD3DRenderer9::ScreenMoveDown()
{
	m_fPosY += 0.02f;
}

// ȭ���� �۰�
void CD3DRenderer9::ScreenScaleDown()
{
	m_fPosZ -= 0.02f;
}

// ȭ���� ũ��
void CD3DRenderer9::ScreenScaleUp()
{
	m_fPosZ += 0.02f;
}

// ȭ���� ���� ũ��� ����
void CD3DRenderer9::ScreenOriginalSize()
{
	m_fPosX = 0.0f;
	m_fPosY = 0.0f;
	m_fPosZ = 1.0f;
}

BOOL CD3DRenderer9::IsScreenOriginalSize()
{
	if ((m_fPosX == 0.0f) &&
		(m_fPosY == 0.0f) &&
		(m_fPosZ == 1.0f))
		return TRUE;

	return FALSE;
}

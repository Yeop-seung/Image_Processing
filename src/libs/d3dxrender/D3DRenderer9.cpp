//#include "stdafx.h"
#include "pch.h"
#include "D3DRenderer9.h"
//#include "motion.h"

// ftp://210.93.53.175/2011

//struct Vertex
//{
//	float	 x, y, z;		// 사각형 좌표
//	D3DCOLOR color;			// 색상
//	float	 tu, tv;		// 텍스쳐 좌표
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


// 생성자
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

// 소멸자
CD3DRenderer9::~CD3DRenderer9()
{
	Close();
	DeleteCriticalSection(&m_stLock);
}

// 락
void CD3DRenderer9::Lock()
{
	EnterCriticalSection(&m_stLock);
}

// 언락
void CD3DRenderer9::Unlock()
{
	LeaveCriticalSection(&m_stLock);
}

// 타이틀바 정보 지정
void CD3DRenderer9::SetTitleInfo(CString strText, D3DCOLOR dwTextColor)
{
	m_strTitle = strText;
	m_dwTitleTextColor = dwTextColor;
}

// 하단 상태 정보 지정
void CD3DRenderer9::SetStatusInfo(CString strText, D3DCOLOR dwTextColor)
{
	m_strStatusText = strText;
	m_dwStatusTextColor = dwTextColor;
}

// 텍스트 데이터 지정
void CD3DRenderer9::SetTextDataInfo(CString strText, D3DCOLOR dwTextColor)
{
	m_strTextString = strText;
	m_dwTextColor = dwTextColor;
}

// 로그를 기록한다.
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

// D3D 생성
HRESULT CD3DRenderer9::Create(HWND hWnd, int nVideoWidth, int nVideoHeight, BOOL bIsLock)
{
	// 부모 윈도우가 없는 경우
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

	// D3D 생성
	if (SUCCEEDED(hr = CreateD3D(hWnd)))
	{
		// 사용될 리소스 생성
		if (FAILED(hr = CreateResource(hWnd, nVideoWidth, nVideoHeight)))
			WriteD3DError(_T("[Error] %s : Failed to create a resource. error=0x%x\n"), __WFUNCTION__, hr);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to create a D3D. error=0x%x\n"), __WFUNCTION__, hr);

	// 생성에 실패한 경우 제거
	if (FAILED(hr))
	{
		DXTRACE_ERR_MSGBOX(_T("[Error] Failed to create a D3D. error=0x%x"), hr);
		Close(FALSE);
	}

	if (bIsLock)
		Unlock();

	return hr;
}

// D3D 재설정
HRESULT CD3DRenderer9::Reset(HWND hWnd, BOOL bIsLock)
{
	if (!::IsWindow(hWnd))
	{
		WriteD3DError(_T("[Error] %s : Invalid parameter.\n"), __WFUNCTION__);
		return E_INVALIDARG;
	}

	// D3D 디바이스가 유효한 경우
	if (!IsValid())
	{
		WriteD3DError(_T("[Error] %s : Invalid device.\n"), __WFUNCTION__);
		return E_POINTER;
	}

	HRESULT hr = E_FAIL;
	CRect	rcWindow;

	// 디바이스 변경 중
	m_bIsDeviceChanging = TRUE;

	GetClientRect(hWnd, &rcWindow);

	if (bIsLock)
		Lock();

	// 디바이스 상실 처리
	OnLostDevice();

	if (rcWindow.Width() <= 0 || rcWindow.Height() <= 0)
		WriteD3DError(_T("[Warning] %s : Invalid windows size. w=%d h=%d\n"), __WFUNCTION__, rcWindow.Width(), rcWindow.Height());

	// 백버퍼 크기 변경
	m_stD3DPresentParam.BackBufferWidth = rcWindow.Width() <= 0 ? 1 : rcWindow.Width();
	m_stD3DPresentParam.BackBufferHeight = rcWindow.Height() <= 0 ? 1 : rcWindow.Height();

	// D3D 초기화
	if (SUCCEEDED(hr = m_pID3DDevice->Reset(&m_stD3DPresentParam)))
	{
		// 리소스 초기화
		if (FAILED(hr = OnResetDevice(hWnd)))
			WriteD3DError(_T("[Error] %s : Failed to reset resource. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, rcWindow.Width(), rcWindow.Height());
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to reset device. error=0x%x\n"), __WFUNCTION__, hr);

	// 초기화에 실패한 경우
	if (FAILED(hr))
	{
		// D3D 재생성
		if (FAILED(hr = Create(hWnd, m_stVideoSize.cx, m_stVideoSize.cy, !bIsLock)))
		{
			WriteD3DError(_T("[Error] %s : Failed to recreate a D3D. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, m_stVideoSize.cx, m_stVideoSize.cy);
			Close(FALSE);
		}
	}

	// D3D 초기화 시 화면을 초기화한다.
	if (SUCCEEDED(hr))
	{
		if (FAILED(hr = ClearScreen(hWnd,  !bIsLock)))
			WriteD3DError(_T("[Error] %s : Failed to initialize a screen. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, m_stVideoSize.cx, m_stVideoSize.cy);
	}
	else
		DXTRACE_ERR_MSGBOX(_T("[Error] Failed to reset a D3D. error=0x%x"), hr);

	if (bIsLock)
		Unlock();

	// 디바이스 변경 완료
	m_bIsDeviceChanging = FALSE;

	return hr;
}

// 장치를 소실한 경우
void CD3DRenderer9::OnLostDevice()
{
	if (m_pID3DSprite)	m_pID3DSprite->OnLostDevice();
	if (m_pID3DFont)		m_pID3DFont->OnLostDevice();
	if (m_plD3DBoxLine)	m_plD3DBoxLine->OnLostDevice();

	// 재설정이 불가능한 리소스 제거
	DestroyResource(TRUE);
}

// 장치 재설정
HRESULT	CD3DRenderer9::OnResetDevice(HWND hWnd)
{
	if (m_pID3DSprite)	m_pID3DSprite->OnResetDevice();
	if (m_pID3DFont)		m_pID3DFont->OnResetDevice();
	if (m_plD3DBoxLine)	m_plD3DBoxLine->OnResetDevice();

	// 재설정이 불가능한 리소스 생성
	return CreateResource(hWnd, m_stVideoSize.cx, m_stVideoSize.cy, TRUE);
}

// 장치를 상실한 경우
BOOL CD3DRenderer9::IsDeviceLost(HWND hWnd, BOOL bIsLock)
{
	if (!IsValid())
		return TRUE;

	HRESULT hr;

	// 현재 디바이스 상태 체크
	if (SUCCEEDED(hr = m_pID3DDevice->TestCooperativeLevel()))
		return FALSE;

	// 장치 상실 : 다음 콜 때 다시 테스트한다.
	if (hr == D3DERR_DEVICELOST)
	{
		WriteD3DError(_T("[Warning] %s : Lost device. error=0x%x\n"), __WFUNCTION__, hr);
		Sleep(100);
		return TRUE;
	}
	// 장치의 드라이버 에러
	else if (hr == D3DERR_DRIVERINTERNALERROR)
	{
		WriteD3DError(_T("[Fatal] %s : Internal driver error. error=0x%x\n"), __WFUNCTION__, hr);
		PostQuitMessage(0);
		return TRUE;
	}
	// 장치 재설정 요청
	else if (hr == D3DERR_DEVICENOTRESET)
	{
		// 장치를 재설정한다.
		if (FAILED(hr = Reset(hWnd, bIsLock)))
		{
			WriteD3DError(_T("[Error] %s : Failed to reset a device in D3DERR_DEVICENOTRESET. error=0x%x\n"), __WFUNCTION__, hr);
			return FALSE;
		}

		return TRUE;
	}
	// 사용자 계정 컨트롤 창이 뜨는 경우 영상이 정지할 때 발생되는 에러
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

// DXVA2 제거
void CD3DRenderer9::Close(BOOL bIsLock)
{
	if (bIsLock)
		Lock();

	DestroyResource();
	DestroyD3D9();

	if (bIsLock)
		Unlock();
}

// D3D 초기화
HRESULT CD3DRenderer9::CreateD3D(HWND hWnd)
{
	HRESULT  hr = E_FAIL;

	// D3D 생성
	if ((m_pID3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
		WriteD3DError(_T("[Error] %s : Failed to create a D3D. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	D3DDISPLAYMODE	stDisplayMode;
	D3DCAPS9		stDisplayInfo;
	D3DFORMAT		eFormat = D3DFMT_UNKNOWN;
	DWORD			dwVertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;

	// 디스플레이 모드 얻기
	if (FAILED(hr = m_pID3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &stDisplayMode)))
	{
		WriteD3DError(_T("[Error] %s : Failed to get a display information. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// 사용하려는 디스플레이 포멧이 윈도우 모드에서 지원되는 지 검사
	if (FAILED(hr = m_pID3D->CheckDeviceType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, stDisplayMode.Format, stDisplayMode.Format, TRUE)))
	{
		WriteD3DError(_T("[Error] %s : Unable to use a display format (%d) in H/W. error=0x%x\n"), __WFUNCTION__, stDisplayMode.Format, hr);
		return hr;
	}

	eFormat = stDisplayMode.Format;

	// 디바이스 설정 얻기
	if (FAILED(hr = m_pID3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &stDisplayInfo)))
	{
		WriteD3DError(_T("[Error] %s : Failed to get a graphic card information. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// 멀티 텍스쳐 렌더링이 지원되지 않는 경우 예외 처리
	if (stDisplayInfo.NumSimultaneousRTs <= 1)
	{
		WriteD3DError(_T("[Error] %s : Unable to use multi-texture in this system. error=0x%x\n"), __WFUNCTION__, hr);
		return E_FAIL;
	}

	if ((stDisplayInfo.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != D3DDEVCAPS_HWTRANSFORMANDLIGHT)
		dwVertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// 	// 그래픽 카드가 하드웨어 텍셀레이션을 지원하는 경우
	// 	if(stDisplayInfo.DevCaps & D3DDEVCAPS_PUREDEVICE && dwVertexProcessing & D3DCREATE_HARDWARE_VERTEXPROCESSING)
	// 		dwVertexProcessing |= D3DCREATE_PUREDEVICE;

	CRect	rcWindow;

	GetClientRect(hWnd, &rcWindow);

	// D3D 생성 옵션 지정
	m_stD3DPresentParam.BackBufferWidth = rcWindow.Width();
	m_stD3DPresentParam.BackBufferHeight = rcWindow.Height();
	m_stD3DPresentParam.BackBufferFormat = eFormat;							// 후면 버퍼 색상 형식
	m_stD3DPresentParam.BackBufferCount = 1;								// 후면 버퍼 수
	m_stD3DPresentParam.SwapEffect = D3DSWAPEFFECT_DISCARD;			// 최적 스왑 효과
	m_stD3DPresentParam.hDeviceWindow = hWnd;								// 디바이스 윈도우
	m_stD3DPresentParam.Windowed = TRUE;								// 윈도우 모드 또는 전체 모드
	m_stD3DPresentParam.EnableAutoDepthStencil = TRUE;
	m_stD3DPresentParam.AutoDepthStencilFormat = D3DFMT_D16;

	m_stD3DPresentParam.MultiSampleType = D3DMULTISAMPLE_NONE;
	m_stD3DPresentParam.MultiSampleQuality = 0;
	m_stD3DPresentParam.Flags = D3DPRESENTFLAG_VIDEO;
	m_stD3DPresentParam.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	m_stD3DPresentParam.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;	// 즉시 표출

	// D3D 디바이스 생성
	if (SUCCEEDED(hr = m_pID3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, dwVertexProcessing | D3DCREATE_FPU_PRESERVE | D3DCREATE_MULTITHREADED,
		&m_stD3DPresentParam, &m_pID3DDevice)))
	{
		BOOL	bUseAntiAlias = TRUE;

		// 멀티 샘플링(안티앨리어싱) 가능 여부 검사
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

// D3D 소멸
void CD3DRenderer9::DestroyD3D9()
{
	// 반드시 생성 순서의 역순으로 제거해야 함!!
	SAFE_RELEASE(m_pID3DDevice);
	SAFE_RELEASE(m_pID3D);
}

// 멀티 샘플링(Anti-Alias) 가능 여부 검사
HRESULT CD3DRenderer9::CheckAntialias(D3DPRESENT_PARAMETERS& stParam, IDirect3D9* pD3D, UINT uAdapter, D3DDEVTYPE eDevType, D3DMULTISAMPLE_TYPE eAntiAliasType)
{
	HRESULT hr = E_FAIL;
	DWORD	QualityBackBuffer = 0;
	DWORD	QualityZBuffer = 0;
	DWORD	dwAliasValue = (DWORD)eAntiAliasType;

	while (dwAliasValue)
	{
		// 후면 버퍼 안티앨리어싱 사용 가능 여부 체크
		if (SUCCEEDED(hr = pD3D->CheckDeviceMultiSampleType(uAdapter, eDevType, stParam.BackBufferFormat, stParam.Windowed,
			(D3DMULTISAMPLE_TYPE)dwAliasValue, &QualityBackBuffer)))
		{
			// 스탠실 버퍼 안티앨리어싱 사용 가능 여부 체크
			if (SUCCEEDED(hr = pD3D->CheckDeviceMultiSampleType(uAdapter, eDevType, stParam.AutoDepthStencilFormat, stParam.Windowed,
				(D3DMULTISAMPLE_TYPE)dwAliasValue, &QualityZBuffer)))
			{
				// 안티앨리어싱이 가능하므로 타입을 지정한다.
				stParam.MultiSampleType = (D3DMULTISAMPLE_TYPE)dwAliasValue;

				// QualityBackBuffer와 QualityZBuffer 중 작은 값 지정
				if (QualityBackBuffer < QualityZBuffer)
					stParam.MultiSampleQuality = QualityBackBuffer - 1;
				else
					stParam.MultiSampleQuality = QualityZBuffer - 1;

				break;
			}
		}

		// 안티앨리어싱 단계를 한단계 하향
		dwAliasValue--;
	}

	// 실패한 경우
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

// 사용될 리소스 제거
HRESULT CD3DRenderer9::CreateResource(HWND hWnd, int nVideoWidth, int nVideoHeight, BOOL bIsReset)
{
	HRESULT hr;
	CRect	rcWindow;

	// D3D가 생성되지 않은 경우
	if (!IsValid())
	{
		WriteD3DError(_T("[Error] %s : Invalid device pointer. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		return E_POINTER;
	}

	// 윈도우 영역 얻기
	GetClientRect(hWnd, &rcWindow);

	// 장치를 Reset하는 경우는 생성하지 않는다.
	if (!bIsReset)
	{
		// 폰트 생성
		if (FAILED(hr = CreateFont()))
		{
			WriteD3DError(_T("[Error] %s : Failed to create a font. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}

		// 스프라이트 생성
		if (FAILED(hr = CreateSprite()))
		{
			WriteD3DError(_T("[Error] %s : Failed to create a sprite. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}

		// 라인 생성
		if (FAILED(hr = CreateLine()))
		{
			WriteD3DError(_T("[Error] %s : Failed to create a line object. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}
	}

	// 비디오를 그릴 텍스쳐 생성
	if (FAILED(hr = CreateVideoTexture(hWnd, nVideoWidth, nVideoHeight)))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a texture for video. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// 상태 아이콘 생성
	if (FAILED(hr = CreateStatusIcon()))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a status icon. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// 프로젝션 좌표 설정
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

// 리소스 제거
void CD3DRenderer9::DestroyResource(BOOL bIsReset)
{
	// 장치를 재설정하는 경우는 삭제하지 않는다.
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

// 스프라이트 생성
HRESULT	CD3DRenderer9::CreateSprite()
{
	SAFE_RELEASE(m_pID3DSprite);
	return D3DXCreateSprite(m_pID3DDevice, &m_pID3DSprite);
}

// 비디오를 그릴 텍스쳐 생성
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

	// 테스트용 이미지 파일 읽기
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

	// 비디오 크기에 오류가 있는 경우 기본값으로 설정
	if (nVideoWidth == 0 || nVideoHeight == 0)
	{
		WriteD3DError(_T("[Warning] %s : Inputted an invalid video size. w=%d h=%d\n"), __WFUNCTION__, nVideoWidth, nVideoHeight);

		nVideoWidth = 704;
		nVideoHeight = 480;
	}

#if USE_RGB24_DRAWING

	// 비디오를 그릴 텍스쳐 생성
	if (FAILED(hr = D3DXCreateTexture(m_pID3DDevice, nVideoWidth, nVideoHeight, 1, D3DUSAGE_DYNAMIC,
		D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pID3DTextureVideo)))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a texture for video. error=0x%x w=%d h=%d mem=%d\n"),
			__WFUNCTION__, hr, nVideoWidth, nVideoHeight, m_pID3DDevice->GetAvailableTextureMem());
		return hr;
	}

#else

	// 비디오를 그릴 텍스쳐 생성
	if (FAILED(hr = D3DXCreateTexture(m_pID3DDevice, nVideoWidth, nVideoHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pID3DTextureVideo)))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a render target texture for video. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, nVideoWidth, nVideoHeight);
		return hr;
	}

	// 비디오를 그릴 서피스 생성
	if (FAILED(hr = m_pID3DDevice->CreateOffscreenPlainSurface(nVideoWidth, nVideoHeight, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pID3DTextureSurface, NULL)))
	{
		WriteD3DError(_T("[Warning] %s : Failed to create a render target texture for video. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, nVideoWidth, nVideoHeight);
		return hr;
	}

#endif

	m_stVideoSize.cx = nVideoWidth;
	m_stVideoSize.cy = nVideoHeight;

	// 텍스쳐를 부드럽게 그리도록 설정
	hr = m_pID3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	hr = m_pID3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	hr = m_pID3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	// 화면에 표시될 비디오 초기화
	if (FAILED(hr = InitializeVideo(D3D_RGB_BLUE)))
	{
		WriteD3DError(_T("[Error] %s : Failed to initialize a texture. error=0x%x w=%d h=%d\n"), __WFUNCTION__, hr, nVideoWidth, nVideoHeight);
		return hr;
	}

#endif

	//	if(SUCCEEDED(hr))
	//	{
	// 		// 비디오 텍스쳐를 그릴 좌표 생성
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

// 비디오를 초기화한다.
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
		// 비디오 초기 색상 지정
		if (FAILED(hr = m_pID3DDevice->ColorFill(pITempTextureSurface, NULL, dwColor)))
			WriteD3DError(_T("[Error] %s : Failed to fill color. error=0x%x\n"), __WFUNCTION__, hr);

		SAFE_RELEASE(pITempTextureSurface);
	}

	return hr;
}

// 사용될 폰트 생성
HRESULT CD3DRenderer9::CreateFont()
{
	HRESULT hr = S_OK;
	HDC		hDC = NULL;

	SAFE_RELEASE(m_pID3DFont);

	// 폰트가 없는 경우 기본 시스템 폰트로 생성
	if (_tcslen(m_stFontInfo.FaceName) <= 0)
	{
		NONCLIENTMETRICS	nc;

		ZeroMemory(&nc, sizeof(NONCLIENTMETRICS));
		nc.cbSize = sizeof(NONCLIENTMETRICS);

		// 시스템 기본 폰트 정보 얻기
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

	// 폰트 생성
	if (FAILED(hr = D3DXCreateFontIndirect(m_pID3DDevice, &m_stFontInfo, &m_pID3DFont)))
		WriteD3DError(_T("[Error] %s : Failed to create a font. error=0x%x\n"), __WFUNCTION__, hr);

	return hr;
}

// 폰트 변경
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

	// 폰트를 생성한다.
	if (FAILED(hr = CreateFont()))
	{
		bSuccess = FALSE;
		WriteD3DError(_T("[Error] %s : Failed to create a font. error=0x%x\n"), __WFUNCTION__, hr);
	}

	Unlock();

	return bSuccess;
}

// 상태 아이콘/비트맵을 생성한다.
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

	// 이미지의 크기를 얻는다.
	if (FAILED(hr = D3DXGetImageInfoFromResource(hInstance, MAKEINTRESOURCE(m_nStatusIconID), &stImageInfo)))
	{
		WriteD3DError(_T("[Error] %s : Failed to get a image for icon. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	m_szIconOrginal.cx = stImageInfo.Width;
	m_szIconOrginal.cy = stImageInfo.Height;

	// 2의 배수로 아이콘의 크기를 바꾸는 경우 좌표에서도 scale되서 제대로 그려지지 않는 문제 발생함.
	// 원본 이미지 사이즈를 2의 배수로 처리
	m_szIconResized.cx = GetIconSize(m_szIconOrginal.cx);
	m_szIconResized.cy = GetIconSize(m_szIconOrginal.cy);

	// 리소스에서 비트맵을 읽어들인다.
	if (FAILED(hr = D3DXCreateTextureFromResource(m_pID3DDevice, hInstance, MAKEINTRESOURCE(m_nStatusIconID), &m_pID3DTextureIcon)))
		WriteD3DError(_T("[Error] %s : Failed to create a texture for icon. error=0x%x\n"), __WFUNCTION__, hr);

	return hr;
}

// 아이콘의 크기를 얻는다 => Sprite는 원본 사이즈를 2의 배수로 설정함.
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

// 모션 라인 생성
HRESULT CD3DRenderer9::CreateLine()
{
	HRESULT hr;

	SAFE_RELEASE(m_plD3DBoxLine);

	// 박스를 그릴 때 사용되는 라인 생성
	if (FAILED(hr = D3DXCreateLine(m_pID3DDevice, &m_plD3DBoxLine)))
	{
		WriteD3DError(_T("[Error] %s : Failed to create a line. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	// 라인의 두께 지정
	if (SUCCEEDED(hr = m_plD3DBoxLine->SetWidth(LINE_THICKNESS_NORMAL)))
	{
		if (FAILED(hr = m_plD3DBoxLine->SetAntialias(TRUE)))
			WriteD3DError(_T("[Error] %s : Failed to set anti-alias mode of the line. error=0x%x\n"), __WFUNCTION__, hr);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to assign a width of the line. error=0x%x\n"), __WFUNCTION__, hr);

	return hr;
}

// 화면을 초기화한다.
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

	// 디바이스 변경 중인 경우는 그리지 않는다.
	if (m_bIsDeviceChanging)
		return S_OK;

	if (bIsLock)
		Lock();

	// 장치를 잃어버린 경우
	if (IsDeviceLost(hWnd, !bIsLock))
	{
		hr = S_FALSE;
		WriteD3DError(_T("[Warning] %s : Lost device. error=0x%x\n"), __WFUNCTION__, E_POINTER);
		goto ClearScreen_Complete;
	}

	// 화면을 지정 색상으로 초기화한다.
	if (FAILED(hr = InitializeVideo(dwColor)))
	{
		WriteD3DError(_T("[Error] %s : Failed to initialize a screen. error=0x%x\n"), __WFUNCTION__, hr);
		goto ClearScreen_Complete;
	}

	GetClientRect(hWnd, &rcWindow);

	// 보여질 화면 위치 지정
	D3DXMatrixIdentity(&matWorld);
	D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 1.0f);

	if (FAILED(hr = m_pID3DDevice->SetTransform(D3DTS_WORLD, &matWorld)))
	{
		WriteD3DError(_T("[Error] %s : Failed to set a world view position. error=0x%x\n"), __WFUNCTION__, hr);
		goto ClearScreen_Complete;
	}

	// 그리기 시작
	if (SUCCEEDED(hr = m_pID3DDevice->BeginScene()))
	{
		if ((hr = m_pID3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, dwColor, 1.0f, 0)))
			WriteD3DError(_T("[Error] %s : Failed to clear scene. error=0x%x\n"), __WFUNCTION__, hr);

		// 문자열 그리기
		if (FAILED(hr = DrawAllString(rcWindow, dwColor)))
			WriteD3DError(_T("[Error] %s : Failed to draw a strings. error=0x%x\n"), __WFUNCTION__, hr);

		if (FAILED(hr = DrawChannelBox(FALSE, rcWindow)))
			WriteD3DError(_T("[Error] %s : Failed to draw a box. error=0x%x\n"), __WFUNCTION__, hr);

		if (FAILED(hr = m_pID3DDevice->EndScene()))
			WriteD3DError(_T("[Error] %s : Failed to end scene. error=0x%x\n"), __WFUNCTION__, hr);

		// 화면 출력
		hr = m_pID3DDevice->Present(NULL, NULL, NULL, NULL);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to begin scene. error=0x%x\n"), __WFUNCTION__, hr);

ClearScreen_Complete:

	if (bIsLock)
		Unlock();

	return hr;
}

// 비디오를 그린다.
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

	// 디바이스 변경 중인 경우는 그리지 않는다.
	if (m_bIsDeviceChanging)
		return S_OK;

	D3DXMATRIX	matWorld;
	CRect		rcWindow;
	HRESULT		hr;

	Lock();

	// 장치를 잃어버린 경우
	if (IsDeviceLost(hWnd, FALSE))
	{
		hr = S_FALSE;
		goto DrawVideo_Complete;
	}

	// 비디오 크기가 다른 경우는 텍스쳐를 재생성한다.
	if (m_stVideoSize.cx != nVideoWidth || m_stVideoSize.cy != nVideoHeight)
	{
		if (FAILED(hr = CreateVideoTexture(hWnd, nVideoWidth, nVideoHeight)))
		{
			WriteD3DError(_T("[Error] %s : Failed to create a texture for video. error=0x%x\n"), __WFUNCTION__, hr);
			goto DrawVideo_Complete;
		}
	}

	// 데이터가 있는 경우
	if (pFrame && nSize > 0)
	{
		// 비디오를 그린다.
		if (FAILED(hr = FillVideo(pFrame, nSize)))
		{
			WriteD3DError(_T("[Error] %s : Failed to fill a video. error=0x%x video size=%d w=%d h=%d\n"), __WFUNCTION__, hr, nSize, nVideoWidth, nVideoHeight);
			goto DrawVideo_Complete;
		}
	}
	else
	{
		// 화면을 지정 색상으로 초기화한다.
		if (FAILED(InitializeVideo(D3D_RGB_GRAY)))
			WriteD3DError(_T("[Error] %s : Failed to initialize a video. error=0x%x video size=%d w=%d h=%d\n"), __WFUNCTION__, hr, nSize, nVideoWidth, nVideoHeight);
	}

	GetClientRect(hWnd, &rcWindow);

	// 보여질 화면 위치 지정
	D3DXMatrixIdentity(&matWorld);
	D3DXMatrixTranslation(&matWorld, m_fPosX, m_fPosY, m_fPosZ);

	if (FAILED(hr = m_pID3DDevice->SetTransform(D3DTS_WORLD, &matWorld)))
	{
		WriteD3DError(_T("[Error] %s : Failed to set a world view position. error=0x%x\n"), __WFUNCTION__, hr);
		goto DrawVideo_Complete;
	}

	// 그리기 시작
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

		// 문자열 그리기
		if (FAILED(hr = DrawAllString(rcWindow, dwColor)))
			WriteD3DError(_T("[Error] %s : Failed to draw a string. error=0x%x\n"), __WFUNCTION__, hr);

		// 아이콘 그리기
		DrawIcon(rcWindow);

		// 채널 박스 그리기
		if (FAILED(hr = DrawChannelBox(TRUE, rcWindow)))
			WriteD3DError(_T("[Error] %s : Failed to draw a box. error=0x%x\n"), __WFUNCTION__, hr);


		if (FAILED(hr = m_pID3DDevice->EndScene()))
			WriteD3DError(_T("[Error] %s : Failed to end scene. error=0x%x\n"), __WFUNCTION__, hr);

		// 화면 출력
		if (FAILED(hr = m_pID3DDevice->Present(NULL, NULL, hWnd, NULL)))
			WriteD3DError(_T("[Error] %s : Failed to present a scene. error=0x%x\n"), __WFUNCTION__, hr);
	}
	else
		WriteD3DError(_T("[Error] %s : Failed to begin scene. error=0x%x\n"), __WFUNCTION__, hr);

DrawVideo_Complete:
	Unlock();

	return hr;
}

// 비디오를 채운다.
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

	// 메인 스트림을 그린다.
	if (FAILED(hr = m_pID3DTextureVideo->LockRect(0, &stLockRect, NULL, D3DLOCK_DISCARD)))
	{
		WriteD3DError(_T("[Error] %s : Failed to lock a texture. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

	pY = (char*)pFrame;
	pU = pY + (m_stVideoSize.cx * m_stVideoSize.cy);
	pV = pU + (m_stVideoSize.cx * m_stVideoSize.cy / 4);

	yv12_to_rgb32_mmx((BYTE*)stLockRect.pBits, stLockRect.Pitch, (BYTE*)pY, (BYTE*)pU, (BYTE*)pV, m_stVideoSize.cx, m_stVideoSize.cx / 2, m_stVideoSize.cx, m_stVideoSize.cy);

	// 락 해제
	if (FAILED(hr = m_pID3DTextureVideo->UnlockRect(0)))
	{
		WriteD3DError(_T("[Error] %s : Failed to unlock a texture. error=0x%x\n"), __WFUNCTION__, hr);
		return hr;
	}

#else

	// 메인 스트림을 그린다.
	if (SUCCEEDED(hr = m_pID3DTextureSurface->LockRect(&stLockRect, NULL, D3DLOCK_DISCARD)))
	{
		pY = (char*)pFrame;
		pU = pY + (m_stVideoSize.cx * m_stVideoSize.cy);
		pV = pU + (m_stVideoSize.cx * m_stVideoSize.cy / 4);

		yv12_to_rgb32_c((BYTE*)stLockRect.pBits, stLockRect.Pitch, (BYTE*)pY, (BYTE*)pU, (BYTE*)pV, m_stVideoSize.cx, m_stVideoSize.cx / 2, m_stVideoSize.cx, m_stVideoSize.cy);
		//yv12_to_rgb32_mmx((BYTE*)stLockRect.pBits, stLockRect.Pitch, (BYTE*)pY, (BYTE*)pU, (BYTE*)pV, m_stVideoSize.cx, m_stVideoSize.cx / 2, m_stVideoSize.cx, m_stVideoSize.cy);
		//YV12toYUY2((BYTE *)pY, (BYTE *)pU, (BYTE *)pV, (BYTE *)stLockRect.pBits, m_stVideoSize.cx, m_stVideoSize.cy, m_stVideoSize.cx, stLockRect.Pitch);
		//yv12_to_yuyv_mmx((BYTE *)stLockRect.pBits, stLockRect.Pitch, (BYTE *)pY, (BYTE *)pU, (BYTE *)pV, m_stVideoSize.cx, m_stVideoSize.cx / 2, m_stVideoSize.cx, m_stVideoSize.cy); 

		// 락 해제
		if (FAILED(hr = m_pID3DTextureSurface->UnlockRect()))
		{
			WriteD3DError(_T("[Error] %s : Failed to unlock a texture. error=0x%x\n"), __WFUNCTION__, hr);
			return hr;
		}

		LPDIRECT3DSURFACE9 pTempTextureSurface = NULL;

		// 그려진 서피스를 텍스쳐에 복사한다.
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

// 지정된 문자열(제목, 상태, 텍스트 등등)을 그린다.
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

	// 좌측 상단 제목
	if (m_strTitle.GetLength() > 0 && m_pID3DFont != NULL)
	{
		rcTitle.SetRect(rcWindow.left + nOffset, rcWindow.top + nOffset, rcWindow.Width(), rcWindow.top + nHeight);
		m_pID3DFont->DrawText(m_pID3DSprite, m_strTitle, -1, &rcTitle, DT_LEFT | DT_TOP, m_dwTitleTextColor);
	}

	// 텍스트 데이터 : 좌측 상단 제목 아래 보여줌
	if (m_strTextString.GetLength() > 0 && m_pID3DFont != NULL)
	{
		rcText.SetRect(rcTitle.left, rcTitle.bottom + nOffset, rcWindow.Width(), rcTitle.bottom + nHeight);
		m_pID3DFont->DrawText(m_pID3DSprite, m_strTextString, -1, &rcText, DT_LEFT | DT_VCENTER, m_dwTitleTextColor);
	}

	// 하단 상태바(에러 상태인경우 가운데 표시. 아닌경우 하단표시)
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

// 상태 아이콘을 그린다.
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

	// DX에서 확장한 이미지의 원본 비율을 맞춰준다.
	D3DXMatrixScaling(&mScale, fXScale, fYScale, 1.0f);
	m_pID3DSprite->SetTransform(&mScale);

	// 아이콘을 그린다.
	stScreenPos.x = fStartPos;

	if ((m_lStatusFlag & SHOW_ICON_PTZ) == SHOW_ICON_PTZ)
	{
		// 비트맵 내부 아이콘 인덱스
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

// 컨트롤의 테두리를 그린다.
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

	// 그릴 영역이 없는 경우
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

	// 비상벨 상태인경우 테두리 붉은색 표시.
	if ((m_lStatusFlag & SHOW_ICON_EMERGENCY) == SHOW_ICON_EMERGENCY)
	{
		dwMotionLineColor = D3D_RGB_RED;
		bMotion = TRUE;
	}
	// 모션인 경우 붉은색 표시
	else if (bMotionDraw && (m_lStatusFlag & SHOW_ICON_MOTION) == SHOW_ICON_MOTION)
	{
		dwMotionLineColor = D3D_RGB_RED;
		bMotion = TRUE;
	}

	// 일반 라인 그리기
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
	// 검색채널 라인
	else if (m_dwBackgroundColor == D3D_RGB_BLUE)
	{
		m_plD3DBoxLine->SetWidth(LINE_THICKNESS_BOLD + LINE_THICKNESS_NORMAL);
		if (SUCCEEDED(hr = m_plD3DBoxLine->Begin()))
		{
			m_plD3DBoxLine->Draw(vList, _countof(vList), m_dwBackgroundColor);
			m_plD3DBoxLine->End();
		}
	}


	// 모션 / 엘리베이터
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



// 화면을 이미지 파일 저장
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

// 왼쪽으로 화면 이동
void CD3DRenderer9::ScreenMoveLeft()
{
	m_fPosX -= 0.02f;
}

// 오른쪽으로 화면 이동
void CD3DRenderer9::ScreenMoveRight()
{
	m_fPosX += 0.02f;
}

// 위로 화면 이동
void CD3DRenderer9::ScreenMoveUp()
{
	m_fPosY -= 0.02f;
}

// 아래로 화면 이동
void CD3DRenderer9::ScreenMoveDown()
{
	m_fPosY += 0.02f;
}

// 화면을 작게
void CD3DRenderer9::ScreenScaleDown()
{
	m_fPosZ -= 0.02f;
}

// 화면을 크게
void CD3DRenderer9::ScreenScaleUp()
{
	m_fPosZ += 0.02f;
}

// 화면을 원래 크기로 변경
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

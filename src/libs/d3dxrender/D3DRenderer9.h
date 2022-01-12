#pragma once

#ifdef _DEBUG
#define D3D_DEBUG_INFO
#endif

#include <dwmapi.h>
#include <mmsystem.h>
#include <tchar.h>
#include <strsafe.h>
#include <initguid.h>

#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>
#include <d3d9types.h>

#include <CGuid.h>
#include <atlbase.h>
#include <atltypes.h>
#include <atlstr.h>
#include <atltrace.h>

#include <ole2.h>
#include <shlwapi.h>



// �⺻ ���� ����
#define D3D_RGB_WHITE				D3DCOLOR_XRGB(255, 255, 255)
#define D3D_RGB_RED					D3DCOLOR_XRGB(255, 0, 0)
#define D3D_RGB_YELLOW				D3DCOLOR_XRGB(255, 255, 0)
#define D3D_RGB_GREEN				D3DCOLOR_XRGB(0, 255, 0)
#define D3D_RGB_CYAN				D3DCOLOR_XRGB(0, 255, 255)
#define D3D_RGB_BLUE				D3DCOLOR_XRGB(0, 0, 255)
#define D3D_RGB_MAGENTA				D3DCOLOR_XRGB(255, 0, 255)
#define D3D_RGB_BLACK				D3DCOLOR_XRGB(0, 0, 0)
#define D3D_RGB_ORANGE				D3DCOLOR_XRGB(255, 128, 0)
#define D3D_RGB_PLPPLE				D3DCOLOR_XRGB(125, 0, 255)
#define D3D_RGB_GRAY				D3DCOLOR_XRGB(128, 128, 128)
#define D3D_RGB_PINK				D3DCOLOR_XRGB(255, 192, 203)

#define COLORREF_D3DCOLOR(c)		D3DCOLOR_XRGB(GetRValue(c), GetGValue(c), GetBValue(c))

// ȭ�� ��ܿ� ǥ���� ���� ���� ������ �ε���
enum
{
	IDX_ICON_PTZ = 0,
	IDX_ICON_AUDIO,
	IDX_ICON_MOTION,
	IDX_ICON_SENSOR,
	IDX_ICON_REC,
	IDX_ICON_EMERGENCY,
	IDX_ICON_MAX,
};

#define SHOW_ICON_AUDIO				0x00000001
#define SHOW_ICON_MOTION			0x00000002
#define SHOW_ICON_PTZ				0x00000004
#define SHOW_ICON_RECORDING			0x00000008
#define SHOW_ICON_SENSOR			0x00000010
#define SHOW_ICON_EMERGENCY			0x00000020
#define SHOW_ICON_FIRE				0x00000040
#define SHOW_ICON_SMOKE				0x00000080
#define SHOW_ICON_OBJECT			0x00000100
#define SHOW_ICON_TAMPER			0x00000200

#define D3DFVF_CUSTOMVERTEX			(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)
#define RECT_OFFSET					3

struct Vertex
{
	float	 x, y, z;		// �簢�� ��ǥ
	D3DCOLOR color;			// ����
	float	 tu, tv;		// �ؽ��� ��ǥ
};
extern Vertex	g_stVertexQuad[4];

// D3D ������
class CD3DRenderer9
{
public:
	CD3DRenderer9();
	~CD3DRenderer9();
	HRESULT	Create(HWND hWnd, int nVideoWidth = 720, int nVideoHeight = 480, BOOL bIsLock = TRUE);
	void	Close(BOOL bIsLock = TRUE);
	HRESULT	Reset(HWND hWnd, BOOL bIsLock = TRUE);

	BOOL	IsValid() { return (m_pID3DDevice ? TRUE : FALSE); }

	void	SetBackgroundColor(D3DCOLOR dwColor = D3D_RGB_BLUE) { m_dwBackgroundColor = dwColor; }
	void	SetTitleInfo(CString strText, D3DCOLOR dwTextColor = D3D_RGB_WHITE);

	void	SetStatusInfo(CString strText, D3DCOLOR dwTextColor = D3D_RGB_WHITE);
	void	SetStatusIconID(int nStatusIconID) { m_nStatusIconID = nStatusIconID; }

	void	SetTextDataInfo(CString strText, D3DCOLOR dwTextColor = D3D_RGB_WHITE);

	HRESULT	DrawVideo(HWND hWnd, LPVOID pFrame, int nSize, int nVideoWidth, int nVideoHeight,
	BOOL bMotionDraw = FALSE);
	HRESULT	ClearScreen(HWND hWnd, BOOL bIsLock = TRUE);

	void	SetStatusFlag(LONG lStatus) { m_lStatusFlag = lStatus; }
	LONG	GetStatusFlag() { return m_lStatusFlag; }

	BOOL	ChangeFont(LOGFONT* pFontInfo, COLORREF clrFontColor);

	HRESULT	SaveImageToFile(D3DXIMAGE_FILEFORMAT eImageType, LPTSTR pFileName);

	void	ScreenMoveLeft();
	void	ScreenMoveRight();

	void	ScreenMoveUp();
	void	ScreenMoveDown();

	void	ScreenScaleDown();
	void	ScreenScaleUp();

	void	ScreenOriginalSize();
	BOOL	IsScreenOriginalSize();

protected:
	void	Lock();
	void	Unlock();

	HRESULT	CreateD3D(HWND hWnd);
	void	DestroyD3D9();

	HRESULT	CreateResource(HWND hWnd, int nVideoWidth, int nVideoHeight, BOOL bIsReset = FALSE);
	void	DestroyResource(BOOL bIsReset = FALSE);

	void	OnLostDevice();
	HRESULT	OnResetDevice(HWND hWnd);
	BOOL	IsDeviceLost(HWND hWnd, BOOL bIsLock);

	HRESULT CheckAntialias(D3DPRESENT_PARAMETERS& stParam, IDirect3D9* pD3D, UINT uAdapter = 0, D3DDEVTYPE eDevType = D3DDEVTYPE_HAL,
		D3DMULTISAMPLE_TYPE eAntiAliasType = D3DMULTISAMPLE_16_SAMPLES);

protected:
	HRESULT	CreateFont();
	HRESULT	CreateVideoTexture(HWND hWnd, int nVideoWidth, int nVideoHeight);
	HRESULT CreateStatusIcon();
	int		GetIconSize(int nSize);

	HRESULT	CreateSprite();

	HRESULT	InitializeVideo(D3DCOLOR dwColor);
	HRESULT	FillVideo(LPVOID pFrame, int nSize);
	void	YV12toYUY2(BYTE* pY, BYTE* pU, BYTE* pV, BYTE* pDest, int nWidth, int nHeight, int nSourcePitch, int nDestPitch);

	HRESULT	CreateLine();
	HRESULT	DrawChannelBox(BOOL bMotionDraw, CRect& rcWindow);

	HRESULT	DrawAllString(CRect& rcWindow, D3DCOLOR dwColor);
	HRESULT	DrawIcon(CRect& rcWindow);

	void	WriteD3DError(LPCTSTR pFormat, ...);

protected:
	// Direct 3D
	LPDIRECT3D9						m_pID3D;						// D3D �ڵ�
	LPDIRECT3DDEVICE9				m_pID3DDevice;					// D3D ����̽�
	D3DPRESENT_PARAMETERS			m_stD3DPresentParam;			// D3D ���� �ɼ�

	LPDIRECT3DTEXTURE9				m_pID3DTextureVideo;			// ������ �ؽ��� ����
	LPDIRECT3DSURFACE9				m_pID3DTextureSurface;			// D3D Surface

	LPD3DXFONT						m_pID3DFont;					// ��Ʈ
	LPD3DXSPRITE					m_pID3DSprite;					// ȭ�� ���� ��� ������
	LPDIRECT3DTEXTURE9				m_pID3DTextureIcon;				// �����ܿ� �ؽ��� ����
	//LPDIRECT3DVERTEXBUFFER9		m_pID3DVertexVideo;				// ������ �׸� ����

	LPD3DXLINE						m_plD3DBoxLine;					// channelbox line

	SIZE							m_stVideoSize;					// ���� ũ��
	float							m_fPosX;						// �� ���� X��ǥ
	float							m_fPosY;						// �� ���� Y��ǥ
	float							m_fPosZ;						// �� ���� Y��ǥ

	// ��Ÿ
	D3DCOLOR						m_dwBackgroundColor;			// ������

	D3DCOLOR						m_dwTitleTextColor;				// Ÿ��Ʋ �� ����
	CString							m_strTitle;						// Ÿ��Ʋ �� ���ڿ�

	int								m_nStatusIconID;				// ���� ������ ���ҽ� ��ȣ
	D3DCOLOR						m_dwStatusTextColor;			// ���¹� ����
	CString							m_strStatusText;				// ���¹� ���ڿ�

	CString							m_strTextString;				// �ؽ�Ʈ(����������/���) ���ڿ�
	D3DCOLOR						m_dwTextColor;					// �ؽ�Ʈ(����������/���) ����

	D3DXFONT_DESC					m_stFontInfo;					// ��Ʈ ����

	LONG							m_lStatusFlag;					// ���� �÷���

	CSize							m_szIconOrginal;				// DX�� �ε��� ���ҽ� ������ ũ��
	CSize							m_szIconResized;				// ���� ���ҽ� ������ ũ��

	CRITICAL_SECTION				m_stLock;						// �۾� ��
	BOOL							m_bIsDeviceChanging;			// ��� ���� ��
};
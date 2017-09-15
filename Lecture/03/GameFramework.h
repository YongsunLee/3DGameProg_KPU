#pragma once
class CGameFramework
{
private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;

	int m_nWndClientWidth;
	int m_nWndClientHeight;

	// 팩토리, 스왑체인, 디바이스 포인터
	IDXGIFactory4 *m_pdxgiFactory;
	IDXGISwapChain3 *m_pdxgiSwapChain;		// 디스플레이 제어
	ID3D12Device *m_pd3dDevice;				// 리소스 생성

	// 다중 샘플링 활성화 및 다중 샘플링 레벨 설정
	bool m_bMsaa4xEnable = false;
	UINT m_nMsaa4xQualityLevels = 0;

	// 후면 버퍼의 갯수
	static const UINT m_nSwapChainBuffers = 2;
	// 현재 스왑체인의 후면 버퍼 인덱스
	UINT m_nSwapChainBufferIndex;

	// 랜더타겟 버퍼, 서술자 힙, 인터페이스 포인터, 랜더 타겟 서술자 원소의 크기
	// RenderTargetView의 약어를 이용하여 Rtv로 적은것.
	ID3D12Resource *m_ppd3dRenderTargetBuffers[m_nSwapChainBuffers];
	ID3D12DescriptorHeap *m_pd3dRtvDescriptorHeap;
	UINT m_nRtvDescriptorIncrementSize;

	// 깊이-스탠실 버퍼, 서술자 힙 인터페이스 포인터, 깊이-스텐실 서술자 원소의 크기
	// DepthStencilView 의 약어를 사용해서 Dsv로 적은것.
	ID3D12Resource *m_pd3dDepthStencilBuffer;
	ID3D12DescriptorHeap *m_pd3dDsvDescriptorHeap;
	UINT m_nDsvDescriptorIncrementSize;

	//명령 큐, 명령 할당자, 명령 리스트 인터페이스 포인터
	ID3D12CommandQueue *m_pd3dCommandQueue;
	ID3D12CommandAllocator *m_pd3dCommandAllocator;
	ID3D12GraphicsCommandList *m_pd3dCommandList;

	// 펜스 인터페이스 포인터, 펜스의 값, 이벤트 핸들
	ID3D12Fence *m_pd3dFence;
	UINT64 m_nFenceValue;
	HANDLE m_hFenceEvent;

#if defined(_DEBUG)
	ID3D12Debug *m_pd3dDebugController;
#endif

	// 뷰포트와 씨져의 사각형
	// 씨저 :: 특정 구역에만 그림을 그린다. (자른구역을 놔두고 그 외의 범위를 버린다.)
	D3D12_VIEWPORT m_d3dViewport;
	D3D12_RECT m_d3dScissorRect;	

public:
	CGameFramework();
	~CGameFramework();

	// 프레임워크 초기화 (주 윈도우가 생생되면 호출)
	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	
	void OnDestroy();

	//스왑 체인, 디바이스, 서술자 힙, 명령 큐/할당자/리스트 생성함수
	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateRtvAndDsvDescriptorHeaps();
	void CreateCommandQueueAndList();

	// 렌더 타겟 뷰, 깊이-스텐실 뷰 생성
	void CreateRenderTargetView();
	void CreateDepthStencilView();

	// 렌더링할 메쉬와 게임 객체 생성, 게임객체 소멸
	void BuildObjects();
	void ReleaseObjects();

	// 프레임워크의 핵심기능(사용자 입력, 애니메이션, 렌더링)을 구성하는 함수
	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	// CPU와 GPU를 동기화 하는 함수 (Fence사용?)
	void WaitForGpuComplete();

	// 윈도우 메세지(키보드, 마우스 입력) 처리
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);



};


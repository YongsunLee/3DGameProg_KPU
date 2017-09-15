#pragma once
class CGameFramework
{
private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;

	int m_nWndClientWidth;
	int m_nWndClientHeight;

	// ���丮, ����ü��, ����̽� ������
	IDXGIFactory4 *m_pdxgiFactory;
	IDXGISwapChain3 *m_pdxgiSwapChain;		// ���÷��� ����
	ID3D12Device *m_pd3dDevice;				// ���ҽ� ����

	// ���� ���ø� Ȱ��ȭ �� ���� ���ø� ���� ����
	bool m_bMsaa4xEnable = false;
	UINT m_nMsaa4xQualityLevels = 0;

	// �ĸ� ������ ����
	static const UINT m_nSwapChainBuffers = 2;
	// ���� ����ü���� �ĸ� ���� �ε���
	UINT m_nSwapChainBufferIndex;

	// ����Ÿ�� ����, ������ ��, �������̽� ������, ���� Ÿ�� ������ ������ ũ��
	// RenderTargetView�� �� �̿��Ͽ� Rtv�� ������.
	ID3D12Resource *m_ppd3dRenderTargetBuffers[m_nSwapChainBuffers];
	ID3D12DescriptorHeap *m_pd3dRtvDescriptorHeap;
	UINT m_nRtvDescriptorIncrementSize;

	// ����-���Ľ� ����, ������ �� �������̽� ������, ����-���ٽ� ������ ������ ũ��
	// DepthStencilView �� �� ����ؼ� Dsv�� ������.
	ID3D12Resource *m_pd3dDepthStencilBuffer;
	ID3D12DescriptorHeap *m_pd3dDsvDescriptorHeap;
	UINT m_nDsvDescriptorIncrementSize;

	//��� ť, ��� �Ҵ���, ��� ����Ʈ �������̽� ������
	ID3D12CommandQueue *m_pd3dCommandQueue;
	ID3D12CommandAllocator *m_pd3dCommandAllocator;
	ID3D12GraphicsCommandList *m_pd3dCommandList;

	// �潺 �������̽� ������, �潺�� ��, �̺�Ʈ �ڵ�
	ID3D12Fence *m_pd3dFence;
	UINT64 m_nFenceValue;
	HANDLE m_hFenceEvent;

#if defined(_DEBUG)
	ID3D12Debug *m_pd3dDebugController;
#endif

	// ����Ʈ�� ������ �簢��
	// ���� :: Ư�� �������� �׸��� �׸���. (�ڸ������� ���ΰ� �� ���� ������ ������.)
	D3D12_VIEWPORT m_d3dViewport;
	D3D12_RECT m_d3dScissorRect;	

public:
	CGameFramework();
	~CGameFramework();

	// �����ӿ�ũ �ʱ�ȭ (�� �����찡 �����Ǹ� ȣ��)
	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	
	void OnDestroy();

	//���� ü��, ����̽�, ������ ��, ��� ť/�Ҵ���/����Ʈ �����Լ�
	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateRtvAndDsvDescriptorHeaps();
	void CreateCommandQueueAndList();

	// ���� Ÿ�� ��, ����-���ٽ� �� ����
	void CreateRenderTargetView();
	void CreateDepthStencilView();

	// �������� �޽��� ���� ��ü ����, ���Ӱ�ü �Ҹ�
	void BuildObjects();
	void ReleaseObjects();

	// �����ӿ�ũ�� �ٽɱ��(����� �Է�, �ִϸ��̼�, ������)�� �����ϴ� �Լ�
	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	// CPU�� GPU�� ����ȭ �ϴ� �Լ� (Fence���?)
	void WaitForGpuComplete();

	// ������ �޼���(Ű����, ���콺 �Է�) ó��
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);



};


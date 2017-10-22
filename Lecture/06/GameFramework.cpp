#include "stdafx.h"
#include "GameFramework.h"

// init 
CGameFramework::CGameFramework()
{
	// D3D Func
	m_pdxgiFactory = nullptr;
	m_pdxgiSwapChain = nullptr;
	m_pd3dDevice = nullptr;

	// CommandList, Queue, Allocator
	m_pd3dCommandAllocator = nullptr;
	m_pd3dCommandQueue = nullptr;
	//m_pd3dPipelineState = nullptr;
	m_pd3dCommandList = nullptr;

	// Rtv
	for (int i = 0; i < m_nSwapChainBuffers; ++i)
		m_ppd3dRenderTargetBuffers[i] = nullptr;
	m_pd3dRtvDescriptorHeap = nullptr;
	m_nRtvDescriptorIncrementSize = 0;

	// Dsv
	m_pd3dDepthStencilBuffer = nullptr;
	m_pd3dDsvDescriptorHeap = nullptr;
	m_nDsvDescriptorIncrementSize = 0;

	// 시작엔 0번째 화면을 가리킨다. 
	m_nSwapChainBufferIndex = 0;

	// Fence
	m_hFenceEvent = nullptr;
	m_pd3dFence = nullptr;
	//m_nFenceValue = 0;
	for (int i = 0; i < m_nSwapChainBuffers; i++) 
		m_nFenceValues[i] = 0; m_pScene = NULL;

	// 윈도우 크기
	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	// 타이머
	_tcscpy_s(m_pszFrameRate, _T("LapProject ("));

}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	// Direct3D 디바이스, 명령 큐,리스트, 스왑체인 등 생성 함수 호출
	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	
	// 05부터 삭제
	//CreateRenderTargetView();
	//CreateDepthStencilView();

	BuildObjects();

	return (true);
}

void CGameFramework::OnDestroy()
{
	//Object 
	ReleaseObjects();
	::CloseHandle(m_hFenceEvent);

#if defined(_DEBUG)
	if (m_pd3dDebugController) 
		m_pd3dDebugController->Release();
#endif

	// Rtv
	for (int i = 0; i < m_nSwapChainBuffers; ++i) {
		if (m_ppd3dRenderTargetBuffers[i])
			m_ppd3dRenderTargetBuffers[i]->Release();
	}
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();
	
	//Dsv
	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	//Command
	if (m_pd3dCommandAllocator) m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	// Fence
	if (m_pd3dFence) m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
	if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();
}

void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);

	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

	/*
	DXGI_SWAP_CHAIN_DESC{
		DXGI_MODE_DESC		BufferDesc		
		DXGI_SAMPLE_DESC	SampleDesc		
		DXGI_USAGE			BufferUsage		 
		UINT				BufferCount
		HWND				OutputWindow
		BOOL				Windowed
		DXGI_SWAP_EFFECT	SwapEffect
		UINT				Flags
	}
	*/
	// DXGI_SWAP_CHAIN_DESC 선언
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	
	//Buffercount :: 후면버퍼의 수를 지정
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	// DXGI_MODE_DESC :: 후면버퍼의 디스플레이 형식을 지정
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight; 
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60; 
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1; 
	
	//DXGI_USAGE :: 후면버퍼에 대한 표면 사용 방식과 CPU의 접근 방법
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; 
	
	//SwapEffect :: 스와핑 처리하는 선택사항 지정
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// 버퍼 내용물 폐기 (Discard)

	dxgiSwapChainDesc.OutputWindow = m_hWnd; 

	//DXGI_SAMPLE_DESC :: 다중 샘플링 품질 지정
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0; 

	// Windowed 전체화면 여부
	dxgiSwapChainDesc.Windowed = TRUE;

	//전체화면 모드에서 바탕화면의 해상도를 바꾸지 않고 후면버퍼의 크기를 바탕화면 크기로 변경한다. 
#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS 
	dxgiSwapChainDesc.Flags = 0; 
#else //전체화면 모드에서 바탕화면의 해상도를 스왑체인(후면버퍼)의 크기에 맞게 변경한다. 
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; 
#endif
		
	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain **)&m_pdxgiSwapChain);
	/*
	// DXGI_MODE_DESC :: 후면버퍼의 디스플레이 형식을 지정
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;

	//DXGI_SAMPLE_DESC :: 다중 샘플링 품질 지정
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	
	//DXGI_USAGE :: 후면버퍼에 대한 표면 사용 방식과 CPU의 접근 방법
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	
	//SwapEffect :: 스와핑 처리하는 선택사항 지정
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	// 버퍼 내용은 폐기
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;		// 
	
	// Flags :: 스왑체인의 동작에 대한 선택사항을 지정 (보통 0)
	dxgiSwapChainDesc.Flags = 0;

	// 전체화면일때
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	// DXGI_MODE_DESC
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	// Windowed 전체화면 여부
	dxgiSwapChainFullScreenDesc.Windowed = true;
	
	// 스왑체인 생성 (Factory에 CreateSwapChain을 통해서
	m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd,
		&dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1**)&m_pdxgiSwapChain);
	*/
	// Alt + Enter 동작 비활성화
	//m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
	// 스왑체인의 현재 후면버퍼 인덱스를 저장
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
}

void CGameFramework::CreateDirect3DDevice()
{
#if defined(_DEBUG)
	D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void**)&m_pd3dDebugController);
	m_pd3dDebugController->EnableDebugLayer();
#endif

	// DXGI Factory 생성
	::CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&m_pdxgiFactory);

	// Adapter (출력장치)
	IDXGIAdapter1 *pd3dAdapter = nullptr;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); ++i) {
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0,
			__uuidof(ID3D12Device), (void**)&m_pd3dDevice)))
			break;
	}

	// 특성 레벨 12를 지원하는 디바이스를 생성할 수 없을때 WARP 디바이스를 생성
	if (!pd3dAdapter){
		ComPtr<IDXGIAdapter> pWarpAdapter;
		m_pdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));
		D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_pd3dDevice));
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;										//Msaa4x 다중 샘플링
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;				//디바이스가 지원하는 다중 샘플의 품질 수준을 확인한다.
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;				//다중 샘플의 품질 수준이 1보다 크면 다중 샘플링을 활성화한다. 
	
	
	//펜스를 생성하고 펜스 값을 0으로 설정한다.
	m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence),
		(void **)&m_pd3dFence);
	//m_nFenceValue = 0;
	
	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	/*펜스와 동기화를 위한 이벤트 객체를 생성한다(이벤트 객체의 초기값을 FALSE이다).
	이벤트가 실행되면(Signal) 이 벤트의 값을 자동적으로 FALSE가 되도록 생성한다.*/
	
	// 뷰포트를 주 윈도우의 클라이언트 영역 전체로 설정
	m_d3dViewport.TopLeftX = 0; 
	m_d3dViewport.TopLeftY = 0; 
	m_d3dViewport.Width = static_cast<float>(m_nWndClientWidth); 
	m_d3dViewport.Height = static_cast<float>(m_nWndClientHeight); 
	m_d3dViewport.MinDepth = 0.0f; 
	m_d3dViewport.MaxDepth = 1.0f;

	//씨저 사각형을 주 윈도우의 클라이언트 영역 전체로 설정한다.
	m_d3dScissorRect = {0,0, m_nWndClientWidth, m_nWndClientHeight};
	
	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));

	// Rtv
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	
	// 랜더 타겟 서술자 힙(서술자의 개수는 스왑체인 버퍼의 개수) 생성
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dRtvDescriptorHeap);
	
	// 렌더 타겟 서술자 힙의 원소크기를 저장
	m_nRtvDescriptorIncrementSize =
		m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Dsv
	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	
	// Dsv 서술자 힙(서술자의 개수는 1)을 생성
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dDsvDescriptorHeap);
	
	// Dsv의 서술자 힙 원소의 크기를 저장
	m_nDsvDescriptorIncrementSize =
		m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void **)&m_pd3dCommandQueue);

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&m_pd3dCommandAllocator);

	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&m_pd3dCommandList);

	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRenderTargetView()
{
	// CPU에서 바라보는 Rtv 서술자의 주소 값
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	for (UINT i = 0; i < m_nSwapChainBuffers; ++i) {
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void **)&m_ppd3dRenderTargetBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dRenderTargetBuffers[i], NULL, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += m_nRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;
	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue,
		__uuidof(ID3D12Resource), (void **)&m_pd3dDepthStencilBuffer);

	//깊이-스텐실 버퍼를 생성한다.
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, NULL, d3dDsvCPUDescriptorHandle);
	//깊이-스텐실 버퍼 뷰를 생성한다. 
}

void CGameFramework::BuildObjects()
{
	m_pScene = new CScene(); 
	if (m_pScene) 
		m_pScene->BuildObjects(m_pd3dDevice);
	
	m_GameTimer.Reset();
}

void CGameFramework::ReleaseObjects()
{
	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;
}

void CGameFramework::ProcessInput()
{
}

void CGameFramework::AnimateObjects()
{
	if (m_pScene) m_pScene->AnimateObjects(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::FrameAdvance()
{
	//타이머의 시간이 갱신되도록 하고 프레임 레이트를 계산한다. 
	m_GameTimer.Tick(0.0f);

	ProcessInput();
	AnimateObjects();

	// 명령 할당자와 명령 리스트를 리셋한다.
	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);
	
	// 뷰포트랑 시저 사각형
	m_pd3dCommandList->RSSetViewports(1, &m_d3dViewport);
	m_pd3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource =
		m_ppd3dRenderTargetBuffers[m_nSwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	// 현재의 렌더 타겟에 해당하는 서술자의 CPU 주소를 계산
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);
	
	// Dsv 주소 계산
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle =
		m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// Dsv
	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	
	// OM 설정
	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, FALSE,
		&d3dDsvCPUDescriptorHandle);

	// 렌더 타겟 뷰 색상
	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle,
		pfClearColor/*Colors::Azure*/, 0, NULL);
	
	// Scene Render
	if (m_pScene) 
		m_pScene->Render(m_pd3dCommandList);

	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	hResult = m_pd3dCommandList->Close();
	
	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList };

	// 명령리스트를 명령큐에 추가하여 실행한다.
	m_pd3dCommandQueue->ExecuteCommandLists(_countof(ppd3dCommandLists), ppd3dCommandLists);
	
	WaitForGpuComplete();
	
	/* 
	//타이머 추가 이전
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters; 
	dxgiPresentParameters.DirtyRectsCount = 0; 
	dxgiPresentParameters.pDirtyRects = NULL; 
	dxgiPresentParameters.pScrollRect = NULL; 
	dxgiPresentParameters.pScrollOffset = NULL; 

	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters); 
	*/

	//타이머 추가 이후
	m_pdxgiSwapChain->Present(0, 0);
	
	/*현재의 프레임 레이트를 문자열로 가져와서 주 윈도우의 타이틀로 출력한다. 
	m_pszBuffer 문자열이 "LapProject ("으로 초기화되었으므로 (m_pszFrameRate+12)에서부터 
	프레임 레이트를 문자열로 출력 하여 “ FPS)” 문자열과 합친다.
		::_itow_s(m_nCurrentFrameRate, (m_pszFrameRate + 12), 37, 10); 
		::wcscat_s((m_pszFrameRate + 12), 37, _T(" FPS)"))
	*/

	m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37); 
	::SetWindowText(m_hWnd, m_pszFrameRate);

	//m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
}

void CGameFramework::WaitForGpuComplete()
{
	//CPU 펜스 값을 증가
	//m_nFenceValue++;
	//const UINT64 nFence = m_nFenceValue;
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];

	// GPU가 펜스의 값을 설정하는 명령을 명령 큐에 추가.
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);
	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		// 펜스의 현재값이 설정한 값보다 작으면 펜스의 현재 값이 설정한 값이 될 때 까지 기다린다.
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
	case WM_KEYUP:
		switch (wParam) {
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
		case VK_RETURN:
			break;
		case VK_F8:
			break;
		case VK_F9:
		{
			BOOL bFullScreenState = FALSE; 
			m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL); 
			
			if (!bFullScreenState) { 
				DXGI_MODE_DESC dxgiTargetParameters; 
				dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM; 
				dxgiTargetParameters.Width = m_nWndClientWidth; 
				dxgiTargetParameters.Height = m_nWndClientHeight; 
				dxgiTargetParameters.RefreshRate.Numerator = 60; 
				dxgiTargetParameters.RefreshRate.Denominator = 1; 
				dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; 
				dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; 
				m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters); 
			}
			m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);
			
			OnResizeBackBuffers();
			break;
		}
		default:
			break;
		}
		break;
	default:
		break;
	}
}

LRESULT CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
	case WM_ACTIVATE:
	{
		if (LOWORD(wParam) == WA_INACTIVE) m_GameTimer.Stop();
		else							   m_GameTimer.Start();
		break;
	}
	/*WM_SIZE 메시지는 윈도우가 생성될 때 한번 호출되거나 윈도우의 크기가 변경될 때 호출된다. 
	주 윈도우의 크기 를 사용자가 변경할 수 없으므로 윈도우의 크기가 변경되는 경우는 윈도우 모드와 전체화면 모드의 전환이 발생할 때 이다.*/
	case WM_SIZE:
	{
		m_nWndClientWidth = LOWORD(lParam);
		m_nWndClientHeight = HIWORD(lParam);
		
		// 윈도우의 크기가 변경되면 후면버퍼의 크기를 변경한다.
		OnResizeBackBuffers();	
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return(0);
}

void CGameFramework::OnResizeBackBuffers()
{
	WaitForGpuComplete();
	
	m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

	for (int i = 0; i < m_nSwapChainBuffers; i++) 
		if (m_ppd3dRenderTargetBuffers[i]) 
			m_ppd3dRenderTargetBuffers[i]->Release(); 
	
	if (m_pd3dDepthStencilBuffer) 
		m_pd3dDepthStencilBuffer->Release(); 

#ifdef _WITH_ONLY_RESIZE_BACKBUFFERS 
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc; 
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc); 
	m_pdxgiSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0); 
	m_nSwapChainBufferIndex = 0; 
#else 
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc; 
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc); 
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, 
									m_nWndClientWidth, 
									m_nWndClientHeight, 
									dxgiSwapChainDesc.BufferDesc.Format, 
									dxgiSwapChainDesc.Flags); 
	m_nSwapChainBufferIndex = 0; 
#endif 

	CreateRenderTargetView(); 
	CreateDepthStencilView();

	m_pd3dCommandList->Close();
	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList }; 
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	
	WaitForGpuComplete();
}

void CGameFramework::MoveToNextFrame()
{
	// 현재 BackBufferIndex
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	
	// Fence의 값
	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex]; 
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue); 

	if (m_pd3dFence->GetCompletedValue() < nFenceValue) { 
		// 이벤트 완료시
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent); 
		::WaitForSingleObject(m_hFenceEvent, INFINITE); 
	}
}


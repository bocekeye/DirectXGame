#include "DirectX12Wrapper.h"
#include "Application.h"
#include <cassert>
#include <array>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

DirectX12Wrapper::DirectX12Wrapper()
{
}

DirectX12Wrapper::~DirectX12Wrapper()
{
}

bool DirectX12Wrapper::Init(Application* app)
{
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugLayer;
	HRESULT r = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	debugLayer->EnableDebugLayer();
#endif

	std::array<D3D_FEATURE_LEVEL, 4> fLevells = 
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};
	//Direct3DDeviceの初期化
	HRESULT result = S_OK;
	D3D_FEATURE_LEVEL decidedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	for (auto flv : fLevells)
	{
		//ReleaseAndGetAddressOfは、dev_のポインタのポインタを表している
		result =  D3D12CreateDevice(
			nullptr,
			flv,	//フューチャーレベル(DirectXのバージョン)
			IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf()));
		if (SUCCEEDED(result))
		{
			decidedFeatureLevel = flv;
			break;
		}
	}
	assert(SUCCEEDED(result));
	if (FAILED(result))
	{
		return false;
	}

#ifdef _DEBUG
	constexpr UINT debugFlag = DXGI_CREATE_FACTORY_DEBUG;
#else
	constexpr UINT debugFlag = 0;
#endif

	//DXGIの初期化
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
		 IID_PPV_ARGS(dxgi_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));
	if (FAILED(result))
	{
		return false;
	}

	//コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC cmdQueDesc = {};
	cmdQueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmdQueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmdQueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmdQueDesc.NodeMask = 0;
	dev_->CreateCommandQueue(&cmdQueDesc,IID_PPV_ARGS(cmdQue_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));
	if (FAILED(result))
	{
		return false;
	}


	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = app->GetWindowWidth();
	scDesc.Height = app->GetWindowHeight();
	scDesc.Stereo = false;//VRの時以外FALSE
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;//アルファは特にいじらない
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA8bitの標準
	//アンチエイリアンシングにかかわってくる
	scDesc.SampleDesc.Count = 1;//AAなし
	scDesc.SampleDesc.Quality = 0;//AAクオリティ最低
	scDesc.Scaling = DXGI_SCALING_STRETCH;//画面のサイズを変えると、それに合わせる
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;//ウィンドウと全画面を切り替え許容
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	scDesc.BufferCount = 2;	//表画面と裏画面の2枚


	ComPtr<IDXGISwapChain1> swapChain;
	result = dxgi_->CreateSwapChainForHwnd(
		cmdQue_.Get(),
		app->GetWindowHandle(),
		&scDesc,
		nullptr,
		nullptr,
		swapChain.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(result));
	result = swapChain.As(&swapChain_);
	assert(SUCCEEDED(result));

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NodeMask = 0;
	rtvHeapDesc.NumDescriptors = 2;//画面をいくつ作るのか(表画面と裏画面)
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//レンダーターゲットビューとして
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = dev_->CreateDescriptorHeap(&rtvHeapDesc, 
		IID_PPV_ARGS(rtvHeaps_.ReleaseAndGetAddressOf()));

	CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle(rtvHeaps_->GetCPUDescriptorHandleForHeapStart(),
		0, 0);
	swapChain_->GetDesc1(&scDesc);
	std::array<ID3D12Resource*, 2> renderTargetResources;
	for (int i = 0; i < scDesc.BufferCount; i++)
	{
		result = swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargetResources[i]));
		dev_->CreateRenderTargetView(renderTargetResources[i],nullptr, descHandle);
		descHandle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	assert(SUCCEEDED(result));

	return true;
}

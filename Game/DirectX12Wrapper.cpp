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
	//プリプロセッサ(_DEBUG)
#ifdef _DEBUG
	//ただ単にデバックレイヤーを有効化している
	//DirectX12は、GPU側でエラーが起きててもデフォだと何も情報がない
	//デバックレイヤーをオンにすることで、GPU側でエラーが出たらログをはくように設定する
	ComPtr<ID3D12Debug> debugLayer;
	HRESULT r = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	debugLayer->EnableDebugLayer();//デバックレイヤーを有効にする
	debugLayer.Reset();//debugLayerオブジェクトを破棄
#endif

	//DirectXDeviceとDXGIは別の仕組みがある
	//この2つは、連携をとって動くが、別物

#ifdef _DEBUG
	constexpr UINT debugFlag = DXGI_CREATE_FACTORY_DEBUG;
#else
	constexpr UINT debugFlag = 0;
#endif

		//DXGI(DirectXGraphicsInterface)の初期化
	HRESULT result = CreateDXGIFactory2(debugFlag,//デバック時は有効
		IID_PPV_ARGS(dxgi_.ReleaseAndGetAddressOf()));	//ComPtr内部ポインタのアドレスを渡す

   //resultはいろんな値が返ってくる
	assert(SUCCEEDED(result));	//SUCCEEDで囲むと、この結果がOK
	if (FAILED(result))			//FAILEDで囲むとこの結果がOUT
	{
		return false;
	}

	//有効なアダプターを検索する
	//グラボのことだと思う
	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0;  dxgi_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;
		//*家のPCがAMDだった場合選ばれない可能性がある
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//機能レベルを最新から4つ前くらいまで用意しておっく
	std::array<D3D_FEATURE_LEVEL, 4> fLevells =
	{
		D3D_FEATURE_LEVEL_12_1,	//最新
		D3D_FEATURE_LEVEL_12_0,	//1世代前
		D3D_FEATURE_LEVEL_11_1,	//2世代前
		D3D_FEATURE_LEVEL_11_0,	//3世代前
	};
	//Direct3DDeviceの初期化
//	result = S_OK;
	D3D_FEATURE_LEVEL decidedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	for (auto flv : fLevells)
	{
		//ReleaseAndGetAddressOfは、dev_のポインタのポインタを表している
		//DirectXの基本オブジェクトとなるD3DDevieceオブジェクトを取得する
		result = D3D12CreateDevice(
			tmpAdapter,	//nullptrを入れれば、自動的に最適なアダプタが選ばれる
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

	//コマンドキューの作成
	//コマンドキューとは、コマンドリストをグラボに押し出して実行するもの
	D3D12_COMMAND_QUEUE_DESC cmdQueDesc = {};
	cmdQueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	//フラグなし
	cmdQueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	//通常順
	cmdQueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	//コマンドリストを普通に実行
	cmdQueDesc.NodeMask = 0;	//基本的に0にする
	dev_->CreateCommandQueue(&cmdQueDesc,IID_PPV_ARGS(cmdQue_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));
	if (FAILED(result))
	{
		return false;
	}


	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = app->GetWindowWidth();	//ウィンドウ幅
	scDesc.Height = app->GetWindowHeight();	//ウィンドウ高さ
	scDesc.Stereo = false;//VRの時以外FALSE
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;//アルファは特にいじらない
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA8bitの標準	(UNORM = 正規化)
	//アンチエイリアンシングにかかわってくる
	scDesc.SampleDesc.Count = 1;//AAなし
	scDesc.SampleDesc.Quality = 0;//AAクオリティ最低
	scDesc.Scaling = DXGI_SCALING_STRETCH;//画面のサイズを変えると、それに合わせる
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;//ウィンドウと全画面を切り替え許容
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;	//バックバッファとして使う
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

	//ディスクリプタヒープを作る(レンダーターゲットビュー用)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NodeMask = 0;	//毎回0
	rtvHeapDesc.NumDescriptors = 2;//画面をいくつ作るのか(表画面と裏画面の2枚)
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//レンダーターゲットビューとして
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = dev_->CreateDescriptorHeap(&rtvHeapDesc, 
		IID_PPV_ARGS(rtvHeaps_.ReleaseAndGetAddressOf()));

	CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle(
		rtvHeaps_->GetCPUDescriptorHandleForHeapStart(),
		0, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	//すでに表画面と裏画面はSwapChain内にあるので、それを取得できるように
	//swapChainの情報を取得しておく
	DXGI_SWAP_CHAIN_DESC1 scDescForRTV = {};
	swapChain_->GetDesc1(&scDescForRTV);

	//スワップチェーンが持ってるバッファ取得用
	std::array<ID3D12Resource*, 2> renderTargetResources;

	//スワップチェーン内のバッファ数分ループする
	for (int i = 0; i < scDesc.BufferCount; i++)
	{
		//スワップチェーンの特定番号のバッファを取得する
		result = swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargetResources[i]));
		dev_->CreateRenderTargetView(renderTargetResources[i],nullptr, descHandle);
		descHandle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	assert(SUCCEEDED(result));

	result = dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(cmdAlloc_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));

	result = dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		 cmdAlloc_.Get(), nullptr, IID_PPV_ARGS(cmdList_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));

	cmdAlloc_->Reset();
	cmdList_->Reset(cmdAlloc_.Get(),nullptr);

	return true;
}

bool DirectX12Wrapper::Update()
{
	cmdAlloc_->Reset();
	cmdList_->Reset(cmdAlloc_.Get(), nullptr);
	auto heapStart = rtvHeaps_->GetCPUDescriptorHandleForHeapStart();

	//レンダーターゲットの指定
	cmdList_->OMSetRenderTargets(1, &heapStart,
		false, nullptr);
	//その時のレンダーターゲットのクリア
	std::array<float, 4> clearColor = { 1.0f,0.1f,1.0f,1.0f };
	cmdList_->ClearRenderTargetView(heapStart, clearColor.data(),
		0, nullptr);
	cmdList_->Close();

	ID3D12CommandList* cmdlists[] = { cmdList_.Get() };
	cmdQue_->ExecuteCommandLists(1, cmdlists);
	swapChain_->Present(0, 0);

	return false;
}

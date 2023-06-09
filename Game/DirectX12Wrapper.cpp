#include "DirectX12Wrapper.h"
#include "Application.h"
#include <cassert>
#include <array>
#include <DirectXMath.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;


DirectX12Wrapper::DirectX12Wrapper()
{
}

DirectX12Wrapper::~DirectX12Wrapper()
{

}

bool DirectX12Wrapper::Init(Application* app)
{
	// プリプロセッサ(_DEBUG)
#ifdef _DEBUG
	// デバッグレイヤーを有効化
	// DirectX12はデフォルトだとGPU側でエラーが起きても情報がない
	// デバッグレイヤーをオンにすることで、GPU側でエラーが出たらログを出力するように設定する
	ComPtr<ID3D12Debug>	debugLayer;
	HRESULT r = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	debugLayer->EnableDebugLayer();		// デバッグレイヤーを有効にする
	debugLayer.Reset();					// デバッグレイヤーオブジェクトを破棄
#endif 

	// DirectXDeviceというのとDXGIという仕組みがあり、この2つが連携をとって動くが、別物
#ifdef _DEBUG
	constexpr UINT debugFlg = DXGI_CREATE_FACTORY_DEBUG;
#else
	constexpr UINT debugFlg = 0;
#endif

	// DXGI(DirectXGraphicsInterface)の初期化
	HRESULT result = CreateDXGIFactory2(debugFlg,
		IID_PPV_ARGS(dxgi_.ReleaseAndGetAddressOf()));

	// resultはいろんな値が帰ってくる
	assert(SUCCEEDED(result));	// SUCCEEDで囲むと、この結果がOK
	if (FAILED(result))			// FAILEDで囲むと、この結果がOUT
	{
		return false;
	}

	std::vector<IDXGIAdapter*> adapters;
	IDXGIAdapter* tmpAdapter = nullptr;
	for (int i = 0; dxgi_->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}
	for (auto adpt : adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);
		std::wstring strDesc = adesc.Description;

		// PCのGPUがAMDの場合、選ばれない可能性がある
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	// 機能レベルを最新から4つ前くらいまで用意しておく
	std::array<D3D_FEATURE_LEVEL, 4> fLevels =
	{
		D3D_FEATURE_LEVEL_12_1,	// 最新
		D3D_FEATURE_LEVEL_12_0,	// 1世代前
		D3D_FEATURE_LEVEL_11_1,	// 2世代前
		D3D_FEATURE_LEVEL_11_0,	// 3世代前
	};

	// Direct3Dの初期化
	D3D_FEATURE_LEVEL decidedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	for (auto flv : fLevels)
	{
		// ReleaseAndGetAddress0fは、dev_のポインタのポインタを表している
		// DirectXの基本オブジェクトとなるD3DDeviceオブジェクトを取得する
		result = D3D12CreateDevice(
			nullptr,	// nullptrを入れれば、自動的に
			flv,		// フィーチャレベル(DirectXのバージョン)
			IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf())
		);
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

	// コマンドキューの作成
	// コマンドキューとは、コマンドリストをグラボに押し出して実行するもの
	D3D12_COMMAND_QUEUE_DESC cmdQueDesc = {};
	cmdQueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;			// フラグなし
	cmdQueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// 通常順
	cmdQueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;			// コマンドリストを普通に実行
	cmdQueDesc.NodeMask = 0;									// 基本的に0
	result = dev_->CreateCommandQueue(&cmdQueDesc, IID_PPV_ARGS(cmdQue_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = app->GetWindowWidth();				// ウィンドウ幅
	scDesc.Height = app->GetWindowHeight();				// ウィンドウ高さ
	scDesc.Stereo = false;								// VRの時以外false
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;		// アルファは特にいじらない
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// RGBA 8bitの標準

	// アンチエイリアシングにかかわってる
	scDesc.SampleDesc.Count = 1;							// AAなし
	scDesc.SampleDesc.Quality = 0;							// AAクオリティ最低
	scDesc.Scaling = DXGI_SCALING_STRETCH;					// 画面のサイズを変えると、それに合わせる
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ウィンドウと全画面を切り替え許容
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	scDesc.BufferCount = 2;									// 表画面と裏画面の二枚

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
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// レンダーターゲットビューとして
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = dev_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeaps_.ReleaseAndGetAddressOf()));

	D3D12_CPU_DESCRIPTOR_HANDLE descHandle = { rtvHeaps_->GetCPUDescriptorHandleForHeapStart() };

	// すでに表画面と裏画面はSwapChain内にあるので、それを取得できるようにswapChainの情報を取得しておく
	DXGI_SWAP_CHAIN_DESC1 scDescForRTV = {};
	swapChain_->GetDesc1(&scDesc);

	// スワップチェーンが持ってるバッファ取得用

	// スワップチェーン内のバッファ数分ループする
	for (int i = 0; i < scDesc.BufferCount; ++i)
	{
		// スワップチェーンの特定番号のバッファを取得する
		result = swapChain_->GetBuffer(i, IID_PPV_ARGS(&rtvResources_[i]));
		dev_->CreateRenderTargetView(rtvResources_[i], nullptr, descHandle);
		descHandle.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	assert(SUCCEEDED(result));

	result = dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(cmdAlloc_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));

	result = dev_->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdAlloc_.Get(),
		nullptr,
		IID_PPV_ARGS(cmdList_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));

	cmdAlloc_->Reset();
	cmdList_->Reset(cmdAlloc_.Get(), nullptr);

	result = dev_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf()));

	//頂点データ(CPUから見えるデータ)
	std::array<XMFLOAT3, 3> vertices =
	{
		XMFLOAT3(-1.0f,-1.0f,0.5f),	//左下
		XMFLOAT3(0.1f,1.0f,0.5f),	//真ん中上
		XMFLOAT3(1.0f,-1.0f,0.5f),	//右下
	};

	//GPUが利用できる「頂点バッファ」を作る
	auto vertResDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMFLOAT3)* vertices.size());
	auto vertHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	//用途はともかくバッファを確保する関数がCreateCommitedResource
	//確保するのですが、ここで用途に応じて確保の仕方を最適化しようとするため、
	//やたら細かい指定が必要になる
	result = dev_->CreateCommittedResource(
		&vertHeapProps,						//確保されるメモリの特質(CPUからいじれる)
		D3D12_HEAP_FLAG_NONE,				//特殊なフラグがあるか？
		&vertResDesc,						//どこに配置すべきか
		D3D12_RESOURCE_STATE_GENERIC_READ,	//最初の用途
		nullptr,							//クリアするためのバッファ		
		IID_PPV_ARGS(vertexBuffer_.ReleaseAndGetAddressOf())
	);
	assert(SUCCEEDED(result));

	//GPU上のメモリを弄れるためにMap関数を実行する
	//GPU上のメモリは弄れないが、双子メモリを
	//利用してCPUからGPUの内容をいじれるようにする
	XMFLOAT3* vertMap = nullptr;
	result = vertexBuffer_->Map(0, nullptr,(void**)&vertMap );
	assert(SUCCEEDED(result));

	//頂点データをGPU上(CPU上の双子メモリ)のバッファにコピーする
	//memcpyみたいな関数ですが、これはSTLの一種で、バッファオーバーランも
	//検出してくれるバージョンなので、こちらを使う
	std::copy(vertices.begin(), vertices.end(), vertMap);
	vertexBuffer_->Unmap(0, nullptr);

	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	//全体のサイズ
	vbView_.SizeInBytes = sizeof(XMFLOAT3) * vertices.size();
	//Strideは歩幅の意味、つまり次のデータまでの距離を示す
	vbView_.StrideInBytes = sizeof(XMFLOAT3);

	ComPtr<ID3DBlob> vsBlob = nullptr;	//頂点シェーダ塊
	ComPtr<ID3DBlob> psBlob = nullptr;	//ピクセルシェーダ塊

	//頂点シェーダのロード(ついでにコンパイル)
	result = D3DCompileFromFile(L"VertexShader.hlsl",
		nullptr,nullptr,"main","vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		vsBlob.ReleaseAndGetAddressOf(),nullptr);
	assert(SUCCEEDED(result));

	result = D3DCompileFromFile(L"PixelShader.hlsl",
		nullptr, nullptr, "main", "ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		psBlob.ReleaseAndGetAddressOf(), nullptr);
	assert(SUCCEEDED(result));

	//入力レイアウト
	D3D12_INPUT_ELEMENT_DESC layoutDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,
														D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}

	};

	//パイプラインオブジェクトを作る
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ppDesc = {};
	//頂点入力
	//IA(InputAssembler)
	ppDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	ppDesc.InputLayout.NumElements = _countof(layoutDesc);
	ppDesc.InputLayout.pInputElementDescs = layoutDesc;

	//頂点シェーダ VS(VertexShader)
	ppDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	ppDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	//ラスタライザ
	ppDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//ピクセルシェーダ
	ppDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	ppDesc.PS.BytecodeLength = psBlob->GetBufferSize();

	//出力マージャー
	ppDesc.NumRenderTargets = 1;							//レンダーターゲットの数
	//これを設定してないと最終的にレンダーターゲットに書き込まれない
	ppDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;			
	ppDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);	//ブレンドステート
	
	ppDesc.NodeMask = 0;

	ppDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	D3D12_ROOT_PARAMETER rootParam = {};
	rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam.DescriptorTable.NumDescriptorRanges = 1;

	//ルートシグネチャ御抜け区とを作る
	ComPtr <ID3DBlob> rootBlob;
	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	//頂点入力(頂点レイアウト)があるときはこれを使う
	//頂点が入力されるということだけを教える設定
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = 0;

	//ルートシグネチャ設定をBlobに書き込む
	result = D3D12SerializeRootSignature(&rsDesc,D3D_ROOT_SIGNATURE_VERSION_1,
			rootBlob.ReleaseAndGetAddressOf(),
			nullptr);
			assert(SUCCEEDED(result));
	//このBlobを元に、RootSignature本体を作る
	result = dev_->CreateRootSignature(0,	//nodeMaskなので、いつも0
			rootBlob->GetBufferPointer(),	//ブロブのアドレス
			rootBlob->GetBufferSize(),		//ブロブのサイズ
			IID_PPV_ARGS(rootSig_.ReleaseAndGetAddressOf()));
			assert(SUCCEEDED(result));
			
	ppDesc.pRootSignature = rootSig_.Get();
	ppDesc.SampleDesc.Count = 1;
	ppDesc.SampleDesc.Quality = 0;

	result = dev_->CreateGraphicsPipelineState(&ppDesc, IID_PPV_ARGS(pipelineState_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));

	return true;
}

bool DirectX12Wrapper::Update()
{
	static float red = 0.0f;

	cmdAlloc_->Reset();
	cmdList_->Reset(cmdAlloc_.Get(), pipelineState_.Get());

	//レンダーターゲットの指定
	auto heapStart = rtvHeaps_->GetCPUDescriptorHandleForHeapStart();
	auto bbIdx = swapChain_->GetCurrentBackBufferIndex();	//今のバックバッファ番号
	heapStart.ptr += bbIdx * dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	auto beforeBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvResources_[bbIdx],					//対象のソース(データの塊)
		D3D12_RESOURCE_STATE_PRESENT,			//前の使い方
		D3D12_RESOURCE_STATE_RENDER_TARGET		//後の使い方
	);

	cmdList_->ResourceBarrier(1, &beforeBarrier);
	//「このレンダーターゲットに書き込むよ」
	cmdList_->OMSetRenderTargets(1, &heapStart, false, nullptr);

	red = fmodf(red + 0.0001f, 1.0f);
	// その時のレンダーターゲットのクリア
	std::array<float, 4> clearColor = { red, 1.0f, 0.0f, 1.0f };
	cmdList_->ClearRenderTargetView(heapStart, clearColor.data(), 0, nullptr);

	//ルートシグネチャとパイプラインステートをセット
	cmdList_->SetGraphicsRootSignature(rootSig_.Get());
	cmdList_->SetPipelineState(pipelineState_.Get());
	//頂点バッファをセットする
	cmdList_->IASetVertexBuffers(
		0,			//スロット 
		1,			//頂点バッファビュー配列の数
		&vbView_);	//頂点バッファビュー本体

	//プリミティブトポロジーの設定
	cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//どこに描画するのかを指定する
	auto viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, 640.0f, 480.0f);
	cmdList_->RSSetViewports(1,&viewport);
	
	D3D12_RECT rect = {};
	rect.left = 0;
	rect.top = 0;
	rect.right = 640;
	rect.bottom = 480;
	cmdList_->RSSetScissorRects(1,&rect);

	//三兆点を描画する
	cmdList_->DrawInstanced(3,1,0,0);

	auto afterBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvResources_[bbIdx],					//対象のソース(データの塊)
		D3D12_RESOURCE_STATE_RENDER_TARGET,		//前の使い方
		D3D12_RESOURCE_STATE_PRESENT			//後の使い方
	);

	cmdList_->ResourceBarrier(1, &afterBarrier);
	
	cmdList_->Close();	//コマンドは最後に、「必ず」クローズする

	ID3D12CommandList* cmdlists[] = { cmdList_.Get() };
	cmdQue_->ExecuteCommandLists(1, cmdlists);
	//GPUの性能が終わったら、dence_野中の値が、fenceValue_に変化する
	//例えば初回なら、最初fence_は0で初期化されてるので0
	//ここで、 ++fenceValue_を渡しているため、GPU性能上の処理が終わったら
	//fence_の中の値は1になる
	cmdQue_->Signal(fence_.Get(), ++fenceValue_);//待たない
	//ここが待ちの本体
	while (fence_->GetCompletedValue() != fenceValue_)
	{
		//何もしない
	}

	swapChain_->Present(0, 0);

	return false;
}
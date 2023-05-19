#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <array>
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;
class Application;

class DirectX12Wrapper
{
private:
	ComPtr<ID3D12Device> dev_;					//DX12デバイス
	ComPtr<IDXGIFactory6> dxgi_;				//DXGI
	ComPtr<IDXGISwapChain4> swapChain_;			//スワップチェーン
	ComPtr<ID3D12CommandQueue>cmdQue_;			//コマンドキュー
	ComPtr<ID3D12GraphicsCommandList> cmdList_;	//コマンドリスト
	ComPtr<ID3D12CommandAllocator>	cmdAlloc_;	//コマンドアロケータ
	ComPtr<ID3D12DescriptorHeap> rtvHeaps_;		//レンダーターゲット用デスクリプタヒープ
	ComPtr<ID3D12Fence> fence_;					// 「待ち」を実装するためのもの
	std::array<ID3D12Resource*, 2> rtvResources_;
	ComPtr<ID3D12Resource> vertexBuffer_;		//頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW vbView_;			//頂点バッファビュー
	ComPtr<ID3D12RootSignature>	rootSig_;
	ComPtr<ID3D12PipelineState>pipelineState_;

	UINT64 fenceValue_ = 0;

public:
	DirectX12Wrapper();
	~DirectX12Wrapper();

	bool Init(Application* app);
	bool Update();
};


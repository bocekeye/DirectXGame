#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;
class Application;

class DirectX12Wrapper
{
private:
	ComPtr<ID3D12Device> dev_;	//DX12デバイス
	ComPtr<IDXGIFactory6> dxgi_;//DXGI
	ComPtr<IDXGISwapChain4> swapChain_;	//スワップチェーン
	ComPtr<ID3D12CommandQueue>cmdQue_;//コマンドキュー
	ComPtr<ID3D12DescriptorHeap> rtvHeaps_;//レンダーターゲット用デスクリプタヒープ

public:
	DirectX12Wrapper();
	~DirectX12Wrapper();

	bool Init(Application* app);
};


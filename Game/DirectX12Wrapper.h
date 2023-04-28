#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;

class DirectX12Wrapper
{
private:
	ComPtr<ID3D12Device> dev_;	//DX12デバイス
	ComPtr<IDXGIFactory6> dxgi_;//DXGI
	ComPtr<IDXGISwapChain4> swapChain_;	//スワップチェーン

public:
	DirectX12Wrapper();
	~DirectX12Wrapper();

	bool Init();
};


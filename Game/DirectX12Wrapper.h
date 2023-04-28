#pragma once
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;
class Application;

class DirectX12Wrapper
{
private:
	ComPtr<ID3D12Device> dev_;	//DX12�f�o�C�X
	ComPtr<IDXGIFactory6> dxgi_;//DXGI
	ComPtr<IDXGISwapChain4> swapChain_;	//�X���b�v�`�F�[��
	ComPtr<ID3D12CommandQueue>cmdQue_;//�R�}���h�L���[
	ComPtr<ID3D12DescriptorHeap> rtvHeaps_;//�����_�[�^�[�Q�b�g�p�f�X�N���v�^�q�[�v

public:
	DirectX12Wrapper();
	~DirectX12Wrapper();

	bool Init(Application* app);
};


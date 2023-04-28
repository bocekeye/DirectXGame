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

bool DirectX12Wrapper::Init()
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
	//Direct3DDevice�̏�����
	HRESULT result = S_OK;
	D3D_FEATURE_LEVEL decidedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	for (auto flv : fLevells)
	{
		//ReleaseAndGetAddressOf�́Adev_�̃|�C���^�̃|�C���^��\���Ă���
		result =  D3D12CreateDevice(
			nullptr,
			flv,	//�t���[�`���[���x��(DirectX�̃o�[�W����)
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

	//DXGI�̏�����
	result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,
		 IID_PPV_ARGS(dxgi_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));
	if (FAILED(result))
	{
		return false;
	}

	const Application& app = Application::GetInstance();
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = app.GetWindowWidth();
	scDesc.Height = app.GetWindowHeight();
	scDesc.Stereo = false;//VR�̎��ȊOFALSE
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;//�A���t�@�͓��ɂ�����Ȃ�
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA8bit�̕W��
	//�A���`�G�C���A���V���O�ɂ�������Ă���
	scDesc.SampleDesc.Count = 1;//AA�Ȃ�
	scDesc.SampleDesc.Quality = 0;//AA�N�I���e�B�Œ�
	scDesc.Scaling = DXGI_SCALING_STRETCH;//��ʂ̃T�C�Y��ς���ƁA����ɍ��킹��
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;//�E�B���h�E�ƑS��ʂ�؂�ւ����e
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;//
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	scDesc.BufferCount = 2;	//�\��ʂƗ���ʂ�2��


	ComPtr<IDXGISwapChain1> swapChain;
	result = dxgi_->CreateSwapChainForHwnd(dev_.Get(),
		Application::GetInstance().GetWindowHandle(),
		&scDesc,
		nullptr,
		nullptr,
		swapChain.ReleaseAndGetAddressOf());
	assert(SUCCEEDED(result));
	result = swapChain.As(&swapChain_);
	assert(SUCCEEDED(result));
	return true;
}

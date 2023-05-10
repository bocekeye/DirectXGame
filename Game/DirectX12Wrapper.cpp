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
	//�v���v���Z�b�T(_DEBUG)
#ifdef _DEBUG
	//�����P�Ƀf�o�b�N���C���[��L�������Ă���
	//DirectX12�́AGPU���ŃG���[���N���ĂĂ��f�t�H���Ɖ�����񂪂Ȃ�
	//�f�o�b�N���C���[���I���ɂ��邱�ƂŁAGPU���ŃG���[���o���烍�O���͂��悤�ɐݒ肷��
	ComPtr<ID3D12Debug> debugLayer;
	HRESULT r = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	debugLayer->EnableDebugLayer();//�f�o�b�N���C���[��L���ɂ���
	debugLayer.Reset();//debugLayer�I�u�W�F�N�g��j��
#endif

	//DirectXDevice��DXGI�͕ʂ̎d�g�݂�����
	//����2�́A�A�g���Ƃ��ē������A�ʕ�

#ifdef _DEBUG
	constexpr UINT debugFlag = DXGI_CREATE_FACTORY_DEBUG;
#else
	constexpr UINT debugFlag = 0;
#endif

		//DXGI(DirectXGraphicsInterface)�̏�����
	HRESULT result = CreateDXGIFactory2(debugFlag,//�f�o�b�N���͗L��
		IID_PPV_ARGS(dxgi_.ReleaseAndGetAddressOf()));	//ComPtr�����|�C���^�̃A�h���X��n��

   //result�͂����Ȓl���Ԃ��Ă���
	assert(SUCCEEDED(result));	//SUCCEED�ň͂ނƁA���̌��ʂ�OK
	if (FAILED(result))			//FAILED�ň͂ނƂ��̌��ʂ�OUT
	{
		return false;
	}

	//�L���ȃA�_�v�^�[����������
	//�O���{�̂��Ƃ��Ǝv��
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
		//*�Ƃ�PC��AMD�������ꍇ�I�΂�Ȃ��\��������
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	//�@�\���x�����ŐV����4�O���炢�܂ŗp�ӂ��Ă�����
	std::array<D3D_FEATURE_LEVEL, 4> fLevells =
	{
		D3D_FEATURE_LEVEL_12_1,	//�ŐV
		D3D_FEATURE_LEVEL_12_0,	//1����O
		D3D_FEATURE_LEVEL_11_1,	//2����O
		D3D_FEATURE_LEVEL_11_0,	//3����O
	};
	//Direct3DDevice�̏�����
//	result = S_OK;
	D3D_FEATURE_LEVEL decidedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	for (auto flv : fLevells)
	{
		//ReleaseAndGetAddressOf�́Adev_�̃|�C���^�̃|�C���^��\���Ă���
		//DirectX�̊�{�I�u�W�F�N�g�ƂȂ�D3DDeviece�I�u�W�F�N�g���擾����
		result = D3D12CreateDevice(
			tmpAdapter,	//nullptr������΁A�����I�ɍœK�ȃA�_�v�^���I�΂��
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

	//�R�}���h�L���[�̍쐬
	//�R�}���h�L���[�Ƃ́A�R�}���h���X�g���O���{�ɉ����o���Ď��s�������
	D3D12_COMMAND_QUEUE_DESC cmdQueDesc = {};
	cmdQueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	//�t���O�Ȃ�
	cmdQueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	//�ʏ폇
	cmdQueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	//�R�}���h���X�g�𕁒ʂɎ��s
	cmdQueDesc.NodeMask = 0;	//��{�I��0�ɂ���
	dev_->CreateCommandQueue(&cmdQueDesc,IID_PPV_ARGS(cmdQue_.ReleaseAndGetAddressOf()));
	assert(SUCCEEDED(result));
	if (FAILED(result))
	{
		return false;
	}


	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = app->GetWindowWidth();	//�E�B���h�E��
	scDesc.Height = app->GetWindowHeight();	//�E�B���h�E����
	scDesc.Stereo = false;//VR�̎��ȊOFALSE
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;//�A���t�@�͓��ɂ�����Ȃ�
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;//RGBA8bit�̕W��	(UNORM = ���K��)
	//�A���`�G�C���A���V���O�ɂ�������Ă���
	scDesc.SampleDesc.Count = 1;//AA�Ȃ�
	scDesc.SampleDesc.Quality = 0;//AA�N�I���e�B�Œ�
	scDesc.Scaling = DXGI_SCALING_STRETCH;//��ʂ̃T�C�Y��ς���ƁA����ɍ��킹��
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;//�E�B���h�E�ƑS��ʂ�؂�ւ����e
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;	//�o�b�N�o�b�t�@�Ƃ��Ďg��
	scDesc.BufferCount = 2;	//�\��ʂƗ���ʂ�2��


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

	//�f�B�X�N���v�^�q�[�v�����(�����_�[�^�[�Q�b�g�r���[�p)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NodeMask = 0;	//����0
	rtvHeapDesc.NumDescriptors = 2;//��ʂ��������̂�(�\��ʂƗ���ʂ�2��)
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	//�����_�[�^�[�Q�b�g�r���[�Ƃ���
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = dev_->CreateDescriptorHeap(&rtvHeapDesc, 
		IID_PPV_ARGS(rtvHeaps_.ReleaseAndGetAddressOf()));

	CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle(
		rtvHeaps_->GetCPUDescriptorHandleForHeapStart(),
		0, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	//���łɕ\��ʂƗ���ʂ�SwapChain���ɂ���̂ŁA������擾�ł���悤��
	//swapChain�̏����擾���Ă���
	DXGI_SWAP_CHAIN_DESC1 scDescForRTV = {};
	swapChain_->GetDesc1(&scDescForRTV);

	//�X���b�v�`�F�[���������Ă�o�b�t�@�擾�p
	std::array<ID3D12Resource*, 2> renderTargetResources;

	//�X���b�v�`�F�[�����̃o�b�t�@�������[�v����
	for (int i = 0; i < scDesc.BufferCount; i++)
	{
		//�X���b�v�`�F�[���̓���ԍ��̃o�b�t�@���擾����
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

	//�����_�[�^�[�Q�b�g�̎w��
	cmdList_->OMSetRenderTargets(1, &heapStart,
		false, nullptr);
	//���̎��̃����_�[�^�[�Q�b�g�̃N���A
	std::array<float, 4> clearColor = { 1.0f,0.1f,1.0f,1.0f };
	cmdList_->ClearRenderTargetView(heapStart, clearColor.data(),
		0, nullptr);
	cmdList_->Close();

	ID3D12CommandList* cmdlists[] = { cmdList_.Get() };
	cmdQue_->ExecuteCommandLists(1, cmdlists);
	swapChain_->Present(0, 0);

	return false;
}

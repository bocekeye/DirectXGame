#include "DirectX12Wrapper.h"
#include "Application.h"
#include <cassert>
#include <array>
#include <DirectXMath.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace DirectX;


DirectX12Wrapper::DirectX12Wrapper()
{
}

DirectX12Wrapper::~DirectX12Wrapper()
{

}

bool DirectX12Wrapper::Init(Application* app)
{
	// �v���v���Z�b�T(_DEBUG)
#ifdef _DEBUG
	// �f�o�b�O���C���[��L����
	// DirectX12�̓f�t�H���g����GPU���ŃG���[���N���Ă���񂪂Ȃ�
	// �f�o�b�O���C���[���I���ɂ��邱�ƂŁAGPU���ŃG���[���o���烍�O���o�͂���悤�ɐݒ肷��
	ComPtr<ID3D12Debug>	debugLayer;
	HRESULT r = D3D12GetDebugInterface(IID_PPV_ARGS(debugLayer.ReleaseAndGetAddressOf()));
	debugLayer->EnableDebugLayer();		// �f�o�b�O���C���[��L���ɂ���
	debugLayer.Reset();					// �f�o�b�O���C���[�I�u�W�F�N�g��j��
#endif 

	// DirectXDevice�Ƃ����̂�DXGI�Ƃ����d�g�݂�����A����2���A�g���Ƃ��ē������A�ʕ�
#ifdef _DEBUG
	constexpr UINT debugFlg = DXGI_CREATE_FACTORY_DEBUG;
#else
	constexpr UINT debugFlg = 0;
#endif

	// DXGI(DirectXGraphicsInterface)�̏�����
	HRESULT result = CreateDXGIFactory2(debugFlg,
		IID_PPV_ARGS(dxgi_.ReleaseAndGetAddressOf()));

	// result�͂����Ȓl���A���Ă���
	assert(SUCCEEDED(result));	// SUCCEED�ň͂ނƁA���̌��ʂ�OK
	if (FAILED(result))			// FAILED�ň͂ނƁA���̌��ʂ�OUT
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

		// PC��GPU��AMD�̏ꍇ�A�I�΂�Ȃ��\��������
		if (strDesc.find(L"NVIDIA") != std::string::npos)
		{
			tmpAdapter = adpt;
			break;
		}
	}

	// �@�\���x�����ŐV����4�O���炢�܂ŗp�ӂ��Ă���
	std::array<D3D_FEATURE_LEVEL, 4> fLevels =
	{
		D3D_FEATURE_LEVEL_12_1,	// �ŐV
		D3D_FEATURE_LEVEL_12_0,	// 1����O
		D3D_FEATURE_LEVEL_11_1,	// 2����O
		D3D_FEATURE_LEVEL_11_0,	// 3����O
	};

	// Direct3D�̏�����
	D3D_FEATURE_LEVEL decidedFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	for (auto flv : fLevels)
	{
		// ReleaseAndGetAddress0f�́Adev_�̃|�C���^�̃|�C���^��\���Ă���
		// DirectX�̊�{�I�u�W�F�N�g�ƂȂ�D3DDevice�I�u�W�F�N�g���擾����
		result = D3D12CreateDevice(
			nullptr,	// nullptr������΁A�����I��
			flv,		// �t�B�[�`�����x��(DirectX�̃o�[�W����)
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

	// �R�}���h�L���[�̍쐬
	// �R�}���h�L���[�Ƃ́A�R�}���h���X�g���O���{�ɉ����o���Ď��s�������
	D3D12_COMMAND_QUEUE_DESC cmdQueDesc = {};
	cmdQueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;			// �t���O�Ȃ�
	cmdQueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;	// �ʏ폇
	cmdQueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;			// �R�}���h���X�g�𕁒ʂɎ��s
	cmdQueDesc.NodeMask = 0;									// ��{�I��0
	result = dev_->CreateCommandQueue(&cmdQueDesc, IID_PPV_ARGS(cmdQue_.ReleaseAndGetAddressOf()));
	if (FAILED(result))
	{
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = app->GetWindowWidth();				// �E�B���h�E��
	scDesc.Height = app->GetWindowHeight();				// �E�B���h�E����
	scDesc.Stereo = false;								// VR�̎��ȊOfalse
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;		// �A���t�@�͓��ɂ�����Ȃ�
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;			// RGBA 8bit�̕W��

	// �A���`�G�C���A�V���O�ɂ�������Ă�
	scDesc.SampleDesc.Count = 1;							// AA�Ȃ�
	scDesc.SampleDesc.Quality = 0;							// AA�N�I���e�B�Œ�
	scDesc.Scaling = DXGI_SCALING_STRETCH;					// ��ʂ̃T�C�Y��ς���ƁA����ɍ��킹��
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// �E�B���h�E�ƑS��ʂ�؂�ւ����e
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	scDesc.BufferCount = 2;									// �\��ʂƗ���ʂ̓�

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
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// �����_�[�^�[�Q�b�g�r���[�Ƃ���
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	result = dev_->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(rtvHeaps_.ReleaseAndGetAddressOf()));

	D3D12_CPU_DESCRIPTOR_HANDLE descHandle = { rtvHeaps_->GetCPUDescriptorHandleForHeapStart() };

	// ���łɕ\��ʂƗ���ʂ�SwapChain���ɂ���̂ŁA������擾�ł���悤��swapChain�̏����擾���Ă���
	DXGI_SWAP_CHAIN_DESC1 scDescForRTV = {};
	swapChain_->GetDesc1(&scDesc);

	// �X���b�v�`�F�[���������Ă�o�b�t�@�擾�p

	// �X���b�v�`�F�[�����̃o�b�t�@�������[�v����
	for (int i = 0; i < scDesc.BufferCount; ++i)
	{
		// �X���b�v�`�F�[���̓���ԍ��̃o�b�t�@���擾����
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

	//���_�f�[�^(CPU���猩����f�[�^)
	std::array<XMFLOAT3, 3> vertices =
	{
		XMFLOAT3(-1.0f,-1.0f,0.5f),	//����
		XMFLOAT3(0.1f,1.0f,0.5f),	//�^�񒆏�
		XMFLOAT3(1.0f,-1.0f,0.5f),	//�E��
	};

	//GPU�����p�ł���u���_�o�b�t�@�v�����
	auto vertResDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMFLOAT3)* vertices.size());
	auto vertHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	//�p�r�͂Ƃ������o�b�t�@���m�ۂ���֐���CreateCommitedResource
	//�m�ۂ���̂ł����A�����ŗp�r�ɉ����Ċm�ۂ̎d�����œK�����悤�Ƃ��邽�߁A
	//�₽��ׂ����w�肪�K�v�ɂȂ�
	result = dev_->CreateCommittedResource(
		&vertHeapProps,						//�m�ۂ���郁�����̓���(CPU���炢�����)
		D3D12_HEAP_FLAG_NONE,				//����ȃt���O�����邩�H
		&vertResDesc,						//�ǂ��ɔz�u���ׂ���
		D3D12_RESOURCE_STATE_GENERIC_READ,	//�ŏ��̗p�r
		nullptr,							//�N���A���邽�߂̃o�b�t�@		
		IID_PPV_ARGS(vertexBuffer_.ReleaseAndGetAddressOf())
	);
	assert(SUCCEEDED(result));

	//GPU��̃�������M��邽�߂�Map�֐������s����
	//GPU��̃������͘M��Ȃ����A�o�q��������
	//���p����CPU����GPU�̓��e���������悤�ɂ���
	XMFLOAT3* vertMap = nullptr;
	result = vertexBuffer_->Map(0, nullptr,(void**)&vertMap );
	assert(SUCCEEDED(result));

	//���_�f�[�^��GPU��(CPU��̑o�q������)�̃o�b�t�@�ɃR�s�[����
	//memcpy�݂����Ȋ֐��ł����A�����STL�̈��ŁA�o�b�t�@�I�[�o�[������
	//���o���Ă����o�[�W�����Ȃ̂ŁA��������g��
	std::copy(vertices.begin(), vertices.end(), vertMap);
	vertexBuffer_->Unmap(0, nullptr);

	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	//�S�̂̃T�C�Y
	vbView_.SizeInBytes = sizeof(XMFLOAT3) * vertices.size();
	//Stride�͕����̈Ӗ��A�܂莟�̃f�[�^�܂ł̋���������
	vbView_.StrideInBytes = sizeof(XMFLOAT3);

	ComPtr<ID3DBlob> vsBlob = nullptr;	//���_�V�F�[�_��
	ComPtr<ID3DBlob> psBlob = nullptr;	//�s�N�Z���V�F�[�_��

	//���_�V�F�[�_�̃��[�h(���łɃR���p�C��)
	result = D3DCompileFromFile(L"VertexShader.hlsl",
		nullptr,nullptr,"main","vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		vsBlob.ReleaseAndGetAddressOf(),nullptr);




	return true;
}

bool DirectX12Wrapper::Update()
{
	static float red = 0.0f;

	cmdAlloc_->Reset();
	cmdList_->Reset(cmdAlloc_.Get(), nullptr);

	//�����_�[�^�[�Q�b�g�̎w��
	auto heapStart = rtvHeaps_->GetCPUDescriptorHandleForHeapStart();
	auto bbIdx = swapChain_->GetCurrentBackBufferIndex();	//���̃o�b�N�o�b�t�@�ԍ�
	heapStart.ptr += bbIdx * dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	auto beforeBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvResources_[bbIdx],					//�Ώۂ̃\�[�X(�f�[�^�̉�)
		D3D12_RESOURCE_STATE_PRESENT,			//�O�̎g����
		D3D12_RESOURCE_STATE_RENDER_TARGET		//��̎g����
	);

	cmdList_->ResourceBarrier(1, &beforeBarrier);
	//�u���̃����_�[�^�[�Q�b�g�ɏ������ނ�v
	cmdList_->OMSetRenderTargets(1, &heapStart, false, nullptr);

	red = fmodf(red + 0.0001f, 1.0f);
	// ���̎��̃����_�[�^�[�Q�b�g�̃N���A
	std::array<float, 4> clearColor = { red, 1.0f, 0.0f, 1.0f };
	cmdList_->ClearRenderTargetView(heapStart, clearColor.data(), 0, nullptr);

	//���_�o�b�t�@���Z�b�g����
	cmdList_->IASetVertexBuffers(0, 1, &vbView_);

	auto afterBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		rtvResources_[bbIdx],					//�Ώۂ̃\�[�X(�f�[�^�̉�)
		D3D12_RESOURCE_STATE_RENDER_TARGET,		//�O�̎g����
		D3D12_RESOURCE_STATE_PRESENT			//��̎g����
	);
	cmdList_->ResourceBarrier(1, &afterBarrier);

	cmdList_->Close();	//�R�}���h�͍Ō�ɁA�u�K���v�N���[�Y����

	ID3D12CommandList* cmdlists[] = { cmdList_.Get() };
	cmdQue_->ExecuteCommandLists(1, cmdlists);
	//GPU�̐��\���I�������Adence_�쒆�̒l���AfenceValue_�ɕω�����
	//�Ⴆ�Ώ���Ȃ�A�ŏ�fence_��0�ŏ���������Ă�̂�0
	//�����ŁA ++fenceValue_��n���Ă��邽�߁AGPU���\��̏������I�������
	//fence_�̒��̒l��1�ɂȂ�
	cmdQue_->Signal(fence_.Get(), ++fenceValue_);//�҂��Ȃ�
	//�������҂��̖{��
	while (fence_->GetCompletedValue() != fenceValue_)
	{
		//�������Ȃ�
	}

	swapChain_->Present(0, 0);

	return false;
}
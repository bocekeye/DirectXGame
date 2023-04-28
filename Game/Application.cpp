#include <DirectXHelpers.h>
#include <Windows.h>
#include <cassert>
#include "Application.h"
#include "DirectX12Wrapper.h"

/// <summary>
/// �E�B���h�E�v���V�[�W��(OS����̃R�[���o�b�N�֐�)
/// </summary>
/// <param name="hWnd"></param>
/// <param name="msg"></param>
/// <param name="wparam"></param>
/// <param name="lparam"></param>
/// <returns></returns>
LRESULT WindowProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);	//���̃A�v���P�[�V�����͏I���
		return 0;
	}
	return DefWindowProc(hWnd, msg, wparam, lparam);
}

constexpr int window_width = 640;
constexpr int window_height = 480;
constexpr wchar_t class_name[] = L"GameWindow";


Application::Application()
{
	//���́A�A�v���P�[�V�����n���h��
	//GetModuleHandle(0)�ł��擾�ł���
	instance_ = GetModuleHandle(0);
	WNDCLASSEX wc = {};
	wc.hInstance = instance_;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.lpszClassName = class_name;

	//windowOS�ɑ΂��āu���̃E�B���h�E�N���X���g���Ǝw���v
	auto result = RegisterClassEx(&wc);
	assert(result != 0);

	RECT wrc = { 0,0,window_width,window_height };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	wHandle_ = CreateWindow(
		wc.lpszClassName,		//OS�����ʂ��邽�߂̍��O
		L"GameWindow ",			//�^�C�g���o�[�̕�����
		WS_OVERLAPPEDWINDOW,	//���ʂɏd�˂���E�B���h�E�̂���
		CW_USEDEFAULT,			//X���W���f�t�H���g�ɂ���
		CW_USEDEFAULT,			//Y���W���f�t�H���g�ɂ���
		wrc.right - wrc.left,	//�E�B���h�E��
		wrc.bottom - wrc.top,	//�E�B���h�E����
		nullptr,				//�e�E�B���h�E�n���h��
		nullptr,				//���j���[�E�B�h�E�n���h��(���j���[���Ȃ��̂�nullptr)
		wc.hInstance,			//�E�B���h�E�쐬���̃A�v���̃n���h��
		nullptr);				//�ǉ��̃A�v���P�[�V�����f�[�^(�g��Ȃ��̂�nullptr)

	assert(wHandle_ != NULL);

	dx12_ = std::make_shared<DirectX12Wrapper>();
	dx12_->Init(this);
}

Application& Application::GetInstance()
{
	static Application instance;
	return instance;
}

void Application::Run()
{
	ShowWindow(wHandle_, SW_SHOW);
	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);	//���b�Z�[�W�̖|��(���z�L�[����ϊ�)
			DispatchMessage(&msg);	//�g��ȕ����b�Z�[�W�����ɉ�
		}
		if (msg.message == WM_QUIT)
		{
			break;
		}
	}
}

HINSTANCE Application::GetInstanceHandle() const
{
	return instance_;
}

HWND Application::GetWindowHandle() const
{
	return wHandle_;
}

int Application::GetWindowWidth() const
{
	return window_width;
}

int Application::GetWindowHeight() const
{
	return window_height;
}

Application::~Application()
{
	UnregisterClass(class_name, instance_);
}

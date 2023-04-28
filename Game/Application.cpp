#include <DirectXHelpers.h>
#include <Windows.h>
#include <cassert>
#include "Application.h"
#include "DirectX12Wrapper.h"

/// <summary>
/// ウィンドウプロシージャ(OSからのコールバック関数)
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
		PostQuitMessage(0);	//このアプリケーションは終わり
		return 0;
	}
	return DefWindowProc(hWnd, msg, wparam, lparam);
}

constexpr int window_width = 640;
constexpr int window_height = 480;
constexpr wchar_t class_name[] = L"GameWindow";


Application::Application()
{
	//実は、アプリケーションハンドは
	//GetModuleHandle(0)でも取得できる
	instance_ = GetModuleHandle(0);
	WNDCLASSEX wc = {};
	wc.hInstance = instance_;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.lpszClassName = class_name;

	//windowOSに対して「このウィンドウクラスを使うと指示」
	auto result = RegisterClassEx(&wc);
	assert(result != 0);

	RECT wrc = { 0,0,window_width,window_height };
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	wHandle_ = CreateWindow(
		wc.lpszClassName,		//OSが識別するための根前
		L"GameWindow ",			//タイトルバーの文字列
		WS_OVERLAPPEDWINDOW,	//普通に重ねられるウィンドウのこと
		CW_USEDEFAULT,			//X座標をデフォルトにする
		CW_USEDEFAULT,			//Y座標をデフォルトにする
		wrc.right - wrc.left,	//ウィンドウ幅
		wrc.bottom - wrc.top,	//ウィンドウ高さ
		nullptr,				//親ウィンドウハンドル
		nullptr,				//メニューウィドウハンドル(メニューもないのでnullptr)
		wc.hInstance,			//ウィンドウ作成元のアプリのハンドル
		nullptr);				//追加のアプリケーションデータ(使わないのでnullptr)

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
			TranslateMessage(&msg);	//メッセージの翻訳(仮想キー等を変換)
			DispatchMessage(&msg);	//使わな方メッセージを次に回す
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

#pragma once
#include <Windows.h>
#include <memory>

class DirectX12Wrapper;

/// <summary>
/// アプリ全体で使うアプリシングルトンクラス
/// </summary>
class Application
{
private:
	std::shared_ptr<DirectX12Wrapper> dx12_;
	HINSTANCE instance_;	//アプリケーションインスタンス	
	HWND wHandle_;			//ウィンドウハンドル
	Application();

	//コピー、代入禁止
	Application(const Application&) = delete;
	void operator = (const Application&) = delete;
public:
	static Application& GetInstance();
	/// <summary>
	/// DirectXアプリケーションを開始する
	/// </summary>
	void Run();
	HINSTANCE GetInstanceHandle() const;
	HWND GetWindowHandle() const;
	int GetWindowWidth()const;
	int GetWindowHeight()const;
	~Application();
};


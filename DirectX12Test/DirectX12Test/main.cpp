#include <Windows.h>
#include <vector>
#ifdef  _DEBUG
#include <iostream>
#endif // _DEBUG

#include <d3d12.h>
#include <dxgi1_6.h>
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define window_width 960
#define window_height 720

using namespace std;

// @brief コンソールがめんにフォーマット付き文字列を表示
// @param format フォーマット（%d とか %f とかの）
// @param 可変長引数
// @remarks この関数はデバッグ用です。デバッグ時にしか動作しません
void DebugOutputFormatString(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf(format, valist);
	va_end(valist);
#endif // _DEBUG
}

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ウィンドウが破棄されたら呼ばれる
	if (msg == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}


#ifdef _DEBUG
int main()
{
	//-------------------------------------------------------------------
	// Direct3D デバイスの初期化
	//-------------------------------------------------------------------
	//
	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;

	//-------------------------------------------------------------------
	// 使用するアダプタ（グラフィックスボード）の特定
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	std::vector<IDXGIAdapter*> adapters;	// アダプターの列挙用
	IDXGIAdapter* tmpAdapter = nullptr;		// 特定の名前を持つアダプタオブジェクトが入る

	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);	// アダプタの説明オブジェクト取得

		std::wstring strDesc = adesc.Description;

		// 探したいアダプタの名前を確認
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}


	//-------------------------------------------------------------------
	// グラフィックスドライバのオブジェクト生成
	// グラフィックスドライバのフィーチャーレベル
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	D3D_FEATURE_LEVEL featureLevel;

	// デバイス（GPU）のオブジェクト生成
	//	D3D12CreateDevice(
	//		nullptr,			    // アダプタ（グラフィックスドライバー）のポインタ
	//		D3D_FEATURE_LEVEL_12_1, // 最低限必要なフィーチャーレベル
	//		IID_PPV_ARGS(&_dev)));

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;	// 生成可能なバージョンが見つかったらループを打ち切り
		}

		if (_dev == nullptr) PostQuitMessage(-1); // 生成不可能なら破棄
	}


	//-------------------------------------------------------------------
	// ウィンドウクラスの生成&登録
	//-------------------------------------------------------------------
	//
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // コールバック関数の指定
	w.lpszClassName = TEXT("DX12Sample");     // アプリケーションクラス名（適当でよい）
	w.hInstance = GetModuleHandle(nullptr);   // ハンドルの取得

	RegisterClassEx(&w); // アプリケーションクラス（ウィンドウクラスの指定をOSに伝える）

	RECT wrc = { 0, 0, (double)window_width, (double)window_height }; // ウィンドウサイズを決める

	// 関数を使ってウィンドウのサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウオブジェクトの生成
	HWND hwnd = CreateWindow(
		w.lpszClassName,	  // クラス名指定
		TEXT("DX12テスト"),    // タイトルバーの文字
		WS_OVERLAPPEDWINDOW,  // タイトルバーと境界線があるウィンドウ
		CW_USEDEFAULT,	      // 表示x座標は OS にお任せ
		CW_USEDEFAULT,		  // 表示y座標は OS にお任せ
		wrc.right - wrc.left, // ウィンドウ幅
		wrc.bottom - wrc.top, // ウィンドウ高
		nullptr,			  // 親ウィンドウハンドル
		nullptr,              // メニューハンドル
		w.hInstance,		  // 呼び出しアプリケーションハンドル
		nullptr				  // 追加パラメーター
	);

	// ウィンドウ表示
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// アプリケーションが終わるときに message が WM_QUIT になる
		if (msg.message == WM_QUIT) break;
	}

	// もうクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);

	//-------------------------------------------------------------------
	// 


#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif // _DEBUG
	DebugOutputFormatString("Show Window Test.");
	getchar();
	return 0;
}


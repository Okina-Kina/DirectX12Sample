#include <Windows.h>
#include <vector>
#ifdef  _DEBUG
#include <iostream>
#endif // _DEBUG

#include <d3d12.h>
#include <DirectXMath.h>
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

//-------------------------------------------------------------------
// デバッグレイヤの有効化
void EnableDebugLayer() {
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer();
	debugLayer->Release();
}

#ifdef _DEBUG
int main()
{
#ifdef _DEBUG
	EnableDebugLayer();
#endif

	//-------------------------------------------------------------------
	// 1. Direct3D デバイスの初期化
	//-------------------------------------------------------------------
	//
	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;

	//-------------------------------------------------------------------
	// 使用するアダプタ（グラフィックスボード）の特定
#ifdef _DEBUG
	auto result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&_dxgiFactory));
#else
	auto result = CreateDXGIFactory(IID_PPV_ARGS(&_dxgiFactory));
#endif

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
	// 2. グラフィックスドライバのオブジェクト生成
	//	  グラフィックスドライバのフィーチャーレベル
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
	// 3. ウィンドウクラスの生成&登録
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

	//-------------------------------------------------------------------
	// 4. コマンドリストの作成とコマンドアロケーター
	//-------------------------------------------------------------------
	//

	// コマンドリストとコマンドアロケータ―の生成
	ID3D12CommandAllocator* _cmdAllocator = nullptr;	// コマンドアロケータ―
	ID3D12GraphicsCommandList* _cmdList = nullptr;		// コマンドリスト

	result = _dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmdAllocator));
	result = _dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmdAllocator, nullptr, IID_PPV_ARGS(&_cmdList));

	// コマンドキューの生成
	ID3D12CommandQueue* _cmdQueue = nullptr;					 // コマンドキュー
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};					 // コマンドキューのディスクリプタ

	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;			 // タイムアウトなし
	cmdQueueDesc.NodeMask = 0;									 // アダプター（グラボ）を1つしか使用しない場合は0でいい
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; // プライオリティは特に指定なし
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;			 // コマンドリストと命令セットの種類を合わせる

	result = _dev->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&_cmdQueue));	// コマンドキュー生成


	//-------------------------------------------------------------------
	// 5. スワップチェーン
	//-------------------------------------------------------------------
	// スワップチェーンオブジェクトの生成
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = { };

	swapchainDesc.Width = window_width;					// 画面解像度【幅】
	swapchainDesc.Height = window_height;				// 画面解像度【高】
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;	// ピクセルフォーマット
	swapchainDesc.Stereo = false;						// ステレオ表示フラグ（3Dディスプレイのステレオモード）
	swapchainDesc.SampleDesc.Count = 1;					// マルチサンプルの指定
	swapchainDesc.SampleDesc.Quality = 0;				// 
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // 
	swapchainDesc.BufferCount = 2;						// バッファ数（ダブルバッファなので２）

	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;				  // バックバッファは伸び縮み可能
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;	  // フリップ後は速やかに破棄
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;		  // 
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // ウィンドウ <=> フルスクリーン切り替え可能

	result = _dxgiFactory->CreateSwapChainForHwnd(
		_cmdQueue,							// コマンドキューオブジェクト
		hwnd,								// ウィンドウハンドル
		&swapchainDesc,						// スワップチェーン設定
		nullptr,							// フルスクリーンの時のスワップチェーン
		nullptr,							// 
		(IDXGISwapChain1**)&_swapchain		// スワップチェーンオブジェクト取得用
	);

	//-------------------------------------------------------------------
	// 5.1. ディスクリプタヒープの作成（スワップチェーン用のRTV）
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};

	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;	// RTV用のディスクリプタを作成
	heapDesc.NodeMask = 0;		 // グラボの識別子
	heapDesc.NumDescriptors = 2; // 表裏の２つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダ側から参照する「ビュー」の情報

	ID3D12DescriptorHeap* rtvHeaps = nullptr;
	result = _dev->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&rtvHeaps));

	// 生成済みのスワップチェーンからディスクリプタを取得
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = _swapchain->GetDesc(&swcDesc);

	std::vector<ID3D12Resource*> _backBuffers(swcDesc.BufferCount);
	for (int idx = 0; idx < swcDesc.BufferCount; ++idx)
	{
		// ディスクリプタヒープのバッファをスワップチェーン上のバッファに登録（表裏）
		result = _swapchain->GetBuffer(idx, IID_PPV_ARGS(&_backBuffers[idx]));

		// ディスクリプタヒープのハンドルの先頭アドレス
		D3D12_CPU_DESCRIPTOR_HANDLE handle
			= rtvHeaps->GetCPUDescriptorHandleForHeapStart();

		// ディスクリプタタイプのサイズに応じてアドレスをずらす
		handle.ptr += idx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// RTV（レンダーターゲットビューの作成）
		_dev->CreateRenderTargetView(
			_backBuffers[idx],	// バッファ
			nullptr,			// 
			handle				// ディスクリプタヒープのハンドルの先頭アドレス
		);
	}

	//-------------------------------------------------------------------
	// 5.2 スワップチェーンを動作

	// 1. コマンドアロケータ―とコマンドリストをクリア（アロケータ―のリセット）
	_cmdAllocator->Reset();

	// 2.【コマンド】レンダーターゲットの設定

	auto backBufferIdx = _swapchain->GetCurrentBackBufferIndex();

	auto rtvH = rtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += backBufferIdx * _dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// バックバッファはRTVとして使用する（リソースバリアの設定）
	D3D12_RESOURCE_BARRIER BarrierDesc = {};
	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;	// 状態遷移する
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Transition.pResource = _backBuffers[backBufferIdx];	// バックバッファリソース
	BarrierDesc.Transition.Subresource = 0;
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;		// 使用前は「PRESENT」状態
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;	// 使用直後に「RTV」に遷移

	_cmdList->ResourceBarrier(1, &BarrierDesc);	// リソースバリアを設定
	_cmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);	// レンダーターゲットを設定

	// 3.【コマンド】レンダーターゲットのクリア
	float clearColor[] = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色
	_cmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	// 4. 【コマンド】コマンドリストのクローズ
	_cmdList->Close();	// コマンドリストはクローズ状態になる

	// 5. コマンドリストの実行	

	// フェンスの生成
	ID3D12Fence* _fence = nullptr;
	UINT64 _fenceVal = 0;

	result = _dev->CreateFence(_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence));

	// コマンドリストの実行
	ID3D12CommandList* cmdlists[] = { _cmdList };
	_cmdQueue->ExecuteCommandLists(
		1,			// 実行するコマンドリスト数
		cmdlists	// コマンドリスト配列の先頭アドレス
	);

	// GPUに対して実行したコマンドリストの命令の終了を通知
	_cmdQueue->Signal(
		_fence,
		++_fenceVal// GPUの処理が完了したあとになっているべき値（フェンス値
	);

	while (_fence->GetCompletedValue() != _fenceVal) {
		auto event = CreateEvent(nullptr, false, false, nullptr); // イベントハンドルの取得
		_fence->SetEventOnCompletion(_fenceVal, event);			  // イベントを発生
		WaitForSingleObject(event, INFINITE);					  // イベントが発生するまで待つ
		CloseHandle(event);										  // イベントハンドルをとじる
	}

	_cmdAllocator->Reset();	// 命令セットを全クリア
	_cmdList->Reset(_cmdAllocator, nullptr);	// クローズ状態の解除

	// 6. 画面のスワップ
	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET; // 前の状態を「RTV」に
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;		 // 今から「PRESENT」状態にする

	_cmdList->ResourceBarrier(1, &BarrierDesc); // リソースの状態遷移後のバリアを設定

	// 画面のフリップ
	_swapchain->Present(
		1,	// フリップするまでの待ちフレーム数（）
		0	// 
	);


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

	// もうウィンドウのクラスは使わないので登録解除する
	UnregisterClass(w.lpszClassName, w.hInstance);



#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif // _DEBUG
	DebugOutputFormatString("Show Window Test.");
	getchar();
	return 0;
}


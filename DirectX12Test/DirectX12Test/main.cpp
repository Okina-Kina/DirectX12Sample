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

// @brief �R���\�[�����߂�Ƀt�H�[�}�b�g�t���������\��
// @param format �t�H�[�}�b�g�i%d �Ƃ� %f �Ƃ��́j
// @param �ϒ�����
// @remarks ���̊֐��̓f�o�b�O�p�ł��B�f�o�b�O���ɂ������삵�܂���
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
	// �E�B���h�E���j�����ꂽ��Ă΂��
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
	// Direct3D �f�o�C�X�̏�����
	//-------------------------------------------------------------------
	//
	ID3D12Device* _dev = nullptr;
	IDXGIFactory6* _dxgiFactory = nullptr;
	IDXGISwapChain4* _swapchain = nullptr;

	//-------------------------------------------------------------------
	// �g�p����A�_�v�^�i�O���t�B�b�N�X�{�[�h�j�̓���
	auto result = CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory));
	std::vector<IDXGIAdapter*> adapters;	// �A�_�v�^�[�̗񋓗p
	IDXGIAdapter* tmpAdapter = nullptr;		// ����̖��O�����A�_�v�^�I�u�W�F�N�g������

	for (int i = 0; _dxgiFactory->EnumAdapters(i, &tmpAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		adapters.push_back(tmpAdapter);
	}

	for (auto adpt : adapters) {
		DXGI_ADAPTER_DESC adesc = {};
		adpt->GetDesc(&adesc);	// �A�_�v�^�̐����I�u�W�F�N�g�擾

		std::wstring strDesc = adesc.Description;

		// �T�������A�_�v�^�̖��O���m�F
		if (strDesc.find(L"NVIDIA") != std::string::npos) {
			tmpAdapter = adpt;
			break;
		}
	}


	//-------------------------------------------------------------------
	// �O���t�B�b�N�X�h���C�o�̃I�u�W�F�N�g����
	// �O���t�B�b�N�X�h���C�o�̃t�B�[�`���[���x��
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0
	};

	D3D_FEATURE_LEVEL featureLevel;

	// �f�o�C�X�iGPU�j�̃I�u�W�F�N�g����
	//	D3D12CreateDevice(
	//		nullptr,			    // �A�_�v�^�i�O���t�B�b�N�X�h���C�o�[�j�̃|�C���^
	//		D3D_FEATURE_LEVEL_12_1, // �Œ���K�v�ȃt�B�[�`���[���x��
	//		IID_PPV_ARGS(&_dev)));

	for (auto lv : levels)
	{
		if (D3D12CreateDevice(tmpAdapter, lv, IID_PPV_ARGS(&_dev)) == S_OK)
		{
			featureLevel = lv;
			break;	// �����\�ȃo�[�W���������������烋�[�v��ł��؂�
		}

		if (_dev == nullptr) PostQuitMessage(-1); // �����s�\�Ȃ�j��
	}


	//-------------------------------------------------------------------
	// �E�B���h�E�N���X�̐���&�o�^
	//-------------------------------------------------------------------
	//
	WNDCLASSEX w = {};

	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = (WNDPROC)WindowProcedure; // �R�[���o�b�N�֐��̎w��
	w.lpszClassName = TEXT("DX12Sample");     // �A�v���P�[�V�����N���X���i�K���ł悢�j
	w.hInstance = GetModuleHandle(nullptr);   // �n���h���̎擾

	RegisterClassEx(&w); // �A�v���P�[�V�����N���X�i�E�B���h�E�N���X�̎w���OS�ɓ`����j

	RECT wrc = { 0, 0, (double)window_width, (double)window_height }; // �E�B���h�E�T�C�Y�����߂�

	// �֐����g���ăE�B���h�E�̃T�C�Y��␳����
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// �E�B���h�E�I�u�W�F�N�g�̐���
	HWND hwnd = CreateWindow(
		w.lpszClassName,	  // �N���X���w��
		TEXT("DX12�e�X�g"),    // �^�C�g���o�[�̕���
		WS_OVERLAPPEDWINDOW,  // �^�C�g���o�[�Ƌ��E��������E�B���h�E
		CW_USEDEFAULT,	      // �\��x���W�� OS �ɂ��C��
		CW_USEDEFAULT,		  // �\��y���W�� OS �ɂ��C��
		wrc.right - wrc.left, // �E�B���h�E��
		wrc.bottom - wrc.top, // �E�B���h�E��
		nullptr,			  // �e�E�B���h�E�n���h��
		nullptr,              // ���j���[�n���h��
		w.hInstance,		  // �Ăяo���A�v���P�[�V�����n���h��
		nullptr				  // �ǉ��p�����[�^�[
	);

	// �E�B���h�E�\��
	ShowWindow(hwnd, SW_SHOW);

	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// �A�v���P�[�V�������I���Ƃ��� message �� WM_QUIT �ɂȂ�
		if (msg.message == WM_QUIT) break;
	}

	// �����N���X�͎g��Ȃ��̂œo�^��������
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


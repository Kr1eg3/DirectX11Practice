#include "Graphics.h"
#include "dxerr.h"
#include <sstream>
#include <d3dcompiler.h>
#include <cmath>
#include <DirectXMath.h>
#include "GraphicsThrowMacros.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

Graphics::Graphics(HWND hWnd) {
	DXGI_SWAP_CHAIN_DESC sd = {};

	// A DXGI_MODE_DESC structure that describes the backbuffer display mode. (BufferDesc)
	// Width and height of the back buffer in pixels. If you specify 0 for Width and Height, DXGI
	// will use the width and height of the client area of the target window.
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;

	// A member of the DXGI_FORMAT enumerated type that describes the display format.
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;

	// A DXGI_RATIONAL structure that describes the refresh rate in hertz of the back buffer.
	// If you specify 0 for the numerator and the denominator, DXGI will use the default refresh rate.
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;

	// A member of the DXGI_MODE_SCANLINE_ORDER enumerated type that describes the scanline drawing mode.
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// A member of the DXGI_MODE_SCANLINE_ORDER enumerated type that describes the scanline drawing mode.
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	// A member of the DXGI_SAMPLE_DESC structure that describes multi-sampling parameters.
	// If you do not want to use multi-sampling, set Count to 1 and Quality to 0. (anti-aliasing)
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	// A member of the DXGI_USAGE enumerated type that describes the surface usage and CPU access options for the back buffer.
	// The back buffer can be used for shader input or render-target output.
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// A value that describes the number of buffers in the swap chain. When you call IDXGIFactory::CreateSwapChain
	// to create a full-screen swap chain, you typically include the front buffer in this value.
	// Set it to 1 for double buffering, or 2 for triple buffering.
	sd.BufferCount = 1;

	// An HWND handle to the output window. This member must not be NULL.
	sd.OutputWindow = hWnd;

	// A Boolean value that specifies whether the output is in windowed mode. TRUE if the output is in windowed mode;
	// otherwise, FALSE.
	// We recommend that you create a windowed swap chain and allow the end user to change the swap chain
	// to full screen through IDXGISwapChain::SetFullscreenState; that is, do not set this member to FALSE
	// to force the swap chain to be full screen. However, if you create the swap chain as full screen,
	// also provide the end user with a list of supported display modes through the BufferDesc member because
	// a swap chain that is created with an unsupported display mode might cause the display to go black and
	// prevent the end user from seeing anything.
	sd.Windowed = TRUE;

	// A member of the DXGI_SWAP_EFFECT enumerated type that describes options
	// for handling the contents of the presentation buffer after presenting a surface.
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// A member of the DXGI_SWAP_CHAIN_FLAG enumerated type that describes options for swap-chain behavior.
	sd.Flags = 0;

	UINT swapCreateFlags = 0u;
#ifndef NDEBUG
	swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	// for checking results of d3d functions
	HRESULT hr;

	// Create device and front/back buffers, and swap chain and rendering context
	GFX_THROW_INFO(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		swapCreateFlags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&pSwap,
		&pDevice,
		nullptr,
		&pContext
	));

	//D3D_FEATURE_LEVEL featureLevel = pDevice->GetFeatureLevel(); some shit with auto feature lvl

	Microsoft::WRL::ComPtr<ID3D11Resource> pBackBuffer;
	GFX_THROW_INFO(pSwap->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer));
	GFX_THROW_INFO(pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pTarget));

	// create depth stencil state
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> pDSState;
	GFX_THROW_INFO(pDevice->CreateDepthStencilState(&dsDesc, &pDSState));

	// bind depth state
	pContext->OMSetDepthStencilState(pDSState.Get(), 1u);

	// create depth stancil texture
	Microsoft::WRL::ComPtr<ID3D11Texture2D> pDepthStencil;
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = 800u;
	descDepth.Height = 600u;
	descDepth.MipLevels = 1u;
	descDepth.ArraySize = 1u;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1u;
	descDepth.SampleDesc.Quality = 0u;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	GFX_THROW_INFO(pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil));

	// create view of depth stencil texture
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0u;
	GFX_THROW_INFO(pDevice->CreateDepthStencilView(pDepthStencil.Get(), &descDSV, &pDepthStencilView));

	// bind depth stencil view to OM
	pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), pDepthStencilView.Get());

	// configure viewport
	D3D11_VIEWPORT vp = {};
	vp.Width = 800.0f;
	vp.Height = 600.0f;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	pContext->RSSetViewports(1u, &vp);

	// init imgui d3d impl
	ImGui_ImplDX11_Init(pDevice.Get(), pContext.Get());
}

void Graphics::EndFrame() {
	// imgui frame end
	if (imguiEnabled) {
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	HRESULT hr;
#ifndef NDEBUG
	infoManager.Set();
#endif
	if (FAILED(hr = pSwap->Present(1u, 0u))) {
		if (hr == DXGI_ERROR_DEVICE_REMOVED) {
			throw GFX_DEVICE_REMOVED_EXCEPT(pDevice->GetDeviceRemovedReason());
		} else {
			throw GFX_EXCEPT(hr);
		}
	}
}

void Graphics::BeginFrame(float red, float green, float blue) noexcept {
	// imgui begin frame
	if (imguiEnabled) {
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	const float color[] = { red, green, blue, 1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);
	pContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
}

void Graphics::DrawIndexed(UINT count) noexcept(!IS_DEBUG) {
	//pContext->DrawIndexed(count, 0u, 0u);
	GFX_THROW_INFO_ONLY(pContext->DrawIndexed(count, 0u, 0u));
}

void Graphics::SetProjection(DirectX::FXMMATRIX proj) noexcept {
	projection = proj;
}

DirectX::XMMATRIX Graphics::GetProjection() const noexcept {
	return projection;
}

void Graphics::EnableImgui() noexcept {
	imguiEnabled = true;
}

void Graphics::DisableImgui() noexcept {
	imguiEnabled = false;
}

bool Graphics::IsImguiEnabled() const noexcept {
	return imguiEnabled;
}

void Graphics::SetCamera(DirectX::FXMMATRIX cam) noexcept {
	camera = cam;
}

Graphics::~Graphics() {
	ImGui_ImplDX11_Shutdown();
}

DirectX::XMMATRIX Graphics::GetCamera() const noexcept {
	return camera;
}

// Graphics exception stuff
Graphics::HrException::HrException( int line,const char * file,HRESULT hr, std::vector<std::string> infoMsgs) noexcept
	:
	Exception(line, file),
	hr(hr) {
	// join all info messages with newlines into single string
	for (const auto& m : infoMsgs) {
		info += m;
		info.push_back('\n');
	}

	// remove final newline if exists
	if (!info.empty()) {
		info.pop_back();
	}
}
const char* Graphics::HrException::what() const noexcept {
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << static_cast<unsigned long>(GetErrorCode()) << ")" << std::endl
		<< "[Error String] " << GetErrorString() << std::endl
		<< "[Description] " << GetErrorDescription() << std::endl;
		if (!info.empty()) {
		oss << "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
		}
		oss << GetOriginString();	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Graphics::HrException::GetType() const noexcept {
	return "DirectX Practice Graphics Exception";
}

HRESULT Graphics::HrException::GetErrorCode() const noexcept {
	return hr;
}

std::string Graphics::HrException::GetErrorString() const noexcept {
	return DXGetErrorStringA(hr);
}

std::string Graphics::HrException::GetErrorDescription() const noexcept {
	char buf[512];
	DXGetErrorDescriptionA(hr, buf, sizeof(buf));
	return buf;
}

std::string Graphics::HrException::GetErrorInfo() const noexcept {
	return info;
}

const char* Graphics::DeviceRemovedException::GetType() const noexcept {
	return "DirectX Practice Graphics Exception [Device Removed] (DXGI_ERROR_DEVICE_REMOVED)";
}

Graphics::InfoException::InfoException(int line, const char * file, std::vector<std::string> infoMsgs) noexcept
	:
	Exception(line, file) {
	// join all info messages with newlines into single string
	for (const auto& m : infoMsgs) {
		info += m;
		info.push_back( '\n' );
	}

	// remove final newline if exists
	if (!info.empty()) {
		info.pop_back();
	}
}

const char* Graphics::InfoException::what() const noexcept {
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
	oss << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Graphics::InfoException::GetType() const noexcept {
	return "DirectX Practice Graphics Info Exception";
}

std::string Graphics::InfoException::GetErrorInfo() const noexcept {
	return info;
}

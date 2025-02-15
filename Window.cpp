#include "Window.h"
#include <sstream>
#include "WindowsThrowMacros.h"
#include "imgui/imgui_impl_win32.h"
//#include "resource.h"

// Window Class Stuff
Window::WindowClass Window::WindowClass::wndClass;

const char* Window::WindowClass::GetName() noexcept {
	return wndClassName;
}

HINSTANCE Window::WindowClass::GetInstance() noexcept {
	return wndClass.hInst;
}

Window::WindowClass::WindowClass() noexcept
	:
	hInst( GetModuleHandle( nullptr ) ) {
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof( wc );
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetInstance();
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	std::wstring className = StringToWString(GetName());
	wc.lpszClassName = className.c_str();
	wc.hIconSm = nullptr;
	RegisterClassEx( &wc );
}

Window::WindowClass::~WindowClass() {
	UnregisterClass(StringToWString(wndClassName).c_str(), GetInstance());
}

// Window Stuff
Window::Window(int width, int height, const char* name)
	:
	width(width),
	height(height) {
	// calculate window size based on desired client region size
	RECT wr {};
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;

	if (AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE) == 0) {
		throw WND_LAST_EXCEPT();
	}

	// create window & get hWnd
	hWnd = CreateWindow(
		StringToWString(WindowClass::GetName()).c_str(), StringToWString(name).c_str(),
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
		nullptr, nullptr, WindowClass::GetInstance(), this );
	if (!hWnd) {
		throw WND_LAST_EXCEPT();
	}

	// newly created windows start off as hidden
	ShowWindow(hWnd, SW_SHOWDEFAULT);

	// Init ImGui Win32 Impl
	ImGui_ImplWin32_Init(hWnd);

	// create graphics object
	pGfx = std::make_unique<Graphics>(hWnd);
}

Window::~Window() {
	ImGui_ImplWin32_Shutdown();
	DestroyWindow(hWnd);
}

void Window::SetTitle(const std::string& title) {
	if (SetWindowText(hWnd, StringToWString(title).c_str()) == 0) {
		throw WND_LAST_EXCEPT();
	}
}

std::optional<int> Window::ProcessMessages() noexcept {
	MSG msg;
	// while queue has messages, remove and dispatch them (but do not block on empty queue)
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		// check for quit because peekmessage does not signal this via return val
		if (msg.message == WM_QUIT) {
			// return optional wrapping int (arg to PostQuitMessage is in wparam) signals quit
			return static_cast<int>(msg.wParam);
		}
		// TranslateMessage will post auxilliary WM_CHAR messages from key msgs
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// return empty optional when not quitting app
	return {};
}

Graphics& Window::Gfx() {
	if (!pGfx) {
		throw WND_NOGFX_EXCEPT();
	}

	return *pGfx;
}

LRESULT WINAPI Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
	if( msg == WM_NCCREATE ) {
		// extract ptr to window class from creation data
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		// set WinAPI-managed user data to store ptr to window class
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		// set message proc to normal (non-setup) handler now that setup is finished
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));
		// forward message to window class handler
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}
	// if we get a message before the WM_NCCREATE message, handle with default handler
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
LRESULT WINAPI Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	// retrieve ptr to window class
	Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	// forward message to window class handler
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
	if(ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return true;
	}

	const auto& imio = ImGui::GetIO();

	switch(msg) {
	case WM_CLOSE:
		PostQuitMessage( 0 );
		return 0;
	case WM_KILLFOCUS:
		keyboard.ClearState();
		break;

	/*********** KEYBOARD MESSAGES ***********/
	case WM_KEYDOWN:
	[[fallthrough]];
	case WM_SYSKEYDOWN:
		// stifle this keyboard message if imgui wants to capture
		if (imio.WantCaptureKeyboard) {
			break;
		}
		if (!(lParam & 0x40000000) || keyboard.AutorepeatIsEnabled()) {
			keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
		}
		break;
	case WM_KEYUP:
	[[fallthrough]];
	case WM_SYSKEYUP:
		// stifle this keyboard message if imgui wants to capture
		if (imio.WantCaptureKeyboard) {
			break;
		}
		keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
		break;
	case WM_CHAR:
		// stifle this keyboard message if imgui wants to capture
		if (imio.WantCaptureKeyboard) {
			break;
		}
		keyboard.OnChar(static_cast<unsigned char>(wParam));
		break;
	/*********** END KEYBOARD MESSAGES ***********/

	/************* MOUSE MESSAGES ****************/
	case WM_MOUSEMOVE: {
		// stifle this mouse message if imgui wants to capture
		if (imio.WantCaptureMouse) {
			break;
		}
		const POINTS pt = MAKEPOINTS( lParam );
		// in client region -> log move, and log enter
		// + capture mouse (if not previously in window)
		if (pt.x >= 0 && pt.x < width && pt.y >= 0 && pt.y < height) {
			mouse.OnMouseMove(pt.x, pt.y);
			if (!mouse.IsInWindow()) {
				SetCapture(hWnd);
				mouse.OnMouseEnter();
			}
		} // not in client -> log move / maintain capture if button down
		else {
			if (wParam & (MK_LBUTTON | MK_RBUTTON)) {
				mouse.OnMouseMove(pt.x, pt.y);
			}
			// button up -> release capture / log event for leaving
			else {
				ReleaseCapture();
				mouse.OnMouseLeave();
			}
		}
		break;
	}
	case WM_LBUTTONDOWN: {
		SetForegroundWindow(hWnd);
		// stifle this mouse message if imgui wants to capture
		if (imio.WantCaptureMouse) {
			break;
		}
		const POINTS pt = MAKEPOINTS( lParam );
		mouse.OnLeftPressed(pt.x, pt.y);
		// release mouse if outside of window
		if (pt.x < 0 || pt.x >= width || pt.y < 0 || pt.y >= height) {
			ReleaseCapture();
			mouse.OnMouseLeave();
		}
		break;
	}
	case WM_RBUTTONDOWN: {
		const POINTS pt = MAKEPOINTS( lParam );
		mouse.OnRightPressed(pt.x, pt.y);
		break;
	}
	case WM_LBUTTONUP: {
		const POINTS pt = MAKEPOINTS( lParam );
		mouse.OnLeftReleased(pt.x, pt.y);
		break;
	}
	case WM_RBUTTONUP: {
		const POINTS pt = MAKEPOINTS( lParam );
		mouse.OnRightReleased(pt.x, pt.y);
		// release mouse if outside of window
		if (pt.x < 0 || pt.x >= width || pt.y < 0 || pt.y >= height) {
			ReleaseCapture();
			mouse.OnMouseLeave();
		}
		break;
	}
	case WM_MOUSEWHEEL: {
		const POINTS pt = MAKEPOINTS(lParam);
		const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		mouse.OnWheelDelta(pt.x, pt.y, delta);
		break;
	}
	/************** END MOUSE MESSAGES **************/

	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// Window Exception Stuff
std::string Window::Exception::TranslateErrorCode(HRESULT hr) noexcept {
	char* pMsgBuf = nullptr;
	const DWORD nMsgLen = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,hr,MAKELANGID( LANG_NEUTRAL,SUBLANG_DEFAULT ),
		reinterpret_cast<LPWSTR>(&pMsgBuf), 0, nullptr);

	if (nMsgLen == 0) return "Unidentified error code";
	std::string errorString = pMsgBuf;
	LocalFree( pMsgBuf );
	return errorString;
}

Window::HrException::HrException(int line, const char* file, HRESULT hr) noexcept
	:
	Exception( line,file ),
	hr( hr )
{}

const char* Window::HrException::what() const noexcept {
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << static_cast<unsigned long>(GetErrorCode()) << ")" << std::endl
		<< "[Description] " << GetErrorDescription() << std::endl
		<< GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* Window::HrException::GetType() const noexcept {
	return "DirectX Practice Window Exception";
}

HRESULT Window::HrException::GetErrorCode() const noexcept {
	return hr;
}

std::string Window::HrException::GetErrorDescription() const noexcept {
	return Exception::TranslateErrorCode(hr);
}

const char* Window::NoGfxException::GetType() const noexcept {
	return "DirectX Practice Window Exception [No Graphics]";
}

#include "Window.h"

Window::WindowClass Window::WindowClass::wndClass;

Window::WindowClass::WindowClass() : hInstance(GetModuleHandleW(nullptr))
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = GetName();

	RegisterClassExW(&wc);
}

Window::WindowClass::~WindowClass()
{
	UnregisterClassW(wndClassName, GetInstance());
}

const wchar_t* Window::WindowClass::GetName()
{
	return wndClassName;
}

HINSTANCE Window::WindowClass::GetInstance()
{
	return wndClass.hInstance;
}

Window::Window(int width, int height, const wchar_t* name) :
	width(width),
	height(height)
{
	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = wr.top + height;

	AdjustWindowRect(&wr, WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

	WIN_CALL(hWnd = CreateWindowW(
		WindowClass::GetName(),
		name,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wr.right - wr.left,
		wr.bottom - wr.top,
		nullptr, nullptr, WindowClass::GetInstance(), this
	));

	ShowWindow(hWnd, SW_SHOWDEFAULT);

	renderer.emplace(hWnd, width, height);
}

Renderer& Window::GetRenderer()
{
	return *renderer;
}

void Window::SetTitle(const std::string& title)
{
	std::wstring wTitle(title.begin(), title.end());
	WIN_CALL(SetWindowTextW(hWnd, wTitle.c_str()));
}

std::optional<int> Window::ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			return static_cast<int>(msg.wParam);
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return {};
}

LRESULT Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE)
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		const auto pWnd = static_cast<Window*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgInterpret));

		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsgInterpret(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	const auto pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_KILLFOCUS:
		keyboard.ClearState();
		break;
	// keyboard msg
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (!(lParam & 0x40000000) || keyboard.AutorepeatIsEnabled())
		{
			keyboard.OnKeyPressed(static_cast<unsigned char>(wParam));
		}
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		keyboard.OnKeyReleased(static_cast<unsigned char>(wParam));
		break;
	case WM_CHAR:
		keyboard.OnChar(static_cast<unsigned char>(wParam));
		break;

	// mouse msg
	// TODO: mouse event
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

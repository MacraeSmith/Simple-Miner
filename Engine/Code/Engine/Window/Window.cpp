#include "Engine/Window/Window.hpp"
#define WIN32_LEAN_AND_MEAN	
#include <windows.h>
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "ThirdParty/imgui/backends/imgui_impl_win32.h"
#include "Engine/ImGui/ImGuiSystem.hpp"

Window* Window::s_mainWindow = nullptr;

//-----------------------------------------------------------------------------------------------
// Handles Windows (Win32) messages/events; i.e. the OS is trying to tell us something happened.
// This function is called back by Windows whenever we tell it to (by calling DispatchMessage).
//
LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND windowHandle, unsigned int wmMessageCode, WPARAM wParam, LPARAM lParam)
{
	InputSystem* input = nullptr;
	if (Window::s_mainWindow != nullptr)
	{
		WindowConfig const& config = Window::s_mainWindow->GetConfig();
		input = config.m_inputSystem;
	}

	if (g_imGuiSystem && g_imGuiSystem->m_isVisible)
	{
		extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		if (ImGui_ImplWin32_WndProcHandler(windowHandle, wmMessageCode, wParam, lParam))
			return true;
	}

	switch (wmMessageCode)
	{
		// App close requested via "X" button, or right-click "Close Window" on task bar, or "Close" from system menu, or Alt-F4
		case WM_CLOSE:
		{
			FireEvent("Quit");
			return 0; // "Consumes" this message (tells Windows "okay, we handled it")
		}
		
		case WM_CHAR:
		{
			EventArgs args;
			args.SetValue("KeyCode",(unsigned char)wParam);
			FireEvent("CharPressed", args);
			return 0;
		}

		// Raw physical keyboard "key-was-just-depressed" event (case-insensitive, not translated)
		case WM_KEYDOWN:
		{
			if (((GetKeyState(VK_CONTROL) & 0x8000) != 0) && (wParam == 'V'))
			{
				FireEvent("PasteRequested");
				return 0;
			}

			EventArgs args;
			args.SetValue("KeyCode", (unsigned char)wParam);
			FireEvent("KeyPressed", args);
			
			return 0;
		}

		// Raw physical keyboard "key-was-just-released" event (case-insensitive, not translated)
		case WM_KEYUP:
		{
			EventArgs args;
			args.SetValue("KeyCode", (unsigned char)wParam);
			FireEvent("KeyReleased", args);
			
			return 0;
		}

		case WM_LBUTTONDOWN:
		{
			EventArgs args;
			args.SetValue("KeyCode", KEYCODE_LEFT_MOUSE_BUTTON);
			FireEvent("KeyPressed", args);
			
			return 0;
		}

		case WM_LBUTTONUP:
		{
			EventArgs args;
			args.SetValue("KeyCode", KEYCODE_LEFT_MOUSE_BUTTON);
			FireEvent("KeyReleased", args);
			
			return 0;
		}

		case WM_RBUTTONDOWN:
		{
			EventArgs args;
			args.SetValue("KeyCode", KEYCODE_RIGHT_MOUSE_BUTTON);
			FireEvent("KeyPressed", args);
			
			return 0;
		}

		case WM_RBUTTONUP:
		{
			EventArgs args;
			args.SetValue("KeyCode", KEYCODE_RIGHT_MOUSE_BUTTON);
			FireEvent("KeyReleased", args);
			
			return 0;
		}

		case WM_MOUSEWHEEL:
		{
			EventArgs args;
			args.SetValue("MouseWheelDelta", (float)GET_WHEEL_DELTA_WPARAM(wParam));
			FireEvent("MouseWheelScrolled", args);
		}
	}

	// Send back to Windows any unhandled/unconsumed messages we want other apps to see (e.g. play/pause in music apps, etc.)
	return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
}


Window::Window(WindowConfig const& config)
	:m_config(config)
{
	s_mainWindow = this;
}

void Window::Startup()
{
	CreateOSWindow();
}

void Window::BeginFrame()
{
	RunMessagePump();
}

void Window::EndFrame()
{
}

void Window::Shutdown()
{
}

WindowConfig const& Window::GetConfig() const
{
	return m_config;
}

void* const& Window::GetDisplayContext() const
{
	return m_displayContext;
}

void Window::SetMouseToCenter()
{
	HWND hwnd = (HWND)Window::s_mainWindow->GetHwnd();
	RECT clientRect;
	POINT centerPos;
	GetClientRect(hwnd, &clientRect);
	centerPos.x = (long)((clientRect.right - clientRect.left) * 0.5f);
	centerPos.y = (long)((clientRect.bottom - clientRect.top) * 0.5f);

	ClientToScreen(hwnd, &centerPos);
	SetCursorPos(centerPos.x, centerPos.y);
}

Vec2 Window::GetNormalizedMouseUV() const
{
	HWND windowHandle = static_cast<HWND>(m_windowHandle);
	POINT cursorCoords;
	RECT clientRect;
	::GetCursorPos(&cursorCoords); //windows screen coords (0,0) is top left
	::ScreenToClient(windowHandle, &cursorCoords); //Get relative to this window's client area
	::GetClientRect(windowHandle, &clientRect); //dimensions of client area (0,0 to width,height)
	float cursorX = (cursorCoords.x) / (float)(clientRect.right);
	float cursorY = (float)(cursorCoords.y) / (float)(clientRect.bottom);
	return Vec2(cursorX, 1.f - cursorY); //flip Y so that we have (0,0) at bottom left
}

void* Window::GetHwnd() const
{
	return m_windowHandle;
}

IntVec2 Window::GetClientDimensions() const
{
	return m_clientDimensions;
}

float Window::GetClientAspect() const
{
	return (float)(m_clientDimensions.x) / (float)(m_clientDimensions.y);;
}

bool Window::IsWindowActive() const
{
	HWND activeWindow = ::GetActiveWindow();
	return activeWindow == m_windowHandle;
}

std::string Window::GetClipboardText()
{
	if (OpenClipboard(nullptr) == FALSE)
	{
		return "";
	}

	HANDLE clipboardDataHandle = GetClipboardData(CF_TEXT);
	if (clipboardDataHandle == nullptr)
	{
		CloseClipboard();
		return "";
	}

	char* clipboardTextPointer = static_cast<char*>(GlobalLock(clipboardDataHandle));
	if (clipboardTextPointer == nullptr)
	{
		CloseClipboard();
		return "";
	}

	std::string clipboardText = clipboardTextPointer;

	GlobalUnlock(clipboardDataHandle);
	CloseClipboard();
	return clipboardText;
}

void Window::RunMessagePump()
{
	MSG queuedMessage;
	for (;; )
	{
		BOOL const wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent)
		{
			break;
		}

		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); // This tells Windows to call our "WindowsMessageHandlingProcedure" (a.k.a. "WinProc") function
	}
}

void Window::CreateOSWindow()
{
	HMODULE applicationInstanceHandle = ::GetModuleHandle(NULL);
	float clientAspect = m_config.m_aspectRatio;
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Define a window style/class
	WNDCLASSEX windowClassDescription;
	memset(&windowClassDescription, 0, sizeof(windowClassDescription));
	windowClassDescription.cbSize = sizeof(windowClassDescription);
	windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
	windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure); // Register our Windows message-handling function
	windowClassDescription.hInstance = GetModuleHandle(NULL);
	windowClassDescription.hIcon = NULL;
	windowClassDescription.hCursor = NULL;
	windowClassDescription.lpszClassName = TEXT("Simple Window Class");
	RegisterClassEx(&windowClassDescription);

	DWORD const windowStyleFlags = m_config.m_fullScreen ? WS_POPUP : WS_CAPTION | WS_BORDER | WS_SYSMENU | WS_OVERLAPPED;
	DWORD const windowStyleExFlags = WS_EX_APPWINDOW;

	// Get desktop rect, dimensions, aspect
	RECT desktopRect;
	HWND desktopWindowHandle = GetDesktopWindow();
	GetClientRect(desktopWindowHandle, &desktopRect);
	float desktopWidth = (float)(desktopRect.right - desktopRect.left);
	float desktopHeight = (float)(desktopRect.bottom - desktopRect.top);
	float desktopAspect = desktopWidth / desktopHeight;

	// Calculate maximum client size (as some % of desktop size)
	const float maxClientFractionOfDesktop = IsFullScreen() ? 1.f : m_config.m_windowScale;
	float clientWidth = desktopWidth * maxClientFractionOfDesktop; 
	float clientHeight = desktopHeight * maxClientFractionOfDesktop;
	if (!IsFullScreen())
	{
		if (clientAspect > desktopAspect)
		{
			// Client window has a wider aspect than desktop; shrink client height to match its width
			clientHeight = clientWidth / clientAspect;
		}
		else
		{
			// Client window has a taller aspect than desktop; shrink client width to match its height
			clientWidth = clientHeight * clientAspect;
		}

		m_clientDimensions = IntVec2((int)clientWidth, (int)clientHeight);
	}

	else
	{
		m_clientDimensions = IntVec2((int)desktopWidth, (int)desktopHeight);
	}

	RECT clientRect;
	// Calculate client rect bounds by centering the client area
	float clientMarginX = 0.5f * (desktopWidth - clientWidth);
	float clientMarginY = 0.5f * (desktopHeight - clientHeight);
	clientRect.left = (int)clientMarginX;
	clientRect.right = clientRect.left + (int)clientWidth;
	clientRect.top = (int)clientMarginY;
	clientRect.bottom = clientRect.top + (int)clientHeight;

	// Calculate the outer dimensions of the physical window, including frame et. al.
	RECT windowRect = clientRect;

	if (IsFullScreen())
	{
		windowRect.left = 0;
		windowRect.top = 0;
		windowRect.right = (int)desktopWidth;
		windowRect.bottom = (int)desktopHeight;
	}

	AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);


	WCHAR windowTitle[1024];
	MultiByteToWideChar(GetACP(), 0, m_config.m_windowTitle.c_str(), -1, windowTitle, sizeof(windowTitle) / sizeof(windowTitle[0]));
	m_windowHandle = CreateWindowEx(
		windowStyleExFlags,
		windowClassDescription.lpszClassName,
		windowTitle,
		windowStyleFlags,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		(HINSTANCE)applicationInstanceHandle,
		NULL);

	if (m_windowHandle == NULL) 
	{
		DWORD error = GetLastError();
		ERROR_AND_DIE(Stringf("CreateWindowEx failed to create the window. ErrorCode: %s", std::to_string(error).c_str()));
	}

	ShowWindow(static_cast<HWND>(m_windowHandle), SW_SHOW);
	SetForegroundWindow(static_cast<HWND>(m_windowHandle));
	SetFocus(static_cast<HWND>(m_windowHandle));

	m_displayContext = GetDC(static_cast<HWND>(m_windowHandle));

	HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
	SetCursor(cursor);
}

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <GLFW/glfw3.h>

#ifndef GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

inline HWND GetMenuWindowHandle(GLFWwindow* window)
{
	return window ? glfwGetWin32Window(window) : nullptr;
}

inline void SetMenuClickThrough(GLFWwindow* window, bool clickThrough)
{
	HWND hwnd = GetMenuWindowHandle(window);
	if (!hwnd)
		return;

	SetLastError(0);
	LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	if (exStyle == 0 && GetLastError() != 0)
		return;

	if (clickThrough)
		exStyle |= WS_EX_TRANSPARENT;
	else
		exStyle &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);

	SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
	SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
}

inline void ApplyMenuInputState(GLFWwindow* window, bool visible)
{
	if (!window)
		return;

	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, visible ? GLFW_FALSE : GLFW_TRUE);
	SetMenuClickThrough(window, !visible);
}

inline void ShowMenu(GLFWwindow* window)
{
	if (!window)
		return;

	ApplyMenuInputState(window, true);
	glfwShowWindow(window);

	HWND hwnd = GetMenuWindowHandle(window);
	if (hwnd) {
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
		BringWindowToTop(hwnd);
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
	}

	glfwFocusWindow(window);
}

inline void HideMenu(GLFWwindow* window)
{
	ApplyMenuInputState(window, false);
}

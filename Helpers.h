#pragma once
#include "iostream"


void ShowMenu(GLFWwindow* window)
{
	if (!window)
		return;

	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
	glfwShowWindow(window);
	glfwFocusWindow(window);
}

void HideMenu(GLFWwindow* window)
{
	if (!window)
		return;

	glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
}

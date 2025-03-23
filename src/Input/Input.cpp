#include "Input.h"

#include "../Lifecycle/Application.h"

#include <GLFW/glfw3.h>

bool Input::isKeyDown(KeyCode keycode) {
	GLFWwindow* windowHandle = Application::get().getWindowHandle();
	int state = glfwGetKey(windowHandle, (int)keycode);
	return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::isMouseButtonDown(MouseButton button) {
	GLFWwindow* windowHandle = Application::get().getWindowHandle();
	int state = glfwGetMouseButton(windowHandle, (int)button);
	return state == GLFW_PRESS;
}

glm::vec2 Input::getMousePosition() {
	GLFWwindow* windowHandle = Application::get().getWindowHandle();

	double x, y;
	glfwGetCursorPos(windowHandle, &x, &y);
	return { (float)x, (float)y };
}

void Input::setCursorMode(CursorMode mode) {
	GLFWwindow* windowHandle = Application::get().getWindowHandle();
	glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
}

float scrollOffsetY = 0.0f;

float Input::getScrollOffsetY() {
	return scrollOffsetY;
}

void Input::resetScrollOffsetY() {
	scrollOffsetY = 0.0f;
}
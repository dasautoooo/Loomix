//
// Created by Leonard Chan on 3/9/25.
//

#include "Camera.h"
#include <iostream>

Camera::Camera()
    : position(glm::vec3(0.0f, 0.0f, 3.0f)), target(glm::vec3(0.0f)),
      up(glm::vec3(0.0f, 1.0f, 0.0f)), yaw(-90.0f), pitch(0.0f), distance(5.0f), fov(45.0f),
      movementSpeed(2.5f), sensitivity(0.1f), firstMouse(true), rightMouseHeld(false), lastX(0.0f),
      lastY(0.0f) {}

glm::mat4 Camera::getViewMatrix() const {
	float x = target.x + distance * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
	float y = target.y + distance * sin(glm::radians(pitch));
	float z = target.z + distance * cos(glm::radians(pitch)) * sin(glm::radians(yaw));
	glm::vec3 eye(x, y, z);
	return glm::lookAt(eye, target, up);
}

void Camera::processKeyboard(float deltaX, float deltaY, float deltaZ) {
	if (!rightMouseHeld)
		return;

	// Calculate current camera forward and right directions
	float yawRad = glm::radians(yaw);
	float pitchRad = glm::radians(pitch);

	glm::vec3 forward;
	forward.x = cos(pitchRad) * cos(yawRad);
	forward.y = sin(pitchRad);
	forward.z = cos(pitchRad) * sin(yawRad);
	forward = glm::normalize(forward);

	glm::vec3 rightVec = glm::normalize(glm::cross(forward, up));

	// Offset the target position (reversed direction)
	target -= deltaZ * forward * movementSpeed * 0.05f;
	target -= deltaX * rightVec * movementSpeed * 0.05f;
	target += deltaY * up * movementSpeed * 0.05f;
}

void Camera::processMouse(float xoffset, float yoffset) {
	if (!rightMouseHeld)
		return; // Rotate only if right mouse is held

	yaw += xoffset * sensitivity;
	pitch += yoffset * sensitivity;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;
}

void Camera::processScroll(float yoffset) {
	distance -= yoffset * 0.1f;
	if (distance < 1.0f)
		distance = 1.0f;
	if (distance > 20.0f)
		distance = 20.0f;
}

void Camera::setRightMouseHeld(bool isHeld) { rightMouseHeld = isHeld; }
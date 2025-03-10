//
// Created by Leonard Chan on 3/9/25.
//

#ifndef CAMERA_H
#define CAMERA_H



#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraMode {
    ORBIT,  // Camera orbits around a target point
    FREE    // Free-fly camera
};

class Camera {
public:
    CameraMode mode;
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;

    float yaw, pitch, distance, fov;
    float movementSpeed, sensitivity;

    bool firstMouse;
    bool rightMouseHeld;
    float lastX, lastY;

    Camera(CameraMode mode = CameraMode::ORBIT);

    glm::mat4 getViewMatrix() const;
    void processKeyboard(float deltaX, float deltaY);
    void processMouse(float xoffset, float yoffset);
    void processScroll(float yoffset);
    void setRightMouseHeld(bool isHeld);
};


#endif //CAMERA_H

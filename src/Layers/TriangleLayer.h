//
// Created by Leonard Chan on 3/10/25.
//

#ifndef TRIANGLELAYER_H
#define TRIANGLELAYER_H

#include "Layer.h"
#include "../Camera.h"
#include "../Utilities/Shader.h"
#include "imgui.h"

class TriangleLayer : public Layer {
public:
    TriangleLayer();

     ~TriangleLayer() override;

    void onUIRender() override;

    void onUpdate(float ts) override;

private:
    uint32_t viewportWidth = 0, viewportHeight = 0;
    uint32_t lastViewportWidth = 0, lastViewportHeight = 0;
    float lastRenderTime = 0.0f;

    GLuint framebuffer, framebufferTexture, rbo;
    GLuint VAO, VBO;

    Shader* shader = nullptr;
    Camera* camera = nullptr;

    void handleCameraInput(float ts);

    void createOrResizeFBO(int width, int height);

    void resizeFramebuffer(int width, int height);

    void setupTriangle();

    void renderToFramebuffer(float ts);
};


#endif //TRIANGLELAYER_H

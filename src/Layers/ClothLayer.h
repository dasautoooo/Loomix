//
// Created by Leonard Chan on 3/11/25.
//

#ifndef CLOTHLAYER_H
#define CLOTHLAYER_H

#include "Layer.h"
#include "../Camera.h"
#include "glad/glad.h"
#include "../Cloth.h"
#include "../Utilities/Shader.h"

class ClothLayer : public Layer {
public:
    ClothLayer();
     ~ClothLayer()override;

    void onUIRender() override;
    void onUpdate(float ts) override;

private:
    // Viewport / Framebuffer
    uint32_t viewportWidth = 0, viewportHeight = 0;
    uint32_t lastViewportWidth = 0, lastViewportHeight = 0;
    float lastRenderTime = 0.0f;

    GLuint framebuffer = 0, framebufferTexture = 0, rbo = 0;

    // Camera & cloth
    Camera* camera = nullptr;
    Cloth* cloth = nullptr;

    // Cloth parameters
    float clothStiffness = 200.0f;
    float clothDamping   = 0.5f;
    float clothMass      = 1.0f;
    int   clothW         = 20;  // cloth grid width
    int   clothH         = 20;  // cloth grid height

    Shader* shader = nullptr;

private:
    void createOrResizeFBO(int width, int height);
    void renderToFramebuffer(float ts);
    void handleCameraInput(float ts);

    // Cloth rendering
    void drawClothWireframe();

    // Helpers
    void setupCloth();
    void cleanupFramebuffer();
};


#endif //CLOTHLAYER_H

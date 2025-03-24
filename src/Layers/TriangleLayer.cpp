//
// Created by Leonard Chan on 3/10/25.
//

#include "TriangleLayer.h"
#include "../Input/Input.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../Utilities/Timer.h"

TriangleLayer::TriangleLayer() : Layer() {
    camera = new Camera();
    camera->distance = 3.0f;
    camera->yaw = -90.0f;
    camera->pitch = 0.0f;
    camera->fov = 45.0f;

    setupTriangle();
    shader = new Shader("simple.vert", "simple.frag");
}

TriangleLayer::~TriangleLayer()  {
    // Cleanup
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &framebufferTexture);
    glDeleteRenderbuffers(1, &rbo);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    shader->deleteShader();
    delete shader;
    delete camera;
}

void TriangleLayer::onUIRender() {
    ImGui::Begin("Settings");
    ImGui::Text("Last render: %.3fms", lastRenderTime);
    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    viewportWidth = ImGui::GetContentRegionAvail().x;
    viewportHeight = ImGui::GetContentRegionAvail().y;

    // If size changed and is nonzero, create/resize FBO
    if ((viewportWidth != lastViewportWidth || viewportHeight != lastViewportHeight) &&
        (viewportWidth > 0 && viewportHeight > 0)) {
        createOrResizeFBO(viewportWidth, viewportHeight);
        lastViewportWidth  = viewportWidth;
        lastViewportHeight = viewportHeight;
        }

    // Display framebuffer texture inside ImGui
    ImGui::Image(
        /* The texture ID: */ ImTextureID((void*)(intptr_t)framebufferTexture),
        /* Size in ImGui space: */ ImVec2((float)viewportWidth, (float)viewportHeight),
        /* UV top-left: */ ImVec2(0, 1),
        /* UV bottom-right: */ ImVec2(1, 0)
    );

    ImGui::End();
    ImGui::PopStyleVar();

}

void TriangleLayer::onUpdate(float ts)  {
    Timer timer;

    handleCameraInput(ts);
    renderToFramebuffer(ts);

    lastRenderTime = timer.elapsedMillis();
}

void TriangleLayer::handleCameraInput(float ts) {
    // 1) Right Mouse Button => camera->rightMouseHeld
    if (Input::isMouseButtonDown(MouseButton::Right)) {
        camera->setRightMouseHeld(true);
        Input::setCursorMode(CursorMode::Locked);
    } else {
        camera->setRightMouseHeld(false);
        Input::setCursorMode(CursorMode::Normal);
    }

    // 2) Mouse Movement => camera->processMouse()
    // Only rotate if RMB is held
    if (camera->rightMouseHeld) {
        glm::vec2 mousePos = Input::getMousePosition();
        if (camera->firstMouse) {
            camera->lastX = mousePos.x;
            camera->lastY = mousePos.y;
            camera->firstMouse = false;
        }
        float xoffset = mousePos.x - camera->lastX;
        float yoffset = camera->lastY - mousePos.y; // reversed y

        camera->lastX = mousePos.x;
        camera->lastY = mousePos.y;

        camera->processMouse(xoffset, yoffset);
    } else {
        // If we release RMB, reset firstMouse
        camera->firstMouse = true;
    }

    // 3) Keyboard WASD => camera->processKeyboard()
    // Only move if RMB is held
	if (camera->rightMouseHeld) {
		float moveSpeed = camera->movementSpeed * ts;
		float dx = 0.0f, dy = 0.0f, dz = 0.0f;
		if (Input::isKeyDown(KeyCode::W)) dz += moveSpeed;
		if (Input::isKeyDown(KeyCode::S)) dz -= moveSpeed;
		if (Input::isKeyDown(KeyCode::A)) dx -= moveSpeed;
		if (Input::isKeyDown(KeyCode::D)) dx += moveSpeed;
		if (Input::isKeyDown(KeyCode::E)) dy += moveSpeed;
		if (Input::isKeyDown(KeyCode::Q)) dy -= moveSpeed;
		camera->processKeyboard(dx, dy, dz);
	}
}

void TriangleLayer::createOrResizeFBO(int width, int height) {
    // If we already have an FBO, reuse it; otherwise create a new one
    if (framebuffer == 0) {
        // Create the FBO
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        // Create texture for color attachment
        glGenTextures(1, &framebufferTexture);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Create renderbuffer for depth/stencil
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    } else {
        // We already have an FBO, so just bind it
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    }

    // (Re)allocate the color texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

    // (Re)allocate the depth/stencil buffer
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // Check status
    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: FBO incomplete, status: " << fboStatus << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TriangleLayer::resizeFramebuffer(int width, int height) {
    if(width == 0 || height == 0) return;  // avoid gl errors if ImGui window is collapsed

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // Resize color texture
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // Resize RBO
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TriangleLayer::setupTriangle() {
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void TriangleLayer::renderToFramebuffer(float ts) {
    // If viewport is 0, skip
    if (viewportWidth == 0 || viewportHeight == 0)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, viewportWidth, viewportHeight);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //-------------------------------
    // Setup camera + projection
    //-------------------------------
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = glm::perspective(
        glm::radians(camera->fov),
        (float)viewportWidth / (float)viewportHeight,
        0.1f,
        100.0f
    );

    // Use simple shader
    shader->use();
    shader->setMat4("uModel", model);
    shader->setMat4("uView", view);
    shader->setMat4("uProjection", projection);

    // Draw the triangle
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

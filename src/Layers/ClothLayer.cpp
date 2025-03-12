//
// Created by Leonard Chan on 3/11/25.
//

#include "ClothLayer.h"

#include "../Utilities/Timer.h"
#include "../Input/Input.h"
#include "imgui.h"

ClothLayer::ClothLayer(){
    // Camera
    camera = new Camera(CameraMode::ORBIT);
    camera->distance  = 3.0f;
    camera->yaw       = -90.0f;
    camera->pitch     = 0.0f;
    camera->fov       = 45.0f;
    camera->movementSpeed = 5.0f;
    camera->sensitivity   = 0.1f;

    // Create cloth
    setupCloth(); // initialize cloth system

    shader = new Shader("simple.vert", "simple.frag");
}

ClothLayer::~ClothLayer(){
    // Cleanup cloth
    delete cloth;
    cloth = nullptr;

    // Cleanup FBO
    cleanupFramebuffer();

    // Cleanup shader
    shader->deleteShader();
    delete shader;
    shader = nullptr;

    // Cleanup camera
    delete camera;
    camera = nullptr;
}

void ClothLayer::onUIRender() {
    // Settings panel
    ImGui::Begin("Settings");
    ImGui::Text("Last render: %.3fms", lastRenderTime);

    // Let user adjust cloth parameters
    if (ImGui::SliderFloat("Stiffness", &clothStiffness, 10.0f, 1000.0f)) {
        cloth->setStiffness(clothStiffness);
    }
    if (ImGui::SliderFloat("Damping", &clothDamping, 0.0f, 2.0f)) {
        cloth->setDamping(clothDamping);
    }
    ImGui::End();

    // Viewport
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");

    viewportWidth  = (uint32_t)ImGui::GetContentRegionAvail().x;
    viewportHeight = (uint32_t)ImGui::GetContentRegionAvail().y;

    // If size changed and is nonzero, create/resize FBO
    if ((viewportWidth != lastViewportWidth || viewportHeight != lastViewportHeight) &&
        (viewportWidth > 0 && viewportHeight > 0))
    {
        createOrResizeFBO(viewportWidth, viewportHeight);
        lastViewportWidth  = viewportWidth;
        lastViewportHeight = viewportHeight;
    }

    // Display framebuffer texture inside ImGui
    ImGui::Image(
        (ImTextureID)(intptr_t)framebufferTexture,
        ImVec2((float)viewportWidth, (float)viewportHeight),
        ImVec2(0, 1),
        ImVec2(1, 0)
    );

    ImGui::End();
    ImGui::PopStyleVar();
}

void ClothLayer::onUpdate(float ts) {
    Timer timer;

    // 1) Handle camera input
    handleCameraInput(ts);

    // 2) Integrate cloth
    cloth->update(ts, glm::vec3(0.0f, -9.81f, 0.0f));

    // Print positions of each particle
    const auto& particles = cloth->getParticles();
    for (size_t i = 0; i < particles.size(); i++) {
        const auto& p = particles[i];
        std::cout << "Particle " << i << ": ("
                  << p.position.x << ", "
                  << p.position.y << ", "
                  << p.position.z << ")\n";
    }

    // 3) Render to the framebuffer
    renderToFramebuffer(ts);

    lastRenderTime = timer.elapsedMillis();
}

void ClothLayer::createOrResizeFBO(int width, int height) {
    // If we already have an FBO, reuse it; otherwise create a new one
    if (framebuffer == 0) {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenTextures(1, &framebufferTexture);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glBindTexture(GL_TEXTURE_2D, framebufferTexture);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    }

    // Allocate color texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

    // Depth-stencil
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "ERROR: FBO incomplete, status: " << fboStatus << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ClothLayer::renderToFramebuffer(float ts) {
    if (viewportWidth == 0 || viewportHeight == 0) return;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, viewportWidth, viewportHeight);

    glEnable(GL_DEPTH_TEST);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup camera & projection
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = glm::perspective(
        glm::radians(camera->fov),
        (float)viewportWidth / (float)viewportHeight,
        0.1f, 100.0f
    );

    // Use simple shader
    shader->use();
    shader->setMat4("uModel", model);
    shader->setMat4("uView", view);
    shader->setMat4("uProjection", projection);

    // Draw cloth as wireframe
    drawClothWireframe();

    // Revert polygon mode if you want
    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ClothLayer::handleCameraInput(float ts) {
    // Right-mouse => camera->rightMouseHeld
    if (Input::isMouseButtonDown(MouseButton::Right)) {
        camera->setRightMouseHeld(true);
        Input::setCursorMode(CursorMode::Locked);
    } else {
        camera->setRightMouseHeld(false);
        Input::setCursorMode(CursorMode::Normal);
    }

    // Mouse Movement => camera->processMouse() if RMB is held
    if (camera->rightMouseHeld) {
        glm::vec2 mousePos = Input::getMousePosition();
        if (camera->firstMouse) {
            camera->lastX = mousePos.x;
            camera->lastY = mousePos.y;
            camera->firstMouse = false;
        }
        float xoffset = mousePos.x - camera->lastX;
        float yoffset = camera->lastY - mousePos.y;

        camera->lastX = mousePos.x;
        camera->lastY = mousePos.y;

        camera->processMouse(xoffset, yoffset);
    } else {
        camera->firstMouse = true;
    }

    // Keyboard movement if free mode + RMB
    if (camera->rightMouseHeld && camera->mode == CameraMode::FREE) {
        float moveSpeed = camera->movementSpeed * ts;
        if (Input::isKeyDown(KeyCode::W)) camera->processKeyboard(0, +moveSpeed);
        if (Input::isKeyDown(KeyCode::S)) camera->processKeyboard(0, -moveSpeed);
        if (Input::isKeyDown(KeyCode::A)) camera->processKeyboard(-moveSpeed, 0);
        if (Input::isKeyDown(KeyCode::D)) camera->processKeyboard(+moveSpeed, 0);
    }
}

void ClothLayer::drawClothWireframe(){
    const auto& particles = cloth->getParticles();
    int clothW = cloth->getClothWidth();
    int clothH = cloth->getClothHeight();

    // We will use glBegin(GL_LINES) to connect neighbors.
    glBegin(GL_LINES);

    // -----------------------
    // 1) Horizontal structural lines
    // -----------------------
    for (int y = 0; y < clothH; y++) {
        for (int x = 0; x < clothW - 1; x++) {
            int i1 = y * clothW + x;
            int i2 = y * clothW + (x + 1);

            glm::vec3 p1 = particles[i1].position;
            glm::vec3 p2 = particles[i2].position;

            glVertex3f(p1.x, p1.y, p1.z);
            glVertex3f(p2.x, p2.y, p2.z);
        }
    }

    // -----------------------
    // 2) Vertical structural lines
    // -----------------------
    for (int y = 0; y < clothH - 1; y++) {
        for (int x = 0; x < clothW; x++) {
            int i1 = y * clothW + x;
            int i2 = (y + 1) * clothW + x;

            glm::vec3 p1 = particles[i1].position;
            glm::vec3 p2 = particles[i2].position;

            glVertex3f(p1.x, p1.y, p1.z);
            glVertex3f(p2.x, p2.y, p2.z);
        }
    }

    // -----------------------
    // 3) Shear lines (diagonals)
    //    Typically within each cell (x, y) => (x+1, y+1) and (x+1, y-1)
    //    For a standard rectangular cloth, we do two diagonals in each cell:
    //       p00 -> p11  and p10 -> p01
    // -----------------------
    for (int y = 0; y < clothH - 1; y++) {
        for (int x = 0; x < clothW - 1; x++) {
            int p00 = y     * clothW + x;
            int p10 = y     * clothW + (x + 1);
            int p01 = (y+1) * clothW + x;
            int p11 = (y+1) * clothW + (x + 1);

            // Shear diagonal #1: p00 -> p11
            glm::vec3 v00 = particles[p00].position;
            glm::vec3 v11 = particles[p11].position;
            glVertex3f(v00.x, v00.y, v00.z);
            glVertex3f(v11.x, v11.y, v11.z);

            // Shear diagonal #2: p10 -> p01
            glm::vec3 v10 = particles[p10].position;
            glm::vec3 v01 = particles[p01].position;
            glVertex3f(v10.x, v10.y, v10.z);
            glVertex3f(v01.x, v01.y, v01.z);
        }
    }

    glEnd(); // end GL_LINES
}

void ClothLayer::setupCloth() {
    // Create the cloth system with default parameters
    cloth = new Cloth(clothW, clothH, 0.1f, clothMass);

    // Pin corners (optional)
    cloth->pinCorners();

    // Set initial stiffness/damping
    cloth->setStiffness(clothStiffness);
    cloth->setDamping(clothDamping);
}

void ClothLayer::cleanupFramebuffer() {
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &framebufferTexture);
    glDeleteRenderbuffers(1, &rbo);
    framebuffer = 0;
    framebufferTexture = 0;
    rbo = 0;
}


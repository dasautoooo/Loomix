//
// Created by Leonard Chan on 3/11/25.
//

#include "ClothLayer.h"

#include "../Input/Input.h"
#include "../Integrators/RK4Integrator.h"
#include "../Utilities/Timer.h"
#include "imgui.h"

ClothLayer::ClothLayer() {
	// Camera
	camera = new Camera();
	camera->distance = 5.0f;
	camera->yaw = 45.0f;
	camera->pitch = 45.0f;
	camera->fov = 45.0f;
	camera->movementSpeed = 5.0f;
	camera->sensitivity = 0.1f;

	// Create cloth
	setupCloth(); // initialize cloth system

	shader = new Shader("simple.vert", "simple.frag");
}

ClothLayer::~ClothLayer() {
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
	ImGui::Begin("Settings");
	ImGui::Text("FPS: %.2f", ImGui::GetIO().Framerate);
	ImGui::Text("Last render: %.3fms", lastRenderTime);
	ImGui::Text("Sim Time: %.2f s", simTime);

	if (ImGui::Button(paused ? "Resume Simulation" : "Pause Simulation")) {
		paused = !paused;
	}

	if (ImGui::Button("Reset Cloth")) {
		simTime = 0.0f;
		delete cloth;
		setupCloth();
	}

	// Add toggle button for input mode
	static bool useSliders = true;
	ImGui::Checkbox("Use Sliders", &useSliders);

	// Integration dt
	if (useSliders) {
		ImGui::SliderFloat("Integration dt", &userDt, 0.0f, 2.0f, "%.4f");
	} else {
		ImGui::InputFloat("Integration dt", &userDt, 0.001f, 0.01f, "%.4f");
	}
	userDt = glm::max(userDt, 0.0f);

	// Particle Mass
	if (useSliders) {
		if (ImGui::SliderFloat("Particle Mass", &clothMass, 0.0f, 10.0f, "%.4f")) {
			cloth->setMass(clothMass);
		}
	} else {
		if (ImGui::InputFloat("Particle Mass", &clothMass, 0.1f, 1.0f, "%.4f")) {
			clothMass = glm::max(clothMass, 0.001f);
			cloth->setMass(clothMass);
		}
	}

	// Structure Springs
	if (useSliders) {
		if (ImGui::SliderFloat("Structure Stiffness", &clothStiffness, 0.0f, 5.0f, "%.4f")) {
			cloth->setStructureSpringConstant(clothStiffness);
		}
		if (ImGui::SliderFloat("Structure Damping", &clothDamping, 0.0f, 2.0f, "%.4f")) {
			cloth->setStructureDamperConstant(clothDamping);
		}
	} else {
		if (ImGui::InputFloat("Structure Stiffness", &clothStiffness, 0.1f, 1.0f, "%.4f")) {
			clothStiffness = glm::max(clothStiffness, 0.0f);
			cloth->setStructureSpringConstant(clothStiffness);
		}
		if (ImGui::InputFloat("Structure Damping", &clothDamping, 0.01f, 0.1f, "%.4f")) {
			clothDamping = glm::max(clothDamping, 0.0f);
			cloth->setStructureDamperConstant(clothDamping);
		}
	}

	// Shear Springs
	if (useSliders) {
		if (ImGui::SliderFloat("Shear Stiffness", &shearStiffness, 0.0f, 5.0f, "%.4f")) {
			cloth->setShearSpringConstant(shearStiffness);
		}
		if (ImGui::SliderFloat("Shear Damping", &shearDamping, 0.0f, 2.0f, "%.4f")) {
			cloth->setShearDamperConstant(shearDamping);
		}
	} else {
		if (ImGui::InputFloat("Shear Stiffness", &shearStiffness, 0.1f, 1.0f, "%.4f")) {
			shearStiffness = glm::max(shearStiffness, 0.0f);
			cloth->setShearSpringConstant(shearStiffness);
		}
		if (ImGui::InputFloat("Shear Damping", &shearDamping, 0.01f, 0.1f, "%.4f")) {
			shearDamping = glm::max(shearDamping, 0.0f);
			cloth->setShearDamperConstant(shearDamping);
		}
	}

	// Bending Springs
	if (useSliders) {
		if (ImGui::SliderFloat("Bending Stiffness", &bendingStiffness, 0.0f, 5.0f, "%.4f")) {
			cloth->setBendingSpringConstant(bendingStiffness);
		}
		if (ImGui::SliderFloat("Bending Damping", &bendingDamping, 0.0f, 2.0f, "%.4f")) {
			cloth->setBendingDamperConstant(bendingDamping);
		}
	} else {
		if (ImGui::InputFloat("Bending Stiffness", &bendingStiffness, 0.1f, 1.0f, "%.4f")) {
			bendingStiffness = glm::max(bendingStiffness, 0.0f);
			cloth->setBendingSpringConstant(bendingStiffness);
		}
		if (ImGui::InputFloat("Bending Damping", &bendingDamping, 0.01f, 0.1f, "%.4f")) {
			bendingDamping = glm::max(bendingDamping, 0.0f);
			cloth->setBendingDamperConstant(bendingDamping);
		}
	}

	// Max Speed
	if (useSliders) {
		if (ImGui::SliderFloat("Max Speed", &maxSpeed, 0.0f, 25.0f, "%.4f")) {
			cloth->setMaxSpeed(maxSpeed);
		}
	} else {
		if (ImGui::InputFloat("Max Speed", &maxSpeed, 0.1f, 1.0f, "%.4f")) {
			maxSpeed = glm::max(maxSpeed, 0.0f);
			cloth->setMaxSpeed(maxSpeed);
		}
	}

	ImGui::Checkbox("Wireframe", &wireframe);

	const char *pinModes[] = {"None", "Four Corners", "Top Corners"};
	if (ImGui::Combo("Pin Mode", &selectedPinMode, pinModes, IM_ARRAYSIZE(pinModes))) {
		pinMode = static_cast<Cloth::PinMode>(selectedPinMode);
		cloth->pinCorners(pinMode);
	}

	const char *integrationMethods[] = {"Explict Euler", "Implicit Euler", "Runge Kutta", "Verlet"};
	if (ImGui::Combo("Integration Methodd", &selectedIntegrator, integrationMethods, IM_ARRAYSIZE(integrationMethods))) {
		integrator = static_cast<Cloth::IntegrationMethod>(selectedIntegrator);
	}

	ImGui::End();

	// Viewport
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Viewport");

	viewportWidth = (uint32_t)ImGui::GetContentRegionAvail().x;
	viewportHeight = (uint32_t)ImGui::GetContentRegionAvail().y;

	// If size changed and is nonzero, create/resize FBO
	if ((viewportWidth != lastViewportWidth || viewportHeight != lastViewportHeight) &&
	    (viewportWidth > 0 && viewportHeight > 0)) {
		createOrResizeFBO(viewportWidth, viewportHeight);
		lastViewportWidth = viewportWidth;
		lastViewportHeight = viewportHeight;
	}

	// Display framebuffer texture inside ImGui
	ImGui::Image((ImTextureID)(intptr_t)framebufferTexture,
	             ImVec2((float)viewportWidth, (float)viewportHeight), ImVec2(0, 1), ImVec2(1, 0));

	ImGui::End();
	ImGui::PopStyleVar();
}

void ClothLayer::onUpdate(float ts) {
	Timer timer;

	// 1) Handle camera input always
	handleCameraInput(ts);

	// 2) If not paused, integrate cloth & accumulate time
	if (!paused) {
		timeAccumulator += ts; // add this frame's real time

		// As long as we have enough accumulated time, do sub-steps
		while (timeAccumulator >= userDt && userDt > 0.0f) {
			cloth->update(userDt);
			simTime += userDt;       // track total sim time
			timeAccumulator -= userDt;
		}
	}

	// 3) Render
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture,
	                       0);

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
	if (viewportWidth == 0 || viewportHeight == 0)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, viewportWidth, viewportHeight);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);

	glClearColor(0.1373f, 0.1373f, 0.1373f, 1.0f);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Setup camera & projection
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view = camera->getViewMatrix();
	glm::mat4 projection = glm::perspective(
	    glm::radians(camera->fov), (float)viewportWidth / (float)viewportHeight, 0.1f, 100.0f);

	// Use simple shader
	shader->use();
	shader->setMat4("uModel", model);
	shader->setMat4("uView", view);
	shader->setMat4("uProjection", projection);

	// Draw cloth as wireframe
	drawClothWireframeVBO();

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

	float scroll = Input::getScrollOffsetY();
	if (scroll != 0.0f) {
		camera->processScroll(scroll);
		Input::resetScrollOffsetY();
	}

	// Keyboard movement if free mode + RMB
	if (camera->rightMouseHeld) {
		float moveSpeed = camera->movementSpeed * ts;
		float dx = 0.0f, dy = 0.0f, dz = 0.0f;
		if (Input::isKeyDown(KeyCode::W))
			dz += moveSpeed;
		if (Input::isKeyDown(KeyCode::S))
			dz -= moveSpeed;
		if (Input::isKeyDown(KeyCode::A))
			dx -= moveSpeed;
		if (Input::isKeyDown(KeyCode::D))
			dx += moveSpeed;
		if (Input::isKeyDown(KeyCode::E))
			dy += moveSpeed;
		if (Input::isKeyDown(KeyCode::Q))
			dy -= moveSpeed;
		camera->processKeyboard(dx, dy, dz);
	}
}

void ClothLayer::drawClothWireframeVBO() {
	int clothW = cloth->getClothWidth();
	int clothH = cloth->getClothHeight();

	const auto &parts = cloth->getParticles(); // Each Particle has pos, velocity, etc.

	// Collect line vertices from positions
	std::vector<glm::vec3> vertices;

	for (int y = 0; y < clothH - 1; y++) {
		for (int x = 0; x < clothW - 1; x++) {
			int p00 = y * clothW + x;
			int p10 = y * clothW + (x + 1);
			int p01 = (y + 1) * clothW + x;
			int p11 = (y + 1) * clothW + (x + 1);

			// First triangle
			vertices.push_back(parts[p00].pos);
			vertices.push_back(parts[p10].pos);
			vertices.push_back(parts[p11].pos);

			// Second triangle
			vertices.push_back(parts[p00].pos);
			vertices.push_back(parts[p11].pos);
			vertices.push_back(parts[p01].pos);
		}
	}

	// Create and bind VAO and VBO
	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(),
	             GL_DYNAMIC_DRAW);

	// Assume the vertex shader uses location 0 for position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
	glEnableVertexAttribArray(0);

	// Draw lines
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());

	// Cleanup: unbind and delete buffers (or keep them for reuse)
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDeleteBuffers(1, &VBO);
	glDeleteVertexArrays(1, &VAO);
}

void ClothLayer::setupCloth() {
	cloth = new Cloth(clothW, clothH, 0.1f);
	cloth->setMass(clothMass);
	cloth->setStructureSpringConstant(clothStiffness);
	cloth->setStructureDamperConstant(clothDamping);
	cloth->setShearSpringConstant(shearStiffness);
	cloth->setShearDamperConstant(shearDamping);
	cloth->setBendingSpringConstant(bendingStiffness);
	cloth->setBendingDamperConstant(bendingDamping);
	cloth->pinCorners(pinMode);
	cloth->setIntegrator(integrator);

	// Calculate cloth center for camera target
	float centerX = (clothW - 1) * 0.1f / 2.0f;
	float centerZ = -(clothH - 1) * 0.1f / 2.0f;
	camera->target = glm::vec3(centerX, 0.0f, centerZ);
}

void ClothLayer::cleanupFramebuffer() {
	glDeleteFramebuffers(1, &framebuffer);
	glDeleteTextures(1, &framebufferTexture);
	glDeleteRenderbuffers(1, &rbo);
	framebuffer = 0;
	framebufferTexture = 0;
	rbo = 0;
}

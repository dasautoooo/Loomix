//
// Created by Leonard Chan on 3/11/25.
//

#ifndef CLOTHLAYER_H
#define CLOTHLAYER_H

#include "../Camera.h"
#include "../Cloth.h"
#include "../Utilities/Shader.h"
#include "Layer.h"
#include "glad/glad.h"

class ClothLayer : public Layer {
  public:
	ClothLayer();
	~ClothLayer() override;

	void onUIRender() override;
	void onUpdate(float ts) override;

  private:
	// Viewport / Framebuffer
	uint32_t viewportWidth = 0, viewportHeight = 0;
	uint32_t lastViewportWidth = 0, lastViewportHeight = 0;
	float lastRenderTime = 0.0f;

	GLuint framebuffer = 0, framebufferTexture = 0, rbo = 0;

	// Camera & cloth
	Camera *camera = nullptr;
	Cloth *cloth = nullptr;

	// Cloth parameters
	float clothStiffness = 3.0f;
	float clothDamping = 0.02f;
	float clothMass = 1.0f;
	int clothW = 20; // cloth grid width
	int clothH = 20; // cloth grid height

	float shearStiffness = 1.0f;
	float shearDamping = 0.01f;
	float bendingStiffness = 0.5f;
	float bendingDamping = 0.005f;

	float maxSpeed = 10.0f;

	int selectedPinMode = static_cast<int>(Cloth::PinMode::TOP_CORNERS);
	Cloth::PinMode pinMode = Cloth::PinMode::TOP_CORNERS;

	int selectedIntegrator = static_cast<int>(Cloth::IntegrationMethod::EXPLICIT_EULER);
	Cloth::IntegrationMethod integrator = Cloth::IntegrationMethod::EXPLICIT_EULER;

	Shader *shader = nullptr;

	bool wireframe = false;

	bool useSliders = true;

	bool paused = false;
	bool pauseOnInstability = false;

	float timeAccumulator = 0.0f;  // accumulates real time
	float userDt = 0.016f; // default to ~60 FPS step

	float simTime = 0.0f;


  private:
	void createOrResizeFBO(int width, int height);
	void renderToFramebuffer(float ts);
	void handleCameraInput(float ts);

	// Cloth rendering
	void drawClothWireframeVBO();

	// Helpers
	void setupCloth();
	void cleanupFramebuffer();
};

#endif // CLOTHLAYER_H

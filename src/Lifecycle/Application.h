//
// Created by Leonard Chan on 3/10/25.
//

#ifndef APPLICATION_H
#define APPLICATION_H

#include <string>
#include <vector>
#include <functional>

#include "../Layers/Layer.h"

static void glfw_error_callback(int error, const char* description);

struct GLFWwindow;

struct ApplicationSpecification {
    std::string name = "Loomix - Cloth Simulation";
    uint32_t width = 1920;
    uint32_t height = 1080;
};

class Application {
public:
    Application(const ApplicationSpecification& applicationSpecification = ApplicationSpecification());
    ~Application();

    static Application& get();

    void run();
    void setMenubarCallback(const std::function<void()>& menubarCallback) { this->menubarCallback = menubarCallback; }

    template<typename T>
    void pushLayer()
    {
        static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
        layerStack.emplace_back(std::make_shared<T>())->onAttach();
    }

    void pushLayer(const std::shared_ptr<Layer>& layer) { layerStack.emplace_back(layer); layer->onAttach(); }

    void close();

    float getTime();
    GLFWwindow* getWindowHandle() const { return windowHandle; }

private:
    void init();
    void shutdown();

private:
    ApplicationSpecification specification;
    GLFWwindow* windowHandle = nullptr;
    bool running = false;

    float timeStep = 0.0f;
    float frameTime = 0.0f;
    float lastFrameTime = 0.0f;

    std::vector<std::shared_ptr<Layer>> layerStack;
    std::function<void()> menubarCallback;

};

Application* createApplication(int argc, char** argv);

#endif //APPLICATION_H

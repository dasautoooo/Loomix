//
// Created by Leonard Chan on 3/10/25.
//

#include "imgui.h"
#include "EntryPoint.h"

class DefaultLayer : public Layer {
public:
    virtual void onUIRender() override {
        ImGui::Begin("Settings");
        ImGui::Text("Last render: %.3fms", lastRenderTime);
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");

        viewportWidth = ImGui::GetContentRegionAvail().x;
        viewportHeight = ImGui::GetContentRegionAvail().y;

        ImGui::End();
        ImGui::PopStyleVar();

    }

private:
    uint32_t viewportWidth = 0, viewportHeight = 0;
    float lastRenderTime = 0.0f;
};

Application* createApplication(int argc, char** argv)
{
    ApplicationSpecification spec;

    Application* app = new Application(spec);
    app->pushLayer<DefaultLayer>();
    app->setMenubarCallback([app]() {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                app->close();
            }
            ImGui::EndMenu();
        }
    });
    return app;
}
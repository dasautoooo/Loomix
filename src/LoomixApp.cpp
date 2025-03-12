//
// Created by Leonard Chan on 3/10/25.
//

#include "imgui.h"
#include "Lifecycle/EntryPoint.h"
#include "Utilities/Shader.h"
#include "Layers/TriangleLayer.h"
#include "Layers/ClothLayer.h"

Application* createApplication(int argc, char** argv)
{
    ApplicationSpecification spec;

    Application* app = new Application(spec);
    // app->pushLayer<TriangleLayer>();
    app->pushLayer<ClothLayer>();
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
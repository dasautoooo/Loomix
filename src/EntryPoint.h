//
// Created by Leonard Chan on 3/10/25.
//

#ifndef ENTRYPOINT_H
#define ENTRYPOINT_H

#include "Application.h"

extern Application* createApplication(int argc, char** argv);
bool applicationRunning = true;

int main(int argc, char** argv) {
    while (applicationRunning) {
        Application* app = createApplication(argc, argv);
        app->run();
        delete app;
    }

    return 0;
}


#endif //ENTRYPOINT_H

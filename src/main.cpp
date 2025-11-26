#include "Core/Application.h"
#include <iostream>

int main() {
    try {
        Application app(1280, 720, "Siatka Glutenowa");
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "BŁĄD KRYTYCZNY: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
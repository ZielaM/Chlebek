#pragma once
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    // ID programu w OpenGL
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();

    void Use();
    
    // Funkcje do wysyłania danych do shadera (Uniforms)
    void SetMat4(const std::string &name, const glm::mat4 &mat) const;
    void SetVec3(const std::string &name, const glm::vec3 &value) const;

private:
    void CheckCompileErrors(unsigned int shader, std::string type);
};
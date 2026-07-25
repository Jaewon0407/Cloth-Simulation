#pragma once
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

class Shader {
public:
    Shader(std::string vertexPath, std::string fragmentPath);
    ~Shader();

    void use() const;
    GLuint id() const { return programId; };

private:
    GLuint programId;

    static std::string readFile(std::string path);
    static GLuint compileShader(GLenum type, const std::string& source);
    void linkShader(GLenum type, GLuint programId);
};



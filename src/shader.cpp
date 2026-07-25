#include "../header/shader.h"

Shader::Shader(std::string vertexPath, std::string fragmentPath) 
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, readFile(vertexPath));
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, readFile(fragmentPath));

    GLuint id = glCreateProgram();
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    
    linkShader(GL_VERTEX_SHADER, id);
    linkShader(GL_FRAGMENT_SHADER, id);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    programId = id;
}

Shader::~Shader()
{
    glDeleteProgram(programId);
}

std::string Shader::readFile(std::string path) 
{
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint Shader::compileShader(GLenum type, const std::string& source) 
{
    const GLchar* src = source.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) 
    {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength, '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::cerr << "Shader compile failed:\n" << log << std::endl;
    }
    return shader;
}

void Shader::linkShader(GLenum type, GLuint programId) 
{
    glLinkProgram(programId);
    GLint success;
    glGetProgramiv(programId, GL_COMPILE_STATUS, &success);
    if (!success) 
    {
        GLint logLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(logLength, '\0');
        glGetProgramInfoLog(programId, logLength, nullptr, log.data());
        std::cerr << "Shader compile failed:\n" << log << std::endl;
    }
}

void Shader::use() const
{
    glUseProgram(programId);
}
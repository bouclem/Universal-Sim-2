#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

namespace usim {

/// Compiles and links a vertex + fragment shader program.
class Shader {
public:
    Shader(const std::string& vertPath, const std::string& fragPath);
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;
    GLuint id() const { return m_program; }

    // Uniform setters
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;
    void setVec3(const std::string& name, const glm::vec3& v) const;
    void setFloat(const std::string& name, float val) const;
    void setInt(const std::string& name, int val) const;

private:
    GLuint m_program = 0;

    static std::string readFile(const std::string& path);
    static GLuint compileShader(GLenum type, const std::string& source);
};

} // namespace usim

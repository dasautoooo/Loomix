//
// Created by Leonard Chan on 3/9/25.
//

#ifndef SHADER_H
#define SHADER_H

#include <fstream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <string>

class Shader {
  public:
	// Shader Program ID
	GLuint ID;

	// Constructor: Reads, compiles, and links the shaders
	Shader(const char *vertexPath, const char *fragmentPath);

	// Activate the shader program
	void use() const;

	// Utility functions to set uniforms
	void setBool(const std::string &name, bool value) const;
	void setInt(const std::string &name, int value) const;
	void setFloat(const std::string &name, float value) const;
	void setVec3(const std::string &name, const glm::vec3 &value) const;
	void setMat4(const std::string &name, const glm::mat4 &value) const;

	// Destructor: Deletes the shader program
	void deleteShader();

  private:
	// Utility function to compile shaders
	GLuint compileShader(const char *code, GLenum type);

	// Utility function to check for shader compilation/linking errors
	void checkCompileErrors(GLuint shader, std::string type);
};

#endif // SHADER_H

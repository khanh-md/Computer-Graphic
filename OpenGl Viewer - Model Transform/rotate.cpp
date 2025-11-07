// template based on material from learnopengl.com
#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
bool loadOBJ(const char* path, std::vector<float>& vertices, std::vector<unsigned int>& indices);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Animation variables
bool animationEnabled = false;
float animationAngle = 0.0f;
float animationSpeed = 0.5f;

// Object positions
glm::vec3 object1Pos(-1.0f, 0.0f, 0.0f);
glm::vec3 object2Pos(1.0f, 0.0f, 0.0f);

// VAOs for two objects
unsigned int VAO1, VBO1, EBO1;
unsigned int VAO2, VBO2, EBO2;
std::vector<float> vertices1, vertices2;
std::vector<unsigned int> indices1, indices2;

std::string readShaderFile(const char* path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Cannot open shader file: " << path << std::endl;
        // Fallback shader sources
        if (strstr(path, ".vs")) {
            return R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vertexColor;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
})";
        } else {
            return R"(#version 330 core
in vec3 vertexColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
})";
        }
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Function to create rotation by aligning axis to z-axis
glm::mat4 rotateAroundAxis(const glm::vec3& axis, float angle) {
    glm::vec3 normalizedAxis = glm::normalize(axis);
    
    // If axis is already aligned with z-axis, just rotate around z
    if (glm::length(normalizedAxis - glm::vec3(0.0f, 0.0f, 1.0f)) < 0.001f) {
        return glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
    }
    
    // Calculate rotation axis and angle to align with z-axis
    glm::vec3 rotationAxis = glm::cross(normalizedAxis, glm::vec3(0.0f, 0.0f, 1.0f));
    float rotationAngle = acos(glm::dot(normalizedAxis, glm::vec3(0.0f, 0.0f, 1.0f)));
    
    // If rotation axis is zero vector (axis is opposite to z), use y-axis as rotation axis
    if (glm::length(rotationAxis) < 0.001f) {
        rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
        rotationAngle = glm::pi<float>();
    } else {
        rotationAxis = glm::normalize(rotationAxis);
    }
    
    // Create the transformation: align to z, rotate, then align back
    glm::mat4 alignToZ = glm::rotate(glm::mat4(1.0f), -rotationAngle, rotationAxis);
    glm::mat4 rotateAroundZ = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 alignBack = glm::rotate(glm::mat4(1.0f), rotationAngle, rotationAxis);
    
    return alignBack * rotateAroundZ * alignToZ;
}

void setupObjectVAO(const std::vector<float>& vertices, const std::vector<unsigned int>& indices, 
                   unsigned int& VAO, unsigned int& VBO, unsigned int& EBO) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Two Objects - Press A for animation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewInit();

    // Read shader files
    std::string vertSource = readShaderFile("source.vs");
    std::string fragSource = readShaderFile("source.fs");
    const char* vertexShaderSource = vertSource.c_str();
    const char* fragmentShaderSource = fragSource.c_str();

    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Load first object
    if (!loadOBJ("data/cube.obj", vertices1, indices1)) {
        std::cout << "Failed to load obj." << std::endl;
    } else {
        std::cout << "Successfully loaded obj" << std::endl;
    }

    // Load second object
        if (!loadOBJ("data/dragon.obj", vertices2, indices2)) {
        std::cout << "Failed to load obj." << std::endl;
    } else {
        std::cout << "Successfully loaded obj" << std::endl;
    }


    // Set up VAOs for both objects
    setupObjectVAO(vertices1, indices1, VAO1, VBO1, EBO1);
    setupObjectVAO(vertices2, indices2, VAO2, VBO2, EBO2);

    glEnable(GL_DEPTH_TEST);


    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create view and projection matrices
        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), 
                                    glm::vec3(0.0f, 0.0f, 0.0f), 
                                    glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        if (animationEnabled) {
            animationAngle += glm::radians(animationSpeed);
        }

        // Calculate the axis between object centers for rotation
        glm::vec3 rotationAxis = glm::normalize(object2Pos - object1Pos);
        
        // Create model matrices for both objects
        glm::mat4 model1 = glm::mat4(1.0f);
        glm::mat4 model2 = glm::mat4(1.0f);

        // Apply animation rotation around the axis connecting object centers
        if (animationEnabled) {
            glm::mat4 rotationMatrix = rotateAroundAxis(rotationAxis, animationAngle);
            
            // Apply rotation to both objects
            model1 = glm::translate(model1, object1Pos) * rotationMatrix;
            model2 = glm::translate(model2, object2Pos) * rotationMatrix;
        } else {
            model1 = glm::translate(model1, object1Pos);
            model2 = glm::translate(model2, object2Pos);
        }

        // GPU Transformation mode
        glUseProgram(shaderProgram);
        
        // Get uniform locations
        unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
        unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
        unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        
        // Pass view and projection matrices (same for both objects)
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Render Object 1
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model1));
        glBindVertexArray(VAO1);
        glDrawElements(GL_TRIANGLES, indices1.size(), GL_UNSIGNED_INT, 0);
        
        // Render Object 2
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model2));
        glBindVertexArray(VAO2);
        glDrawElements(GL_TRIANGLES, indices2.size(), GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO1);
    glDeleteBuffers(1, &VBO1);
    glDeleteBuffers(1, &EBO1);
    glDeleteVertexArrays(1, &VAO2);
    glDeleteBuffers(1, &VBO2);
    glDeleteBuffers(1, &EBO2);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Animation toggle
    static bool aPressed = false;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS && !aPressed) {
        animationEnabled = !animationEnabled;
        std::cout << "Animation: " << (animationEnabled ? "ENABLED" : "DISABLED") << std::endl;
        aPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) {
        aPressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

bool loadOBJ(const char* path, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    std::vector<float> temp_vertices;
    std::vector<unsigned int> vertexIndices;
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Cannot open file: " << path << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string lineHeader;
        iss >> lineHeader;
        
        if (lineHeader == "v") {
            float x, y, z;
            if (!(iss >> x >> y >> z)) {
                continue;
            }
            temp_vertices.push_back(x);
            temp_vertices.push_back(y);
            temp_vertices.push_back(z);
        }
        else if (lineHeader == "f") {
            std::string v1, v2, v3;
            if (!(iss >> v1 >> v2 >> v3)) {
                continue;
            }
            
            // Handle different face formats: "v", "v/vt", "v/vt/vn", "v//vn"
            unsigned int vertexIndex[3];
            
            // Parse first vertex
            std::stringstream ss1(v1);
            std::string token;
            std::getline(ss1, token, '/');
            vertexIndex[0] = std::stoi(token) - 1;
            
            // Parse second vertex  
            std::stringstream ss2(v2);
            std::getline(ss2, token, '/');
            vertexIndex[1] = std::stoi(token) - 1;
            
            // Parse third vertex
            std::stringstream ss3(v3);
            std::getline(ss3, token, '/');
            vertexIndex[2] = std::stoi(token) - 1;
            
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
        }
    }
    file.close();
    
    if (temp_vertices.empty() || vertexIndices.empty()) {
        std::cout << "No valid vertex or face data found in: " << path << std::endl;
        return false;
    }
    
    // Create separate triangles data structure
    // For each face, we'll create separate vertices with position and color
    for (unsigned int i = 0; i < vertexIndices.size(); i++) {
        unsigned int vertexIndex = vertexIndices[i];
        
        if (vertexIndex >= temp_vertices.size() / 3) {
            std::cout << "Invalid vertex index: " << vertexIndex << std::endl;
            continue;
        }
        
        // Position (x, y, z)
        vertices.push_back(temp_vertices[vertexIndex * 3]);
        vertices.push_back(temp_vertices[vertexIndex * 3 + 1]);
        vertices.push_back(temp_vertices[vertexIndex * 3 + 2]);
        
        // Color (based on position for visualization)
        float r = (temp_vertices[vertexIndex * 3] + 1.0f) / 2.0f;
        float g = (temp_vertices[vertexIndex * 3 + 1] + 1.0f) / 2.0f;
        float b = (temp_vertices[vertexIndex * 3 + 2] + 1.0f) / 2.0f;
        vertices.push_back(r);
        vertices.push_back(g);
        vertices.push_back(b);
        
        // For indexed drawing
        indices.push_back(i);
    }
    
    std::cout << "Loaded OBJ file successfully: " << path << std::endl;
    std::cout << "Total vertices: " << vertices.size() / 6 << std::endl;
    std::cout << "Total triangles: " << vertexIndices.size() / 3 << std::endl;
    
    return true;
}
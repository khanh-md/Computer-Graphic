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

// Transformation variables
glm::vec3 translation(0.0f, 0.0f, 0.0f);
glm::vec3 rotation(0.0f, 0.0f, 0.0f);
glm::vec3 scale(1.0f, 1.0f, 1.0f);
float rotationSpeed = 1.0f;
float moveSpeed = 0.005f;
float scaleSpeed = 0.01f;

// 0 = CPU transformation, 1 = GPU transformation
int transformationMode = 1;

std::string readShaderFile(const char* path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Cannot open shader file: " << path << std::endl;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Transformations Demo - Press SPACE to toggle mode", NULL, NULL);
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

    std::vector<float> originalVertices;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Load OBJ file
    if (!loadOBJ("data/cube.obj", originalVertices, indices)) {
        std::cout << "Failed to load obj file." << std::endl;
    } else {
        std::cout << "Successfully loaded obj file" << std::endl;
    }

    vertices = originalVertices;

    unsigned int numVertices = vertices.size() / 6; // Each vertex has 6 floats (3 position, 3 color)

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
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

    // Enable depth testing for 3D
    glEnable(GL_DEPTH_TEST);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Create transformation matrices
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        // Build model matrix from transformations
        model = glm::translate(model, translation);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, scale);

        // Set up view and projection matrices
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
        projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        if (transformationMode == 0) {
            // CPU Transformation: Apply transformations to vertices directly
            vertices = originalVertices;
            for (size_t i = 0; i < vertices.size(); i += 6) {
                glm::vec4 position(vertices[i], vertices[i+1], vertices[i+2], 1.0f);
                glm::vec4 transformed = model * position;
                vertices[i] = transformed.x;
                vertices[i+1] = transformed.y;
                vertices[i+2] = transformed.z;
            }
            
            // Update vertex buffer with transformed vertices
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            
            // Use simple shader without matrix multiplication
            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);
            
            if (indices.empty()) {
                glDrawArrays(GL_TRIANGLES, 0, numVertices);
            } else {
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }
        } else {
            // GPU Transformation: Send matrices to shader
            glUseProgram(shaderProgram);
            
            // Get uniform locations
            unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
            unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
            unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
            
            // Pass matrices to shader
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
            
            glBindVertexArray(VAO);
            if (indices.empty()) {
                glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);
            } else {
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Transformation mode toggle
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !spacePressed) {
        transformationMode = 1 - transformationMode;
        std::cout << "Transformation mode: " << (transformationMode == 0 ? "CPU" : "GPU") << std::endl;
        spacePressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
        spacePressed = false;
    }
    
    // Translation controls
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        translation.y += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        translation.y -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        translation.x -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        translation.x += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        translation.z -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        translation.z += moveSpeed;
    
    // Rotation controls
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        rotation.x += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        rotation.x -= rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        rotation.y += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        rotation.y -= rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        rotation.z += rotationSpeed;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        rotation.z -= rotationSpeed;
    
    // Scaling controls
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        scale += glm::vec3(scaleSpeed);
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        scale -= glm::vec3(scaleSpeed);
    
    // Reset transformations
    static bool rPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !rPressed) {
        translation = glm::vec3(0.0f);
        rotation = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
        std::cout << "Transformations reset" << std::endl;
        rPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
        rPressed = false;
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
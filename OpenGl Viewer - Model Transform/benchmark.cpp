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
#include <chrono>
#include <iomanip>
#include <algorithm>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
bool loadOBJ(const char* path, std::vector<float>& vertices, std::vector<unsigned int>& indices);
void createTestMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, int vertexCount);
void generatePerformanceReport();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Transformation variables
glm::vec3 translation(0.0f, 0.0f, 0.0f);
glm::vec3 rotation(0.0f, 0.0f, 0.0f);
glm::vec3 scale(1.0f, 1.0f, 1.0f);
float rotationSpeed = 1.0f;
float moveSpeed = 0.05f;
float scaleSpeed = 0.1f;

//  0 = CPU transformation, 1 = GPU transformation
int transformationMode = 1;

// Performance testing variables
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
public:
    void start() { startTime = std::chrono::high_resolution_clock::now(); }
    double stop() { 
        auto endTime = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(endTime - startTime).count();
    }
};

struct PerformanceStats {
    double frameTime;
    double transformationTime;
    double drawTime;
    double memoryUsage;
    int fps;
    size_t vertexCount;
    std::string method;
};

std::vector<PerformanceStats> performanceResults;
bool benchmarking = false;
int currentBenchmarkVertexCount = 0;
std::vector<int> benchmarkSizes = {100, 500, 1000, 5000, 10000, 25000};
int benchmarkIndex = 0;
int framesRendered = 0;
const int BENCHMARK_FRAMES = 60; // Measure over 60 frames

std::string readShaderFile(const char* path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Cannot open shader file: " << path << std::endl;
        exit(1);
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

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Transformations Demo - Press B to benchmark, SPACE to toggle mode", NULL, NULL);
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
    
    // Load OBJ file - try multiple possible file names
    const char* objFile = "data/cube.obj";
    bool objLoaded = loadOBJ(objFile, originalVertices, indices);

    if (objLoaded) {
        std::cout << "Successfully loaded obj file: " << objFile << std::endl;
    } else {
        std::cout << "Failed to load obj file: " << std::endl;
    }

    vertices = originalVertices;

    unsigned int numVertices = vertices.size() / 6; // Each vertex has 6 floats (3 position, 3 color)

    unsigned int VBO, VAO, EBO;
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

    // Enable depth testing for 3D
    glEnable(GL_DEPTH_TEST);

    // Performance measurement variables
    PerformanceTimer frameTimer, transformTimer, drawTimer;
    PerformanceStats currentStats;
    double totalFrameTime = 0, totalTransformTime = 0, totalDrawTime = 0;

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // Benchmark setup
        if (benchmarking && benchmarkIndex < benchmarkSizes.size() && framesRendered == 0) {
            // Setup new benchmark mesh
            currentBenchmarkVertexCount = benchmarkSizes[benchmarkIndex];
            createTestMesh(originalVertices, indices, currentBenchmarkVertexCount);
            vertices = originalVertices;
            
            // Update buffers
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
            
            std::cout << "Benchmarking " << currentBenchmarkVertexCount << " vertices (" 
                      << (transformationMode == 0 ? "CPU" : "GPU") << " mode)..." << std::endl;
        }

        frameTimer.start();
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

        transformTimer.start();
        
        if (transformationMode == 0) {
            // CPU Transformation
            vertices = originalVertices;
            for (size_t i = 0; i < vertices.size(); i += 6) {
                glm::vec4 position(vertices[i], vertices[i+1], vertices[i+2], 1.0f);
                glm::vec4 transformed = model * position;
                vertices[i] = transformed.x;
                vertices[i+1] = transformed.y;
                vertices[i+2] = transformed.z;
            }
        }
        
        currentStats.transformationTime = transformTimer.stop();

        drawTimer.start();
        
        if (transformationMode == 0) {
            // CPU Mode: Update VBO and draw
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            
            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);
            
            if (indices.empty()) {
                glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);
            } else {
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }
        } else {
            // GPU Mode: Send matrices to shader
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
        
        currentStats.drawTime = drawTimer.stop();
        currentStats.frameTime = frameTimer.stop();
        currentStats.fps = 1000.0 / currentStats.frameTime;
        currentStats.vertexCount = vertices.size() / 6;

        // Benchmark data collection
        if (benchmarking && benchmarkIndex < benchmarkSizes.size()) {
            totalFrameTime += currentStats.frameTime;
            totalTransformTime += currentStats.transformationTime;
            totalDrawTime += currentStats.drawTime;
            framesRendered++;

            if (framesRendered >= BENCHMARK_FRAMES) {
                // Calculate averages
                currentStats.frameTime = totalFrameTime / BENCHMARK_FRAMES;
                currentStats.transformationTime = totalTransformTime / BENCHMARK_FRAMES;
                currentStats.drawTime = totalDrawTime / BENCHMARK_FRAMES;
                currentStats.fps = 1000.0 / currentStats.frameTime;
                currentStats.method = (transformationMode == 0 ? "CPU" : "GPU");
                currentStats.vertexCount = currentBenchmarkVertexCount;
                
                performanceResults.push_back(currentStats);
                
                std::cout << "Completed: " << currentBenchmarkVertexCount << " vertices - " 
                          << currentStats.method << " - " << currentStats.fps << " FPS" << std::endl;

                // Reset for next test
                totalFrameTime = totalTransformTime = totalDrawTime = 0;
                framesRendered = 0;
                
                // Switch to next test
                if (transformationMode == 0) {
                    // Just completed CPU test, switch to GPU for same size
                    transformationMode = 1;
                    std::cout << "Switching to GPU mode for same mesh size..." << std::endl;
                } else {
                    // Completed both CPU and GPU for this size, move to next size
                    transformationMode = 0;  // Reset to CPU for next size
                    benchmarkIndex++;
                    
                    if (benchmarkIndex < benchmarkSizes.size()) {
                        currentBenchmarkVertexCount = benchmarkSizes[benchmarkIndex];
                        std::cout << "Moving to next mesh size: " << currentBenchmarkVertexCount << " vertices" << std::endl;
                    } else {
                        // All benchmarks completed
                        benchmarking = false;
                        std::cout << "All benchmarks completed! Generating report..." << std::endl;
                        generatePerformanceReport();
                    }
                }
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
    
    // Start benchmark with B key
    static bool bPressed = false;
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bPressed) {
        benchmarking = true;
        benchmarkIndex = 0;
        framesRendered = 0;
        performanceResults.clear();
        transformationMode = 0; // Start with CPU
        std::cout << "Starting performance benchmark..." << std::endl;
        bPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE) {
        bPressed = false;
    }
    
    // Debug benchmark state with D key
    static bool dPressed = false;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS && !dPressed) {
        dPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_RELEASE) {
        dPressed = false;
    }
    
    // Only process transformation controls if not benchmarking
    if (!benchmarking) {
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
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void createTestMesh(std::vector<float>& vertices, std::vector<unsigned int>& indices, int vertexCount) {
    vertices.clear();
    indices.clear();
    
    // Create a simple grid mesh with the specified number of vertices
    int gridSize = (int)sqrt(vertexCount);
    if (gridSize < 2) gridSize = 2;
    
    // Create vertices in a grid
    for (int y = 0; y < gridSize; y++) {
        for (int x = 0; x < gridSize; x++) {
            float xPos = (float)x / (gridSize - 1) * 2.0f - 1.0f;
            float yPos = (float)y / (gridSize - 1) * 2.0f - 1.0f;
            
            // Position
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(0.0f);
            
            // Color (based on position)
            vertices.push_back((xPos + 1.0f) / 2.0f);
            vertices.push_back((yPos + 1.0f) / 2.0f);
            vertices.push_back(0.5f);
        }
    }
    
    // Create indices for triangles
    for (int y = 0; y < gridSize - 1; y++) {
        for (int x = 0; x < gridSize - 1; x++) {
            int topLeft = y * gridSize + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * gridSize + x;
            int bottomRight = bottomLeft + 1;
            
            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    std::cout << "Created test mesh with " << vertices.size()/6 << " vertices and " 
              << indices.size()/3 << " triangles" << std::endl;
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

void generatePerformanceReport() {
    std::cout << "\n\n==========================================" << std::endl;
    std::cout << "        PERFORMANCE TEST REPORT" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "\nDetailed Performance Data:" << std::endl;
    std::cout << "==================================================================================" << std::endl;
    std::cout << "| Vertices | Method | Frame Time | Transform Time | Draw Time | FPS  | Speedup |" << std::endl;
    std::cout << "==================================================================================" << std::endl;
    
    // Group by vertex count and ensure we have both CPU and GPU for each size
    for (int vertexCount : benchmarkSizes) {
        PerformanceStats cpuStats, gpuStats;
        bool hasCPU = false, hasGPU = false;
        
        // Find CPU and GPU results for this vertex count
        for (const auto& stats : performanceResults) {
            if (stats.vertexCount == vertexCount) {
                if (stats.method == "CPU") {
                    cpuStats = stats;
                    hasCPU = true;
                } else if (stats.method == "GPU") {
                    gpuStats = stats;
                    hasGPU = true;
                }
            }
        }
        
        // Only print if we have both CPU and GPU results
        if (hasCPU && hasGPU) {
            double speedup = cpuStats.frameTime / gpuStats.frameTime;
            std::cout << "| " << std::setw(8) << vertexCount 
                      << " | CPU   | " << std::setw(10) << cpuStats.frameTime << "ms"
                      << " | " << std::setw(13) << cpuStats.transformationTime << "ms"
                      << " | " << std::setw(8) << cpuStats.drawTime << "ms"
                      << " | " << std::setw(4) << cpuStats.fps
                      << " | " << std::setw(7) << "---" << " |" << std::endl;
                      
            std::cout << "| " << std::setw(8) << vertexCount 
                      << " | GPU   | " << std::setw(10) << gpuStats.frameTime << "ms"
                      << " | " << std::setw(13) << gpuStats.transformationTime << "ms"
                      << " | " << std::setw(8) << gpuStats.drawTime << "ms"
                      << " | " << std::setw(4) << gpuStats.fps
                      << " | " << std::setw(7) << speedup << "x |" << std::endl;
        } else {
            std::cout << "| " << std::setw(8) << vertexCount 
                      << " | " << (hasCPU ? "CPU" : "GPU") << "   | " << std::setw(10) << "MISSING" << "ms"
                      << " | " << std::setw(13) << "MISSING" << "ms"
                      << " | " << std::setw(8) << "MISSING" << "ms"
                      << " | " << std::setw(4) << "---"
                      << " | " << std::setw(7) << "---" << " |" << std::endl;
        }
    }
    
    // Find crossover point where GPU becomes faster
    int crossoverPoint = -1;
    for (int i = 0; i < benchmarkSizes.size() - 1; i++) {
        int v1 = benchmarkSizes[i];
        int v2 = benchmarkSizes[i+1];
        PerformanceStats cpu1, cpu2, gpu1, gpu2;
        bool hasV1 = false, hasV2 = false;
        
        for (const auto& stats : performanceResults) {
            if (stats.vertexCount == v1) {
                if (stats.method == "CPU") cpu1 = stats;
                if (stats.method == "GPU") gpu1 = stats;
            }
            if (stats.vertexCount == v2) {
                if (stats.method == "CPU") cpu2 = stats;
                if (stats.method == "GPU") gpu2 = stats;
            }
        }
        
        if (cpu1.vertexCount > 0 && gpu1.vertexCount > 0 && 
            cpu2.vertexCount > 0 && gpu2.vertexCount > 0) {
            if (cpu1.frameTime > gpu1.frameTime && cpu2.frameTime > gpu2.frameTime) {
                crossoverPoint = v1;
                break;
            }
        }
    }
    
    // Calculate average speedup across all sizes
    double totalSpeedup = 0.0;
    int speedupCount = 0;
    for (int vertexCount : benchmarkSizes) {
        PerformanceStats cpuStats, gpuStats;
        bool hasCPU = false, hasGPU = false;
        
        for (const auto& stats : performanceResults) {
            if (stats.vertexCount == vertexCount) {
                if (stats.method == "CPU") { cpuStats = stats; hasCPU = true; }
                if (stats.method == "GPU") { gpuStats = stats; hasGPU = true; }
            }
        }
        
        if (hasCPU && hasGPU) {
            totalSpeedup += (cpuStats.frameTime / gpuStats.frameTime);
            speedupCount++;
        }
    }
    
    double averageSpeedup = speedupCount > 0 ? totalSpeedup / speedupCount : 0.0;
    
    // Additional analysis
    std::cout << "\nPerformance:" << std::endl;
    for (int vertexCount : benchmarkSizes) {
        PerformanceStats cpuStats, gpuStats;
        bool hasCPU = false, hasGPU = false;
        
        for (const auto& stats : performanceResults) {
            if (stats.vertexCount == vertexCount) {
                if (stats.method == "CPU") { cpuStats = stats; hasCPU = true; }
                if (stats.method == "GPU") { gpuStats = stats; hasGPU = true; }
            }
        }
        
        if (hasCPU && hasGPU) {
            double speedup = cpuStats.frameTime / gpuStats.frameTime;
            std::cout << "- " << vertexCount << " vertices: CPU=" << cpuStats.frameTime 
                      << "ms, GPU=" << gpuStats.frameTime << "ms, Speedup=" << speedup << "x" << std::endl;
        }
    }
}

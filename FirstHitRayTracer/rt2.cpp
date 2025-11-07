#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <algorithm>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const char* vertexShaderSource = R"glsl(
#version 330 core
layout (location=0) in vec2 aPos;
layout (location=1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D screenTexture;
void main() {
    FragColor = texture(screenTexture, TexCoord);
}
)glsl";

bool isPerspective = true;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        isPerspective = !isPerspective;
        std::cout << "Switched to " << (isPerspective ? "Perspective" : "Orthographic") << " view." << std::endl;
    }
}


struct Vec3 {
    float x,y,z;
    Vec3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}
    Vec3 operator+(const Vec3 &b) const { return Vec3(x+b.x,y+b.y,z+b.z);}
    Vec3 operator-(const Vec3 &b) const { return Vec3(x-b.x,y-b.y,z-b.z);}
    Vec3 operator-() const {
    return Vec3(-x, -y, -z);
    }

    Vec3 operator*(float s) const { return Vec3(x*s,y*s,z*s);}
    Vec3 operator*(const Vec3 &b) const {
        return Vec3(x * b.x, y * b.y, z * b.z);
    }
    float dot(const Vec3 &b) const { return x*b.x + y*b.y + z*b.z;}
    float length() const { return std::sqrt(x*x+y*y+z*z);}
    Vec3 normalize() const { float len=length(); return (*this)*(1.f/len);}
    Vec3 cross(const Vec3 &b) const {
        return Vec3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
    }
};
inline Vec3 operator*(float s, const Vec3& v) {
    return v * s;  // Calls Vec3::operator*(float)
}

struct Ray {
    Vec3 origin;
    Vec3 direction; // normalized
};

struct Sphere {
    Vec3 center;
    float radius;
    uint8_t r,g,b;
};
struct Triangle {
    Vec3 v0,v1,v2;
    uint8_t r,g,b;
};

struct Plane {
    Vec3 point; 
    Vec3 normal; 
    uint8_t r,g,b;
};

struct HitInfo {
    float t;
    Vec3 position;
    Vec3 normal;
    uint8_t r,g,b;
};

struct Light {
    Vec3 position;
    Vec3 color;
};

// Ray-object intersection 
bool intersectSphere(const Ray &ray, const Sphere &sph, float &t) {
    Vec3 oc=ray.origin - sph.center;
    float a=1.0f; 
    float b=2.0f*ray.direction.dot(oc);
    float c=oc.dot(oc)-sph.radius*sph.radius;
    float disc=b*b -4*a*c;
    if(disc<0) return false;
    float sq=sqrtf(disc);
    float t0=(-b - sq)/(2*a);
    float t1=(-b + sq)/(2*a);
    if(t0 > 0.001f) { t=t0; return true;}
    if(t1 > 0.001f) { t=t1; return true;}
    return false;
}
bool intersectTriangle(const Ray &ray, const Triangle &tri, float &t) {
    const float EPSILON = 1e-7f;
    Vec3 edge1 = tri.v1 - tri.v0;
    Vec3 edge2 = tri.v2 - tri.v0;
    Vec3 h = ray.direction.cross(edge2);
    float a = edge1.dot(h);
    if (std::abs(a) < EPSILON) return false; // parallel
    float f = 1.0f / a;
    Vec3 s = ray.origin - tri.v0;
    float u = f * s.dot(h);
    if (u < 0.0f || u > 1.0f) return false;
    Vec3 q = s.cross(edge1);
    float v = f * ray.direction.dot(q);
    if (v < 0.0f || u + v > 1.0f) return false;
    float tempT = f * edge2.dot(q);
    if (tempT > EPSILON) {
        t = tempT;
        return true;
    }
    return false;
}

bool intersectPlane(const Ray& ray, const Plane& plane, float& t) {
    float denom = ray.direction.dot(plane.normal);
    if (fabs(denom) < 1e-6f) return false; // parallel
    float num = (plane.point - ray.origin).dot(plane.normal);
    t = num / denom;
    return (t > 0.001f);
}

Vec3 getSphereNormal(const Sphere &s, const Vec3 &point) {
    return (point - s.center).normalize();
}

Vec3 getTriangleNormal(const Triangle &tri) {
    return (tri.v1 - tri.v0).cross(tri.v2 - tri.v0).normalize();
}

Vec3 reflect(const Vec3& I, const Vec3& N) {
    return I - 2.0f * (I.dot(N)) * N;
}


Light light = { Vec3(2.f,5.f,5.f), Vec3(1.f,1.f,1.f) };

Vec3 shade(const HitInfo &hit, const Ray &ray, const Light &light, const std::vector<Sphere> &spheres, const std::vector<Triangle> &triangles) {
    Vec3 ambientColor(0.1f, 0.1f, 0.1f);
    Vec3 objectColor(hit.r / 255.f, hit.g / 255.f, hit.b / 255.f);

    Vec3 lightDir = (light.position - hit.position).normalize();

    Ray shadowRay;
    shadowRay.origin = hit.position + hit.normal * 0.001f;
    shadowRay.direction = lightDir;

    float distToLight = (light.position - hit.position).length();
    bool inShadow = false;

    for (const auto& s : spheres) {
        float t;
        if (intersectSphere(shadowRay, s, t)) {
            if (t < distToLight) {
                inShadow = true;
                break;
            }
        }
    }

    if (!inShadow) {
        for (const auto &tri : triangles) {
            float t;
            if (intersectTriangle(shadowRay, tri, t)) {
                if (t < distToLight) {
                    inShadow = true;
                    break;
                }
            }
        }
    }

    if(inShadow) {
        return objectColor * ambientColor;
    }

    float diff = std::max(hit.normal.dot(lightDir), 0.0f);
    Vec3 diffuse = objectColor * light.color * diff * 0.7f;

    Vec3 viewDir = (ray.origin - hit.position).normalize();
    Vec3 reflectDir = (2.0f * hit.normal.dot(lightDir) * hit.normal - lightDir).normalize();
    float spec = std::pow(std::max(viewDir.dot(reflectDir), 0.0f), 32);
    Vec3 specular = light.color * spec * 0.2f;

    Vec3 color = objectColor * ambientColor + diffuse + specular;
    color.x = std::min(color.x, 1.f);
    color.y = std::min(color.y, 1.f);
    color.z = std::min(color.z, 1.f);

    return color;
}

Vec3 trace(const Ray& ray, const std::vector<Sphere>& spheres, const std::vector<Triangle>& triangles, const Plane& plane, const Light& light, int depth = 0)
{
    if (depth > 2) return Vec3(0.1f,0.1f,0.1f); // recursion limit

    HitInfo closestHit; 
    closestHit.t = 1e20f; 
    bool hitSomething = false;

    for (const auto& s: spheres) {
        float t;
        if (intersectSphere(ray,s,t) && t < closestHit.t) {
            closestHit.t = t;
            closestHit.position = ray.origin + ray.direction * t;
            closestHit.normal = (closestHit.position - s.center).normalize();
            closestHit.r = s.r; closestHit.g=s.g; closestHit.b=s.b;
            hitSomething = true;
        }
    }

    for (const auto& tri: triangles) {
        float t;
        if (intersectTriangle(ray,tri,t) && t < closestHit.t) {
            closestHit.t = t;
            closestHit.position = ray.origin + ray.direction * t;
            closestHit.normal = getTriangleNormal(tri);
            closestHit.r = tri.r; closestHit.g=tri.g; closestHit.b=tri.b;
            hitSomething = true;
        }
    }

    float tPlane;
    if (intersectPlane(ray, plane, tPlane) && tPlane < closestHit.t) {
        closestHit.t = tPlane;
        closestHit.position = ray.origin + ray.direction * tPlane;
        closestHit.normal = plane.normal;
        closestHit.r = plane.r; closestHit.g=plane.g; closestHit.b=plane.b;
        hitSomething = true;

        // Reflection for glaze
        Vec3 reflectDir = reflect(ray.direction, closestHit.normal).normalize();
        Ray reflectRay = {closestHit.position + closestHit.normal * 0.001f, reflectDir};
        Vec3 reflectedColor = trace(reflectRay, spheres, triangles, plane, light, depth+1);

        Vec3 baseColor(plane.r/255.f, plane.g/255.f, plane.b/255.f);
        return 0.3f * baseColor + 0.7f * reflectedColor;
    }

    if (hitSomething) {
        return shade(closestHit, ray, light, spheres, triangles);
    }

    return Vec3(0.1f,0.1f,0.1f); // background
}


void framebuffer_size_callback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window){
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window,true);
}

int main(){
    // Initialize GLFW
    if(!glfwInit()){
        std::cerr << "Failed to init GLFW\n"; return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif

    GLFWwindow* window=glfwCreateWindow(600,600,"First Hit Ray Tracer",NULL,NULL);
    if(!window){
        std::cerr << "Failed to create window\n"; glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // Init GLEW
    glewExperimental=GL_TRUE;
    if(glewInit() != GLEW_OK){
        std::cerr << "Failed to init GLEW\n"; glfwTerminate(); return -1;
    }

    // Compile shaders
    auto compileShader=[](GLenum type,const char* source){
        GLuint shader=glCreateShader(type);
        glShaderSource(shader,1,&source,nullptr);
        glCompileShader(shader);
        int success; char infoLog[512];
        glGetShaderiv(shader,GL_COMPILE_STATUS,&success);
        if(!success){
            glGetShaderInfoLog(shader,512,nullptr,infoLog);
            std::cerr << "Shader failed compilation:\n" << infoLog << "\n";
        }
        return shader;
    };

    GLuint vertShader=compileShader(GL_VERTEX_SHADER,vertexShaderSource);
    GLuint fragShader=compileShader(GL_FRAGMENT_SHADER,fragmentShaderSource);

    GLuint program=glCreateProgram();
    glAttachShader(program,vertShader);
    glAttachShader(program,fragShader);
    glLinkProgram(program);
    int success; char infoLog[512];
    glGetProgramiv(program,GL_LINK_STATUS,&success);
    if(!success){
        glGetProgramInfoLog(program,512,nullptr,infoLog);
        std::cerr << "Program linking failed:\n" << infoLog << "\n";
    }
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    float vertices[]={
        // positions // Texture Coords
        -1.f,  1.f, 0.f, 1.f,
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 1.f,
         1.f, -1.f, 1.f, 0.f,
         1.f,  1.f, 1.f, 1.f
    };

    GLuint VAO,VBO;
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    // Positions
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    // TexCoords
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    const int WIDTH=600, HEIGHT=600;
    std::vector<uint8_t> image(WIDTH*HEIGHT*3);

    std::vector<Sphere> spheres={
        {{-0.5f,0.f,0.f},0.4f,0,0,255}, // Blue sphere
        {{ 0.5f,0.f,0.f},0.3f,0,255,0}  // Green sphere
    };
    Vec3 tetraVerts[4] = {
        {1.5f, 0.5f, 0.f},
        {1.0f, -0.5f, 0.5f},
        {2.0f, -0.5f, 0.5f},
        {1.5f, -0.5f, -0.5f}
    };
    std::vector<Triangle> tetrahedron = {
        {tetraVerts[0], tetraVerts[1], tetraVerts[2], 255,0,255},
        {tetraVerts[0], tetraVerts[2], tetraVerts[3], 255,0,255},
        {tetraVerts[0], tetraVerts[3], tetraVerts[1], 255,0,255},
        {tetraVerts[1], tetraVerts[3], tetraVerts[2], 255,0,255}
    };
    Plane floor = {{0,-0.6f,0}, {0,1,0}, 200,200,200}; 

    std::vector<Triangle> triangles = tetrahedron;

    float radius = 4.f;
    float angle = 0.f;
    float lastTime = glfwGetTime();
    //int frameNumber = 0;
    glUseProgram(program);

    GLuint texID;
    glGenTextures(1,&texID);

    while(!glfwWindowShouldClose(window)){
        // Time management for rotation
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        angle += deltaTime * 0.5f; // rotation

        // Camera position around Y axis
        Vec3 camPos = {radius * std::sin(angle), 1.f, radius * std::cos(angle)};
        Vec3 lookAt = {0.f, 0.f, 0.f};
        Vec3 camDir = (lookAt - camPos).normalize();

        // Orthonormal basis for camera plane
        Vec3 worldUp = {0.f,1.f,0.f};
        Vec3 camRight = camDir.cross(worldUp).normalize();
        Vec3 camUp = camRight.cross(camDir);

        float fov = 60.0f; // degrees
        float aspect = float(WIDTH) / float(HEIGHT);
        float perspectiveScale = tanf((fov * 0.5f) * (M_PI / 180.0f));
        float orthoScale = 2.0f; // Controls the "zoom" of the orthographic view

        spheres[0].center.y = 0.5f * std::sin(currentTime);
        spheres[1].center.y = 0.5f * std::sin(currentTime + 3.1415f);

        // Generate rays per pixel
        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                float ndcX = ((x + 0.5f) / WIDTH) * 2.f - 1.f;
                float ndcY = ((y + 0.5f) / HEIGHT) * 2.f - 1.f;

                Ray ray;
                if (isPerspective) {
                    float px = ndcX * aspect * perspectiveScale;
                    float py = ndcY * perspectiveScale;
                    Vec3 rayDir = (camRight * px + camUp * py + camDir).normalize();
                    ray = {camPos, rayDir};
                } else {
                    float px = ndcX * aspect * orthoScale;
                    float py = ndcY * orthoScale;
                    Vec3 rayOrigin = camPos + camRight * px + camUp * py;
                    ray = {rayOrigin, camDir}; // Ray direction is always forward
                }

                Vec3 col = trace(ray, spheres, triangles, floor, light, 0);

                // Clamp and write to image buffer
                int idx = 3 * (y * WIDTH + x);
                image[idx]   = std::min(255, int(std::max(0.f, col.x) * 255));
                image[idx+1] = std::min(255, int(std::max(0.f, col.y) * 255));
                image[idx+2] = std::min(255, int(std::max(0.f, col.z) * 255));
            }
        }
        // Update texture
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,WIDTH,HEIGHT,0,GL_RGB,GL_UNSIGNED_BYTE,image.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

        glClearColor(0.2f,0.3f,0.3f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES,0,6);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
        processInput(window);

        // Generate render images
        // char filename[256];
        // snprintf(filename, sizeof(filename), "frames/frame_%04d.png", frameNumber++);
        // stbi_write_png(filename, WIDTH, HEIGHT, 3, image.data(), WIDTH * 3);
    }

    // Cleanup
    glDeleteVertexArrays(1,&VAO);
    glDeleteBuffers(1,&VBO);
    glDeleteTextures(1,&texID);
    glDeleteProgram(program);

    glfwTerminate();
    return 0;
}


#pragma once
#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
//#include "Headers/stb_image.h"
#include "stb_image.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Particle.h"
#include "Mesh.h"
#include "Shader_s.h"

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

#include <random>

using namespace std;

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

struct Edge{
    Vertex_H *v0;
    Vertex_H *v1;
    float lenght;
};

class Model {
public:
    /* Model data */
    vector<Texture_H> textures_loaded; // An array of textures that have been loaded.
    vector<Mesh> meshes; //Mesh指的并非 图元三角形，而是整个模型下的一个子模型。例如，房子有：烟囱、窗户、大门、屋顶等。每一个都属于一个scene的子Mesh 
    string directory;
    bool gammaCorrection;

    // 模拟用
    std::vector<Edge> edgeList;
    // bool handleCollision = true; // 是否处理碰撞

    string name;


    Model(string const& path, bool gamma = false) : gammaCorrection(gamma) {
        //stbi_set_flip_vertically_on_load(true);
        loadModel(path);
        this->name = path;
    }
    void Draw(Shader& shader) {
        for (unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shader);
    }
    // for change model
    void cleanup() {
        for (const Texture_H& texture : textures_loaded) {
            glDeleteTextures(1, &texture.id);
        }
        for (Mesh& mesh : meshes) {
            mesh.cleanup();
        }
    }

    void simulate(float deltatime) {
        glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);
        float edgeCompliance = 100.0f;
        float volumeCompliance = 0.0f;

        // pre-solve
        for(unsigned int i = 0; i < meshes.size(); i++){
            for(unsigned int j = 0; j < meshes[i].vertices.size(); j++){
                Vertex_H &particle = meshes[i].vertices[j];

                particle.OldPosition = particle.Position;
                particle.Velocity = gravity * deltatime;
                particle.Position += particle.Velocity * deltatime;
                if(particle.Position.y < 0.0f){
                    particle.Position = particle.OldPosition;
                    particle.Position.y = 0.0f;
                }
            }
        }

        // solve
        // TODO
        // solve edge
        float alpha = edgeCompliance / deltatime / deltatime;
        glm::vec3 grads = glm::vec3(0.0f, 0.0f, 0.0f);
        for(unsigned int i = 0; i < edgeList.size(); i++){
            Vertex_H* particle0 = edgeList[i].v0;
            Vertex_H* particle1 = edgeList[i].v1;
            float restLenght = edgeList[i].lenght;
            float w0 = 1 / particle0->mass;
            float w1 = 1 / particle0->mass;

            grads = particle0->Position - particle1->Position;
            glm::vec3 length = glm::normalize(grads);
            float currentLength = glm::length(grads);
            grads = grads / length;
            float Constrain = currentLength - restLenght;
            float w = w0 + w1;
            float s = -Constrain / (w + alpha);

            particle0->Position += (grads * s * w0);
            particle1->Position += (-grads * s * w1); //相反方向运动
        }

        // solve volume
    


        // post-solve
        for(unsigned int i = 0; i < meshes.size(); i++){
            for(unsigned int j = 0; j < meshes[i].vertices.size(); j++){
                Vertex_H &particle = meshes[i].vertices[j];
                particle.Velocity = particle.Position - particle.OldPosition;
            }
        }

        // 碰撞检测
        
    }


private:
    /* functions */
    void loadModel(string const& path) {
        Assimp::Importer importer;
        /* ReadFile: 1. path��
                     2.��Post-processing��:
                     - aiProcess_Triangulate: If the model is not composed of triangles, then convert all primitives into triangles
                     - aiProcess_FlipUVs: OpenGL texture is inverted compared to Y axis
                     - aiProcess_GenNormals: If the model does not contain a normal, create a normal for each vertex
                     - aiProcess_SplitLargeMeshes: Splitting a larger Mesh into smaller sub-Meshes is very useful if the rendering has a maximum vertex limit and can only render smaller Meshes.
                     - aiProcess_OptimizeMeshes: Combine multiple small meshes into one large mesh to reduce draw calls and optimize
        */
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));

        // DFS Recursively traverse all nodes. The nodes of the model exist in a tree structure (such as a car model)
        processNode(scene->mRootNode, scene);
    }

    /* DFS

    RootNode
        ������ Node_1 (mNumMeshes = 1, mNumChildren = 2)
        ��   ������ Node_1_1 (mNumMeshes = 1, mNumChildren = 0)
        ��   ������ Node_1_2 (mNumMeshes = 0, mNumChildren = 0)
        ������ Node_2 (mNumMeshes = 2, mNumChildren = 1)
            ������ Node_2_1 (mNumMeshes = 1, mNumChildren = 0)
    */
    void processNode(aiNode* node, const aiScene* scene) {

        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            // save mesh data
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    // Assimp To Mesh
    /*  3 things:
            1. Get vertex data
            2. Get the Mesh index
            3. Get material data
    */
    
    Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
        vector<Vertex_H> vertices;
        vector<unsigned int> indices;
        vector<Texture_H> textures;

        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex_H vertex;
            glm::vec3 vector; // Assimp has its own set of data types for vectors, matrices, and strings, 
            // which must be converted through a vec3

            vertex.index = i; // for simulation, index

            // Position
            vector.x = mesh->mVertices[i].x;
            vector.y = mesh->mVertices[i].y;
            vector.z = mesh->mVertices[i].z;

            vertex.Position = vector;

            // for simulation
            
            vertex.Velocity = glm::vec3(0.0f, 0.0f, 0.0f);
            vertex.Acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
            vertex.OldVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
            vertex.mass = 1.0f;
            vertex.OldPosition = vertex.Position;
          
            // Normal
            if (mesh->HasNormals()) {
                vector.x = mesh->mNormals[i].x;
                vector.y = mesh->mNormals[i].y;
                vector.z = mesh->mNormals[i].z;
                vertex.Normal = vector;
            }

            // TexCoords
            // Assimp allows the model to have up to 8 different texture coordinates on a vertex, 
            // but we generally only use one, so we only look at 0
            //                      ��
            if (mesh->mTextureCoords[0]) {
                glm::vec2 vec;

                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;
                // tangent
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // bitangent
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // A face is composed of multiple triangles 
        // (if there are quads or polygons, they are usually cut into triangles),
        // we need to load the drawing order
        // Then use glDrawElements to drawing
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];

            if(face.mNumIndices != 3) continue; // 不是三角图元！

            unsigned int i0 = face.mIndices[0];
            unsigned int i1 = face.mIndices[1];
            unsigned int i2 = face.mIndices[2];

            // 添加边 （边为两个指针 + 边的长度）
            float l1 = glm::length(vertices[i0].Position - vertices[i1].Position);
            float l2 = glm::length(vertices[i1].Position - vertices[i2].Position);
            float l3 = glm::length(vertices[i2].Position - vertices[i0].Position);

            //std::cout << l1 << ", " << l2 << ", " << l3 << std::endl;

            edgeList.push_back({ &vertices[i0], &vertices[i1], l1});
            edgeList.push_back({ &vertices[i1], &vertices[i2], l2});
            edgeList.push_back({ &vertices[i2], &vertices[i1], l3});

            
            // edgeList.push_back({ &vertices[i0], &vertices[i1], glm::length(vertices[i0].Position - vertices[i1].Position)});
            // edgeList.push_back({ &vertices[i1], &vertices[i2], glm::length(vertices[i1].Position - vertices[i2].Position)});
            // edgeList.push_back({ &vertices[i2], &vertices[i1], glm::length(vertices[i2].Position - vertices[i0].Position)});

            /* 
            indices.push_back(face.mIndices[i0]);
            indices.push_back(face.mIndices[i1]);
            indices.push_back(face.mIndices[i2]);
            */
            
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
            
        }

        // ------------模拟代码--------------------


        // material: diffuse, specular, normal, height
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. diffuse maps
        vector<Texture_H> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        // 2. specular maps
        vector<Texture_H> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<Texture_H> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<Texture_H> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

        // return a mesh object created from the extracted mesh data
        return Mesh(vertices, indices, textures);
    }
    
    vector<Texture_H> loadMaterialTextures(aiMaterial* mat, aiTextureType type, string typeName) {
        vector<Texture_H> textures;
        // GetTextureCount: Checks whether a certain type of texture exists, and if so, returns the number of existing textures.
        for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            // Get the i-th texture position and store it in an aiString
            mat->GetTexture(type, i, &str);

            std::cout << "str: " << str.C_Str() << std::endl;
            // Skip flag is used to check whether a texture has been loaded.
            bool skip = false;
            // Traverse the array of textures that have been loaded. 
            // If the texture being loaded exists in the array, 
            // get the texture from the array directly through the index
            for (unsigned int j = 0; j < textures_loaded.size(); j++) {
                // strcmp Compares two strings
                if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    break;
                }
            }
            // If the texture has not been loaded, then load it
            if (!skip) {
                Texture_H texture;
                texture.id = TextureFromFile(str.C_Str(), this->directory);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        return textures;
    }

    int hashPos(glm::vec3 pos){
        float x = pos.x;
        float y = pos.y;
        float z = pos.z;
        int64_t hash = (int64_t(x) * 92837111) ^ (int64_t(y) * 689287499) ^ (int64_t(z) * 283923481);
        
        return static_cast<int>(std::abs(hash));
    }
};

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma) {
    string filename = string(path);
    filename = directory + '/' + filename;

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RED;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else {
        std::cout << "Texture failed to load at path: " << filename << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

#endif
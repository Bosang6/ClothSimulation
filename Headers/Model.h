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
#include <set>

#include <random>

using namespace std;

unsigned int TextureFromFile(const char* path, const string& directory, bool gamma = false);

struct Edge{
    Vertex_H *v0;
    Vertex_H *v1;
    float lenght;
    int triangleIndex; // for simulation, index
    int triangleIndex2; // for simulation, index
};



class Model {
public:
    /* Model data */
    vector<Texture_H> textures_loaded; // An array of textures that have been loaded.
    vector<Mesh> meshes; //Mesh指的并非 图元三角形，而是整个模型下的一个子模型。例如，房子有：烟囱、窗户、大门、屋顶等。每一个都属于一个scene的子Mesh 
    string directory;
    bool gammaCorrection;

    // 模拟用
    std::vector<EdgeIndex> edgeIndices; // 边索引
    std::vector<Triangle> triangles; // 三角形索引

    std::vector<Edge> edgeList;
    std::vector<Edge> bendingEdges; // 绑定边
    std::map<int, std::vector<Vertex_H*>> triangleVertices; // 三角形顶点索引
    std::map<std::pair<int, int>, int> edgeToTriangle; // 边 -> 三角形索引
    bool handleCollision = true; // 是否处理碰撞

    int vertexLoaded = 0; // 顶点数量

    string name;

    // 检测一个三角形是否bending了三个不同的三角形
    std::vector<int> bendingTriangles; // 用于存储bending的三角形索引
    std::map<std::pair<int, int>, std::vector<int>> edgeWithVertex; // 边 -> bending三角形索引
    std::vector<Vertex_H*> allParticles; // 所有粒子
    std::vector<Vertex_H*> unionParticles; // 静态粒子（不可移动，如衣架）


    Model(string const& path, int vertexCount,bool gamma = false) : gammaCorrection(gamma) {
        //stbi_set_flip_vertically_on_load(true);
        this->vertexLoaded = vertexCount;
        loadModel(path);
        this->name = path;
        //bendEdges(); // 5400
        //bendEdges2(); // 5400
        bendEdges3(); //40266
        std::cout << "bending edges: " << bendingEdges.size() << std::endl;
        std::cout << "edge list size: " << edgeList.size() << std::endl;
        //bendingToEdges();
        // std::cout << "bending edges: " << bendingEdges.size() << std::endl;
        // std::cout << "edge list size: " << edgeList.size() << std::endl;
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
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            cout << "ERROR::ASSIMP::" << importer.GetErrorString() << endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));

        // DFS Recursively traverse all nodes. The nodes of the model exist in a tree structure (such as a car model)
        processNode(scene->mRootNode, scene);
    
        for (Mesh& mesh : meshes) {
            for (auto& e : mesh.tempEdgeList) {
                Vertex_H* v0 = &mesh.vertices[e.i0];
                Vertex_H* v1 = &mesh.vertices[e.i1];
                edgeList.push_back({ v0, v1, e.length, e.triangleIndex });
            }
            for(Triangle& t : mesh.triangles){
                int i = t.index;
                int i0 = t.i0;
                int i1 = t.i1;
                int i2 = t.i2;
                triangleVertices[i].push_back(&mesh.vertices[i0]);
                triangleVertices[i].push_back(&mesh.vertices[i1]);
                triangleVertices[i].push_back(&mesh.vertices[i2]);
            }
            for(auto& v : mesh.vertices) {
                allParticles.push_back(&v);
            }
        }

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

            vertex.index = i + vertexLoaded; // for simulation, index

            if(vertexLoaded == 0){
                vertex.modelIndex = 0;
            }
            else{
                vertex.modelIndex = 1;
            }

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
            vertex.initPosition = vertex.Position; // 初始位置
          
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
        for (int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];

            if(face.mNumIndices != 3) continue; // 不是三角图元！

            int i0 = face.mIndices[0];
            int i1 = face.mIndices[1];
            int i2 = face.mIndices[2];

            // 添加边 （边为两个指针 + 边的长度）
            float l1 = glm::length(vertices[i0].Position - vertices[i1].Position);
            float l2 = glm::length(vertices[i1].Position - vertices[i2].Position);
            float l3 = glm::length(vertices[i2].Position - vertices[i0].Position);

            //std::cout << l1 << ", " << l2 << ", " << l3 << std::endl;
            // a粒子 b粒子 ab长度 i三角图元下标
            // edgeList.push_back({ &vertices[i0], &vertices[i1], l1, i});
            // edgeList.push_back({ &vertices[i1], &vertices[i2], l2, i});
            // edgeList.push_back({ &vertices[i2], &vertices[i0], l3, i});

            edgeIndices.push_back({ i0, i1, i, l1 });
            edgeIndices.push_back({ i1, i2, i, l2 });
            edgeIndices.push_back({ i2, i0, i, l3 });

            // i 三角图元下标 对应的三个顶点
            //triangleVertices[i] = { &vertices[i0], &vertices[i1], &vertices[i2] };
            triangles.push_back({ i ,i0, i1, i2 });
            // triangleVertices[i].push_back(&vertices[i0]);
            // triangleVertices[i].push_back(&vertices[i1]);
            // triangleVertices[i].push_back(&vertices[i2]);
            
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
        return Mesh(vertices, indices, textures, edgeIndices, triangles);
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

    /*
        绑定边过程：
        1. 对边进行排序，按照三角图元下标、顶点下标排序
        2. 遍历边列表，提取边的两个顶点和三角图元索引
        3. 使用边的两个顶点的索引作为键，三角图元索引作为值，存储在 edgeToTriangle 中
        4. 如果边不存在，则添加到 edgeToTriangle 中
        5. 如果边已经存在，则获取左边相邻三角形的索引，并将右边三角形的顶点与左边三角形的顶点进行比较
        6. 如果左边三角形有这个顶点，则删除，否则添加
        7. 绑定边：如果左边三角形和右边三角形的顶点集合中只剩下两个顶点，则创建一个新的边，并将其添加到 bendingEdges 中
        8. 清空顶点集合
    */
    void bendEdges() {
        bendingTriangles.resize(triangleVertices.size(), 0); // 测试

        // 对边进行排序
        std::sort(edgeList.begin(), edgeList.end(), [](const Edge &a, const Edge &b) {
            if(a.triangleIndex != b.triangleIndex) {
                return a.triangleIndex < b.triangleIndex; // 按照三角图元下标排序
            }
            if(a.v0->index != b.v0->index) {
                return a.v0->index < b.v0->index; // 按照第一个顶点下标排序
            }
            return a.v1->index < b.v1->index; // 按照第二个顶点下标排序
        });

        // 把边加入 edgeToTriangle 中，如果有相同的边， 提取边所对应的三角图元索引
        for (int i = 0; i < edgeList.size(); i++) {
            Edge &edge = edgeList[i];
            int a = edge.v0->index;
            int b = edge.v1->index;
            int c = edge.triangleIndex; // 右边 相邻三角形的索引
            std::pair<int, int> edgeKey;
            if (a > b) {
                edgeKey = std::make_pair(b, a);
            }
            else{
                edgeKey = std::make_pair(a, b);
            }

            // 如果边不存在，则添加
            if(edgeToTriangle.find(edgeKey) == edgeToTriangle.end()) {
                edgeToTriangle[edgeKey] = c; // 如果没有这个边，则添加
            }
            else {
                // 获取 左边 相邻三角形的索引
                int triangleIndex = edgeToTriangle[edgeKey];

                // std::vector<Vertex_H*>& verticesLeft = triangleVertices[triangleIndex];

                // std::set<Vertex_H*> vertexSet;
                // for(Vertex_H* v : verticesLeft) {
                //     vertexSet.insert(v);
                // }

                // std::vector<Vertex_H*>& verticesRight = triangleVertices[c]; // 获取右边三角形的顶点
                // for(Vertex_H* v : verticesRight) {
                //     if(vertexSet.find(v) == vertexSet.end()) {
                //         vertexSet.insert(v);
                //     }
                //     else{
                //         vertexSet.erase(v); // 如果左边三角形有这个顶点，则删除
                //     }
                // }
                auto& vertsA = triangleVertices[triangleIndex];
                auto& vertsB = triangleVertices[c];

                std::set<Vertex_H*> vertexSet(vertsA.begin(), vertsA.end());
                for (Vertex_H* v : vertsB) {
                    if (!vertexSet.erase(v)) {
                        vertexSet.insert(v);
                    }
                }

                // 绑定边
                Edge bendingEdge;
                if(vertexSet.size() == 2) {
                    auto it = vertexSet.begin();
                    bendingEdge.v0 = *it; // 第一个顶点
                    it++;
                    bendingEdge.v1 = *it; // 第二个顶点
                    bendingEdge.lenght = glm::length(bendingEdge.v0->Position - bendingEdge.v1->Position);
                    bendingEdge.triangleIndex = c; // 绑定边的三角图元索引
                    bendingEdges.push_back(bendingEdge);
                }
                
                vertexSet.clear(); // 清空顶点集合

                bendingTriangles[triangleIndex]++; // 记录绑定的三角图元索引
            }
        
        }

        for(int i = 0; i < bendingTriangles.size(); i++) {
            std::cout << "Triangle " << i << " bending count: " << bendingTriangles[i] << std::endl;
        }

    }

    void bendEdges2() {
        // 对边进行排序
        std::cout << "Edges: " << edgeList.size() << std::endl;
        std::sort(edgeList.begin(), edgeList.end(), [](const Edge &a, const Edge &b) {
            if(a.triangleIndex != b.triangleIndex) {
                return a.triangleIndex < b.triangleIndex; // 按照三角图元下标排序
            }
            if(a.v0->index != b.v0->index) {
                return a.v0->index < b.v0->index; // 按照第一个顶点下标排序
            }
            return a.v1->index < b.v1->index; // 按照第二个顶点下标排序
        });

        // 对于任意一条边，存在对应独立顶点
        for(Triangle &t : triangles){
            int a = t.i0;
            int b = t.i1;
            int c = t.i2;

            std::pair<int, int> edgeAB;
            std::pair<int, int> edgeAC;
            std::pair<int, int> edgeBC;

            if(a > b) {
                edgeAB = std::make_pair(b, a);
            }
            else{
                edgeAB = std::make_pair(a, b);
            }
            if(a > c) {
                edgeAC = std::make_pair(c, a);
            }
            else{
                edgeAC = std::make_pair(a, c);
            }
            if(b > c) {
                edgeBC = std::make_pair(c, b);
            }
            else{
                edgeBC = std::make_pair(b, c);
            }

            edgeWithVertex[edgeAB].push_back(c);
            edgeWithVertex[edgeAC].push_back(b);
            edgeWithVertex[edgeBC].push_back(a);
        }

        for (const auto& [_, triangleList] : edgeWithVertex) {
            // triangleList 就是 std::vector<int>
            if (triangleList.size() >= 2) {
                // 绑定边
                int t0 = triangleList[0];
                int t1 = triangleList[1];

                Edge bendingEdge;
                bendingEdge.v0 = allParticles[t0];
                bendingEdge.v1 = allParticles[t1];
                bendingEdge.lenght = glm::length(bendingEdge.v0->Position - bendingEdge.v1->Position);
                bendingEdge.triangleIndex = -1; // 绑定边的三角图元索引
                bendingEdges.push_back(bendingEdge);
            }
        }
    }

    void bendEdges3() {
        for(Edge &e1 : edgeList) {
            for(Edge &e2 : edgeList) {
                if(e1.v0->Position == e2.v0->Position && e1.v1->Position == e2.v1->Position ||
                   e1.v0->Position == e2.v1->Position && e1.v1->Position == e2.v0->Position) {
                    Edge bendingEdge;
                    std::set<Vertex_H*> vertexSet; // 用于存储三角形索引
                    std::vector<Vertex_H*> &vertices = triangleVertices[e1.triangleIndex]; // 引用
                    vertexSet.insert(e1.v0);
                    vertexSet.insert(e1.v1);
                    for(Vertex_H* v : vertices) {
                        if (vertexSet.find(v) != vertexSet.end()) {
                            vertexSet.erase(v); // 如果左边三角形有这个顶点，则删除
                        }
                        else {
                            vertexSet.insert(v); // 如果右边三角形有这个顶点，则添加
                        }
                    }
                    vertices = triangleVertices[e2.triangleIndex];
                    for(Vertex_H* v : vertices) {
                        if (vertexSet.find(v) != vertexSet.end()) {
                            vertexSet.erase(v); // 如果左边三角形有这个顶点，则删除
                        }
                        else {
                            vertexSet.insert(v); // 如果右边三角形有这个顶点，则添加
                        }
                    }
                    if(vertexSet.size() == 2) {
                        auto it = vertexSet.begin();
                        bendingEdge.v0 = *it; // 第一个顶点
                        it++;
                        bendingEdge.v1 = *it; // 第二个顶点
                        bendingEdge.lenght = glm::length(bendingEdge.v0->Position - bendingEdge.v1->Position);
                        bendingEdge.triangleIndex = e1.triangleIndex; // 绑定边的三角图元索引
                        bendingEdges.push_back(bendingEdge);
                    }
                }
            }
        }
    }

    // 绑定边到边列表
    // 并清空绑定边和边到三角形的映射
    void bendingToEdges(){
        edgeList.insert(edgeList.end(), bendingEdges.begin(), bendingEdges.end());
        bendingEdges.clear(); // 清空绑定边
        edgeToTriangle.clear(); // 清空边到三角形的映射
    }


    // TODO
    void edgesUnion(){
        for(Edge &e1 : edgeList) {
            for(Edge &e2 : edgeList) {
                if(e1.v0->Position == e2.v0->Position && e1.v1->Position == e2.v1->Position) {
                    e2 = e1; // 如果两个边的顶点位置相同，则合并
                }
            }
        }
    }

    // TODO
    void particleUnion(){
        for(Vertex_H* p1 : allParticles) {
            int i = 0;
            for(Vertex_H* p2 : allParticles) {
                if(p1->Position == p2->Position && p1 != p2) {
                    // 如果两个粒子的位置相同，则合并
                    p2 = p1; // 将p2指向p1
                    // 删除p2
                }
            }
        }
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
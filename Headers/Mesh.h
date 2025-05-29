#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader_s.h"

#include <string>
#include <vector>

using namespace std;

#define MAX_BONE_INFLUENCE 4


struct Vertex_H {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

    /* 模拟参数 */
    glm::vec3 Velocity;
    glm::vec3 OldVelocity;
    glm::vec3 OldPosition;
    glm::vec3 Acceleration;
    glm::vec3 newPosition = glm::vec3(0.0f, 0.0f, 0.0f); // Jacobbian method
    int index = -1; // 用于索引
    float mass;
    float radius = 0.03f; // 0.03-0.15, 0.05-0.2 ...

    glm::vec3 Tangent;
    glm::vec3 Bitangent;

    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture_H {
    unsigned int id;
    string type;
    /*
    Used to compare the path of the Texture to be loaded with the Texture that has been loaded. 
    The purpose is to avoid re-loading the Texture that has already been loaded.
    */ 
    string path; 
};

class Mesh {
    public:
        /* Mesh data */
        vector<Vertex_H> vertices;
        vector<unsigned int> indices;
        vector<Texture_H> textures;
        unsigned int VAO;
        unsigned int VBO, EBO;

        /* functions */
        Mesh(vector<Vertex_H> vertices, vector<unsigned int> indices, vector<Texture_H> textures){
            this->vertices = vertices;
            this->indices = indices;
            this->textures = textures;

            setupMesh();
        }

        void Draw(Shader shader){
            unsigned int diffuseNr = 1;
            unsigned int specularNr = 1;
            unsigned int normalNr = 1;
            unsigned int heightNr = 1;

            for(unsigned int i = 0; i < textures.size(); i++){
                glActiveTexture(GL_TEXTURE0 + i);

                string number;
                string name = textures[i].type;
                if(name == "texture_diffuse")
                    number = std::to_string(diffuseNr++);
                else if(name == "texuture_specular")
                    number = std::to_string(specularNr++);
                else if(name == "texture_normal")
                    number = std::to_string(normalNr++);
                else if(name == "texture_height")
                    number = std::to_string(heightNr++);

                glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);

                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            }

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glActiveTexture(GL_TEXTURE0);
        }
        // for change model
        void cleanup(){
            if(VAO != 0){
                glDeleteVertexArrays(1, &VAO);
                VAO = 0;
            }
            if(VBO != 0){
                glDeleteBuffers(1, &VBO);
                VBO = 0;
            }
            if(EBO != 0){
                glDeleteBuffers(1, &EBO);
                EBO = 0;
            }
        }
        // 获取模型所有顶点数据
        vector<Vertex_H> *getVertecs(){
            return &vertices;
        }

        void updateVertexPositions() {
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex_H), &vertices[0]);
        }

    private:
        /* rendering data */
        void setupMesh(){
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            
            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex_H), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);	
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_H), (void*)0);
            // vertex normals
            glEnableVertexAttribArray(1);	
                                                                                // offsetof(struct，var)
                                                                                // Returns the distance between the variable and the first byte in the Vertex structure, in bytes
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_H), (void*)offsetof(Vertex_H, Normal));
            // vertex texture coords
            glEnableVertexAttribArray(2);	
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_H), (void*)offsetof(Vertex_H, TexCoords));
            // vertex tangent
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_H), (void*)offsetof(Vertex_H, Tangent));
            // vertex bitangent
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_H), (void*)offsetof(Vertex_H, Bitangent));
		    // ids
		    glEnableVertexAttribArray(5);
		    glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex_H), (void*)offsetof(Vertex_H, m_BoneIDs));

		    // weights
		    glEnableVertexAttribArray(6);
	    	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex_H), (void*)offsetof(Vertex_H, m_Weights));
            glBindVertexArray(0);
        }
};

#endif
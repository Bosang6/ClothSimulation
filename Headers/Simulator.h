#pragma once
#include "Mesh.h"
#include "Hash.h"
#include "Model.h"
#include <vector>
#include <omp.h>

class Simulator {
public:
    std::vector<Vertex_H*>& allParticles;
    std::unordered_set<Vertex_H*>& staticParticles;
    std::vector<Model>& models;
    float thickness;
    Hash& hash;
    int iterCount = 2;
    float collisionRadius = 0.2f;
    glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);

    Simulator(std::vector<Vertex_H*>& allParticles,
              std::unordered_set<Vertex_H*>& staticParticles,
              std::vector<Model>& models,
              Hash& hash)
        : allParticles(allParticles), staticParticles(staticParticles), models(models), hash(hash) {}

    // void simulate(float deltaTime, int numSubSteps) {
    //     float dt = deltaTime / numSubSteps;
    //     float maxVelocity = 0.2f * thickness / dt; // 经验公式， 限制速度的最大值，防止粒子移动太快穿透别的粒子或者地面

    //     hash.clear(); // 清空哈希表
    //     hash.insertParticles(allParticles);
    //     hash.partialSum(); // 计算每个cell的粒子数量前缀和
    //     hash.insertParticleMap(); // 将粒子指针插入到哈希表中

    //     step(deltaTime);
    // }

    void step(float deltaTime) {
        float dt = std::min(deltaTime, 0.02f); // 限制最大时间步长，避免过大导致不稳定
        float alpha = 100.0f / dt / dt; // xpbd
        // 1. 预测新位置
        for (auto& p : allParticles) {
            if (staticParticles.count(p)) continue;
            p->OldPosition = p->Position;
            p->Velocity += gravity * dt;
            p->Position += p->Velocity * dt;
        }

        // 2. 处理地面碰撞
        solveGroundCollision();

        // 2. 多次迭代（约束+碰撞）
        for (int iter = 0; iter < iterCount; ++iter) {
            hash.clear();
            hash.insertParticles(allParticles);
            hash.partialSum();
            hash.insertParticleMap();
            // 边长约束
            for (auto& model : models) {
                //for (auto& edge : model.edgeList)
                auto& edges = model.edgeList; 
                #pragma omp parallel for
                for(int i = 0; i < edges.size(); i++) {
                    Edge& edge = edges[i];
                    Vertex_H* p0 = edge.v0;
                    Vertex_H* p1 = edge.v1;
                    float restLength = edge.lenght;
                    float w0 = 1.0f / p0->mass;
                    float w1 = 1.0f / p1->mass;
                    // float w0 = staticParticles.count(p0) ? 0.0f : 1.0f / p0->mass;
                    // float w1 = staticParticles.count(p1) ? 0.0f : 1.0f / p1->mass;
                    glm::vec3 diff = p0->Position - p1->Position;   //   Xi - Xj
                    float len = glm::length(diff);                  // ||Xi - Xj||
                    if (len < 1e-6f) continue;
                    glm::vec3 dir = diff / len;
                    float constraint = len - restLength;            // ||Xi - Xj|| - restLength
                    float s = -constraint / (w0 + w1 + alpha); // 计算位移

                    p0->Position += diff * s * (1 / w0); // 更新位置
                    p1->Position += diff * (-s * (1 / w1)); // 更新位置
                    // float C = len - restLength;
                    // float w = w0 + w1;
                    // float s = -C / (w + 1e-6f);
                    // if(w0>0) p0->Position += dir * s * w0;
                    // if(w1>0) p1->Position -= dir * s * w1;
                }
            }
            // 碰撞
            hash.queryAndCollideAll(collisionRadius);
        }

        // 3. 更新速度
        for (auto& p : allParticles) {
            if (staticParticles.count(p)) continue;
            p->Velocity = (p->Position - p->OldPosition) / dt;
        }
    }

    // jacobi 爆炸！FUCK!
    void substep(float deltaTime) {
        float sdt = deltaTime;
        std::vector<int> counts(allParticles.size(), 0); // Ni
    
        for(unsigned int i  = 0; i < iterCount; i++){
            // 1. 位置更新
            for (auto& p : allParticles) {
                if (staticParticles.count(p)) continue;
                //p->OldPosition = p->Position;
                p->Velocity += gravity * sdt;
                p->Position += p->Velocity * sdt;
            }
            // 2. 多次迭代（约束+碰撞）
            for (int iter = 0; iter < 3; ++iter){
                for(Vertex_H* p : allParticles) {
                    p->newPosition = glm::vec3(0.0f, 0.0f, 0.0f); // 初始化新位置
                }
                for (auto& model : models){
                    auto& edges = model.edgeList; 
                    #pragma omp parallel for
                    for(int i = 0; i < edges.size(); i++){
                        Edge& edge = edges[i]; 
                        Vertex_H* p0 = edge.v0;
                        Vertex_H* p1 = edge.v1;
                        int id0 = p0->index;
                        if(id0 < 0) continue; // 确保索引有效
                        int id1 = p1->index;
                        if(id1 < 0) continue; // 确保索引有效
                        float restLength = edge.lenght;
                        glm::vec3 diff = p0->Position - p1->Position; //   Xi - Xj
                        float len = glm::length(diff);                // ||Xi - Xj||
                        if (len < 1e-6f) continue; // 避免除以0
                        float constrain = len - restLength;           // ||Xi - Xj|| - restLength
                        glm::vec3 normal = diff / len;                // 归一化方向向量

                        glm::vec3 offset = normal * constrain * 0.5f;
                        if(glm::length(offset) > 1.0f){
                            offset = glm::normalize(offset) * 0.01f; // 确保偏移量不超过1.0f
                        }

                        #pragma omp atomic
                        p0->newPosition.x += offset.x; // 更新新位置
                        #pragma omp atomic
                        p0->newPosition.y += offset.y; // 更新新位置
                        #pragma omp atomic
                        p0->newPosition.z += offset.z; // 更新新位置
                        #pragma omp atomic
                        p1->newPosition.x -= offset.x; // 更新新位置
                        #pragma omp atomic
                        p1->newPosition.y -= offset.y; // 更新新位置
                        #pragma omp atomic
                        p1->newPosition.z -= offset.z; // 更新新位置
                        #pragma omp atomic
                        counts[id0]++;
                        #pragma omp atomic
                        counts[id1]++;
                        // p0->newPosition += offset; // 更新新位置
                        // p1->newPosition -= offset; // 更新新位置
                        // counts[id0]++; // 更新计数
                        // counts[id1]++; // 更新计数
                    }
                    float alpha = 20.0f; // 更新系数
                    for(auto& p : allParticles) {
                        if (staticParticles.count(p)) continue;
                        // 更新位置
                        if(p->index < 0) continue; // 确保索引有效
                        p->Position += (p->newPosition + alpha * p->Position) / float(counts[p->index] + alpha); // alpha = 1
                    }
                }

            }

            // 3. 更新速度
            for (auto& p : allParticles) {
                if (staticParticles.count(p)) continue;
                p->Velocity += (p->Position - p->OldPosition) / sdt;
                p->OldPosition = p->Position; // 更新旧位置
            }

        }
    }

    void solveGroundCollision(){
        for (auto& p : allParticles) {
            if (staticParticles.count(p)) continue;
            if (p->Position.y < 0.5f * p->radius) { // 如果y的坐标小于粒子半径，则产生地面碰撞
                float damping = 1.0f; // 阻尼系数
                glm::vec3 dx = p->Position - p->OldPosition;
                p->Position = p->Position + p->Velocity * -damping; // 更新位置
                p->Position.y = 0.5f * p->radius; // 确保粒子位置在地面上
            }
        }
    }
};
#pragma once
#include "Mesh.h"
#include "Hash.h"
#include "Model.h"
#include <vector>
#include <omp.h>

class Simulator {
public:
    std::vector<Vertex_H*>& allParticles;
    std::vector<Edge*>& edges; // 用于边长约束
    std::vector<Edge*>& bendingEdges; // 用于弯曲约束
    std::unordered_set<Vertex_H*>& staticParticles;
    std::vector<Model>& models;
    float thickness = 0.8f; // 粒子厚度
    Hash& hash;
    int iterCount = 2;
    glm::vec3 gravity = glm::vec3(0.0f, -9.8f, 0.0f);

    Simulator(std::vector<Vertex_H*>& allParticles,
              std::vector<Edge *>& edges,
              std::vector<Edge *>& bendingEdges,
              std::unordered_set<Vertex_H*>& staticParticles,
              std::vector<Model>& models,
              Hash& hash)
        : allParticles(allParticles), edges(edges), bendingEdges(bendingEdges),staticParticles(staticParticles), models(models), hash(hash) {}

    void simulate(float deltaTime, int numSubSteps) {
        float dt = deltaTime / numSubSteps;
        float maxVelocity = 0.2f * thickness / dt; // 经验公式， 限制速度的最大值，防止粒子移动太快穿透别的粒子或者地面

        hash.clear(); // 清空哈希表
        hash.insertParticles(allParticles);
        hash.partialSum(); // 计算每个cell的粒子数量前缀和
        hash.insertParticleMap(); // 将粒子指针插入到哈希表中
        hash.queryAll(thickness);

        for (int i = 0; i < numSubSteps; ++i){
            // hash.clear();
            // hash.insertParticles(allParticles);
            // hash.partialSum();
            // hash.insertParticleMap();
            // hash.queryAll(thickness); // 必须重建邻接表
            // 1. 预测新位置
            for (auto& p : allParticles) {
                if (staticParticles.count(p) || p->mass <= 0.0f) continue;
                p->Velocity += gravity * dt;
                float vel = glm::length(p->Velocity);
                if (vel > maxVelocity) {
                    p->Velocity = p->Velocity * (maxVelocity / vel); // 限制速度
                }
                p->OldPosition = p->Position;
                p->Position += p->Velocity * dt;
            }
            // 2. 处理地面碰撞
            solveGroundCollision();
    
            // 3. 约束
            solveContraints(dt);

            // 4. 碰撞处理
            solveCollisions(dt);

            // hash.clear();
            // hash.insertParticles(allParticles);
            // hash.partialSum();
            // hash.insertParticleMap();
            // hash.queryAndCollideAll(collisionRadius);

            // 5. 速度修正
            for (auto& p : allParticles) {
                if (staticParticles.count(p)) continue;
                p->Velocity = (p->Position - p->OldPosition) / dt;
            }

        }



    }

    // void step(float deltaTime) {
    //     float dt = std::min(deltaTime, 0.02f); // 限制最大时间步长，避免过大导致不稳定
    //     float alpha = 100.0f / dt / dt; // xpbd
    //     // 1. 预测新位置
    //     for (auto& p : allParticles) {
    //         if (staticParticles.count(p)) continue;
    //         p->OldPosition = p->Position;
    //         p->Velocity += gravity * dt;
    //         p->Position += p->Velocity * dt;
    //     }

    //     // 2. 处理地面碰撞
    //     solveGroundCollision();

    //     // 2. 多次迭代（约束+碰撞）
    //     for (int iter = 0; iter < iterCount; ++iter) {
    //         hash.clear();
    //         hash.insertParticles(allParticles);
    //         hash.partialSum();
    //         hash.insertParticleMap();
    //         // 边长约束
    //         for (auto& model : models) {
    //             //for (auto& edge : model.edgeList)
    //             auto& edges = model.edgeList; 
    //             #pragma omp parallel for
    //             for(int i = 0; i < edges.size(); i++) {
    //                 Edge& edge = edges[i];
    //                 Vertex_H* p0 = edge.v0;
    //                 Vertex_H* p1 = edge.v1;
    //                 float restLength = edge.lenght;
    //                 float w0 = 1.0f / p0->mass;
    //                 float w1 = 1.0f / p1->mass;
    //                 // float w0 = staticParticles.count(p0) ? 0.0f : 1.0f / p0->mass;
    //                 // float w1 = staticParticles.count(p1) ? 0.0f : 1.0f / p1->mass;
    //                 glm::vec3 diff = p0->Position - p1->Position;   //   Xi - Xj
    //                 float len = glm::length(diff);                  // ||Xi - Xj||
    //                 if (len < 1e-6f) continue;
    //                 glm::vec3 dir = diff / len;
    //                 float constraint = len - restLength;            // ||Xi - Xj|| - restLength
    //                 float s = -constraint / (w0 + w1 + alpha); // 计算位移

    //                 p0->Position += diff * s * (1 / w0); // 更新位置
    //                 p1->Position += diff * (-s * (1 / w1)); // 更新位置
    //                 // float C = len - restLength;
    //                 // float w = w0 + w1;
    //                 // float s = -C / (w + 1e-6f);
    //                 // if(w0>0) p0->Position += dir * s * w0;
    //                 // if(w1>0) p1->Position -= dir * s * w1;
    //             }
    //         }
    //         // 碰撞
    //         hash.queryAndCollideAll(thickness);
    //     }

    //     // 3. 更新速度
    //     for (auto& p : allParticles) {
    //         if (staticParticles.count(p)) continue;
    //         p->Velocity = (p->Position - p->OldPosition) / dt;
    //     }
    // }

    // jacobi 爆炸！FUCK!
    // void substep(float deltaTime) {
    //     float sdt = deltaTime;
    //     std::vector<int> counts(allParticles.size(), 0); // Ni
    
    //     for(unsigned int i  = 0; i < iterCount; i++){
    //         // 1. 位置更新
    //         for (auto& p : allParticles) {
    //             if (staticParticles.count(p)) continue;
    //             //p->OldPosition = p->Position;
    //             p->Velocity += gravity * sdt;
    //             p->Position += p->Velocity * sdt;
    //         }
    //         // 2. 多次迭代（约束+碰撞）
    //         for (int iter = 0; iter < 3; ++iter){
    //             for(Vertex_H* p : allParticles) {
    //                 p->newPosition = glm::vec3(0.0f, 0.0f, 0.0f); // 初始化新位置
    //             }
    //             for (auto& model : models){
    //                 auto& edges = model.edgeList; 
    //                 #pragma omp parallel for
    //                 for(int i = 0; i < edges.size(); i++){
    //                     Edge& edge = edges[i]; 
    //                     Vertex_H* p0 = edge.v0;
    //                     Vertex_H* p1 = edge.v1;
    //                     int id0 = p0->index;
    //                     if(id0 < 0) continue; // 确保索引有效
    //                     int id1 = p1->index;
    //                     if(id1 < 0) continue; // 确保索引有效
    //                     float restLength = edge.lenght;
    //                     glm::vec3 diff = p0->Position - p1->Position; //   Xi - Xj
    //                     float len = glm::length(diff);                // ||Xi - Xj||
    //                     if (len < 1e-6f) continue; // 避免除以0
    //                     float constrain = len - restLength;           // ||Xi - Xj|| - restLength
    //                     glm::vec3 normal = diff / len;                // 归一化方向向量

    //                     glm::vec3 offset = normal * constrain * 0.5f;
    //                     if(glm::length(offset) > 1.0f){
    //                         offset = glm::normalize(offset) * 0.01f; // 确保偏移量不超过1.0f
    //                     }

    //                     #pragma omp atomic
    //                     p0->newPosition.x += offset.x; // 更新新位置
    //                     #pragma omp atomic
    //                     p0->newPosition.y += offset.y; // 更新新位置
    //                     #pragma omp atomic
    //                     p0->newPosition.z += offset.z; // 更新新位置
    //                     #pragma omp atomic
    //                     p1->newPosition.x -= offset.x; // 更新新位置
    //                     #pragma omp atomic
    //                     p1->newPosition.y -= offset.y; // 更新新位置
    //                     #pragma omp atomic
    //                     p1->newPosition.z -= offset.z; // 更新新位置
    //                     #pragma omp atomic
    //                     counts[id0]++;
    //                     #pragma omp atomic
    //                     counts[id1]++;
    //                     // p0->newPosition += offset; // 更新新位置
    //                     // p1->newPosition -= offset; // 更新新位置
    //                     // counts[id0]++; // 更新计数
    //                     // counts[id1]++; // 更新计数
    //                 }
    //                 float alpha = 20.0f; // 更新系数
    //                 for(auto& p : allParticles) {
    //                     if (staticParticles.count(p)) continue;
    //                     // 更新位置
    //                     if(p->index < 0) continue; // 确保索引有效
    //                     p->Position += (p->newPosition + alpha * p->Position) / float(counts[p->index] + alpha); // alpha = 1
    //                 }
    //             }

    //         }

    //         // 3. 更新速度
    //         for (auto& p : allParticles) {
    //             if (staticParticles.count(p)) continue;
    //             p->Velocity += (p->Position - p->OldPosition) / sdt;
    //             p->OldPosition = p->Position; // 更新旧位置
    //         }

    //     }
    // }

    void solveGroundCollision(){
        for (auto& p : allParticles) {
            if (staticParticles.count(p)) continue;
            if (p->Position.y < 0.5f * p->radius) { // 如果y的坐标小于粒子半径，则产生地面碰撞
                float damping = 1.0f; // 阻尼系数
                glm::vec3 dx = p->Position - p->OldPosition;
                p->Position += dx * -damping; // 更新位置
                p->Position.y = 0.5f * p->radius; // 确保粒子位置在地面上
            }
        }
    }

    void solveContraints(float dt){
        for (auto& e : edges){
            //printf("edge a Pos: %f, %f, %f, b Pos: %f, %f, %f, lenght: %f\n", e[0].v0->Position.x, e[0].v0->Position.y, e[0].v0->Position.z, e[0].v1->Position.x, e[0].v1->Position.y, e[0].v1->Position.z, e[0].lenght);
            //for(int i = 0; i < edges.size(); i++){
                //Edge& edge = edges[i];
                Vertex_H* p0 = e->v0;
                Vertex_H* p1 = e->v1;
                float restLength = e->lenght;
                float w0 = staticParticles.count(p0) ? 0.0f : 1.0f / p0->mass;
                float w1 = staticParticles.count(p1) ? 0.0f : 1.0f / p1->mass;

                float w = w0 + w1;
                if(w == 0.0f) continue; // 避免除以0

                glm::vec3 diff = p0->Position - p1->Position; //   Xi - Xj
                float len = glm::length(diff);                // ||Xi - Xj||
                if(len < 1e-6f) continue; // 避免除以0
                glm::vec3 dir = diff / len;

                float C = len - restLength;                   // ||Xi - Xj|| - restLength
                //printf("C: %f\n", C);
                //float alpha = 0.0001f; // compliances / dt / dt;
                float alpha = 0.1f / dt / dt; // compliances / dt / dt;
                float s = -C / (w + alpha); // 计算位移
                p0->Position += dir * s * w0; // 更新位置
                p1->Position -= dir * s * w1; // 更新位置

            //}
        }

        for (auto& e : bendingEdges){
            //printf("edge a Pos: %f, %f, %f, b Pos: %f, %f, %f, lenght: %f\n", e[0].v0->Position.x, e[0].v0->Position.y, e[0].v0->Position.z, e[0].v1->Position.x, e[0].v1->Position.y, e[0].v1->Position.z, e[0].lenght);
            //for(int i = 0; i < edges.size(); i++){
                //Edge& edge = edges[i];
                Vertex_H* p0 = e->v0;
                Vertex_H* p1 = e->v1;
                float restLength = e->lenght;
                float w0 = staticParticles.count(p0) ? 0.0f : 1.0f / p0->mass;
                float w1 = staticParticles.count(p1) ? 0.0f : 1.0f / p1->mass;

                float w = w0 + w1;
                if(w == 0.0f) continue; // 避免除以0

                glm::vec3 diff = p0->Position - p1->Position; //   Xi - Xj
                float len = glm::length(diff);                // ||Xi - Xj||
                if(len < 1e-6f) continue; // 避免除以0
                glm::vec3 dir = diff / len;

                float C = len - restLength;                   // ||Xi - Xj|| - restLength
                //printf("C: %f\n", C);
                //float alpha = 0.01f; // compliances / dt / dt;
                float alpha = 1.0f / dt / dt; // compliances / dt / dt;
                float s = -C / (w + alpha); // 计算位移
                p0->Position += dir * s * w0; // 更新位置
                p1->Position -= dir * s * w1; // 更新位置

            //}
        }
        
    }

    void solveCollisions(float dt){
        float thickness2 = thickness * thickness;

        for(int id0 = 0; id0 < allParticles.size(); id0++){
            Vertex_H* particle0 = allParticles[id0];
            if(particle0->mass == 0.0f) continue; // 跳过质量为0的粒子

            int first = hash.firstAdjId[id0]; // 获取第一个邻接粒子的位置
            int last = hash.firstAdjId[id0 + 1]; // 获取最后一个邻接粒子的位置

            if(last > hash.adjIds.size() || first < 0 || last < first) {
                std::cout << "Error: Invalid range for adjIds. first: " << first << ", last: " << last << ", size: " << hash.adjIds.size() << std::endl;
                continue; // 如果范围不合法，跳过
            }

            for(int j = first; j < last; j++){
                int id1 = hash.adjIds[j]; // 获取邻接粒子的索引
                Vertex_H* particle1 = allParticles[id1]; // 获取邻接粒子
                if(particle1->mass == 0.0f || id0 == id1) continue; // 跳过质量为0的粒子

                glm::vec3 diff = particle1->Position - particle0->Position; // 计算两个粒子之间的差向量
            
                float dist = glm::length(diff); // 计算距离的平方

                //printf("dist: %f, thickness: %f\n", dist, thickness);
                float dist2 = dist * dist; // 平方距离

                if(dist2 > thickness2 || dist2 == 0.0f) continue; // 如果距离大于厚度的平方或者距离为0，则跳过

                float restDist = glm::length(particle0->initPosition - particle1->initPosition); // 计算初始位置的平方距离
                float restDist2 = restDist * restDist; // 平方距离

                float minDist = thickness; // 最小距离 thickness = 0.01f

                if(dist2 > thickness2) continue; // 如果距离大于厚度的平方，则跳过

                if(restDist2 < thickness2)
                    minDist = restDist;

                // 位置修正
                diff = diff * (minDist - dist / dist); // 计算新的位置差向量
                particle0->Position -= diff * 0.5f; // 更新位置
                particle1->Position += diff * 0.5f; // 更新邻接粒子的位置
                //printf("yes\n");

                // 速度修正
                glm::vec3 diffNewOld0 = particle0->Position - particle0->OldPosition; // 计算位置差向量
                glm::vec3 diffNewOld1 = particle1->Position - particle1->OldPosition; // 计算邻接粒子的位置差向量

                glm::vec3 averageVelocity = (diffNewOld0 + diffNewOld1) * 0.5f; // 计算平均速度

                diffNewOld0 = averageVelocity - diffNewOld0; // 计算新的位置差向量
                diffNewOld1 = averageVelocity - diffNewOld1; // 计算邻接粒子的新的位置差向量

                float friction = 0.1f; // 摩擦系数

                particle0->Position += diffNewOld0 * friction; // 更新位置
                particle1->Position += diffNewOld1 * friction; // 更新邻接粒子的位置
            }

        }
    }

};
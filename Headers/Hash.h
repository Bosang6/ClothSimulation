#pragma once
#ifndef HASH_H
#define HASH_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.h"

#include <vector>

struct Cell{
    int count = 0;
    std::vector<Vertex_H*> particles;
};

class Hash{
    int tableSize;
    std::vector<Cell> cellCount;
    float hashing = 0.15f;

    vector<Vertex_H*> particleMap;

    // ⬇️ 新增：静态粒子集合指针
    const std::unordered_set<Vertex_H*>* staticParticles;

public:
    // Hash(int particlesSize){
    //     this->tableSize = particlesSize * 2 + 1;
    //     cellCount.resize(tableSize + 1);
    //     particleMap.resize(particlesSize);
    // }

    Hash(int particleCount, const std::unordered_set<Vertex_H*>* statics = nullptr)
        : staticParticles(statics)
    {
        tableSize = particleCount * 2 + 1;
        cellCount.resize(tableSize + 1);
        particleMap.resize(particleCount);
    }

    // 计数 + 链表
    void insertParticles(const vector<Vertex_H*> &vertices){
        for(unsigned int i = 0; i < vertices.size(); i++){
            int index = hashPos(vertices[i]->Position);
            if(index >= 0 && index < cellCount.size()){
                cellCount[index].count += 1;
                cellCount[index].particles.push_back(vertices[i]);
            }
        }
    }

    void partialSum(){
        for(unsigned int i = 1; i <= tableSize; i++){
            cellCount[i].count += cellCount[i - 1].count;
        }
    }

    // 第 i 个cell 为 前 0 ~ i-1 个cell中所有粒子的总和
    int checkParticles(int index){
        return cellCount[index + 1].count - cellCount[index].count;
    }

    // void insertParticleMap(){
    //     for(unsigned int i = 1; i < cellCount.size(); i++){
    //         int num = cellCount[i].count - cellCount[i - 1].count;
    //         if(num != 0){
    //             for(unsigned int j = cellCount[i].count; j > cellCount[i - 1].count; j--){
    //                 cellCount[i].count = cellCount[i].count - 1;
    //                 int index = cellCount[i].count; // particleMap的下标
    //                 particleMap[index] = cellCount[i].particles.back(); // 将list最顶部的指针保存到particleMap中
    //                 cellCount[i].particles.pop_back(); // 移除list最顶部的指针
    //             }
    //         }
    //     }
    // }

    void insertParticleMap() {
        int writeIndex = 0;
        for (size_t i = 0; i < cellCount.size(); ++i) {
            for (Vertex_H* p : cellCount[i].particles) {
                if (writeIndex < (int)particleMap.size()) {
                    particleMap[writeIndex++] = p;
                } else {
                    std::cerr << "[Error] particleMap overflow at index " << writeIndex << std::endl;
                    break;
                }
            }
            cellCount[i].particles.clear(); // optional: 清理粒子链表
        }
    }



    int hashPos(glm::vec3 pos){
        int x = static_cast<int>(floor(pos.x / hashing));
        int y = static_cast<int>(floor(pos.y / hashing));
        int z = static_cast<int>(floor(pos.z / hashing));
        int64_t hash = (int64_t(x) * 92837111) ^ (int64_t(y) * 689287499) ^ (int64_t(z) * 283923481);
        
        return static_cast<int>(std::abs(hash)) % tableSize;
    }

    void queryAndCollideAll(float radius){
        for(Vertex_H *p : particleMap){
            if(p)
                queryAndCollide(*p, radius);
        }
    }

    // void queryAndCollide(Vertex_H &particle, float radius) {
    //     glm::vec3 pos = particle.Position;
    //     int x = static_cast<int>(floor(pos.x / hashing));
    //     int y = static_cast<int>(floor(pos.y / hashing));
    //     int z = static_cast<int>(floor(pos.z / hashing));

    //     int r = static_cast<int>(ceil(radius / hashing)); // 半径范围能影响几个 cell

    //     for (int dx = -r; dx <= r; dx++) {
    //         for (int dy = -r; dy <= r; dy++) {
    //             for (int dz = -r; dz <= r; dz++) {
    //                 int64_t hx = int64_t(x + dx) * 92837111;
    //                 int64_t hy = int64_t(y + dy) * 689287499;
    //                 int64_t hz = int64_t(z + dz) * 283923481;

    //                 int64_t h = (hx ^ hy ^ hz);
    //                 int index = static_cast<int>(std::abs(h) % tableSize);

    //                 int start = (index == 0) ? 0 : cellCount[index - 1].count;
    //                 int end = cellCount[index].count;

    //                 for (int i = start; i < end; i++) {
    //                     Vertex_H *other = particleMap[i];
    //                     if (&particle == other) continue;

    //                     float dist2 = glm::length(particle.Position - other->Position);
    //                     if (dist2 < radius * radius) {
    //                         // 碰撞响应：如反弹、拉开位置、计算力等等
    //                         glm::vec3 normal = glm::normalize(particle.Position - other->Position);
    //                         float overlap = radius - sqrt(dist2);
    //                         particle.Position += 0.5f * overlap * normal;
    //                         other->Position  -= 0.5f * overlap * normal;
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }

    void queryAndCollide(Vertex_H &particle, float radius) {
        glm::vec3 pos = particle.Position;
        int x = static_cast<int>(floor(pos.x / hashing));
        int y = static_cast<int>(floor(pos.y / hashing));
        int z = static_cast<int>(floor(pos.z / hashing));

        int r = static_cast<int>(ceil(radius / hashing)); // 搜索邻域

        for (int dx = -r; dx <= r; dx++) {
            for (int dy = -r; dy <= r; dy++) {
                for (int dz = -r; dz <= r; dz++) {
                    int64_t hx = int64_t(x + dx) * 92837111;
                    int64_t hy = int64_t(y + dy) * 689287499;
                    int64_t hz = int64_t(z + dz) * 283923481;
                    int64_t h = hx ^ hy ^ hz;
                    int index = static_cast<int>(std::abs(h) % tableSize);

                    if (index < 0 || index >= (int)cellCount.size()) continue;

                    int start = (index == 0) ? 0 : cellCount[index - 1].count;
                    int end = cellCount[index].count;

                    if (start < 0 || end > (int)particleMap.size()) continue;

                    for (int i = start; i < end; ++i) {
                        Vertex_H* other = particleMap[i];
                        if (!other || &particle == other) continue; // 空指针保护 & 自身跳过

                        glm::vec3 diff = particle.Position - other->Position;
                        float dist2 = glm::dot(diff, diff);
                        if (dist2 < radius * radius && dist2 > 1e-8f) {
                            //glm::vec3 normal = glm::normalize(diff);
                            // float overlap = radius - sqrt(dist2);
                            
                            // float correction = std::min(overlap * 0.5f, 0.01f);

                            // particle.Position += 0.5f * overlap * normal;
                            // other->Position  -= 0.5f * overlap * normal;

                            // particle.Position += correction * normal;
                            // other->Position  -= correction * normal;

                            glm::vec3 normal = glm::normalize(diff);
                            float overlap = radius - sqrt(dist2);
                            float maxMove = 0.1f;
                            float correction = std::min(overlap, maxMove);

                            if (staticParticles && staticParticles->count(other)) {
                                // ⬅️ other 是静态粒子，只推 particle
                                particle.Position += correction * normal;
                            } else {
                                // ⬅️ 普通粒子对碰撞
                                particle.Position += 0.5f * correction * normal;
                                other->Position  -= 0.5f * correction * normal;
                            }
                        }
                    }
                }
            }
        }
    }

    void clear() {
        for (auto& cell : cellCount) {
            cell.count = 0;
            cell.particles.clear();
        }
    }

    // void queryAll(float maxDist){
    //     int num = 0;
    //     float maxDist2 = maxDist * maxDist;
        
    //     for(unsigned int i = 0; i < particleMap.size(); i++){
    //         Vertex_H *p = particleMap[i];
    //         if(!p) continue;
            
    //         int index = i;

    //     }
    // }


};

#endif
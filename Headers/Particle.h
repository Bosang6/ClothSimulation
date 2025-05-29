#pragma onece
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Node {
public:
	double mass;
	bool isPined;
	glm::vec2 texCoord;
	glm::vec3 normal;
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 force;
	glm::vec3 acceleration;

public:
	Node(glm::vec3 pos, glm::vec3 n) {
		mass = 1.0;
		isPined = false;
		normal = n;
		position = pos;
		velocity = glm::vec3(0.0f, 0.0f, 0.0f);
		force = glm::vec3(0.0f, 0.0f, 0.0f);
		acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
	}
	~Node(void) {}

	void addForce(glm::vec3 f) {
		force += f;
	}

	void integrate(double timeStep) {
		if (!isPined) {
			acceleration = glm::vec3(force.x / mass, force.y / mass, force.z / mass);
			velocity += glm::vec3(acceleration.x * timeStep, acceleration.y * timeStep, acceleration.z * timeStep);
			position += glm::vec3(velocity.x * timeStep, velocity.y * timeStep, velocity.z * timeStep);
		}
		force = glm::vec3(0.0f, 0.0f, 0.0f);
	}

};
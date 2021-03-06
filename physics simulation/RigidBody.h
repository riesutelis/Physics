#pragma once
#include "Body.h"

class RigidBody :
	public Body
{
public:
	RigidBody();
	~RigidBody();

	// set and get methods
	void setAngVel(const glm::vec3 & omega) { m_angVel = omega; }
	void setAngAccl(const glm::vec3 & alpha) { m_angAcc = alpha; }
	void setInvInertia(const glm::mat3 & invInertia) { m_invInertia = invInertia; }
	void setMass(const float & m);

	glm::vec3 getAngVel() { return m_angVel; }
	glm::vec3 getAngAcc() { return m_angAcc; }
	//glm::mat3 getInvInertia() { return m_invInertia; }
	glm::mat3 getInvInertia() { return Body::getMesh().getRotate() * glm::mat4(m_invInertia) * glm::transpose(Body::getMesh().getRotate()); }
	void scale(const glm::vec3 & vect);


private:
	float m_density;
	glm::mat3 m_invInertia; // Inverse Inertia
	glm::vec3 m_angVel; // angular velocity
	glm::vec3 m_angAcc; // angular acceleration

	// Calculates inertia. Used by setMass() and scale()
	glm::mat3 calculateInertia();
};
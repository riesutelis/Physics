#pragma once
#include <glm/glm.hpp>
#include <iostream>
#include <vector>

class Body; // forward declaration to avoid circular dependencies

class Force
{
public:
	Force() {}
	~Force() {}

	virtual glm::vec3 apply(float mass, const glm::vec3 & pos, const glm::vec3 & vel);
};


/*
** GRAVITY CLASS
*/
class Gravity : public Force
{

public:
	// constructors
	Gravity() {}
	Gravity(const glm::vec3 & gravity) { m_gravity = gravity; }

	// get and set methods
	glm::vec3 getGravity() const { return m_gravity; }
	void setGravity(glm::vec3 gravity) { m_gravity = gravity; }

	// physics
	glm::vec3 apply(float mass, const glm::vec3 & pos, const glm::vec3 & vel);

private:
	glm::vec3 m_gravity = glm::vec3(0.0f, -9.8f, 0.0f);

};

/*
** DRAG CLASS
*/
class Drag : public Force
{
public:
	Drag() {}
	glm::vec3 apply(float mass, const glm::vec3 & pos, const glm::vec3 & vel);
private:
};


class Hooke : public Force
{
public:
	Hooke() {}
	Hooke(Body * attachedTo, float ks, float kd, float rest) {
		m_ks = ks; m_kd = kd; m_rest = rest; m_attachedTo = attachedTo;
	}
	/*Hooke(Body * b1, Body * b2, float ks, float kd, float rest) {
	m_ks = ks; m_kd = kd; m_rest = rest; m_b1 = b1; m_b2 = b2;
	}*/

	// get and set methods
	// you can write these yourself as necessary

	// physics
	glm::vec3 apply(float mass, const glm::vec3 & pos, const glm::vec3 & vel);

private:
	float m_ks; // spring stiffness
	float m_kd; // damping coefficient
	float m_rest; // spring rest length

	Body * m_attachedTo; // pointer to the body connected to one extremity of the spring
						 //	Body * m_b1; // pointer to the body connected to one extremity of the spring
						 //	Body * m_b2; // pointer to the body connected to the other extremity

};


class SurfaceDrag : public Force
{
public:
	SurfaceDrag() {}
	SurfaceDrag(std::vector<Body*> attachedTo, glm::vec3 * wind) {
		m_attachedTo = attachedTo; m_wind = wind;
	}


	// get and set methods
	// you can write these yourself as necessary

	// physics
	glm::vec3 apply(float mass, const glm::vec3 & pos, const glm::vec3 & vel);

private:

	//Body * m_self; // pointer the particle the force is applied to
	glm::vec3 * m_wind;
	std::vector<Body*> m_attachedTo; // pointer to other particles attached to this one

};


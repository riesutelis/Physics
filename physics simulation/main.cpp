#pragma once
// Math constants
#define _USE_MATH_DEFINES
#include <cmath>  
#include <random>

// Std. Includes
#include <string>
#include <time.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include "glm/ext.hpp"


// Other Libs
#include "SOIL2/SOIL2.h"

// project includes
#include "Application.h"
#include "Shader.h"
#include "Mesh.h"
#include "Body.h"
#include "RigidBody.h"
#include "Particle.h"


// time
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Struckt for storing spring values
struct spring
{
	float ks;
	float kd;
	float rest;
};



// Constants
const glm::vec3 acc_g = glm::vec3(0.0f, -9.8f, 0.0f);
//const float bounce_damper = 0.96f;
//const spring sp = { 15.0f, 1.0f, 0.3f };	// values for stiffnes, damper, rest distance




// Integrate using semi-implicit Euler integration
void integrate(Particle &p, double dt)
{
	p.setVel(p.getVel() + p.getAcc() * dt);
	p.translate(p.getVel() * dt);
}




// Integrate using semi-implicit Euler integration
void integrate(RigidBody &rb, double dt)
{
	// Angular components
	rb.setAngVel(rb.getAngVel() + rb.getAngAcc() * dt);
	// Create a skew-simetric matrix for w
	glm::mat3 angVelSkew = glm::matrixCross3(rb.getAngVel());
	// Create 3x3 rotation matrix from rb rotation matrix
	glm::mat3 R = glm::mat3(rb.getRotate());
	// update rotation matrix
	R += dt * angVelSkew * R;
	R = glm::orthonormalize(R);
	rb.setRotate(glm::mat4(R));
	// Non-angular components
	rb.setVel(rb.getVel() + rb.getAcc() * dt);
	rb.translate(rb.getVel() * dt);
}



// Apply impulse / rb - what to apply the impulse to / impulseMagnitude - magnitude of the impulse / r - point of contact - center of mass / normal - collision surface normal
void applyImpulse(RigidBody &rb, float impulseMagnitude, glm::vec3 r, glm::vec3 normal)
{
	rb.setAngVel(rb.getAngVel() + impulseMagnitude * rb.getInvInertia() * glm::cross(r, normal));
	rb.setVel(rb.getVel() + impulseMagnitude / rb.getMass() * normal);
}



// Calculates and applies the collision impluse with friction
void applyGroundCollision(RigidBody &rb, glm::vec3 colPoint)
{
	glm::vec3 r = colPoint - rb.getPos();
	// Calculate relative velocity
	glm::vec3 vr = rb.getVel() + glm::cross(rb.getAngVel(), r);

	// Reduce collision response and reduce spinning when the whelocity of an object is below a threshold
	float cor = rb.getCor();
	if (glm::length2(vr) < 1.0f && glm::length2(rb.getVel()) < 0.64f)
	{
		cor *= glm::length2(vr);
		if (glm::length2(vr) < 0.01f && glm::length2(rb.getVel()) < 0.01f)
			cor = 0.0f;
		// Recalculate relative velocity
		//	tmp = rb.getVel() + glm::cross(rb.getAngVel(), r);

		// If the velocity og the object is below a threshold, make it static
		if (glm::length2(rb.getVel()) < 0.025f && glm::length2(rb.getAngVel()) < 0.025f)
			rb.isStatic = true;
	}
	// Calculate magnitude of the impulse
	float jr = -1.0f * (1.0f + cor) * glm::dot(vr, glm::vec3(0.0f, 1.0f, 0.0f));
	jr /= 1.0f / rb.getMass() + glm::dot(glm::vec3(0.0f, 1.0f, 0.0f), glm::cross(rb.getInvInertia() * glm::cross(r, glm::vec3(0.0f, 1.0f, 0.0f)), r));
	// Apply the impulse
	applyImpulse(rb, jr, r, glm::vec3(0.0f, 1.0f, 0.0f));

	// Coulomb's friction model
	// Calculate tangental velocity
	glm::vec3 vt = vr - glm::dot(vr, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec3(0.0f, 1.0f, 0.0f);
	float vtl = glm::length(vt);
	// Calculate tangental impulse
	vt = -0.6f * abs(jr) * vt / glm::length(vt);
	// Calculate max friction
	float jtmax = vtl * rb.getMass() + glm::length(rb.getAngVel()) / glm::length(rb.getInvInertia() * glm::cross(r, vt / glm::length(vt)));
	if (glm::length2(vt) > jtmax * jtmax)
	{
		vt = vt / glm::length(vt) * jtmax;
		vtl = jtmax;
	}
	if (glm::length2(vt))
		applyImpulse(rb, glm::length(vt), r, vt / glm::length(vt));
}



// main function
int main()
{
	// create application
	Application app = Application::Application();
	app.initRender();
	Application::camera.setCameraPosition(glm::vec3(0.0f, 5.0f, 15.0f));

	// create ground plane
	Mesh plane = Mesh::Mesh();
	// scale it up x5
	plane.scale(glm::vec3(5.0f, 5.0f, 5.0f));
	//plane.setShader(Shader("resources/shaders/core.vert", "resources/shaders/core.frag"));
	plane.setShader(Shader("resources/shaders/physics.vert", "resources/shaders/physics.frag"));


	// Make a shader to assign to particles
	//Shader shader_green = Shader("resources/shaders/core.vert", "resources/shaders/core_blue.frag");
	Shader shader_green = Shader("resources/shaders/physics.vert", "resources/shaders/physics_green.frag");
	Shader shader_yellow = Shader("resources/shaders/core.vert", "resources/shaders/core_yellow.frag");

	std::vector<RigidBody> physicsObjects = std::vector<RigidBody>();

	// Debug particle
	Particle debugParticle = Particle();
	debugParticle.getMesh().setShader(shader_yellow);

	// Rigid bodies
	physicsObjects.push_back(RigidBody());
	physicsObjects[0].setMesh(Mesh(Mesh::CUBE));
	physicsObjects[0].getMesh().setShader(shader_green);
	physicsObjects[0].setMass(2.0f);
	physicsObjects[0].scale(glm::vec3(2.0f, 6.0f, 2.0f));
	physicsObjects[0].setPos(glm::vec3(-4.0f, 5.0f, 0.0f));
	physicsObjects[0].setCor(1.0f);


	physicsObjects[0].setPos(glm::vec3(0.0f, 5.0f, 0.0f));
	//	physicsObjects[0].setCor(1.0f);
	//	physicsObjects[0].setAngVel(glm::vec3(0.0f, 0.0f, 1.5f));
	//	physicsObjects[0].setCor(0.7f);
	//	physicsObjects[0].setAngVel(glm::vec3(0.1f, 0.1f, 0.1f));
	//	physicsObjects[0].rotate(M_PI_2, glm::vec3(1.0f, 0.0f, 0.0f));
	//	physicsObjects[0].setVel(glm::vec3(-18.0f, 0.0f, 0.0f));
	//	physicsObjects[0].setPos(glm::vec3(8.0f, 3.0f, 0.0f));
	physicsObjects[0].setAngVel(glm::vec3(0.1f, 0.1f, 0.1f));

	physicsObjects[0].setCor(0.5f);
	physicsObjects[0].addForce(new Gravity());


	// Room corners
	glm::vec3 roomCorner1 = glm::vec3(-10.0f, 0.0f, -10.0f);
	glm::vec3 roomCorner2 = glm::vec3(10.0f, 10.0f, 10.0f);



	////////////////////////////////////////////////////////////////////////////////
	// Variables for storing temporary values
	glm::vec3 tmp = glm::vec3(0.0f, 0.0f, 0.0f);

	// Time stuff
	double time = 0.0;
	double dt = 0.01;
	double currentTime = (double)glfwGetTime();
	double timeAccumulator = 0.0;

	// State variables to store positions for interpolation
	std::vector<glm::vec3> prevState;
	std::vector<glm::vec3> currState;
	for (RigidBody &rb : physicsObjects)
		prevState.push_back(rb.getPos());
	for (RigidBody &rb : physicsObjects)
		currState.push_back(rb.getPos());

	double timeFromStart = 0.0f;
	///////////////////////////////////////////////////////////////////////////////



	// Game loop
	while (!glfwWindowShouldClose(app.getWindow()))
	{
		// Reseting the positions of particles to be calculated on full steps
		for (int i = 0; i < currState.size(); i++)
			physicsObjects[i].setPos(currState[i]);

		// Timekeeping
		double newTime = (double)glfwGetTime();
		double frameTime = newTime - currentTime;
		timeFromStart += frameTime;
		//frameTime *= 1.5f;	//////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (frameTime > 0.25)
			frameTime = 0.25;
		currentTime = newTime;
		timeAccumulator += frameTime;


		// Do fixed updates while time available
		while (timeAccumulator >= dt)
		{
			// Update prevState
			for (int i = 0; i < physicsObjects.size(); i++)
				prevState[i] = physicsObjects[i].getPos();

			// Apply all forces before integrating
			for (RigidBody &rb : physicsObjects)
			{
				if (rb.isStatic)
					continue;
				rb.setAcc(rb.applyForces(rb.getPos(), rb.getVel()));
			}

			int i = 0;
			// Integrate
			for (RigidBody &rb : physicsObjects)
			{
				// Ignore objects marked as static
				if (rb.isStatic)
					continue;

				integrate(rb, dt);
			}
			// Collisions
			for (RigidBody &rb : physicsObjects)
			{
				// Collision detection
				int nOfCollisions = 0;
				std::vector<glm::vec3> collisionPoints = std::vector<glm::vec3>();
				std::vector<glm::vec3> worldVertices = std::vector<glm::vec3>();
				for (int j = 0; j < rb.getMesh().getVertices().size(); j++)
				{
					worldVertices.push_back(rb.getMesh().getModel() * glm::vec4(rb.getMesh().getVertices()[j].getCoord(), 1.0f));
					worldVertices[j] /= (rb.getMesh().getModel() * glm::vec4(rb.getMesh().getVertices()[j].getCoord(), 1.0f)).w;
				}
				// Move the rigid body above gorund
				tmp = glm::vec3(0.0f);
				for (glm::vec3 wv : worldVertices)
					if (wv[1] < tmp[1])
						tmp = wv;
				if (tmp[1] < 0.0f)
					rb.translate(glm::vec3(0.0f, -tmp[1], 0.0f));
				// Count number of collisions
				for (glm::vec3 wv : worldVertices)
					if (wv[1] <= 0.0f)
					{
						collisionPoints.push_back(wv);
						nOfCollisions++;
					}
				tmp = glm::vec3(0.0f);
				for (glm::vec3 colPoint : collisionPoints)
					tmp += colPoint;
				tmp /= nOfCollisions;
				glm::vec3 colMidpoint = tmp;

				// Apply impulses to collision points
				if (nOfCollisions > 0)
				{
					applyGroundCollision(rb, colMidpoint);
				}

				currState[i] = rb.getPos();
				i++;
			}
			timeAccumulator -= dt;
			time += dt;
		}

		const double alpha = timeAccumulator / dt;
		for (int i = 0; i < physicsObjects.size(); i++)
			physicsObjects[i].setPos(alpha * prevState[i] + (1.0 - alpha) * currState[i]);

		/*
		**	INTERACTION
		*/
		// Manage interaction
		app.doMovement(frameTime);


		/*
		**	RENDER
		*/
		// clear buffer
		app.clear();
		// draw groud plane
		app.draw(plane);
		// draw particles
		for (RigidBody rb : physicsObjects)
			app.draw(rb.getMesh());

	//	app.draw(debugParticle.getMesh());
		app.display();


		// Framerate
		std::cout << 1.0f / frameTime << " fps" << std::endl;
	}


	app.terminate();

	return EXIT_SUCCESS;
}


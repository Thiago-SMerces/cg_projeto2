#include "openglwindow.hpp"

#include <imgui.h>

#include <cppitertools/itertools.hpp>
#include <glm/gtx/fast_trigonometry.hpp>

#include <glm/gtc/matrix_inverse.hpp>
#include "imfilebrowser.h"

void OpenGLWindow::handleEvent(SDL_Event& event) {
	// Mouse handler
	glm::ivec2 mousePosition;

	// Keyboard events
  	if (event.type == SDL_KEYDOWN) {
    	if (event.key.keysym.sym == SDLK_LEFT) {
    		m_panSpeed = -1.0f;
		}
      	if (event.key.keysym.sym == SDLK_RIGHT) {
        	m_panSpeed = 1.0f;
		}
		if (event.key.keysym.sym == SDLK_a) {
    		m_truckSpeed = -1.0f;
		}
      	if (event.key.keysym.sym == SDLK_d) {
        	m_truckSpeed = 1.0f;
		}
      	if (event.key.keysym.sym == SDLK_UP && m_dollySpeed < 5.0f) {
        	m_dollySpeed += 0.5f;
		}
      	if (event.key.keysym.sym == SDLK_DOWN &&  m_dollySpeed > -5.0f) {
        	m_dollySpeed -= 0.5f;
		}
      	if (event.key.keysym.sym == SDLK_SPACE) {
			m_viewMatrix = 
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
			
        	m_dollySpeed -= 1.0f;
		}
  	}
  	if (event.type == SDL_KEYUP) {
		if (event.key.keysym.sym == SDLK_LEFT && m_panSpeed < 0) {
			m_panSpeed = 0.0f;
		}
		if (event.key.keysym.sym == SDLK_RIGHT && m_panSpeed > 0) {
			m_panSpeed = 0.0f;
		}
		if (event.key.keysym.sym == SDLK_a && m_truckSpeed < 0) {
    		m_truckSpeed = 0.0f;
		}
      	if (event.key.keysym.sym == SDLK_d && m_truckSpeed > 0) {
        	m_truckSpeed = 0.0f;
		}
		if (event.key.keysym.sym == SDLK_UP && m_dollySpeed > 0) {
			m_dollySpeed -= 0.01f;
		}
		if (event.key.keysym.sym == SDLK_DOWN && m_dollySpeed < 0) {
			m_dollySpeed += 0.01f;
		}
		if (event.key.keysym.sym == SDLK_SPACE) {
			m_dollySpeed += 0.01f;
			m_trackBall.mousePress(mousePosition);
			m_trackBall.mouseRelease(mousePosition);
		}
	}

	// Mouse events
	SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

	if (event.type == SDL_MOUSEMOTION) {
		m_trackBall.mouseMove(mousePosition);
	}
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		m_trackBall.mousePress(mousePosition);
	}
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		m_trackBall.mouseRelease(mousePosition);
	}
	if (event.type == SDL_MOUSEWHEEL) {
		m_zoom += (event.wheel.y > 0 ? 0.5f : -0.5f) / 5.0f;
		m_zoom = glm::clamp(m_zoom, -1.5f, 1.0f);
	}
}

void OpenGLWindow::initializeGL() {
	// Define clear color to a light blue
  	abcg::glClearColor(61.0f / 255.0f,  255.0f / 255.0f, 255.0f / 255.0f, 1.0f);

	// Enable depth buffering
	abcg::glEnable(GL_DEPTH_TEST);

	// Create programs
	for (const auto& name : m_shaderNames) {
		const auto program{createProgramFromFile(getAssetsPath() + name + ".vert",
												getAssetsPath() + name + ".frag")};
		m_programs.push_back(program);
	}

	// Load model
	m_sonic.loadObj(getAssetsPath() + "sonic2/sonic-2.obj");
	m_friends.loadObj(getAssetsPath() + "amy/sonic-series-amy-rose.obj");
	m_floor.loadObj(getAssetsPath() + "floor2/floor.obj");

	m_sonic.setupVAO(m_programs[0]);
	m_floor.setupVAO(m_programs[0]);
  	m_friends.setupVAO(m_programs[0]);

	// Camera at (0,0,0) and looking towards the negative z
	m_viewMatrix =
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
					glm::vec3(0.0f, 1.0f, 0.0f));

	// Setup friends
	for (const auto index : iter::range(m_numFriends)) {
		auto &position{m_friendPositions.at(index)};
		auto &rotation{m_friendRotations.at(index)};

		randomizeFriend(position, rotation);
	}

	m_trianglesToDraw = m_sonic.getNumTriangles();
	restart();
}

void OpenGLWindow::randomizeFriend(glm::vec3 &position, glm::vec3 &rotation) {
	// Get random position
	// x and y coordinates in the range [-10, 10]
	// z coordinates in the range [-50, 0]
	std::uniform_real_distribution<float> distPosXY(-10.0f, 10.0f);
	std::uniform_real_distribution<float> distPosZ(-50.0f, 0.0f);

  	position = glm::vec3(distPosXY(m_randomEngine), 0.02f, distPosZ(m_randomEngine));

	// Get random rotation axis
	std::uniform_real_distribution<float> distRotAxis(-1.0f, 1.0f);

	// Loc rotation on x and z axis (0.0f), so friends only rotate "sideways"
  	rotation = glm::normalize(glm::vec3(0.0f, distRotAxis(m_randomEngine), 0.0f));
}

void OpenGLWindow::paintGL() {
  	update();

	abcg::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	abcg::glViewport(0, 0, m_viewportWidth, m_viewportHeight);

	// Use currently selected program
	const auto program{m_programs.at(m_currentProgramIndex)};
	abcg::glUseProgram(program);

	// Get location of uniform variables
	const GLint viewMatrixLoc{abcg::glGetUniformLocation(program, "viewMatrix")};
	const GLint projMatrixLoc{abcg::glGetUniformLocation(program, "projMatrix")};
	const GLint modelMatrixLoc{abcg::glGetUniformLocation(program, "modelMatrix")};
	const GLint normalMatrixLoc{abcg::glGetUniformLocation(program, "normalMatrix")};

	// Floor definitions
	glm::vec3 floor_position {0.0f, -0.15f, 0.0f};
	glm::mat4 floorMatrix{1.0f};
	
	floorMatrix = glm::translate(floorMatrix, floor_position);
	floorMatrix = glm::scale(floorMatrix, glm::vec3(15.f));
	std::uniform_real_distribution<float> distRotAxisF(-1.0f, 1.0f);

    // Set uniform variable
    abcg::glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &floorMatrix[0][0]);

    const auto floorViewMatrix{glm::mat3(m_viewMatrix)};
    const glm::mat3 normalFMatrix{glm::inverseTranspose(floorViewMatrix)};
    abcg::glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalFMatrix[0][0]);
    m_floor.render();

	// Sonic definitions
	glm::vec3 sonic_position {0.0f, 0.02f, 0.0f};
	m_sonicMatrix = glm::translate(m_sonicMatrix, sonic_position);
  	m_sonicMatrix = glm::scale(m_sonicMatrix, glm::vec3(0.4f));

	// Set uniform variables used by every scene object
	abcg::glUniformMatrix4fv(viewMatrixLoc, 1, GL_FALSE, &m_viewMatrix[0][0]);
	abcg::glUniformMatrix4fv(projMatrixLoc, 1, GL_FALSE, &m_projMatrix[0][0]);

	// Set uniform variables of the current object
	abcg::glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &m_sonicMatrix[0][0]);

	const auto modelViewMatrix{glm::mat3(m_viewMatrix * m_sonicMatrix)};
	const glm::mat3 normalMatrix{glm::inverseTranspose(modelViewMatrix)};
	abcg::glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalMatrix[0][0]);

  	m_sonic.render(m_trianglesToDraw);

	// Render each friend
	for (const auto index : iter::range(m_numFriends)) {
		const auto &position{m_friendPositions.at(index)};
		const auto &rotation{m_friendRotations.at(index)};

		// Compute model matrix of the current friend
		glm::mat4 friendMatrix{1.0f};
		friendMatrix = glm::translate(friendMatrix, position);
		friendMatrix = glm::scale(friendMatrix, glm::vec3(0.3f));
		friendMatrix = glm::rotate(friendMatrix, m_angle, rotation);

		// Set uniform variable
		abcg::glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &friendMatrix[0][0]);

		const auto friendViewMatrix{glm::mat3(m_viewMatrix)};
		const glm::mat3 normalFMatrix{glm::inverseTranspose(friendViewMatrix)};
		abcg::glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, &normalFMatrix[0][0]);

		m_friends.render();
	}

	abcg::glUseProgram(0);
}

void OpenGLWindow::paintUI() {
  	abcg::OpenGLWindow::paintUI();
	// For this app we don't need sliders or widgets, so we increase the view space
	// with a clearer window
	{
		abcg::glDisable(GL_CULL_FACE);
		abcg::glFrontFace(GL_CCW);
		const auto aspect{static_cast<float>(m_viewportWidth) /
							static_cast<float>(m_viewportHeight)};
		m_projMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 5.0f);
		static std::size_t currentIndex = 0;
		if (static_cast<int>(currentIndex) != m_currentProgramIndex) {
			m_currentProgramIndex = static_cast<int>(currentIndex);
			m_sonic.setupVAO(m_programs.at(m_currentProgramIndex));
		}
	}
}

void OpenGLWindow::resizeGL(int width, int height) {
  	m_viewportWidth = width;
  	m_viewportHeight = height;

  	m_trackBall.resizeViewport(width, height);
}

void OpenGLWindow::terminateGL() {
	m_sonic.terminateGL();
	m_floor.terminateGL();
	m_friends.terminateGL();
	for (const auto& program : m_programs) {
		abcg::glDeleteProgram(program);
	}
}

void OpenGLWindow::update() {
 	m_sonicMatrix = m_trackBall.getRotation();

  	// Wait 1 second before restarting
  	if (m_gameData.m_state != State::Playing && m_restartWaitTimer.elapsed() > 1) {
    	restart();
    	return;
  	}
    else if (m_gameData.m_state == State::Playing) {
        checkCollisions();
        m_sonicMatrix = m_trackBall.getRotation();

		// Animate angle by 90 degrees per second
		const float deltaTime{static_cast<float>(getDeltaTime())};
		m_angle = glm::wrapAngle(m_angle + glm::radians(90.0f) * deltaTime);

		// Update Sonic
		x_position += deltaTime * 2.0f * m_panSpeed;
		x_rotation += deltaTime * 2.0f * m_truckSpeed;
		m_viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f + m_zoom),
                  glm::vec3(x_rotation, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_sonicMatrix = glm::translate(m_sonicMatrix, glm::vec3(x_position, 0.0f, 0.0f));
		
		// Update friends
		for (const auto index : iter::range(m_numFriends)) {
			auto &position{m_friendPositions.at(index)};
			auto &rotation{m_friendRotations.at(index)};

			// Z coordinate increases by 5 units per second
			position.z += deltaTime * 2.0f * m_dollySpeed;

			// If this friend is behind the threshol, select a new random position and
			// orientation, and move it back to -50
			// The values are smaller than the class examples, because our normal matrix
			// applies some differences in the rendering
			if (position.z > 25.0f) {
				randomizeFriend(position, rotation);
				position.z = -50.0f;  // Back to -50
			}
		}
    }
}

void OpenGLWindow::restart() {
  	m_gameData.m_state = State::Playing;
    x_position = 0;
}

void OpenGLWindow::checkCollisions() {
  	// Careful not to reach screen limits
    if (x_position < -1.0f || x_position > +1.0f) {
			m_gameData.m_state = State::GameOver;
			m_restartWaitTimer.restart();
	  }

	// The following is still under development since the collisions are not so easily calculated
	// Check collision between sonic and friends
	for (const auto index : iter::range(m_numFriends)) {
    	auto &position{m_friendPositions.at(index)};
    
		if (x_position == position.x) {
		m_gameData.m_state = State::GameOver;
				m_restartWaitTimer.restart();
		}
	}
}
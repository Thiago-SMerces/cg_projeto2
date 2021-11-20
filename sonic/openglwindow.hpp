#ifndef OPENGLWINDOW_HPP_
#define OPENGLWINDOW_HPP_

#include "abcg.hpp"
#include "model.hpp"
#include "trackball.hpp"

#include <random>

class OpenGLWindow : public abcg::OpenGLWindow {
	protected:
		void handleEvent(SDL_Event& ev) override;
		void initializeGL() override;
		void paintGL() override;
		void paintUI() override;
		void resizeGL(int width, int height) override;
		void terminateGL() override;

	private:
		//  To suit our needs, lets only make the normal shader available
		std::vector<const char*> m_shaderNames{"normal"};
		std::vector<GLuint> m_programs;

		static const int m_numFriends{700};

		int m_currentProgramIndex{-1};
		int m_viewportWidth{};
		int m_viewportHeight{};
		int m_trianglesToDraw{};

		float m_dollySpeed{0.0f};
		float m_truckSpeed{0.0f};
		float m_panSpeed{0.0f};
		float m_angle{};
		float x_position{0.0f};
		float x_rotation{0.0f};
		float m_zoom{};

		std::default_random_engine m_randomEngine;

		std::array<glm::vec3, m_numFriends> m_friendPositions;
		std::array<glm::vec3, m_numFriends> m_friendRotations;

		std::vector<Vertex> m_vertices;
		std::vector<GLuint> m_indices;

		glm::mat4 m_sonicMatrix{1.0f};
		glm::mat4 m_viewMatrix{1.0f};
		glm::mat4 m_projMatrix{1.0f};

		abcg::ElapsedTimer m_restartWaitTimer;

		Model m_sonic;
		Model m_friends;
		Model m_floor;

		GameData m_gameData;
		
		TrackBall m_trackBall;

		void randomizeFriend(glm::vec3 &position, glm::vec3 &rotation);
		void update();
		void checkCollisions();
		void restart();
};

#endif
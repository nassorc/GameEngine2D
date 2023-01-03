#pragma once

#include "Scene.h"
#include "GameEngine.h"
#include "EntityManager.h"
#include "Physics.h"
#include <fstream>
#include <cstdlib>
#include <cmath>


// <Texture specification> ===========================
// Player GX GY CW CH SX SY SM GY B
//		GX, GY     x, y	               float float (starting position)
//		CW, CH     BoundingBox W/H     float float
//		SX         Horizontal speed    float
//		SY         Jump speed          float
//		SM         Max speed           float
//		GY         Gravity             float
//		B          Bullet animation    string

class Scene_Play : public Scene {
	struct PlayerConfig {
		float X, Y, CX, CY, SPEED, MAXSPEED, JUMP, GRAVITY;
		std::string WEAPON;
	};
protected:
	std::string              m_levelPath;
	std::shared_ptr<Entity>  m_player;
	PlayerConfig             m_playerConfig;
	bool                     m_drawTextures   = true;
	bool                     m_drawCollision  = false;
	bool                     m_drawGrid       = false;
	bool                     m_ghostPlayer    = false;
	bool                     m_editorMode     = true;
	bool                     m_captureLevel   = false;
	Physics                  m_physics;

	sf::Vector2f             m_mPos;
	sf::RectangleShape       m_cursor;

	void init(const std::string& levelPath) {
		// register action
		registerAction(sf::Keyboard::P, "PAUSE");
		registerAction(sf::Keyboard::Escape, "QUIT");
		registerAction(sf::Keyboard::T, "TOGGLE_TEXTURE");
		registerAction(sf::Keyboard::C, "TOGGLE_COLLISION");
		registerAction(sf::Keyboard::G, "TOGGLE_GRID");
		registerAction(sf::Keyboard::D, "RIGHT");
		registerAction(sf::Keyboard::A, "LEFT");
		registerAction(sf::Keyboard::W, "UP");
		registerAction(sf::Keyboard::S, "DOWN");
		registerAction(sf::Keyboard::Space, "SHOOT");
		registerAction(sf::Keyboard::Z, "CAPTURE");


		// load grid text asset
		//m_gridText.setCharacterSize(12);
		//m_gridText.setFont(m_game->assets().getFont("Tech"));

		// set font and font size


		// load level config
		loadLevel(levelPath);


	}
	sf::Vector2f gridToMidPixel(float gridX, float gridY, std::shared_ptr<Entity> entity) {
		// TODO: takes in a grid (x, y) position and an Entity
		//		 Return a vector indicating where the center position of the entity should be
		//		 You must use the entity's animation size to position it correctly
		//       The size of the grid width and height is stored in m_gridSize.x and m_gridSize.y
		//       The bottom-left corner of the animation should align with the bottom left of the grid cell
		auto& animation = entity->getComponent<CAnimation>();
		gridX = gridX * 64 + (animation.animation.getSize().x / 2);
		gridY = m_game->window().getSize().y - (gridY * 64) - (animation.animation.getSize().y / 2);
		return sf::Vector2f(gridX, gridY);
	}

	void loadLevel(const std::string& path) {
		// level specification
		// @ 18:42 tile and dec
		// reset the entity manager every time we load a level
		m_entityManager = EntityManager();
		
		std::ifstream fin(path, std::ios::in);
		std::cout << "Loading level\n";
		if (!fin) {
			std::cout << "Could not load level" << std::endl;
			exit(EXIT_FAILURE);
		}
		std::string type;
		while (fin >> type) {
			if (type == "Tile") {
				float x, y;
				std::string aniName;
				fin >> aniName >> x >> y;
				auto tile = m_entityManager.addEntity("Tile");

				auto& ani = tile->addComponent<CAnimation>(m_game->getAssets().getAnimation(aniName, true));

				// call grid to mid function. Need animation component
				sf::Vector2f pos = gridToMidPixel(x, y, tile);
				tile->addComponent<CTransform>(pos, sf::Vector2f(0.f, 0.f), 0);
				tile->addComponent<CBoundingBox>(sf::Vector2f(ani.animation.getSize().x, ani.animation.getSize().y));
				tile->addComponent<CDraggable>();
			}
			else if (type == "Dec") {
				float x, y;
				std::string aniName;
				fin >> aniName >> x >> y;
				auto tile = m_entityManager.addEntity("Tile");

				auto& ani = tile->addComponent<CAnimation>(m_game->getAssets().getAnimation(aniName, true));

				x = x * 64 + (ani.animation.getSize().x / 2);
				y = m_game->window().getSize().y - (y * 64) - (ani.animation.getSize().y / 2);

				tile->addComponent<CTransform>(sf::Vector2f(x, y), sf::Vector2f(0.f, 0.f), 0);
				tile->addComponent<CDraggable>();

			}
			else if (type == "Player") {
				float x, y, cx, cy, sx, sy, sm, gy;
				std::string b;
				fin >> x >> y >> cx >> cy >> sx >> sy >> sm >> gy >> b;
				m_playerConfig.X = x;         m_playerConfig.Y = y;        m_playerConfig.CX = cx;
				m_playerConfig.CY = cy;       m_playerConfig.SPEED = sx;   m_playerConfig.JUMP = sy;
				m_playerConfig.MAXSPEED = sm; m_playerConfig.GRAVITY = gy; m_playerConfig.WEAPON = b;
			}
		}

		spawnPlayer();
	}

	void spawnPlayer() {
		m_player = m_entityManager.addEntity("Player");
		auto& ani = m_player->getComponent<CAnimation>();

		m_player->addComponent<CInput>();
		m_player->addComponent<CAnimation>(m_game->getAssets().getAnimation("Stand", true));

		float x, y;

		x = m_playerConfig.X * 64 + (ani.animation.getSize().x / 2);
		y = m_game->window().getSize().y - (m_playerConfig.Y * 64) - (ani.animation.getSize().y / 2);

		m_player->addComponent<CTransform>(sf::Vector2f(x, y), sf::Vector2f(0.f, 0.f), 0);
		m_player->addComponent<CBoundingBox>(sf::Vector2f(m_playerConfig.CX, m_playerConfig.CY));
		m_player->addComponent<CGravity>(m_playerConfig.GRAVITY);
		m_player->addComponent<CState>();
	}

	void spawnBullet(std::shared_ptr<Entity> entity) {
		float direction = entity->getComponent<CAnimation>().animation.getSprite().getScale().x;
		auto bullet = m_entityManager.addEntity("Bullet");
		auto& animation = bullet->addComponent<CAnimation>(m_game->getAssets().getAnimation("Bullet", true));
		bullet->addComponent<CTransform>(sf::Vector2f(entity->getComponent<CTransform>().pos.x, entity->getComponent<CTransform>().pos.y), sf::Vector2f(15 * direction, 0), 0);
		bullet->addComponent<CBoundingBox>(sf::Vector2f(20, 20));
		bullet->addComponent<CLifespan>(40);
	}
	bool IsInside(sf::Vector2f pos, std::shared_ptr<Entity> e) {
		auto ePos = e->getComponent<CTransform>().pos;
		auto size = e->getComponent<CAnimation>().animation.getSize();

		float dx = fabs(pos.x - ePos.x);
		float dy = fabs(pos.y - ePos.y);

		return (dx <= size.x / 2) && (dy <= size.y / 2);
	}
	sf::Vector2f windowToWorld(const sf::Vector2f& window) const {
		// x coord of variable view is the "world" x coord of the center of the window
		auto view = m_game->window().getView();
		// world x left hand side
		float wx = view.getCenter().x - m_game->window().getSize().x / 2;
		float wy = view.getCenter().y - m_game->window().getSize().y / 2;
		return sf::Vector2f(window.x + wx, window.y + wy);
	}
	void sDoAction(const Action& action) {
		if (action.type() == "START") {
			if (action.name() == "RIGHT") m_player->getComponent<CInput>().right = true;
			else if (action.name() == "LEFT") m_player->getComponent<CInput>().left = true;
			else if (action.name() == "UP") m_player->getComponent<CInput>().up = true;
			else if (action.name() == "DOWN") m_player->getComponent<CInput>().down = true;
			else if (action.name() == "TOGGLE_COLLISION") m_drawCollision = !m_drawCollision;
			else if (action.name() == "TOGGLE_TEXTURE") m_drawTextures = !m_drawTextures;
			else if (action.name() == "SHOOT") m_player->getComponent<CInput>().shoot = true;
			else if (action.name() == "CAPTURE") m_captureLevel = !m_captureLevel;
			else if (action.name() == "LEFT_CLICK") {
				sf::Vector2f worldPos = windowToWorld(action.pos());
				worldPos.x = ((int)worldPos.x / 64);
				worldPos.y = ((int)worldPos.y / 64);
				std::cout << "Mouse Clicked " << worldPos.x << " " << worldPos.y << std::endl;

				for (auto e : m_entityManager.getEntities()) {
					// if mouse pointer is in entity and has draggable component
					if (e->hasComponent<CDraggable>() && IsInside(windowToWorld(action.pos()), e)) {
						// flip value
						e->getComponent<CDraggable>().dragging = !e->getComponent<CDraggable>().dragging;
					}
				}

			}
			else if (action.name() == "MOUSE_MOVE") {
				// lock cursor into a grid cell

				sf::Vector2f wp = windowToWorld(action.pos());

				m_mPos.x = ((int)wp.x / 64) * 64 + 32;
				m_mPos.y = ((int)wp.y / 64) * 64 + 32;
			}
		}
		else if (action.type() == "END") {
			if (action.name() == "RIGHT") m_player->getComponent<CInput>().right = false;
			if (action.name() == "LEFT") m_player->getComponent<CInput>().left = false;
			if (action.name() == "UP") m_player->getComponent<CInput>().up = false;
			if (action.name() == "DOWN") m_player->getComponent<CInput>().down = false;
			if (action.name() == "SHOOT") m_player->getComponent<CInput>().shoot = false; m_player->getComponent<CInput>().canShoot = true;

		}
	}

	void sMovement() {
		// movement, gravity, max speed, scaling, set animation state
		sf::Vector2f playerVelocity(0, m_player->getComponent<CTransform>().velocity.y);
		if (m_player->getComponent<CInput>().up && m_player->getComponent<CInput>().canJump) {
			m_player->getComponent<CInput>().canJump = false;
			m_player->getComponent<CState>().state = "Jumping";
			playerVelocity.y = m_playerConfig.JUMP;
		}
		if (m_player->getComponent<CInput>().down) {
			playerVelocity.y = 10;
		}
		if (m_player->getComponent<CInput>().right) {
			playerVelocity.x = 10;
			m_player->getComponent<CAnimation>().animation.getSprite().setScale(sf::Vector2f(1, 1));
			if (m_player->getComponent<CInput>().canJump) m_player->getComponent<CState>().state = "Running";
		}
		else if (m_player->getComponent<CInput>().left) {
			playerVelocity.x = -10;
			m_player->getComponent<CAnimation>().animation.getSprite().setScale(sf::Vector2f(-1, 1));
			if (m_player->getComponent<CInput>().canJump) m_player->getComponent<CState>().state = "Running";

		}
		if (m_player->getComponent<CInput>().shoot && m_player->getComponent<CInput>().canShoot) {
			spawnBullet(m_player);
			m_player->getComponent<CInput>().canShoot = false;
		}
		//transform.prevPos.x = transform.pos.x;

		m_player->getComponent<CTransform>().prevPos = m_player->getComponent<CTransform>().pos;
		m_player->getComponent<CTransform>().velocity = playerVelocity;

		for (auto e : m_entityManager.getEntities()) {
			if (e->hasComponent<CGravity>()) {
				e->getComponent<CTransform>().velocity.y += e->getComponent<CGravity>().gravity;
				if (e->getComponent<CTransform>().velocity.y > m_playerConfig.MAXSPEED) {
					e->getComponent<CTransform>().velocity.y = m_playerConfig.MAXSPEED;
				}
			}
			e->getComponent<CTransform>().pos += e->getComponent<CTransform>().velocity;
		}
	}
	
	void sCollision() {
		// Hint @ 1:32:00
		// REMEMBER: SFML's (0,0) position if on the top left corner
		//			 Jumping will have a negative y component
		//			 gravity will have a positive y component
		//			 something below something else will have a y value greater than it
		//			 something above something else will have have a y value less than it

		// TODO: implement physics::getoverlap() function, use it inside this function

		// TODO: implement bullet / tile collisions
		//		 Destroy the tile if it has a brick animation
		// TODO: Implement player / tile collisions and resolutions
		//		 update the cstate component of the player to store whether it is
		//		 currently on the ground or in the air. This will be used by the animation system
		// TODO: check to see if the player has fallen down a holse (y > height())
		// TODO: Don't let the player walk of the left side of the map

		// player collision
		for (auto& e : m_entityManager.getEntities()) {
			if (e->tag() == "Player") continue;
			if (e->tag() == "Bullet") continue;
			if (!e->hasComponent<CBoundingBox>()) continue;

			sf::Vector2f overlap = m_physics.GetOverlap(m_player, e);
			if (overlap.x > 0 && overlap.y > 0) {
				auto& transform = m_player->getComponent<CTransform>();
				sf::Vector2f prevOverlap = m_physics.GetPreviousOverlap(m_player, e);
				if (prevOverlap.x > 0 && e->getComponent<CTransform>().pos.y > transform.pos.y) {
					// top box collision
					transform.pos.y -= overlap.y;
					transform.velocity.y = 0;
					if (transform.velocity.x == 0 && transform.velocity.y == 0) {
						m_player->getComponent<CState>().state = "Standing";
					}
					m_player->getComponent<CInput>().canJump = true;

				}
				else if (prevOverlap.x > 0 && e->getComponent<CTransform>().pos.y < transform.pos.y) {
					// bottom box collision
					transform.velocity.y = 0; // gravity
					transform.pos.y += overlap.y;

					if (e->getComponent<CAnimation>().animation.getName() == "Question") {
						// add coin
						auto coin = m_entityManager.addEntity("Tile");
						auto& ani = coin->addComponent<CAnimation>(m_game->getAssets().getAnimation("Coin", true));
						sf::Vector2f pos(0,0);
						pos.x = e->getComponent<CTransform>().pos.x;
						pos.y = e->getComponent<CTransform>().pos.y - 64;
						coin->addComponent<CTransform>(pos, sf::Vector2f(0.f, 0.f), 0);

						// replace question block animation with question2
						e->addComponent<CAnimation>(m_game->getAssets().getAnimation("Question2", true));
				
					}
				}
				else if (prevOverlap.y > 0 && e->getComponent<CTransform>().pos.x > transform.pos.x) {
					// right box collision
					transform.pos.x -= overlap.x;
				}
				else if (prevOverlap.y > 0 && e->getComponent<CTransform>().pos.x < transform.pos.x) {
					// left box collision
					transform.pos.x += overlap.x;
				}
			}
		}

		if (m_player->getComponent<CTransform>().pos.x <= m_player->getComponent<CBoundingBox>().halfSize.x) {
			m_player->getComponent<CTransform>().pos.x = m_player->getComponent<CBoundingBox>().halfSize.x;
		}
		if (m_player->getComponent<CTransform>().pos.y >= m_game->window().getSize().y) {
			m_player->setNotActive();
			spawnPlayer();
		}

		for (auto& b : m_entityManager.getEntities("Bullet")) {
			for (auto& e : m_entityManager.getEntities("Tile")) {
				if (e->getComponent<CAnimation>().animation.getName() == "Brick") {

					sf::Vector2f overlap = m_physics.GetOverlap(b,e);
					if (overlap.x > 0 && overlap.y > 0) {
						sf::Vector2f pos = e->getComponent<CTransform>().pos;
						b->setNotActive();
						e->setNotActive();

						auto explosion = m_entityManager.addEntity("Dec");
						auto& animation = explosion->addComponent<CAnimation>(m_game->getAssets().getAnimation("Explosion", true));
						explosion->addComponent<CTransform>(pos, sf::Vector2f(0.f, 0.f), 0);
					}
				}
			}
		}
	}
	void sAnimation() {
		// complete animation class code first

		// todo:set the animation of player based on cstate component
		// todo:for each entity with an animation, call entity->getComponent<CAnimation>().animation.update()
		//		if the animation is not repeated, and it had ended, destroy the entity

		if (m_player->getComponent<CState>().state == "Standing" && m_player->getComponent<CAnimation>().animation.getName() != "Stand") {
			float xs{ m_player->getComponent<CAnimation>().animation.getSprite().getScale().x };
			auto& animation = m_player->addComponent<CAnimation>(m_game->getAssets().getAnimation("Stand", true));
			animation.animation.getSprite().setScale(xs, 1);
		}
		if (m_player->getComponent<CState>().state == "Jumping" && m_player->getComponent<CAnimation>().animation.getName() != "Air") {
			m_player->addComponent<CAnimation>(m_game->getAssets().getAnimation("Air", true));
		}
		if (m_player->getComponent<CState>().state == "Running" && m_player->getComponent<CAnimation>().animation.getName() != "Run") {
			m_player->addComponent<CAnimation>(m_game->getAssets().getAnimation("Run", true));
		}

		for (auto& e : m_entityManager.getEntities()) {
			if (e->hasComponent<CAnimation>()) {
				e->getComponent<CAnimation>().animation.update();
			}
			if (e->getComponent<CAnimation>().animation.getName() == "Explosion" || e->getComponent<CAnimation>().animation.getName() == "Coin") {
				if (e->getComponent<CAnimation>().animation.hadEnded()) {
					e->setNotActive();
				}
			}
		}

		// code will be paired with collision and movement ^
		// system movement will change the state of the player
		// collision will detect if player if in the air or on the ground
		// if collision detects player should be running, movement will continue the logic
		/*if (m_player->getComponent<CState>().state == "Air") {
			m_player->addComponent<CAnimation>(m_game->getAssets().getAnimation("Air");
		}*/


		// one cycled animation solution
		// loop through every entities that are meant to cycle one time like coins and explosions
		// if hasEnded() == true we destroy the entity
		// hasEnded: if currentFrame == totalFrame(lastFrame)
	}

	void sRender() {
		// render implementation @ 1:32:00
		// may contain implementation of boudning box

		sf::View view(sf::FloatRect(0.f, 0.f, m_game->window().getSize().x, m_game->window().getSize().y));

		m_entityManager.update();
		m_game->window().clear(sf::Color(111, 113, 232));
		if (m_player->getComponent<CTransform>().pos.x > m_game->window().getSize().x / 2) {
			float cx{ m_player->getComponent<CTransform>().pos.x - (m_game->window().getSize().x / 2) };
			view.move(cx, 0.f);
		}

		m_game->window().setView(view);

		for (auto& e : m_entityManager.getEntities()) {
			if (e->hasComponent<CAnimation>()) {
				auto& transform = e->getComponent<CTransform>();

				//transform.pos += transform.velocity;
				e->getComponent<CAnimation>().animation.getSprite().setPosition(transform.pos.x, transform.pos.y);
				if (m_drawTextures) {
					m_game->window().draw(e->getComponent<CAnimation>().animation.getSprite());
				}

			}
		}
		if (m_drawCollision) {
			for (auto& e : m_entityManager.getEntities()) {
				if (e->hasComponent<CBoundingBox>()) {
					auto& box = e->getComponent<CBoundingBox>();
					auto& transform = e->getComponent<CTransform>();
					sf::RectangleShape rect;
					rect.setSize(sf::Vector2f(box.size.x - 1, box.size.y - 1));
					rect.setOrigin(sf::Vector2f(box.halfSize.x, box.halfSize.y));
					rect.setPosition(transform.pos.x, transform.pos.y);
					rect.setFillColor(sf::Color(0, 0, 0, 0));
					rect.setOutlineColor(sf::Color(255, 255, 255, 255));
					rect.setOutlineThickness(1);
					m_game->window().draw(rect);
				}
			}
		}

		// draw cursor
		m_cursor.setOutlineThickness(2);
		m_cursor.setOutlineColor(sf::Color(0, 0, 0));
		m_cursor.setFillColor(sf::Color(0, 0, 0, 0));
		m_cursor.setSize(sf::Vector2f(64, 64));
		m_cursor.setOrigin(32, 32);
		m_cursor.setPosition(m_mPos.x, m_mPos.y);
		m_game->window().draw(m_cursor);

		m_game->window().display();

	}

	void sLifespan() {
		// subtract n to the lifespan of all bullets
		for (auto e : m_entityManager.getEntities("Bullet")) {
			e->getComponent<CLifespan>().remaining -= 1;
			if (e->getComponent<CLifespan>().remaining == 0) {
				e->setNotActive();
			}
		}
	}

	void onEnd() {
		// end of scene
		// when the scene ends, change back to menu
		// m_game change scene
		// called when you hit escape

	}

	void sDragAndDrop() {
		for (auto e : m_entityManager.getEntities()) {
			if (e->hasComponent<CDraggable>() && e->getComponent<CDraggable>().dragging) {
				if (e->getComponent<CAnimation>().animation.getSize().x > 64 && e->getComponent<CAnimation>().animation.getSize().y > 64) {
					float dx{ e->getComponent<CAnimation>().animation.getSize().x / 64 * 2};
					float dy{ e->getComponent<CAnimation>().animation.getSize().y / 64 * 2 };

					e->getComponent<CTransform>().pos.x = m_mPos.x + e->getComponent<CAnimation>().animation.getSize().x / dx;
					e->getComponent<CTransform>().pos.y = m_mPos.y - e->getComponent<CAnimation>().animation.getSize().y / dy;
				}
				else {
					e->getComponent<CTransform>().pos = m_mPos;

				}
			}
		}
	}

	void sCaptureLevel() {
		// display all entities x and y position
		for (auto e : m_entityManager.getEntities()) {
			std::string type, animName;
			float x, y;

			type = e->tag();
			animName = e->getComponent<CAnimation>().animation.getName();
			x = (e->getComponent<CTransform>().pos.x - (e->getComponent<CAnimation>().animation.getSize().x / 2)) / 64;
			//y = (e->getComponent<CAnimation>().animation.getSize().x / 2) + (e->getComponent<CTransform>().pos.y / 64) + m_game->window().getSize().y;
			y = fabs(-(e->getComponent<CTransform>().pos.y - m_game->window().getSize().y + e->getComponent<CAnimation>().animation.getSize().x / 2) / 64);
			std::cout << type << " " << animName << " " << x << " " << y << std::endl;

		
		}
		m_captureLevel = false;
	
		//gridX = gridX * 64 + (animation.animation.getSize().x / 2);
		//                                     
		//gridY = m_game->window().getSize().y - (gridY * 64) - (animation.animation.getSize().y / 2);
		// window.y + halfsize 
	}

public:
	Scene_Play(GameEngine* gameEngine, const std::string& levelPath)
		: Scene(gameEngine)
		, m_levelPath(levelPath)
	{
		init(m_levelPath);
	}

	void update() {
		sMovement();
		sLifespan();
		sCollision();
		sAnimation();
		if (m_editorMode) {
			sDragAndDrop();
		}
		if (m_captureLevel) {
			sCaptureLevel();
		}
		sRender();
	}
};

// 1 frame befoer collision, moving dowwards will set y velocity to 10.
// SCollision will not detect any collision
// render will add the velocity to the player, and will go beyond a bounding box.
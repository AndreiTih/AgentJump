//********************************************************************************************************************************
//								Agent Jump: A Doodle Jump clone based on the playbuffer tutorial
//										https://github.com/sumo-digital-academy/playbuffer
// 
//												Written by Andrei Tihoan
//********************************************************************************************************************************

#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"
#include "MainGame.h"

const int DISPLAY_WIDTH = 1280;
const int DISPLAY_HEIGHT = 720;
const int DISPLAY_SCALE = 1;

// How many vertical pixels away from the highest platform must the player be for the game to start generating more platforms.
#define DISTANCE_UNTIL_GEN_PLAT 5000
#define NR_PLAT_PER_GENERATION 30

// How many vertical pixels below the player must a platform be for it to get destroyed.
const int PLAT_DESTROY_HEIGHT = DISPLAY_HEIGHT * 1.5;
// Chance for a platform to contain a spring
const float SPRING_CHANCE = 0.1f;

bool isCameraFollowingPlayer = true;

// Min and max x coordinate values for generated platforms.
const int GENERATE_MIN_X = 100;
const int GENERATE_MAX_X = DISPLAY_WIDTH - 100;

#define INTRO_TIME_IN_SECONDS 4
float introTimeElapsed = 0;



#define INITIAL_PLATFORM_VELOCITY_BOOST -18.0f
#define INITIAL_SPRING_VELOCITY_BOOST -25.0f
float platformVelocityBoost = INITIAL_PLATFORM_VELOCITY_BOOST;
float springVelocityBoost = INITIAL_SPRING_VELOCITY_BOOST;
int coinValue = 5000;

// Power up data: I'm guessing in a real game you'd want to place this data 
// somewhere in a text file maybe? And think up some generic system around that.

// JUMP BOOST:
// The jump applies a multiplier to both platformVelocityBoost and springVelocityBoost



// How many platforms to generate at the start of the game
#define INIT_NR_OF_PLATFORMS_GENERATED 100

// Variables that determine the difficulty of the generated platforms. i.e how many vertical pixels apart are they from one another.
#define INIT_GENERATE_PLAT_MIN_Y_DELTA 50
#define INIT_GENERATE_PLAT_Y_RANDOM_RANGE 100

int generatePlatformsMinYDelta = INIT_GENERATE_PLAT_MIN_Y_DELTA;
int generatePlatformsYRandomRange = INIT_GENERATE_PLAT_Y_RANDOM_RANGE;

// Coin generation variables
int generateCoinsMinYDelta = 1000;
int generateCoinsYRandomRange = 500;



// Highest height (lowest y coordinate) reached.
int maxHeightReached = 0;

int highestPlatformYPos = 0;
int previousHighestPlatformYPos = 0;

// y acceleration for the player GameObject.
#define GRAVITY 0.2f







struct Player
{
	float maxSpeed = 4.25f;
	float inputAcceleration = 1.0f;

	const Vector2D startingPos = { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 };
	
	// WARING: There's this unspoken coupling between this array and the PowerUpType enum. 
	// It is assumed that the enum identifying the powerup also serves as an index into this array
	// Take care when adding extra powerups to add them in the right order
	PowerUp powerUps[NR_POWERUPS] =
	{
		{ PowerUp::PowerUpType::TYPE_JUMP_BOOST,0 , MAX_COUNT_POWER_UP_JUMP, POWER_UP_JUMP_COLLISION_RADIUS, SPAWN_CHANCE_POWER_UP_JUMP, "star","star"}
	};
};
Player g_player;


struct GameState
{
	int highScore = 0;
	int score = 0;
	int coinsCollected = 0;
	// Represents the position the camera attempts to get at. Each frame the camera will get closer to the cameraTarget.
	// This is so that the camera doesn't directly follow the player but kind of animates towards them.
	Point2f cameraTarget = { 0,0 };
	GameStateEnum state = STATE_INTRO;
};
GameState g_gameState;




// The entry point for a PlayBuffer program
void MainGameEntry(int, char* [])
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::CreateGameObject(TYPE_PLAYER, g_player.startingPos, 50, "agent8_fall");
	Play::CreateGameObject(TYPE_GUI, { 0,0 }, 0, "star");

	StartGame(STATE_INTRO);
}

void StartGame(GameStateEnum initialGameState)
{
	g_gameState.state = initialGameState;

	// Reset score related variables
	g_gameState.score = 0;
	g_gameState.coinsCollected = 0;
	maxHeightReached = 0;

	ResetPlayerPowerUps();

	// Reset generation related variables
	generatePlatformsMinYDelta = INIT_GENERATE_PLAT_MIN_Y_DELTA;
	generatePlatformsYRandomRange = INIT_GENERATE_PLAT_Y_RANDOM_RANGE;


	for (int platID : Play::CollectGameObjectIDsByType(TYPE_PLATFORM))
	{
		Play::DestroyGameObject(platID);
	}
	for (int coinID : Play::CollectGameObjectIDsByType(TYPE_COIN))
	{
		Play::DestroyGameObject(coinID);
	}
	for (int springID : Play::CollectGameObjectIDsByType(TYPE_SPRING))
	{
		Play::DestroyGameObject(springID);
	}

	
	GenerateNextChunk(INIT_NR_OF_PLATFORMS_GENERATED, GENERATE_MIN_X, GENERATE_MAX_X, DISPLAY_HEIGHT + 100, true);



	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	player.pos = g_player.startingPos;
	player.velocity = { 0,0 };
	player.acceleration = { 0, GRAVITY };

	isCameraFollowingPlayer = true;

	if (initialGameState == STATE_PLAY)
		Play::StartAudioLoop("music");
}



void ResetPlayerPowerUps()
{
	// Reset powerups themselves
	for (int i = 0; i < NR_POWERUPS; i++)
	{
		g_player.powerUps[i].count = 0;
	}
	// Reset player stats affected by the powerup

	// JUMP_BOOST POWERUP
	platformVelocityBoost = INITIAL_PLATFORM_VELOCITY_BOOST;
	springVelocityBoost   = INITIAL_SPRING_VELOCITY_BOOST;

	// ETC...
}

// Update the player's stats according to the current powerups they have
void ApplyPowerUps()
{
	// JUMP_BOOST POWERUP
	PowerUp& jumpPowerUp = g_player.powerUps[PowerUp::PowerUpType::TYPE_JUMP_BOOST];
	platformVelocityBoost = INITIAL_PLATFORM_VELOCITY_BOOST * jumpPowerUp.jump_multipliers[jumpPowerUp.count - 1];
	springVelocityBoost   = INITIAL_SPRING_VELOCITY_BOOST   * jumpPowerUp.jump_multipliers[jumpPowerUp.count - 1];

	// SOME OTHER POWERUP...
}



// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	Play::DrawBackground();

	UpdatePlayer();
	UpdateCamera();
	UpdatePlatformsAndSprings();
	UpdateCoinsAndStars();
	UpdatePowerUps();
	UpdateIntroTimer(elapsedTime);
	CheckDeathCondition();
	UpdateLoseScreen();
	UpdateScore();
	UpdateGUI();
	UpdatePlatformGeneration();

	DrawAllObjects();

	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

void DrawAllObjects()
{
	// Add the rest here and refactor the drawings out of the other functions.
	DrawAllObjectsOfType(TYPE_POWERUP);
}


void DrawAllObjectsOfType(GameObjectType type)
{
	for (int ob_id : Play::CollectGameObjectIDsByType(type))
	{
		Play::DrawObject(Play::GetGameObject(ob_id));
	}
}

void UpdateIntroTimer(float elapsedTime)
{
	if (g_gameState.state != STATE_INTRO)
		return;

	introTimeElapsed += elapsedTime;

	Play::DrawFontText("64px", "GAME STARTS IN ... " + std::to_string((int)(INTRO_TIME_IN_SECONDS - introTimeElapsed)) + "seconds",
		{ DISPLAY_WIDTH / 2 , DISPLAY_HEIGHT / 2 - 200 }, Play::CENTRE);

	if (introTimeElapsed >= INTRO_TIME_IN_SECONDS)
	{
		g_gameState.state = STATE_PLAY;
		Play::StartAudioLoop("music");
		introTimeElapsed = 0;
	}
}
void UpdateCamera()
{
	if (isCameraFollowingPlayer)
	{
		GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
		// First update the cameraTarget.
		g_gameState.cameraTarget.y = player.pos.y - (DISPLAY_HEIGHT / 2);

		// Then interpolate the real camera towards the target camera
		float deltaY = (g_gameState.cameraTarget.y - Play::cameraPos.y ) / 4;
		Play::cameraPos.y += deltaY;
	}
}

// If the player is close enough to the highest platform, generate more platforms.
void UpdatePlatformGeneration()
{
	if (Play::GetGameObjectByType(TYPE_PLAYER).pos.y - highestPlatformYPos < DISTANCE_UNTIL_GEN_PLAT)
	{
		GenerateNextChunk(NR_PLAT_PER_GENERATION, GENERATE_MIN_X, GENERATE_MAX_X, highestPlatformYPos, false);
	}
}

void UpdateScore()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	if (player.pos.y < maxHeightReached)
	{
		g_gameState.score = -player.pos.y + g_gameState.coinsCollected * coinValue;
		maxHeightReached = player.pos.y;
	}
}

void UpdateGUI()
{
	Play::drawSpace = Play::SCREEN;
	// Score GUI:
	Play::DrawFontText("64px", "Score: " + std::to_string(g_gameState.score) + " Highest Score: " + std::to_string(g_gameState.highScore),
		{ 30 , 30 }, Play::LEFT);
	
	// Use this object to draw stuff on screen
	GameObject& brush_obj = Play::GetGameObjectByType(TYPE_GUI);
	
	// Power ups GUI:

	// Iterate through all power ups
	int xOffset = 0;
	for (int i = 0; i < NR_POWERUPS; i++)
	{
		PowerUp& powerUp = g_player.powerUps[i];
		for (int j = 0; j < powerUp.count; j++)
		{
			xOffset -= 50;
			brush_obj.spriteId = PlayGraphics::Instance().GetSpriteId(powerUp.gui_icon_sprite_name);
			brush_obj.pos = { DISPLAY_WIDTH + xOffset, 50};
			Play::DrawObject(brush_obj);
		}
	}
	Play::drawSpace = Play::WORLD;
}

void UpdateLoseScreen()
{
	if (g_gameState.state == STATE_DEAD)
	{
		if (g_gameState.score > g_gameState.highScore)
		{
			g_gameState.highScore = g_gameState.score;
		}

		isCameraFollowingPlayer = false;

		Play::drawSpace = Play::SCREEN;
		Play::DrawFontText("64px", "Highest Score: " + std::to_string(g_gameState.highScore),
			{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 100 }, Play::CENTRE);

		Play::DrawFontText("64px", "PRESS R TO RESTART",
			{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }, Play::CENTRE);
		Play::drawSpace = Play::WORLD;

		Play::StopAudioLoop("music");

		if (Play::KeyPressed('R'))
		{
			StartGame(STATE_PLAY);
		}
	}
}

void CheckDeathCondition()
{
	if (g_gameState.state == STATE_DEAD)
		return;

	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	if (player.velocity.y > 36)
	{
		g_gameState.state = STATE_DEAD;
		Play::PlayAudio("die");
	}
}


// The generatePlatforms variables change as the player ascends. These are the largest values they will have.
const int generatePlatformsMinYDeltaMax = 450;
const int generatePlatformsYRandomRangeMax = 150;

// The height at which the platform generation will create the most difficult platforms
const float HIGHEST_DIFFICULTY_HEIGHT = -50000;

void GeneratePlatformsAndSprings(int nrPlatforms, int minX, int maxX, int minY, bool generateInitialPlatform)
{
	int platY = minY;

	// Generate initial platform below the player
	if (generateInitialPlatform)
	{
		Play::CreateGameObject(TYPE_PLATFORM, { g_player.startingPos.x, DISPLAY_HEIGHT + 100 }, 40, "spanner");
		nrPlatforms = nrPlatforms - 1;
	}

	// Linearly adjust the vertical spacing between the platforms depending on how high up the 
	// player has gotten. This makes the game more challenging the higher up the player goes.	
	if ((minY < 0) && (minY > HIGHEST_DIFFICULTY_HEIGHT))
	{
		float heightToHighestDiffHeightRatio = minY / HIGHEST_DIFFICULTY_HEIGHT;
		generatePlatformsMinYDelta = heightToHighestDiffHeightRatio * generatePlatformsMinYDeltaMax;
		generatePlatformsYRandomRange = heightToHighestDiffHeightRatio * generatePlatformsYRandomRangeMax;
	}

	for (int i = 0; i < nrPlatforms; i++)
	{
		int platX = rand() % (maxX - minX) + minX;
		int deltaY = rand() % generatePlatformsYRandomRange + generatePlatformsMinYDelta;
		platY = platY - deltaY;

		Play::CreateGameObject(TYPE_PLATFORM, { platX,platY }, 40, "spanner");

		if (rand() % 100 < SPRING_CHANCE * 100)
		{
			Play::CreateGameObject(TYPE_SPRING, { (platX - 40 + rand() % 80), platY - 52 }, 10, "spring");
		}

	}
	previousHighestPlatformYPos = highestPlatformYPos;
	highestPlatformYPos = platY;
}


void GenerateCoins(int minY, int maxY, int minX, int maxX)
{
	int coinY = minY;
	while (coinY > maxY)
	{
		int coinX = rand() % (maxX - minX) + minX;
		int deltaY = rand() % generateCoinsYRandomRange + generateCoinsMinYDelta;
		coinY = coinY - deltaY;

		int coinID = Play::CreateGameObject(TYPE_COIN, { coinX, coinY }, 40, "coin");

		// Remove the coin if it's inside a platform. Inefficient way of doing it since memory is being allocated
		// for the coin and then deleted. Alternatively could use the coin position and platform position
		// variables to determine if they're inside each other and not create the coin object in the first place.
		GameObject& coin = Play::GetGameObject(coinID);
		for (int platID : Play::CollectGameObjectIDsByType(TYPE_PLATFORM))
		{
			if (Play::IsColliding(Play::GetGameObject(platID), coin))
			{
				Play::DestroyGameObject(coinID);
				break;
			}
		}
	}
}

// Generates everything that is generatable: Platforms and springs, coins, powerups
// The 'chunk size' of what is generated is determined by the nr of platforms in the generation.
void GenerateNextChunk(int nrPlatforms, int minX, int maxX, int minY, bool generateInitialPlatform)
{
	// TODO: Potential refactor. All 3 of these functions do very similar things.
	// Maybe abstract these into one generic function.
	GeneratePlatformsAndSprings(nrPlatforms, minX, maxX, minY, generateInitialPlatform);
	GenerateCoins(previousHighestPlatformYPos, highestPlatformYPos, minX, maxX);	
	//TODO: Change the powerup sprite to something appropriate
	//Generate the JUMP BOOST powerup:
	GeneratePowerUp(previousHighestPlatformYPos, highestPlatformYPos, minX, maxX, g_player.powerUps[PowerUp::PowerUpType::TYPE_JUMP_BOOST]);
}

void GeneratePowerUp(int minY, int maxY, int minX, int maxX, PowerUp& powerUp)
{

	int powerUpY = minY;
	while (powerUpY > maxY)
	{
		int powerUpX = rand() % (maxX - minX) + minX;
		int deltaY = rand() % powerUp.generatePowerUpYRandomRange + powerUp.generatePowerUpMinYDelta;
		powerUpY = powerUpY - deltaY;

		int powerUpID;
		if (rand() % 100 < g_player.powerUps[PowerUp::PowerUpType::TYPE_JUMP_BOOST].spawn_chance * 100)
		{
			powerUpID = Play::CreateGameObject(TYPE_POWERUP, { powerUpX, powerUpY }, powerUp.collision_radius, powerUp.world_sprite_name);
		}
		else
		{
			continue;
		}
		
		GameObject& powerUpObj = Play::GetGameObject(powerUpID);
		powerUpObj.powerUpType = powerUp.type;
		
		//////////////////// Remove the power up if it's been spawned on top of something else /////////////////////////
		//(TODO: If you detect that it would be spawned on top of something else, maybe make an algorithm to check around for good places to spawn it)
		// Remove it if it's inside a platform
		bool isDestroyed = false;
		for (int platID : Play::CollectGameObjectIDsByType(TYPE_PLATFORM))
		{
			if (Play::IsColliding(Play::GetGameObject(platID), powerUpObj))
			{
				Play::DestroyGameObject(powerUpID);
				isDestroyed = true;
				break;
			}
		}
		if (isDestroyed)
			continue;
		// Remove it if it's inside a coin
		for (int coinID : Play::CollectGameObjectIDsByType(TYPE_COIN))
		{
			if (Play::IsColliding(Play::GetGameObject(coinID), powerUpObj))
			{
				Play::DestroyGameObject(powerUpID);
				isDestroyed = true;
				break;
			}
		}
		if (isDestroyed)
			continue;
		// Remove it if it's inside another powerup
		for (int otherPowerUpID : Play::CollectGameObjectIDsByType(TYPE_POWERUP))
		{
			if (otherPowerUpID == powerUpID)
				continue;
			if (Play::IsColliding(Play::GetGameObject(otherPowerUpID), powerUpObj))
			{
				Play::DestroyGameObject(powerUpID);
				isDestroyed = true;
				break;
			}
		}
		
	}
}

void UpdatePlatformsAndSprings()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	for (int platID : Play::CollectGameObjectIDsByType(TYPE_PLATFORM))
	{
		GameObject& platform = Play::GetGameObject(platID);

		if ((platform.pos.y - player.pos.y) > PLAT_DESTROY_HEIGHT)
		{
			Play::DestroyGameObject(platID);
		}
		else
		{
			Play::DrawObject(platform);
		}
	}
	for (int springID : Play::CollectGameObjectIDsByType(TYPE_SPRING))
	{
		GameObject& spring = Play::GetGameObject(springID);

		if ((spring.pos.y - player.pos.y) > PLAT_DESTROY_HEIGHT)
		{
			Play::DestroyGameObject(springID);
		}
		else
		{
			if (Play::IsAnimationComplete(spring))
			{
				spring.animSpeed = 0;
				spring.frame = 0;
			}

			Play::UpdateGameObject(spring);
			Play::DrawObject(spring);
		}
	}
}

void UpdateCoinsAndStars()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	for (int coinID : Play::CollectGameObjectIDsByType(TYPE_COIN))
	{
		GameObject& coin = Play::GetGameObject(coinID);
		bool hasCollided = false;
		if (Play::IsColliding(player, coin))
		{
			// Create 4 stars upon colliding with the player
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.5f)
			{
				int id = Play::CreateGameObject(TYPE_STAR, player.pos, 0, "star");
				GameObject& obj_star = Play::GetGameObject(id);
				obj_star.rotSpeed = 0.1f;
				obj_star.acceleration = { 0.0f, 0.5f };
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}
			hasCollided = true;

			// Update score: It might seem a bit silly not to just add the coinValue to the score. 
			// But because of the way the score is updated it needs to be done this way. Since the 
			// score is just a reflection of the lowest y coordinate reached, if we were to sum 
			// coinValue to the score it would get overridden in the next call to UpdateScore().
			g_gameState.coinsCollected += 1;
			// Forcing a score update since the score only updates when the player surpasses the lowest y coordinate.
			g_gameState.score = -maxHeightReached + g_gameState.coinsCollected * coinValue;
			Play::PlayAudio("collect");
		}

		if (hasCollided || ((coin.pos.y - player.pos.y) > PLAT_DESTROY_HEIGHT))
			Play::DestroyGameObject(coinID);
		else
			Play::DrawObject(coin);
	}

	for (int starID : Play::CollectGameObjectIDsByType(TYPE_STAR))
	{
		GameObject& star = Play::GetGameObject(starID);
		Play::UpdateGameObject(star);
		Play::DrawObjectRotated(star);
		if (!Play::IsVisible(star))
			Play::DestroyGameObject(starID);
	}
}


void UpdatePowerUps()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	for (int powerUpID : Play::CollectGameObjectIDsByType(TYPE_POWERUP))
	{
		GameObject& powerUpObj = Play::GetGameObject(powerUpID);
		// Only apply collision logic if you can pick up the powerUp
		if (g_player.powerUps[powerUpObj.powerUpType].count >= g_player.powerUps[powerUpObj.powerUpType].max_count)
			continue;
		
		bool hasCollided = false;
		if (Play::IsColliding(player, powerUpObj))
		{
			hasCollided = true;

			g_player.powerUps[powerUpObj.powerUpType].count++;
			Play::PlayAudio("collect");
			ApplyPowerUps();
		}
		if (hasCollided || ((powerUpObj.pos.y - player.pos.y) > PLAT_DESTROY_HEIGHT))
			Play::DestroyGameObject(powerUpID);
	}
}

void UpdatePlayer()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	if (g_gameState.state == STATE_PLAY)
	{
		HandlePlayerControls();
		HandlePlayerCollision();
	}
	else if (g_gameState.state == STATE_DEAD)
	{
		if (Play::IsVisible(player))
		{
			Play::UpdateGameObject(player);
		}
	}

	Play::DrawObject(player);
}

void HandlePlayerControls()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	if (Play::KeyDown(VK_LEFT))
	{
		player.acceleration = { -g_player.inputAcceleration , player.acceleration.y };
	}
	else if (Play::KeyDown(VK_RIGHT))
	{
		player.acceleration = { g_player.inputAcceleration, player.acceleration.y };
	}
	else
	{
		player.acceleration = { 0, player.acceleration.y };
		player.velocity = { 0 , player.velocity.y };
	}
	Play::UpdateGameObject(player);

	if (Play::IsLeavingDisplayArea(player, Play::HORIZONTAL))
		player.pos.x = player.oldPos.x;

	if (player.velocity.x > g_player.maxSpeed)
		player.velocity.x = g_player.maxSpeed;
	else if (player.velocity.x < -g_player.maxSpeed)
		player.velocity.x = -g_player.maxSpeed;
}

void HandlePlayerCollision()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	for (int springID : Play::CollectGameObjectIDsByType(TYPE_SPRING))
	{
		GameObject& spring = Play::GetGameObject(springID);
		if (Play::IsColliding(player, spring))
		{
			// if player is moving down
			if (player.velocity.y > 0)
			{
				switch (rand() % 3)
				{
				case 0:
					Play::PlayAudio("boing1");
					break;
				case 1:
					Play::PlayAudio("boing2");
					break;
				case 2:
					Play::PlayAudio("boing3");
					break;
				}
				spring.animSpeed = 1;
				player.velocity = { player.velocity.x, springVelocityBoost };
			}
		}
	}

	for (int platID : Play::CollectGameObjectIDsByType(TYPE_PLATFORM))
	{
		GameObject& platform = Play::GetGameObject(platID);
		if (Play::IsColliding(player, platform))
		{
			// player is moving down
			if (player.velocity.y > 0)
			{
				Play::PlayAudio("platformHit");
				player.velocity = { player.velocity.x, platformVelocityBoost };
			}
		}
	}
}
// Gets called once when the player quits the game 
int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

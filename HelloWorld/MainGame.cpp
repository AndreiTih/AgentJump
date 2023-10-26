//********************************************************************************************************************************
//								Agent Jump: A Doodle Jump clone based on the playbuffer tutorial
//										https://github.com/sumo-digital-academy/playbuffer
// 
//												Written by Andrei Tihoan
//********************************************************************************************************************************

#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

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

float platformVelocityBoost = -18.0f;
float springVelocityBoost = -25.0f;
int coinValue = 5000;

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

// y acceleration for the player GameObject.
#define GRAVITY 0.2f

enum GameObjectType
{
	TYPE_PLAYER = 0,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_PLATFORM,
	TYPE_SPRING
};


struct PlayerStats
{
	float maxSpeed = 4.25f;
	float inputAcceleration = 1.0f;

	const Vector2D startingPos = { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 };
};
PlayerStats playerStats;

enum GameStateEnum
{
	STATE_INTRO = 0,
	STATE_PLAY,
	STATE_DEAD,
};
struct GameState
{
	int highScore = 0;
	int score = 0;
	int coinsCollected = 0;
	GameStateEnum state = STATE_INTRO;
};
GameState gameState;


void GeneratePlatformsCoinsAndSprings(int nrPlatforms, int minX, int maxX, int minY, bool generateInitialPlatform);
void GenerateCoins(int maxPosY, int minX, int maxX, int minY);
void UpdatePlatformGeneration();
void UpdateIntroTimer(float elapsedTime);
void UpdateCamera();
void UpdatePlayer();
void HandlePlayerControls();
void HandlePlayerCollision();
void UpdatePlatformsAndSprings();
void UpdateCoinsAndStars();
void UpdateScore();
void CheckDeathCondition();
void UpdateLoseScreen();
void StartGame(GameStateEnum initialGameState);

// The entry point for a PlayBuffer program
void MainGameEntry(int, char* [])
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::CreateGameObject(TYPE_PLAYER, playerStats.startingPos, 50, "agent8_fall");

	StartGame(STATE_INTRO);
}

void StartGame(GameStateEnum initialGameState)
{
	gameState.state = initialGameState;

	// Reset score related variables
	gameState.score = 0;
	gameState.coinsCollected = 0;
	maxHeightReached = 0;

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

	GeneratePlatformsCoinsAndSprings(100, GENERATE_MIN_X, GENERATE_MAX_X, DISPLAY_HEIGHT + 100, true);

	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	player.pos = playerStats.startingPos;
	player.velocity = { 0,0 };
	player.acceleration = { 0, GRAVITY };

	isCameraFollowingPlayer = true;

	if (initialGameState == STATE_PLAY)
		Play::StartAudioLoop("music");
}


// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	Play::DrawBackground();

	UpdatePlayer();
	UpdateCamera();
	UpdatePlatformsAndSprings();
	UpdateCoinsAndStars();
	UpdateIntroTimer(elapsedTime);
	CheckDeathCondition();
	UpdateLoseScreen();
	UpdateScore();
	UpdatePlatformGeneration();

	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

void UpdateIntroTimer(float elapsedTime)
{
	if (gameState.state != STATE_INTRO)
		return;

	introTimeElapsed += elapsedTime;

	Play::DrawFontText("64px", "GAME STARTS IN ... " + std::to_string((int)(INTRO_TIME_IN_SECONDS - introTimeElapsed)) + "seconds",
		{ DISPLAY_WIDTH / 2 , DISPLAY_HEIGHT / 2 - 200 }, Play::CENTRE);

	if (introTimeElapsed >= INTRO_TIME_IN_SECONDS)
	{
		gameState.state = STATE_PLAY;
		Play::StartAudioLoop("music");
		introTimeElapsed = 0;
	}
}
void UpdateCamera()
{
	if (isCameraFollowingPlayer)
	{
		GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
		Play::cameraPos.y = player.pos.y - (DISPLAY_HEIGHT / 2);
	}
}

// If the player is close enough to the highest platform, generate more platforms.
void UpdatePlatformGeneration()
{
	if (Play::GetGameObjectByType(TYPE_PLAYER).pos.y - highestPlatformYPos < DISTANCE_UNTIL_GEN_PLAT)
	{
		GeneratePlatformsCoinsAndSprings(NR_PLAT_PER_GENERATION, GENERATE_MIN_X, GENERATE_MAX_X, highestPlatformYPos, false);
	}
}

void UpdateScore()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	if (player.pos.y < maxHeightReached)
	{
		gameState.score = -player.pos.y + gameState.coinsCollected * coinValue;
		maxHeightReached = player.pos.y;
	}

	Play::drawSpace = Play::SCREEN;
	Play::DrawFontText("64px", "Score: " + std::to_string(gameState.score) + " Highest Score: " + std::to_string(gameState.highScore),
		{ 30 , 30 }, Play::LEFT);
	Play::drawSpace = Play::WORLD;
}

void UpdateLoseScreen()
{
	if (gameState.state == STATE_DEAD)
	{
		if (gameState.score > gameState.highScore)
		{
			gameState.highScore = gameState.score;
		}

		isCameraFollowingPlayer = false;

		Play::drawSpace = Play::SCREEN;
		Play::DrawFontText("64px", "Highest Score: " + std::to_string(gameState.highScore),
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
	if (gameState.state == STATE_DEAD)
		return;

	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	if (player.velocity.y > 36)
	{
		gameState.state = STATE_DEAD;
		Play::PlayAudio("die");
	}
}


// The generatePlatforms variables change as the player ascends. These are the largest values they will have.
const int generatePlatformsMinYDeltaMax = 450;
const int generatePlatformsYRandomRangeMax = 150;

// The height at which the platform generation will create the most difficult platforms
const float HIGHEST_DIFFICULTY_HEIGHT = -50000;

void GeneratePlatformsCoinsAndSprings(int nrPlatforms, int minX, int maxX, int minY, bool generateInitialPlatform)
{
	int platY = minY;

	// Generate initial platform below the player
	if (generateInitialPlatform)
	{
		Play::CreateGameObject(TYPE_PLATFORM, { playerStats.startingPos.x, DISPLAY_HEIGHT + 100 }, 40, "spanner");
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
	GenerateCoins(highestPlatformYPos, platY, minX, maxX);

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
			gameState.coinsCollected += 1;
			// Forcing a score update since the score only updates when the player surpasses the lowest y coordinate.
			gameState.score = -maxHeightReached + gameState.coinsCollected * coinValue;
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

void UpdatePlayer()
{
	GameObject& player = Play::GetGameObjectByType(TYPE_PLAYER);
	if (gameState.state == STATE_PLAY)
	{
		HandlePlayerControls();
		HandlePlayerCollision();
	}
	else if (gameState.state == STATE_DEAD)
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
		player.acceleration = { -playerStats.inputAcceleration , player.acceleration.y };
	}
	else if (Play::KeyDown(VK_RIGHT))
	{
		player.acceleration = { playerStats.inputAcceleration, player.acceleration.y };
	}
	else
	{
		player.acceleration = { 0, player.acceleration.y };
		player.velocity = { 0 , player.velocity.y };
	}
	Play::UpdateGameObject(player);

	if (Play::IsLeavingDisplayArea(player, Play::HORIZONTAL))
		player.pos.x = player.oldPos.x;

	if (player.velocity.x > playerStats.maxSpeed)
		player.velocity.x = playerStats.maxSpeed;
	else if (player.velocity.x < -playerStats.maxSpeed)
		player.velocity.x = -playerStats.maxSpeed;
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

#pragma once

//////////////// POWER UP INFO ////////////////////////////////////////////////
// Maybe in a more sleek arhitecture all this data is stored in some file and read from disk?

#define MAX_COUNT_POWER_UP_JUMP 4
#define POWER_UP_JUMP_COLLISION_RADIUS 40.0f
#define SPAWN_CHANCE_POWER_UP_JUMP 0.2f
#define NR_POWERUPS 1 // Total nr of power ups in the game.
struct PowerUp
{
	// WARING: There's this unspoken coupling between this enum and the powerUps array in the Player class
	// holding all power ups a player has. This enum also serves as an index into that array. Take care
	// when adding extra power ups, to add them into that array into the right order.
	enum PowerUpType
	{
		TYPE_JUMP_BOOST = 0
	};

	PowerUpType type;
	unsigned int count;
	unsigned int max_count;
	float collision_radius;
	float spawn_chance;
	const char* world_sprite_name;
	const char* gui_icon_sprite_name;

	// PowerUp generation variables
	int generatePowerUpMinYDelta = 2000;
	int generatePowerUpYRandomRange = 500;


	// Only applies to the jump power up. In the future I'll refactor this so that there's a
	// PowerUpJump struct that extends the PowerUp class. But I ran into some memory complications
	// when trying to hold an array of PowerUp and introduce instances of PowerUpJump. I think
	// it's a concept called object slicing. I reckon I can get around it by holding an array of pointers
	// to PowerUp instead, but that makes it so that I have to dynamically allocate each power up. Then I
	// 'd have to make a destructor. I'll learn about these things another day.
	float jump_multipliers[MAX_COUNT_POWER_UP_JUMP] = { 1.25f,1.50f,1.75f,2.0f };
};

enum GameStateEnum
{
	STATE_INTRO = 0,
	STATE_PLAY,
	STATE_DEAD,
};

enum GameObjectType
{
	TYPE_PLAYER = 0,
	TYPE_COIN,
	TYPE_POWERUP,
	TYPE_STAR,
	TYPE_PLATFORM,
	TYPE_SPRING,
	TYPE_GUI
};


void GeneratePlatformsAndSprings(int nrPlatforms, int minX, int maxX, int minY, bool generateInitialPlatform);
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
void ResetPlayerPowerUps();
void GenerateNextChunk(int nrPlatforms, int minX, int maxX, int minY, bool generateInitialPlatform);
void GeneratePowerUp(int minY, int maxY, int minX, int maxX, PowerUp& powerUp);
void DrawAllObjects();
void DrawAllObjectsOfType(GameObjectType type);
void UpdatePowerUps();
void UpdateGUI();




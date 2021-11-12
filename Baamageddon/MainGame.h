#pragma once

///////////////////////////////////////////////////////////////////////////
// Baamageddon - A simple platform game using the PlayBuffer framework.
// Copyright 2020 Sumo Digital Limited
///////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------------------------

enum PlayState
{
	STATE_START = 0,
	STATE_APPEAR,
	STATE_PLAY,
	STATE_DEAD,
	STATE_WAIT
};

//-------------------------------------------------------------------------

enum SheepState
{
	STATE_IDLE = 0,
	STATE_WALKING,
	STATE_AIRBORNE,
};

//-------------------------------------------------------------------------

enum SheepDirection
{
	DIRECTION_LEFT = 0,
	DIRECTION_RIGHT = 1
}; 

//-------------------------------------------------------------------------

enum GameObjectType
{
	TYPE_NOONE = -1,
	TYPE_SHEEP,
	TYPE_ISLAND,
	TYPE_CLOUD,
	TYPE_DOUGHNUT,
	TYPE_SPRINKLE,
	TYPE_SPIKE,
	TYPE_WOLF,
	TYPE_BUSH,
}; 

//-------------------------------------------------------------------------

struct Platform
{
	AABB box;
	int platform_id;
};

//-------------------------------------------------------------------------

struct Spike
{
	AABB box;
	int spike_id;
};

//-------------------------------------------------------------------------

struct GameState
{
	const int jumpTimeMax = 30;
	int score = 0;
	int jumpTime = jumpTimeMax;
	bool isJumping = false;
	float jumpMultiplier = .5f;
	PlayState playState = STATE_START;
	SheepState sheepState = STATE_IDLE;
	SheepDirection sheepDirection = DIRECTION_RIGHT;
	std::vector< Platform > vPlatforms;
	std::vector< Spike > vSpikes;
	Point2f cameraTarget{ 0.0f, 0.0f };
}; 

//-------------------------------------------------------------------------

void CreatePlatforms();

void CreateSpikes();

void DrawScene();

void UpdateWolves();

void UpdateBushes();

void UpdateDoughnuts();

void UpdateSprinkles();

void HandleSpikeCollision();

void UpdateDestroyed();

void UpdateGamePlayState();

void SetAirborne(GameObject& obj_sheep);

void DrawCollisionBounds(GameObject& obj_sheep);

void DisplayDebugInfo(GameObject& obj_sheep);

void PrintMouseCoordinateList();

void TestAABBSegmentTest();

void RandomBaa();

void LoadLevel();

//-------------------------------------------------------------------------

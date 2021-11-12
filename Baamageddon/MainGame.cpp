///////////////////////////////////////////////////////////////////////////
// Baamageddon - A simple platform game using the PlayBuffer framework.
// Copyright 2020 Sumo Digital Limited
///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// List Of Implemented Features - James Heenan
// 1. Spikes
// 2. Wolves
// 3. Hold to Jump Higher
// 4. Bouncy Bush
// 5. Swinging Blade
// 6. Exit Doughnut
///////////////////////////////////////////////////////////////////////////

#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER

//-------------------------------------------------------------------------

#include "Play.h"
#include "AABB.h"
#include "MainGame.h"

//-------------------------------------------------------------------------

constexpr int DISPLAY_WIDTH = 1280;
constexpr int DISPLAY_HEIGHT = 720;
constexpr int DISPLAY_SCALE = 1;

constexpr float SHEEP_WALK_SPEED = 5.0f;
constexpr float SHEEP_JUMP_IMPULSE = 3.f;

constexpr const char* SHEEP_IDLE_LEFT_SPRITE_NAME = "spr_sheep1_idle_left";
constexpr const char* SHEEP_IDLE_RIGHT_SPRITE_NAME = "spr_sheep1_idle_right";
constexpr const char* SHEEP_WALK_LEFT_SPRITE_NAME = "spr_sheep1_walk_left";
constexpr const char* SHEEP_WALK_RIGHT_SPRITE_NAME = "spr_sheep1_walk_right";
constexpr const char* SHEEP_JUMP_LEFT_SPRITE_NAME = "spr_sheep1_jump_left";
constexpr const char* SHEEP_JUMP_RIGHT_SPRITE_NAME = "spr_sheep1_jump_right";

constexpr const char* ISLAND_A_SPRITE_NAME = "spr_island_A";
constexpr const char* ISLAND_B_SPRITE_NAME = "spr_island_B";
constexpr const char* ISLAND_C_SPRITE_NAME = "spr_island_C";
constexpr const char* ISLAND_D_SPRITE_NAME = "spr_island_D";

const Point2f SHEEP_COLLISION_HALFSIZE = { 40,40 };

constexpr const char* DOUGHNUT_SPRITE_NAME = "spr_doughnut_12";
constexpr const char* SPRINKLE_SPRITE_NAME = "spr_sprinkle";
constexpr const char* SCORE_TAB_SPRITE_NAME = "spr_score_tab";

constexpr const char* WOLF_SPRITE_NAME_LEFT = "spr_wolf_left";
constexpr const char* WOLF_SPRITE_NAME_RIGHT = "spr_wolf_right";

constexpr const char* BUSH_SPRITE_NAME = "spr_bouncy_bush";

constexpr const char* BLADE_SPRITE_NAME = "spr_swinging_blade";

constexpr const char* FINAL_SPRITE_NAME = "level_exit";

constexpr int LEFT_SCREEN_BOUND = 100;
constexpr int RIGHT_SCREEN_BOUND = DISPLAY_WIDTH - LEFT_SCREEN_BOUND;

constexpr int FLOOR_BOUND = DISPLAY_HEIGHT * 2;


//-------------------------------------------------------------------------

static GameState gameState;


//-------------------------------------------------------------------------
// The entry point for a Play program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground( "Data\\Backgrounds\\spr_background.png" );
	Play::StartAudioLoop( "soundscape" );
	Play::ColourSprite( "64px", Play::cBlack );
	LoadLevel();
	CreatePlatforms();
	CreateSpikes();
	CreateBlades();
	gameState.cameraTarget = Point2f( DISPLAY_WIDTH, DISPLAY_HEIGHT ) - Point2f( DISPLAY_WIDTH / 2.0f, DISPLAY_HEIGHT / 2.0f);
	Play::SetCameraPosition( gameState.cameraTarget );
}

//-------------------------------------------------------------------------
// Called by the PlayBuffer once for each frame of the game (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	static float fTotalGameTime = 0.f;
	fTotalGameTime += elapsedTime;

	Point2f cameraDiff = gameState.cameraTarget - Point2f( DISPLAY_WIDTH / 2.0f, DISPLAY_HEIGHT / 2.0f ) - Play::GetCameraPosition();
	Play::SetCameraPosition( Play::GetCameraPosition() + cameraDiff/8.0f );

	Play::BeginTimingBar( Play::cBlue );
	DrawScene();

	Play::ColourTimingBar( Play::cRed );
	UpdateGamePlayState();
	Play::ColourTimingBar( Play::cGreen );
	UpdateDoughnuts();
	UpdateSprinkles();
	UpdateWolves();
	UpdateBushes();
	UpdateBlades();
	HandleSpikeCollision();

	Play::SetDrawingSpace( Play::SCREEN );
	Play::DrawSprite( Play::GetSpriteId( SCORE_TAB_SPRITE_NAME ), { DISPLAY_WIDTH / 2, 35 }, 0 );
	Play::DrawFontText("64px", "SCORE: " + std::to_string(gameState.score), { DISPLAY_WIDTH / 2, 28 }, Play::CENTRE);
	Play::SetDrawingSpace( Play::WORLD );

	Play::ColourTimingBar( Play::cWhite );
	Play::DrawTimingBar( { 5, DISPLAY_HEIGHT - 15 }, { 250, 10 } );
	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

//-------------------------------------------------------------------------
// Gets called once when the player quits the game 
int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

//-------------------------------------------------------------------------
void CreatePlatforms( void )
{
	std::vector<int> vPlatforms = Play::CollectGameObjectIDsByType( TYPE_ISLAND );

	for( int id_platform : vPlatforms )
	{
		GameObject& obj_platform = Play::GetGameObject( id_platform );

		if( obj_platform.spriteId == Play::GetSpriteId( ISLAND_A_SPRITE_NAME ) )
		{
			Platform p = { { obj_platform.pos + Point2f( 24, 12 ), { 116, 15 } }, id_platform };
			gameState.vPlatforms.push_back( p );
		}

		if( obj_platform.spriteId == Play::GetSpriteId( ISLAND_B_SPRITE_NAME ) )
		{
			Platform p = { { obj_platform.pos + Point2f( 0, 10 ), { 250, 15 } }, id_platform };
			gameState.vPlatforms.push_back( p );
		}

		if( obj_platform.spriteId == Play::GetSpriteId( ISLAND_C_SPRITE_NAME ) )
		{
			Platform p = { { obj_platform.pos + Point2f( 0, 70 ), { 250, 15 } }, id_platform };
			gameState.vPlatforms.push_back( p );
		}

		if( obj_platform.spriteId == Play::GetSpriteId( ISLAND_D_SPRITE_NAME ) )
		{
			Platform p = { { obj_platform.pos + Point2f( 10, 50 ), { 200, 15 } }, id_platform };
			gameState.vPlatforms.push_back( p );
		}
	}
}

//-------------------------------------------------------------------------
void CreateSpikes(void)
{
	std::vector<int> vSpikes = Play::CollectGameObjectIDsByType(TYPE_SPIKE);

	for (int id_spike : vSpikes)
	{
		GameObject& obj_spike = Play::GetGameObject(id_spike);
		Spike s = { {obj_spike.pos , { 45, 15 } }, id_spike };
		gameState.vSpikes.push_back(s);
	}
}

//-------------------------------------------------------------------------
void CreateBlades(void)
{
	std::vector<int> vBlades = Play::CollectGameObjectIDsByType(TYPE_BLADE);

	for (int id_blade : vBlades)
	{
		GameObject& obj_blade = Play::GetGameObject(id_blade);
		Play::MoveMatchingSpriteOrigins(BLADE_SPRITE_NAME, 0, -150);
	}
}

//-------------------------------------------------------------------------
void DrawObjectsOfType( GameObjectType type )
{
	for( int id : Play::CollectGameObjectIDsByType( type ) )
	{
		GameObject& obj = Play::GetGameObject( id );
		Play::DrawSprite( obj.spriteId, obj.pos, obj.frame);
	}
}

//-------------------------------------------------------------------------
void DrawScene()
{
	Play::DrawBackground();

	Play::ColourTimingBar( Play::cYellow );

	DrawObjectsOfType( TYPE_ISLAND );
	DrawObjectsOfType( TYPE_DOUGHNUT );
	DrawObjectsOfType( TYPE_SPRINKLE );
	DrawObjectsOfType( TYPE_SPIKE );
	DrawObjectsOfType( TYPE_WOLF );
	DrawObjectsOfType( TYPE_BUSH );
}

//-------------------------------------------------------------------------
void HandlePlatformCollision(GameObject& obj_sheep)
{
	AABB sheepAABB = { obj_sheep.pos, SHEEP_COLLISION_HALFSIZE };

	int hitCount = 0;

	for (const Platform& rPlatform : gameState.vPlatforms )
	{
		GameObject& obj_platform = Play::GetGameObject(rPlatform.platform_id);
		Vector2f positionOut;

		// Sweep X first
		if (obj_sheep.velocity.null != 0.0f && obj_sheep.velocity.y > 0.0f)
		{
			if (AABBSweepTest(rPlatform.box, sheepAABB, { obj_sheep.velocity.null, 0.f }, positionOut))
			{
				positionOut.null += obj_sheep.velocity.null * -0.5f; // bounce out to avoid ticking.
				obj_sheep.pos = positionOut;
				sheepAABB.pos = positionOut;
				obj_sheep.velocity.null = 0.f;
				obj_sheep.acceleration.null = 0.f;
				++hitCount;
			}
		}

		// When Airborne we sweep Y movement.
		if (gameState.sheepState == STATE_AIRBORNE && obj_sheep.pos.y < rPlatform.box.pos.y && obj_sheep.velocity.y > 0) //Added passing through from the under side of the platform
		{
			if (AABBSweepTest(rPlatform.box, sheepAABB, { 0.f, obj_sheep.velocity.y }, positionOut))
			{
				obj_platform.pos.y = obj_sheep.pos.y;
				obj_sheep.pos = positionOut;
				obj_sheep.pos.y += -1;
				gameState.sheepState = STATE_IDLE;
				obj_sheep.velocity.y = 0.f;
				obj_sheep.acceleration.y = 0.f;
				++hitCount;
			}
		}
		else if (gameState.sheepState != STATE_AIRBORNE)
		{
			// Check safe ground below us.
			if (AABBSweepTest(rPlatform.box, sheepAABB, { 0.f, 5.f }, positionOut))
			{
				++hitCount;
			}
		}

		// We processed a platform, jump out.
		if (hitCount > 0)
		{
			return;
		}
		Play::UpdateGameObject(obj_platform);
	}

	// We missed all platforms
	if (gameState.sheepState != STATE_AIRBORNE)
	{
		obj_sheep.velocity.y = 0.f;
		SetAirborne(obj_sheep);
	}
}

//-------------------------------------------------------------------------
//Using the same collision detection as platforms to create a better bounding box for the rectangular spikes
void HandleSpikeCollision() 
{
	//commented out code was from previous iteration using the Playbuffer radius collider
	GameObject& obj_sheep = Play::GetGameObjectByType(TYPE_SHEEP);
	AABB sheepAABB = { obj_sheep.pos, SHEEP_COLLISION_HALFSIZE };

	for (const Spike& rSpike : gameState.vSpikes)
	{
		Vector2f positionOut;
		if (AABBSweepTest(rSpike.box, sheepAABB, { obj_sheep.velocity.null, 0.f }, positionOut))
		{
			obj_sheep.acceleration += {0, -6}; //Flings Sheep up for a moment to add a bit of flair to the death
			Play::UpdateGameObject(obj_sheep);
			if (gameState.playState == STATE_PLAY)
				gameState.playState = STATE_DEAD;
		}

	}

	//for (int id_spike : vSpikes)
	//{
	//	GameObject& obj_spike = Play::GetGameObject(id_spike);

	//	if (Play::IsColliding(obj_spike, obj_sheep))
	//	{
	//		obj_sheep.acceleration += {0, -6}; //Flings Sheep up for a moment to add a bit of flair to the death
	//		Play::UpdateGameObject(obj_sheep);
	//		if(gameState.playState == STATE_PLAY)
	//			gameState.playState = STATE_DEAD;
	//	}
	//}
}

//-------------------------------------------------------------------------
//Added Debug info for Spike Collisions
//Added Debug info for distance between player and Wolves
void DrawCollisionBounds(GameObject& obj_sheep)
{
	for (const Platform& platform : gameState.vPlatforms)
	{
		DrawAABB( platform.box, Play::cBlue );
	}
	for (const Spike& spike : gameState.vSpikes)
	{
		DrawAABB( spike.box, Play::cRed );
	}
	std::vector<int> vWolves = Play::CollectGameObjectIDsByType(TYPE_WOLF);
	for (int id_wolf : vWolves)
	{
		GameObject& obj_wolf = Play::GetGameObject(id_wolf);
		Play::DrawLine(obj_sheep.pos, obj_wolf.pos, Play::cRed);
	}
	DrawAABB({ obj_sheep.pos, SHEEP_COLLISION_HALFSIZE }, Play::cBlue );
}

//-------------------------------------------------------------------------
void DisplayDebugInfo(GameObject& obj_sheep)
{
	Play::SetDrawingSpace( Play::SCREEN );

	std::ostringstream info;
	info.precision(2);
	info << std::fixed;
	info << " Playstate:" << gameState.playState;
	info << "\nSheepstate:" << gameState.sheepState;

	info << " p{ " << obj_sheep.pos.null << ", " << obj_sheep.pos.y << " } ";
	info << " v{ " << obj_sheep.velocity.null << ", " << obj_sheep.velocity.y << " } ";
	info << " a{ " << obj_sheep.acceleration.null << ", " << obj_sheep.acceleration.y << " } ";
	info << " a{ " << abs(obj_sheep.pos.null - Play::GetGameObjectByType(TYPE_WOLF).pos.null) << ", " << obj_sheep.acceleration.y << " } ";

	Play::DrawDebugText({ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 50 }, info.str().c_str());

	Play::SetDrawingSpace( Play::WORLD );
}

//-------------------------------------------------------------------------
void SetAirborne(GameObject& obj_sheep)
{
	gameState.sheepState = STATE_AIRBORNE;
}

//-------------------------------------------------------------------------
void UpdateSheep(GameObject& obj_sheep)
{
	switch (gameState.sheepState)
	{
	case STATE_IDLE:
		// fall through
	case STATE_WALKING:
	{
		if (Play::KeyDown(VK_LEFT))
		{
			obj_sheep.velocity = { -SHEEP_WALK_SPEED, 0 };
			Play::SetSprite(obj_sheep, SHEEP_WALK_LEFT_SPRITE_NAME, 1.0f);
			gameState.sheepDirection = DIRECTION_LEFT;
			gameState.sheepState = STATE_WALKING;
		}
		else if (Play::KeyDown(VK_RIGHT))
		{
			obj_sheep.velocity = { SHEEP_WALK_SPEED, 0 };
			Play::SetSprite(obj_sheep, SHEEP_WALK_RIGHT_SPRITE_NAME, 1.0f);
			gameState.sheepDirection = DIRECTION_RIGHT;
			gameState.sheepState = STATE_WALKING;
		}
		else
		{
			Play::SetSprite(obj_sheep, gameState.sheepDirection ? SHEEP_IDLE_RIGHT_SPRITE_NAME : SHEEP_IDLE_LEFT_SPRITE_NAME, 0.333f);
			obj_sheep.velocity.null *= 0.5;
			obj_sheep.acceleration = { 0, 0 };
		}

		if (Play::KeyPressed(VK_SPACE))
		{
			gameState.isJumping = true;
			Play::SetSprite(obj_sheep, gameState.sheepDirection ? SHEEP_JUMP_RIGHT_SPRITE_NAME : SHEEP_JUMP_LEFT_SPRITE_NAME, 1.f);
			obj_sheep.velocity.y = -SHEEP_JUMP_IMPULSE;
			SetAirborne(obj_sheep);
			RandomBaa();
		}

	} break;

	case STATE_AIRBORNE:
	{
		if (gameState.isJumping == false) //Code to handle whether in freefall or mid jump
			obj_sheep.acceleration = { 0.f, 0.7f };	// Gravity
		else if (Play::KeyDown(VK_SPACE) && gameState.jumpTime > 0)
		{
			
			obj_sheep.velocity.y += -SHEEP_JUMP_IMPULSE * gameState.jumpMultiplier;
			gameState.jumpMultiplier *= 0.75; //multiplier reduces the amount the veolcity increases by, tending towards 0. Gives a bit of an arc
			--gameState.jumpTime;
		}
		else
		{
			obj_sheep.rotation = 0;
			gameState.jumpMultiplier = 0.5;
			gameState.jumpTime = gameState.jumpTimeMax;
			gameState.isJumping = false;
		}
		if (Play::KeyDown(VK_LEFT))
		{
			obj_sheep.velocity.null = -SHEEP_WALK_SPEED;
			Play::SetSprite(obj_sheep, SHEEP_JUMP_LEFT_SPRITE_NAME, 1.0f);
		}
		else if (Play::KeyDown(VK_RIGHT))
		{
			obj_sheep.velocity.null = SHEEP_WALK_SPEED;
			Play::SetSprite(obj_sheep, SHEEP_JUMP_RIGHT_SPRITE_NAME, 1.0f);
		}
		if (gameState.jumpTime < 23 )
			gameState.sheepDirection ? obj_sheep.rotation += 0.25f : obj_sheep.rotation -= 0.25f;
	} break;
	};

	switch (gameState.sheepState)
	{
	case STATE_IDLE:
		obj_sheep.acceleration = { 0, 0 };
		obj_sheep.velocity = { 0, 0 };
		break;
	case STATE_WALKING:
		// check platform collision and enter idle or falling/jumping.
		break;
	case STATE_AIRBORNE:
		// if collision with platform then enter idle
		if (obj_sheep.pos.y > FLOOR_BOUND)
			gameState.playState = STATE_DEAD;
		break;
	default:
		break;
	};
	if(gameState.playState != STATE_DEAD)
		HandlePlatformCollision(obj_sheep);

	gameState.cameraTarget = obj_sheep.pos;
	
}

//-------------------------------------------------------------------------
/* Updates the Wolves that have various stages.
*	1. Idle - where we use frame 1 as if they're sitting around
*	2. Alert - Notifies the player that they're about to pounce after a certain range.
*			   A wolf will stay alert no matter what y position the player approaches from,
*			   But if they player stays below them, they will not pounce.
*	3. Pounce - Wolf jumps in an arc. During this, they will kill the sheep if they collide
*	4. Back Turned - If the player is able to get behind the wolf, they will stay idle. 
*					 Colliding during this will result in the wolf dying and the player will
*					 Gain Points
*/
void UpdateWolves()
{
	GameObject& obj_sheep = Play::GetGameObjectByType(TYPE_SHEEP);
	std::vector<int> vWolves = Play::CollectGameObjectIDsByType(TYPE_WOLF);


	static bool hasCollided = false;
	for (int id_wolf : vWolves)
	{
		GameObject& obj_wolf = Play::GetGameObject(id_wolf);
		float xDistance = abs(obj_sheep.pos.null - obj_wolf.pos.null);
		if (obj_wolf.frame != 2) 
		{
			if (xDistance > 500 || obj_sheep.pos.null > obj_wolf.pos.null)
			{
				obj_wolf.frame = 1;
				Play::UpdateGameObject(obj_wolf);
			}
			else
			{
				obj_wolf.frame = 0;
				if (xDistance < 200 && obj_wolf.pos.y + 100 > obj_sheep.pos.y)
				{
					obj_wolf.frame = 2;
				}
				Play::UpdateGameObject(obj_wolf);
			}
		}
		else if(hasCollided == false)
		{
			obj_wolf.velocity = {-7, -6};
			obj_wolf.acceleration += { 0, 0.5f };	
			if (Play::IsColliding(obj_wolf, obj_sheep))
			{
				gameState.playState = STATE_DEAD;
			}
		}
		if (Play::IsColliding(obj_wolf, obj_sheep) && hasCollided == false)
		{
			obj_wolf.acceleration = { 0 , 0.5f };
			obj_wolf.velocity.y -= 7.f;
			Play::UpdateGameObject(obj_wolf);
			gameState.score += 1000;
			hasCollided = true;
		}
		if (obj_wolf.pos.y > FLOOR_BOUND)
		{
			Play::DestroyGameObject(id_wolf);
			hasCollided = false;
		}
		Play::UpdateGameObject(obj_wolf);
	}

}

//-------------------------------------------------------------------------
void UpdateBlades()
{
	GameObject& obj_sheep = Play::GetGameObjectByType(TYPE_SHEEP);
	std::vector<int> vBlades = Play::CollectGameObjectIDsByType(TYPE_BLADE);
	std::vector<int> vNull = Play::CollectGameObjectIDsByType(TYPE_NULL_BLADE);
	for (int i = 0; i < vBlades.size(); i++)
	{
		GameObject& obj_blade = Play::GetGameObject(vBlades.at(0));
		if (gameState.null <= 2 * (PLAY_PI))
		{
			gameState.null += 0.04f;
			obj_blade.rotation += 0.04f;
		}
		else
		{
			gameState.null = 0;
			obj_blade.rotation = 0;
		}
		GameObject& obj_null = Play::GetGameObject(vNull.at(0));
		obj_null.pos = { obj_blade.pos.null + (270 * cos(gameState.null + PLAY_PI/2 )), obj_blade.pos.y + 270 * sin(gameState.null + PLAY_PI/2)};
		Play::UpdateGameObject(obj_null);
		
		Play::UpdateGameObject(obj_blade);
		Play::DrawObjectRotated(obj_blade);

		if (Play::IsColliding(obj_sheep, obj_null))
		{
			gameState.playState = STATE_DEAD;
		}
	}	
}

//-------------------------------------------------------------------------
void UpdateBushes()
{
	GameObject& obj_sheep = Play::GetGameObjectByType(TYPE_SHEEP);
	std::vector<int> vBushes = Play::CollectGameObjectIDsByType(TYPE_BUSH);

	for (int id_bush : vBushes)
	{
		GameObject& obj_bush = Play::GetGameObject(id_bush);
		
		if (Play::IsColliding(obj_bush, obj_sheep))
		{
			Play::SetSprite(obj_bush, BUSH_SPRITE_NAME, 1.0f);
			if(Play::KeyDown(VK_SPACE))
				obj_sheep.velocity.y = -32;
			else obj_sheep.velocity.y = -20;
		}
		if(Play::IsAnimationComplete(obj_bush))
		{
			Play::SetSprite(obj_bush, BUSH_SPRITE_NAME, 0.f);
			obj_bush.frame = 0;
		}
		Play::UpdateGameObject(obj_bush);
	}
	
}

//-- ----------------------------------------------------------------------
void UpdateSprinkles()
{
	std::vector<int> vSprinkles = Play::CollectGameObjectIDsByType(TYPE_SPRINKLE);

	for (int id_sprinkle : vSprinkles)
	{
		GameObject& obj_sprinkle = Play::GetGameObject(id_sprinkle);

		Play::UpdateGameObject(obj_sprinkle);

		if (!Play::IsVisible(obj_sprinkle))
			Play::DestroyGameObject(id_sprinkle);
	}
}

//-------------------------------------------------------------------------
//Added Final Doughnut logic here
void UpdateDoughnuts()
{
	GameObject& obj_sheep = Play::GetGameObjectByType(TYPE_SHEEP);
	std::vector<int> vDoughnuts = Play::CollectGameObjectIDsByType(TYPE_DOUGHNUT);

	gameState.doughnutsLeft = vDoughnuts.size();

	for (int id_doughnut : vDoughnuts)
	{
		GameObject& obj_doughnut = Play::GetGameObject(id_doughnut);
		bool hasCollided = false;

		if (Play::IsColliding(obj_doughnut, obj_sheep))
		{
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.25f)
			{
				int id = Play::CreateGameObject(TYPE_SPRINKLE, obj_sheep.pos, 0, SPRINKLE_SPRITE_NAME);
				GameObject& obj_sprinkle = Play::GetGameObject(id);
				obj_sprinkle.rotSpeed = 0.1f;
				obj_sprinkle.acceleration = { 0.0f, 0.5f };
				Play::SetGameObjectDirection(obj_sprinkle, 16, rad * PLAY_PI);

			}
			hasCollided = true;
			gameState.score += 500;
			Play::PlayAudio( "munch" );
		}

		Play::UpdateGameObject(obj_doughnut);

		if (hasCollided)
			Play::DestroyGameObject(id_doughnut);
	}

	if (gameState.doughnutsLeft == 0)
	{
		bool hasCollided = false;
		GameObject& obj_final = Play::GetGameObjectByType(TYPE_FINAL);
		Play::DrawObject(obj_final);
		Play::SetSprite(obj_final, FINAL_SPRITE_NAME, 1.f);
		Play::UpdateGameObject(obj_final);
		if (Play::IsColliding(obj_final, obj_sheep))
		{
			int sprinkles = 10;
			while (sprinkles >= 0) 
			{
				for (float rad{ 0.05f }; rad < 2.0f; rad += 0.05f)
				{
					int id = Play::CreateGameObject(TYPE_SPRINKLE, obj_final.pos, 0, SPRINKLE_SPRITE_NAME);
					GameObject& obj_sprinkle = Play::GetGameObject(id);
					obj_sprinkle.rotSpeed = 0.1f;
					obj_sprinkle.acceleration = { 0.0f, 0.5f };
					Play::SetGameObjectDirection(obj_sprinkle, 16, rad * PLAY_PI);
				}
				--sprinkles;
			}
			hasCollided = true;
			
			Play::PlayAudio("munch");
		}

		if (hasCollided)
			Play::DestroyGameObject(obj_final.GetId());
	}
}

//-------------------------------------------------------------------------
void UpdateGamePlayState()
{
	GameObject& obj_sheep = Play::GetGameObjectByType(TYPE_SHEEP);

	switch (gameState.playState)
	{
	case STATE_START:

		for( int id_obj : Play::CollectAllGameObjectIDs() )
			Play::DestroyGameObject( id_obj );

		LoadLevel();

		for( int id_doughnut : Play::CollectGameObjectIDsByType( TYPE_DOUGHNUT ) )
		{
			GameObject& obj = Play::GetGameObject( id_doughnut );
			obj.animSpeed = 0.0f;
			obj.frame = rand();
		}

		gameState.playState = STATE_APPEAR;
		return;

	case STATE_APPEAR:
		obj_sheep.velocity = { 0, 0 };
		obj_sheep.acceleration = { 0, 0.5f };
		Play::SetSprite(obj_sheep, SHEEP_JUMP_RIGHT_SPRITE_NAME, 0);
		obj_sheep.rotation = 0;
		gameState.playState = STATE_PLAY;
		gameState.sheepState = STATE_AIRBORNE;
		RandomBaa();
		break;

	case STATE_PLAY:

		UpdateSheep(obj_sheep);

		if (Play::KeyPressed(VK_DELETE))
		{
			gameState.playState = STATE_DEAD;
		}

		break;

	case STATE_DEAD:
		Play::DestroyGameObjectsByType( TYPE_DOUGHNUT );

		gameState.playState = STATE_WAIT;

		break;
	case STATE_WAIT:
		gameState.sheepState = STATE_IDLE;
		if(gameState.sheepDirection == DIRECTION_RIGHT)
			Play::SetSprite(obj_sheep, SHEEP_IDLE_RIGHT_SPRITE_NAME, 0);
		else
			Play::SetSprite(obj_sheep, SHEEP_IDLE_LEFT_SPRITE_NAME, 0);
		obj_sheep.rotation += 0.25f;
		obj_sheep.acceleration = { 0 , 0.5f };
		obj_sheep.velocity.y += 1.f;
		if (Play::KeyPressed(VK_SPACE) == true)
		{
			gameState.playState = STATE_START;
			gameState.score = 0;
		}
		break;

	} // End of switch

	Play::UpdateGameObject(obj_sheep);
	Play::DrawObjectRotated(obj_sheep);

	// Debug Visualisation
	static bool s_bEnableDebug = false;
	if (Play::KeyPressed(VK_HOME))
		s_bEnableDebug = !s_bEnableDebug;

	if (s_bEnableDebug)
	{
		DisplayDebugInfo(obj_sheep);
		DrawCollisionBounds(obj_sheep);
		TestAABBSegmentTest();
		PrintMouseCoordinateList();
	}
}

//-------------------------------------------------------------------------
void PrintMouseCoordinateList()
{
	static bool pressed = false;
	if (!pressed && Play::GetMouseButton(Play::LEFT))
	{
		char text[32];
		sprintf_s(text, 32, "{%.0f, %.0f},\n", Play::GetMousePos().null, Play::GetMousePos().y);
		OutputDebugStringA(text);
		pressed = true;
	}
	if (pressed && !Play::GetMouseButton(Play::LEFT))
	{
		pressed = false;
	}
}

//-------------------------------------------------------------------------
void TestAABBSegmentTest()
{
	static float t = 0;
	t += 0.04f;
	if (t > PLAY_PI) t -= PLAY_PI * 2.f;

	Vector2f b{ sin(t) * 200.f + 250.f, cos(t) * 200.f + 250.f };
	Vector2f a{ 400.f, 800.f };

	float t0;
	if (AABBSegmentTest(gameState.vPlatforms[0].box, a, b, t0))
	{
		Vector2f c = a + (b - a) * t0;
		Play::DrawLine(a, c, { 100,0,0 });
	}
	else
	{
		Play::DrawLine(a, b, { 100,100,0 });
	}

	AABB testBox = { a, Vector2f(20,30) };

	Vector2f constrainedPos;
	if (AABBSweepTest( gameState.vPlatforms[0].box, testBox, b - a, constrainedPos))
	{
		testBox.pos = constrainedPos;
		DrawAABB(testBox, { 100, 0, 100 });
	}

	DrawAABB(testBox, { 100, 100, 0 });

	testBox.pos = constrainedPos;
	DrawAABB(testBox, { 100, 100, 100 });
}


//-------------------------------------------------------------------------
// Loads the objects from the Baamageddon\Level.lev file
void LoadLevel( void )
{
	std::ifstream levelfile;
	levelfile.open( "Level.lev" );

	std::string sType, sX, sY, sSprite;

	std::getline( levelfile, sType );

	while( !levelfile.eof() )
	{
		std::getline( levelfile, sType );
		std::getline( levelfile, sX );
		std::getline( levelfile, sY );
		std::getline( levelfile, sSprite );

		if( sType == "TYPE_SHEEP" )
			Play::CreateGameObject( TYPE_SHEEP, { std::stof( sX ), std::stof( sY ) }, 50, sSprite.c_str() );

		if( sType == "TYPE_ISLAND" )
			Play::CreateGameObject( TYPE_ISLAND, { std::stof( sX ), std::stof( sY ) }, 0, sSprite.c_str() );

		if( sType == "TYPE_DOUGHNUT" )
			Play::CreateGameObject( TYPE_DOUGHNUT, { std::stof( sX ), std::stof( sY ) }, 30, sSprite.c_str() );

		if (sType == "TYPE_SPIKE")
			Play::CreateGameObject(TYPE_SPIKE, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str());

		if (sType == "TYPE_WOLF")
			Play::CreateGameObject(TYPE_WOLF, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str());

		if (sType == "TYPE_BUSH")
			Play::CreateGameObject(TYPE_BUSH, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str());

		if (sType == "TYPE_BLADE")
		{
			Play::CreateGameObject(TYPE_BLADE, { std::stof(sX), std::stof(sY) }, 5, sSprite.c_str());
			Play::CreateGameObject(TYPE_NULL_BLADE, { std::stof(sX), std::stof(sY) + 270 }, 70, "");
		}

		if (sType == "TYPE_FINAL")
			Play::CreateGameObject(TYPE_FINAL, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str());
			
		
	}

	levelfile.close();
}


void RandomBaa( void ) 
{
		switch( rand() % 5 )
		{
			case 0: Play::PlayAudio( "baa1" ); break;
			case 1: Play::PlayAudio( "baa2" ); break;
			case 2: Play::PlayAudio( "baa3" ); break;
			case 3: Play::PlayAudio( "baa4" ); break;
			case 4: Play::PlayAudio( "baa5" ); break;
		}
}
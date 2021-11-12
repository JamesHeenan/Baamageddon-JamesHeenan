////////////////////////////////////////////////////////////////////////////////////
// A simple platform game editor for Baamageddon using the PlayBuffer framework.
// Copyright 2020 Sumo Digital Limited
///////////////////////////////////////////////////////////////////////////////////
//
// Note that this editor loads all its sprites from the Baamageddon project 
// directory and modifies the data in Baamageddon\Level.lev to change the level.
// Debugging->Working Directory should be to be set to ..\Baamageddon
//
///////////////////////////////////////////////////////////////////////////////////
#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

constexpr int DISPLAY_WIDTH = 1280;
constexpr int DISPLAY_HEIGHT = 720;
constexpr int DISPLAY_SCALE = 1;

constexpr int SNAP_PIXELS = 32;

const Point2f HALF_DISPLAY{ DISPLAY_WIDTH / 2.0f, DISPLAY_HEIGHT / 2.0f };

constexpr const char* SHEEP_SPRITE_NAME = "spr_sheep1_idle_right";

constexpr const char* ISLAND_A_SPRITE_NAME = "spr_island_A";
constexpr const char* ISLAND_B_SPRITE_NAME = "spr_island_B";
constexpr const char* ISLAND_C_SPRITE_NAME = "spr_island_C";
constexpr const char* ISLAND_D_SPRITE_NAME = "spr_island_D";

constexpr const char* DOUGHNUT_SPRITE_NAME = "spr_doughnut_12";

constexpr const char* SPIKE_SPRITE_NAME = "spr_spikes";

constexpr const char* WOLF_SPRITE_NAME = "spr_wolf_left_3";

constexpr const char* BUSH_SPRITE_NAME = "spr_bouncy_bush_4";

constexpr const char* BLADE_SPRITE_NAME = "spr_swinging_blade";

constexpr const char* FINAL_SPRITE_NAME = "spr_invisible_marker";

constexpr int FLOOR_BOUND = DISPLAY_HEIGHT * 2;

enum GameObjectType
{
	TYPE_NOONE = -1,
	TYPE_SHEEP,
	TYPE_ISLAND,
	TYPE_DOUGHNUT,
	TYPE_SPIKE,
	TYPE_WOLF,
	TYPE_BUSH,
	TYPE_BLADE,
	TYPE_FINAL,
	TOTAL_TYPES
};

const char* SPRITE_NAMES[TOTAL_TYPES][4] =
{
	{ SHEEP_SPRITE_NAME, SHEEP_SPRITE_NAME, SHEEP_SPRITE_NAME, SHEEP_SPRITE_NAME },
	{ ISLAND_A_SPRITE_NAME, ISLAND_B_SPRITE_NAME, ISLAND_C_SPRITE_NAME, ISLAND_D_SPRITE_NAME },
	{ DOUGHNUT_SPRITE_NAME, DOUGHNUT_SPRITE_NAME, DOUGHNUT_SPRITE_NAME, DOUGHNUT_SPRITE_NAME },
	{ SPIKE_SPRITE_NAME, SPIKE_SPRITE_NAME, SPIKE_SPRITE_NAME, SPIKE_SPRITE_NAME },
	{ WOLF_SPRITE_NAME, WOLF_SPRITE_NAME, WOLF_SPRITE_NAME, WOLF_SPRITE_NAME },
	{ BUSH_SPRITE_NAME, BUSH_SPRITE_NAME, BUSH_SPRITE_NAME, BUSH_SPRITE_NAME },
	{ BLADE_SPRITE_NAME, BLADE_SPRITE_NAME, BLADE_SPRITE_NAME, BLADE_SPRITE_NAME, },
	{ FINAL_SPRITE_NAME, FINAL_SPRITE_NAME, FINAL_SPRITE_NAME, FINAL_SPRITE_NAME,}
};

struct EditorState
{
	int score = 0;
	GameObjectType editMode = TYPE_SHEEP;
	Point2f cameraTarget{ 0.0f, 0.0f };
	float zoom = 1.0f;
	int selectedObj = -1;
	Point2f selectedOffset{ 0.0f, 0.0f };
	int saveCooldown = 0;
};

EditorState editorState;

void HandleControls();
void DrawScene();
void DrawUserInterface();
bool PointInsideSpriteBounds( Point2f testPos, GameObject& obj );
void DrawObjectsOfType( GameObjectType type );
void SaveLevel();
void LoadLevel();

//-------------------------------------------------------------------------
// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground( "Data\\Backgrounds\\spr_background.png" );
	editorState.cameraTarget = HALF_DISPLAY;
	Play::ColourSprite( "64px", Play::cBlack );
	LoadLevel();
}

//-------------------------------------------------------------------------
// Called by the PlayBuffer once for each frame of the game (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	static float fTotalGameTime = 0.f;
	fTotalGameTime += elapsedTime;

	HandleControls();
	DrawScene();
	DrawUserInterface();

	Play::PresentDrawingBuffer();
	return Play::KeyDown( VK_ESCAPE );
}

//-------------------------------------------------------------------------
// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

//-------------------------------------------------------------------------
// Camera movement, object placement / deletion etc.
void HandleControls( void )
{
	const int CAMERA_SPEED = 10;

	static bool drag = false;
	static Point2f dragOrigin{ 0.0f, 0.0f };

	if( Play::KeyPressed( 'S' ) && editorState.saveCooldown < 0 )
		SaveLevel();

	if( Play::KeyDown( VK_RIGHT ) )
		editorState.cameraTarget.null += CAMERA_SPEED / editorState.zoom;

	if( Play::KeyDown( VK_LEFT ) )
		editorState.cameraTarget.null -= CAMERA_SPEED / editorState.zoom;

	if( Play::KeyDown( VK_UP ) )
		editorState.cameraTarget.y -= CAMERA_SPEED / editorState.zoom;

	if( Play::KeyDown( VK_DOWN ) )
		editorState.cameraTarget.y += CAMERA_SPEED / editorState.zoom;

	if( Play::KeyPressed( VK_OEM_MINUS ) )
		editorState.zoom -= 0.1f;

	if( Play::KeyPressed( VK_OEM_PLUS ) )
		editorState.zoom += 0.1f;

	if( editorState.zoom < 0.2f )
		editorState.zoom = 0.2f;

	if( editorState.zoom > 1.0f )
		editorState.zoom = 1.0f;

	if( Play::KeyPressed( VK_SPACE ) )
	{
		switch( editorState.editMode )
		{
			case TYPE_SHEEP: editorState.editMode = TYPE_ISLAND; break;
			case TYPE_ISLAND: editorState.editMode = TYPE_DOUGHNUT; break;
			case TYPE_DOUGHNUT: editorState.editMode = TYPE_SPIKE; break;
			case TYPE_SPIKE: editorState.editMode = TYPE_WOLF; break;
			case TYPE_WOLF: editorState.editMode = TYPE_BUSH; break;
			case TYPE_BUSH: editorState.editMode = TYPE_BLADE; break;
			case TYPE_BLADE: editorState.editMode = TYPE_FINAL; break;
			case TYPE_FINAL: editorState.editMode = TYPE_SHEEP; break;
		}
		editorState.selectedObj = -1;
	}

	Point2f mouseWorldPos = ( Play::GetMousePos() + Play::GetCameraPosition() ) / editorState.zoom;
	Point2f mouseWorldSnapPos = mouseWorldPos;
	mouseWorldSnapPos.null -= (int)mouseWorldSnapPos.null % SNAP_PIXELS;
	mouseWorldSnapPos.y -= (int)mouseWorldSnapPos.y % SNAP_PIXELS;

	if( Play::GetMouseButton( Play::LEFT ) )
	{
		if( editorState.selectedObj == -1 )
		{
			for( int id : Play::CollectGameObjectIDsByType( editorState.editMode ) )
			{
				GameObject& obj = Play::GetGameObject( id );
				if( PointInsideSpriteBounds( mouseWorldPos, obj ) )
				{
					editorState.selectedObj = obj.GetId();
					editorState.selectedOffset = obj.pos - mouseWorldSnapPos;
				}
			}

			if( editorState.selectedObj == -1 )
			{
				switch( editorState.editMode )
				{
					case TYPE_SHEEP:
						Play::GetGameObjectByType( TYPE_SHEEP ).pos = mouseWorldPos;
						break;
					case TYPE_FINAL:
						Play::GetGameObjectByType(TYPE_FINAL).pos = mouseWorldPos;
						break;
					default:
						editorState.selectedObj = Play::CreateGameObject( editorState.editMode, mouseWorldSnapPos, 50, SPRITE_NAMES[static_cast<int>( editorState.editMode )][0] );
						editorState.selectedOffset = { 0.0f, 0.0f };
						break;
				}
			}

		}
		else
		{
			GameObject& obj = Play::GetGameObject( editorState.selectedObj );
			obj.pos = mouseWorldSnapPos + editorState.selectedOffset;

			if( Play::KeyDown( '1' ) )
				obj.spriteId = Play::GetSpriteId( SPRITE_NAMES[static_cast<int>( editorState.editMode )][0] );

			if( Play::KeyDown( '2' ) )
				obj.spriteId = Play::GetSpriteId( SPRITE_NAMES[static_cast<int>( editorState.editMode )][1] );

			if( Play::KeyDown( '3' ) )
				obj.spriteId = Play::GetSpriteId( SPRITE_NAMES[static_cast<int>( editorState.editMode )][2] );

			if( Play::KeyDown( '4' ) )
				obj.spriteId = Play::GetSpriteId( SPRITE_NAMES[static_cast<int>( editorState.editMode )][3] );
		}
	}
	else
	{
		editorState.selectedObj = -1;
	}

	if( Play::GetMouseButton( Play::RIGHT ) )
	{
		for( int id : Play::CollectGameObjectIDsByType( editorState.editMode ) )
		{
			GameObject& obj = Play::GetGameObject( id );
			if( PointInsideSpriteBounds( mouseWorldPos, obj ) )
			{
				if( obj.type != TYPE_SHEEP )
					Play::DestroyGameObject( id );
			}
		}
	}

	Play::SetCameraPosition( ( editorState.cameraTarget * editorState.zoom ) - HALF_DISPLAY );
}

//-------------------------------------------------------------------------
void DrawScene( void )
{
	Play::DrawBackground();

	DrawObjectsOfType( TYPE_ISLAND );
	DrawObjectsOfType( TYPE_DOUGHNUT );
	DrawObjectsOfType( TYPE_SHEEP );
	DrawObjectsOfType( TYPE_SPIKE );
	DrawObjectsOfType( TYPE_WOLF );
	DrawObjectsOfType( TYPE_BUSH );
	DrawObjectsOfType(TYPE_BLADE);
	DrawObjectsOfType(TYPE_FINAL);

	if( editorState.selectedObj != -1 )
	{
		GameObject& obj = Play::GetGameObject( editorState.selectedObj );
		Point2f origin = Play::GetSpriteOrigin( obj.spriteId );
		Point2f size = { Play::GetSpriteWidth( obj.spriteId ), Play::GetSpriteHeight( obj.spriteId ) };
		Play::DrawRect( ( obj.pos - origin ) * editorState.zoom, ( obj.pos - origin + size ) * editorState.zoom, Play::cWhite );
		std::string s = "X:" + std::to_string( (int)( obj.pos.null + 0.5f ) ) + " / Y:" + std::to_string( (int)( obj.pos.y + 0.5f ) );
		Play::DrawDebugText( ( obj.pos - origin + Point2f( size.null / 2.0f, -10.0f / editorState.zoom ) ) * editorState.zoom, s.c_str(), Play::cWhite );
	}

}

//-------------------------------------------------------------------------
void DrawUserInterface( void )
{
	Play::SetDrawingSpace( Play::SCREEN );
	std::string sMode;
	switch( editorState.editMode )
	{
		case TYPE_SHEEP: sMode = "PLAYER"; break;
		case TYPE_ISLAND: sMode = "ISLANDS"; break;
		case TYPE_DOUGHNUT: sMode = "DONUTS"; break;
		case TYPE_SPIKE: sMode = "SPIKES"; break;
		case TYPE_WOLF: sMode = "WOLVES"; break;
		case TYPE_BUSH: sMode = "BUSHES"; break;
		case TYPE_BLADE: sMode = "SWINGING BLADE"; break;
		case TYPE_FINAL: sMode = "FINAL DONUT"; break;
	}

	Play::DrawRect( { 0, 0 }, { DISPLAY_WIDTH, 50 }, Play::cYellow, true );
	Play::DrawFontText( "64px", "MODE : " + sMode, { DISPLAY_WIDTH / 2, 25 }, Play::CENTRE );
	Play::DrawFontText( "64px", std::to_string( (int)( ( editorState.zoom * 100.0f ) + 0.5f ) ) + "%", { DISPLAY_WIDTH / 6, 25 }, Play::CENTRE );
	Play::DrawFontText( "64px", std::to_string( Play::CollectGameObjectIDsByType( editorState.editMode ).size() ) + " " + sMode, { ( DISPLAY_WIDTH * 5 ) / 6, 25 }, Play::CENTRE );

	float yBounds = FLOOR_BOUND * editorState.zoom - Play::GetCameraPosition().y;
	Play::DrawLine( { 0, yBounds }, { DISPLAY_WIDTH, yBounds }, Play::cBlack );
	Play::DrawDebugText( { DISPLAY_WIDTH / 2, yBounds + 10 }, "AUTOMATIC DEATH LEVEL", Play::cBlack );

	Play::DrawDebugText( { 20, DISPLAY_HEIGHT - 20 }, "HOLD 'H' FOR CONTROLS", Play::cBlack, false );

	if( Play::KeyDown( 'H' ) )
	{
		Play::DrawRect( { DISPLAY_WIDTH / 4, DISPLAY_HEIGHT / 4 }, { DISPLAY_WIDTH * 3 / 4, DISPLAY_HEIGHT * 3 / 4 }, Play::cBlack, true );
		Play::DrawRect( { DISPLAY_WIDTH / 4, DISPLAY_HEIGHT / 4 }, { DISPLAY_WIDTH * 3 / 4, DISPLAY_HEIGHT * 3 / 4 }, Play::cMagenta, false );
		int y = ( DISPLAY_HEIGHT / 4 ) + 50;
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y }, "LEVEL EDITOR HELP", Play::cMagenta );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 20 }, "---------------------", Play::cWhite );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 40 }, "NOTE: YOU CAN ONLY INTERACT WITH OBJECTS SPECIFIED BY THE MODE", Play::cWhite );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 40 }, "SPACE BAR = CHANGE OBJECT MODE", Play::cMagenta );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 20 }, "LEFT MOUSE BUTTON = ADD OR SELECT OBJECT", Play::cWhite );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 20 }, "LEFT MOUSE DRAG = MOVE OBJECT", Play::cMagenta );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 20 }, "LEFT MOUSE DRAG AND KEYS 1-4 = CHANGE OBJECT SPRITE", Play::cWhite );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 20 }, "RIGHT MOUSE BUTTON = DELETE OBJECT", Play::cMagenta );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 20 }, "ARROW KEYS = SCROLL", Play::cWhite );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 20 }, "PLUS AND MINUS KEYS = ZOOM IN AND OUT", Play::cMagenta );
		Play::DrawDebugText( { DISPLAY_WIDTH / 2, y += 20 }, "F1 = SHOW DEBUG INFO", Play::cWhite );
	}

	if( --editorState.saveCooldown > 0 )
	{
		Play::DrawRect( { 0, DISPLAY_HEIGHT - 50 }, { DISPLAY_WIDTH, DISPLAY_HEIGHT }, Play::cOrange, true );
		Play::DrawFontText( "64px", "OVERWRITING LEVEL", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 25 }, Play::CENTRE );
	}

	Play::SetDrawingSpace( Play::WORLD );
}


//-------------------------------------------------------------------------
void DrawObjectsOfType( GameObjectType type )
{
	for( int id : Play::CollectGameObjectIDsByType( type ) )
	{
		GameObject& obj = Play::GetGameObject( id );
		Play::DrawSpriteRotated( obj.spriteId, obj.pos * editorState.zoom, 0, 0, 1.0f * editorState.zoom );
	}
}

//-------------------------------------------------------------------------
bool PointInsideSpriteBounds( Point2f testPos, GameObject& obj )
{
	Point2f origin = Play::GetSpriteOrigin( obj.spriteId );
	Point2f size = { Play::GetSpriteWidth( obj.spriteId ), Play::GetSpriteHeight( obj.spriteId ) };
	Point2f topLeft = obj.pos - origin;
	Point2f botRight = topLeft + size;
	return testPos.null > topLeft.null && testPos.null < botRight.null&& testPos.y > topLeft.y && testPos.y < botRight.y;
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
			Play::CreateGameObject( TYPE_SPIKE, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str() );

		if (sType == "TYPE_WOLF")
			Play::CreateGameObject( TYPE_WOLF, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str());

		if (sType == "TYPE_BUSH")
			Play::CreateGameObject( TYPE_BUSH, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str() );

		if (sType == "TYPE_BLADE")
			Play::CreateGameObject(TYPE_BLADE, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str());

		if (sType == "TYPE_FINAL")
			Play::CreateGameObject(TYPE_FINAL, { std::stof(sX), std::stof(sY) }, 30, sSprite.c_str());
	}

	levelfile.close();
}

//-------------------------------------------------------------------------
// Outputs the objects to the Baamageddon\Level.lev file
void SaveLevel( void )
{
	std::ofstream levelfile;
	levelfile.open( "Level.lev" );

	levelfile << "// This file is auto-generated by the Level Editor - it's not advisable to edit it directly as changes may be overwritten!\n";

	for( int id : Play::CollectAllGameObjectIDs() )
	{
		GameObject& obj = Play::GetGameObject( id );
		switch( obj.type )
		{
			case TYPE_SHEEP:
				levelfile << "TYPE_SHEEP\n";
				break;
			case TYPE_ISLAND:
				levelfile << "TYPE_ISLAND\n";
				break;
			case TYPE_DOUGHNUT:
				levelfile << "TYPE_DOUGHNUT\n";
				break;
			case TYPE_SPIKE:
				levelfile << "TYPE_SPIKE\n";
				break;
			case TYPE_WOLF:
				levelfile << "TYPE_WOLF\n";
				break;
			case TYPE_BUSH:
				levelfile << "TYPE_BUSH\n";
				break;
			case TYPE_BLADE:
				levelfile << "TYPE_BLADE\n";
				break;
			case TYPE_FINAL:
				levelfile << "TYPE_FINAL\n";
				break;

		}
		levelfile << std::to_string( obj.pos.null ) + "f\n" << std::to_string( obj.pos.y ) + "f\n";
		levelfile << Play::GetSpriteName( obj.spriteId ) << "\n";
	}
	
	levelfile.close();

	editorState.saveCooldown = 100;
}
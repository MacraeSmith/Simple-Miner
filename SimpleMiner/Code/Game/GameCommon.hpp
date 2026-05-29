#pragma once
#include "Engine/Math/IntRange.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/Vec2.hpp"
#include <cstdint>
class RendererDX11;
class App;
class RandomNumberGenerator;
class InputSystem;
struct Rgba8;
class AudioSystem;
class Window;
class Game;
class JobSystem;

extern RendererDX11* g_renderer;
extern App* g_app;
extern Game* g_game;
extern RandomNumberGenerator* g_rng;
extern InputSystem* g_inputSystem;
extern AudioSystem* g_audioSystem;
extern Window* g_window;
extern JobSystem* g_jobSystem;

extern bool g_debugMode;
extern bool g_invertMouseX;
extern bool g_invertMouseY;
extern float g_mouseSensitivity;

extern bool g_invertControllerViewX;
extern bool g_invertControllerViewY;
extern float g_controllerSensitivity;

//Screen Size
extern float g_SCREEN_SIZE_X;
extern float g_SCREEN_SIZE_Y;
extern float g_SCREEN_CENTER_X;
extern float g_SCREEN_CENTER_Y;

//Chunks
constexpr int CHUNK_BITS_X = 3;
constexpr int CHUNK_BITS_Y = 3;
constexpr int CHUNK_BITS_Z = 8;

constexpr int CHUNK_SIZE_X = 1 << CHUNK_BITS_X;
constexpr int CHUNK_SIZE_Y = 1 << CHUNK_BITS_Y;
constexpr int CHUNK_SIZE_Z = 1 << CHUNK_BITS_Z;
constexpr int NUM_BLOCKS_IN_CHUNK = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

constexpr int CHUNK_MAX_X = CHUNK_SIZE_X - 1;
constexpr int CHUNK_MAX_Y = CHUNK_SIZE_Y - 1;
constexpr int CHUNK_MAX_Z = CHUNK_SIZE_Z - 1;
constexpr float WORLD_MAX_HEIGHT = (float)CHUNK_MAX_Z * 1.15f;
constexpr float WORLD_MIN_HEIGHT = (float)CHUNK_MAX_Z * -0.15f;

// Strides (these are *value increments*, not bits)
constexpr int STRIDE_X = 1;
constexpr int STRIDE_Y = (1 << CHUNK_BITS_X);                  
constexpr int STRIDE_Z = (1 << (CHUNK_BITS_X + CHUNK_BITS_Y));

// Bit offsets for each axis in packed index
constexpr int BITS_STRIDE_X = 0;
constexpr int BITS_STRIDE_Y = CHUNK_BITS_X;
constexpr int BITS_STRIDE_Z = CHUNK_BITS_X + CHUNK_BITS_Y;

// Masks
constexpr int CHUNK_MASK_X = (CHUNK_SIZE_X - 1);
constexpr int CHUNK_MASK_Y = ((CHUNK_SIZE_Y - 1) << BITS_STRIDE_Y);
constexpr int CHUNK_MASK_Z = ((CHUNK_SIZE_Z - 1) << BITS_STRIDE_Z);


#if defined _DEBUG
	constexpr int MAX_NUM_WORKER_THREADS = 50;
	constexpr int MAX_NUM_CHUNK_JOBS = 50;
	constexpr int MAX_NUM_CHUNK_REGENS_PER_FRAME = 4;
	constexpr int CHUNK_ACTIVATION_RANGE = 50;
#else
	constexpr int MAX_NUM_WORKER_THREADS = 50;
	constexpr int MAX_NUM_CHUNK_JOBS = 10;
	constexpr int MAX_NUM_CHUNK_REGENS_PER_FRAME = 8;
	constexpr int CHUNK_ACTIVATION_RANGE = 450;
#endif

constexpr int MAX_NUM_CHUNK_DELETE_PER_FRAME = 50;
constexpr int MAX_NUM_CHUNK_ACTIVATIONS_PER_FRAME = 1;
constexpr double MAX_TIME_WORLD_UPDATE = 0.08;

constexpr int CHUNK_DEACTIVATION_RANGE = CHUNK_ACTIVATION_RANGE + CHUNK_SIZE_X + CHUNK_SIZE_Y;
constexpr int CHUNK_ACTIVATION_RADIUS_X = 1 + (int)(CHUNK_ACTIVATION_RANGE / CHUNK_SIZE_X);
constexpr int CHUNK_ACTIVATION_RADIUS_Y = 1 + (int)(CHUNK_ACTIVATION_RANGE / CHUNK_SIZE_Y);
constexpr int MAX_ACTIVE_CHUNKS = (2 * CHUNK_ACTIVATION_RADIUS_X) * (2 * CHUNK_ACTIVATION_RADIUS_Y);


//Noise Generation
//----------------------------------------------------------------------------------------
constexpr uint8_t CHUNK_VERSION = 10;
constexpr int MAX_TREE_RADIUS = 7;
constexpr int NOISE_SIZE_X = (CHUNK_SIZE_X + (MAX_TREE_RADIUS * 2));
constexpr int NOISE_SIZE_Y = (CHUNK_SIZE_Y + (MAX_TREE_RADIUS * 2));
constexpr int NOISE_SIZE_2D = NOISE_SIZE_X * NOISE_SIZE_Y;
constexpr int NOISE_SIZE_3D = NOISE_SIZE_2D * CHUNK_SIZE_Z;

//Liquid
////----------------------------------------------------------------------------------------
constexpr uint8_t LIQUID_INFLUENCE = 5;
constexpr float WATER_FLOW_DELAY = 0.15f;
constexpr float LAVA_FLOW_DELAY = 0.75f;
constexpr bool SOLIDIFY_LIQUID = true;
constexpr bool FLOWING_WATER = true;

//Light
//----------------------------------------------------------------------------------------
constexpr uint8_t SKY_LIGHT_INFLUENCE = 15;
constexpr uint8_t GLOWSTONE_LIGHT_INFLUENCE = 15;
constexpr uint8_t MAX_LIGHT_INFLUENCE = 15;


//Player physics
//----------------------------------------------------------------------------------------
constexpr float PLAYER_HEIGHT = 1.8f;
constexpr float PLAYER_HALF_HEIGHT = PLAYER_HEIGHT * 0.5f;
constexpr float PLAYER_WIDTH = 0.6f;
constexpr float PLAYER_HALF_WIDTH = PLAYER_WIDTH * 0.5f;
constexpr float PLAYER_XY_GROUND_SPEED = 6.f;
constexpr float PLAYER_XY_AIR_SPEED = 10.f;
constexpr float PLAYER_XY_GROUND_DRAG = 8.f;
constexpr float PLAYER_XY_AIR_DRAG = 8.f;
constexpr float PLAYER_JUMP_IMPULSE = 7.f;
constexpr float PLAYER_EYE_HEIGHT = 1.65f;
constexpr float PLAYER_COLLISION_RAYCAST_OFFSET = 0.01f;
constexpr float PLAYER_GROUNDED_RAYCAST_LENGTH = 0.01f;

constexpr float WATER_DRAG = 12.f;
constexpr float WATER_BOYANCY_FORCE = 10.f;
constexpr float GRAVITY_ACCELERATION = 18.f;
constexpr float OVER_SHOULDER_CAM_DISTANCE = 4.f;

constexpr bool IGNORE_LIGHTING = false;

//Tnt
const FloatRange TNT_TIMER_RANGE = FloatRange(3.f, 4.f);
const IntRange TNT_EXPLOSION_RADIUS_RANGE = IntRange(8, 12);
constexpr float TNT_EXPLOSION_FORCE = 20.f;
constexpr float TNT_BOX_RADIUS = 0.475f;

//Debug
constexpr float DEBUG_LINE_THICKNESS = .2f;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine2D(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 color);



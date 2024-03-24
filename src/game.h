#ifndef GAME_H
#define GAME_H

#include "assets/assets.h"
#include "types.h"
#include <raylib.h>

// Forward declarations
struct EntityRepository;
struct AssetsTextureCache;
struct AssetsSpriteFrameCache;
struct AssetsAnimationCache;
struct Input;

typedef enum { MAL_GAME, MAL_LEVEL, MAL_FRAME, MAL_COUNT } MemoryArenaLifetime;

typedef enum { CAM_MAIN, CAM_PLAYER_2, CAM_COUNT } CameraType;

typedef struct GameConfig {
    u32 screen_width;
    u32 screen_height;
    void *memory;
    u32 players_count;
} GameConfig;

typedef struct GameState {
    b32 initialised;
    GameConfig config;
    MemoryArenaHandle *memory_by_lifetime[MAL_COUNT];
    struct EntityRepository *entity_repository;
    struct AssetsTextureCache *texture_cache;
    struct AssetsSpriteFrameCache *sprite_frame_cache;
    struct AssetsAnimationCache *animation_cache;
    AssetsTextureId pitch_texture_id;
    RenderTexture2D render_texture;
    Camera2D camera;
    Time time;
    DeltaTime dt;
} GameState;

GameState *game_init(GameConfig gameConfig);
void game_destroy(GameState *game_state);
void game_update(GameState *game_state, struct Input *input);
void game_draw(GameState *game_state);

#endif // !GAME_H

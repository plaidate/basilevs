// Basilevs — Playdate port
//
// A 1-bit bullet-hell shmup, ported to the Playdate (raylib-pd) from
// https://github.com/bredlej/basilevs by bredlej (geoco). The gameplay —
// entities, bullet patterns, timings, the level-1 spawn timeline — follows
// the upstream C++20 original; this is an independent C11 implementation
// written for the raylib-pd backend. Upstream is MIT-licensed (per its
// README); this port is likewise MIT, with attribution to bredlej. See
// LICENSE and README.md.
//
// The game simulates on the original 160x144 (Game Boy sized) playfield and
// scales to the Playdate LCD through a render texture, so all upstream
// coordinates, radii and speeds are preserved as-is.

#ifndef BASILEVS_GAME_H
#define BASILEVS_GAME_H

#include "raylib.h"
#include <stdbool.h>

// --- Playfield (upstream config.h) -----------------------------------------
#define FRAME_W 160
#define FRAME_H 144
// Bullets live until they leave this generous rect around the playfield
#define BOUND_X (-64)
#define BOUND_Y (-64)
#define BOUND_W 260
#define BOUND_H 260

#define ENEMY_BULLET_MAX 200
#define PLAYER_BULLET_MAX 100
#define ENEMY_MAX 16

// Foreground/background: the upstream near-white/near-black 1-bit palette
#define COLOR_FG CLITERAL(Color){ 240, 246, 240, 255 }
#define COLOR_BG CLITERAL(Color){ 34, 35, 35, 255 }

// --- Playdate screen layout -------------------------------------------------
// 160x144 * (240/144) = 266.67x240: playfield centered, UI in the gutters
#define SCREEN_W 400
#define SCREEN_H 240
#define VIEW_W 267
#define VIEW_H 240
#define VIEW_X ((SCREEN_W - VIEW_W)/2)

// --- Assets -----------------------------------------------------------------
typedef enum {
    TEX_PLAYER = 0,         // player.png          224x32, 7 frames
    TEX_BG,                 // basilevs_bg_001.png 160x864, 6 vertical frames
    TEX_BULLET_TENTACLE,    // bullet8.png         8x8
    TEX_BULLET_PLAYER,      // bullet8-002.png     8x8
    TEX_TENTACLE,           // tentacle-0002.png   288x16, 18 frames
    TEX_MOSQUITO,           // mosquito-0001.png   272x16, 17 frames
    TEX_BULLET_MOSQUITO,    // bullet8-003.png     48x8, 6 frames
    TEX_TITLE,              // basilevs.png        160x144 title art
    TEX_COUNT
} TextureId;

// --- Sprites ----------------------------------------------------------------
typedef struct {
    TextureId texture;
    int frameCount;         // total frames in the strip
    int frame;              // currently visible frame
    float frameW;           // width of one frame in the texture
    float animAccum;        // seconds since last frame advance
    float animFps;          // upstream fps_speed (12 everywhere)
    float rotation;         // degrees, player bullets only
} Sprite;

// --- Bullets ----------------------------------------------------------------
typedef enum {
    BULLET_STRAIGHT,        // fly_towards_direction
    BULLET_ROTATE           // fly_and_rotate: +15 deg & +5 speed every 0.5s
} BulletKind;

typedef struct {
    Sprite sprite;
    Vector2 pos;
    Vector2 dir;
    float speed;
    Vector2 colCenter;      // collision circle offset from pos
    float colRadius;
    bool collidable;        // enemy bullets the player can shoot down
    float damage;
    float elapsed;          // BULLET_ROTATE timer
    BulletKind kind;
} Bullet;

typedef struct {
    Bullet items[ENEMY_BULLET_MAX];
    int count;
    int max;
} BulletPool;

// --- Enemies ----------------------------------------------------------------
typedef enum { ENEMY_TENTACLE, ENEMY_MOSQUITO } EnemyKind;

// Upstream state machine: init -> arrival -> alive -> (hit) dying -> gone.
// One player bullet kills: the damage event transitions straight to the
// destroyed animation; the hp field upstream is never decremented
typedef enum { ST_INIT, ST_ARRIVAL, ST_ALIVE, ST_DYING, ST_GONE } EnemyState;

#define PATH_MAX 4

typedef struct {
    EnemyKind kind;
    EnemyState state;
    Sprite sprite;
    Vector2 pos;
    float speed;
    Vector2 path[PATH_MAX];
    int pathCount;
    int pathIndex;
    double activateAfter;   // seconds from level start
    bool active;            // shooting/collidable window
    double elapsed;         // time since level start
    double lastEmission;
    Vector2 colCenter;
    float colRadius;
    // idle/destroyed animation frame ranges, following the upstream
    // "advance while frame <= end-1" rule
    int idleBegin, idleEnd;
    int dieBegin, dieEnd;
    bool dieEnded;
} Enemy;

// --- World ------------------------------------------------------------------
typedef enum { GAME_TITLE, GAME_PLAY, GAME_OVER } GameState;

typedef struct {
    GameState state;
    float bgY;              // current frame window y into the 864px strip
    float bgAccum;

    Vector2 playerPos;
    Sprite playerSprite;
    float playerHp;
    double playerEmission;

    Enemy enemies[ENEMY_MAX];
    int enemyCount;

    BulletPool enemyBullets;
    BulletPool playerBullets;

    int kills;
    bool shotSoundQueued;   // upstream sounds_queue collapsed to one flag
} World;

// entities.c
void WorldReset(World *w);
void PlayerUpdate(World *w, float dt, bool left, bool right, bool up, bool down, bool shoot);
void EnemiesUpdate(World *w, float dt);
void BulletsUpdate(BulletPool *pool, float dt);
void SpriteAnimate(Sprite *s, float dt);

// world.c
void PoolClear(BulletPool *pool, int max);
Bullet *PoolAdd(BulletPool *pool);
void PoolRemoveAt(BulletPool *pool, int index);
void CollisionChecks(World *w);
void CleanupBullets(World *w);

#endif // BASILEVS_GAME_H

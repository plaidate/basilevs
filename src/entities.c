// Basilevs — entity behaviours and the level-1 timeline
//
// Independent C implementation of the mechanics of bredlej's Basilevs
// (https://github.com/bredlej/basilevs); see game.h and README.md for
// provenance notes.

#include "game.h"
#include "raymath.h"
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Sprite animation: all upstream sprites advance at fps_speed=12 relative to
// a 60Hz loop; here time-based so the cadence survives the 50Hz Playdate cap
void SpriteAnimate(Sprite *s, float dt)
{
    s->animAccum += dt;
    if (s->animAccum >= 1.0f/s->animFps)
    {
        s->animAccum = 0.0f;
        s->frame++;
        if (s->frame > s->frameCount - 1) s->frame = 0;
    }
}

// Enemy variant: honours the idle/destroyed frame ranges and the
// non-repeating destroyed animation
static void EnemyAnimate(Enemy *e, float dt)
{
    Sprite *s = &e->sprite;
    s->animAccum += dt;
    if (s->animAccum < 1.0f/s->animFps) return;
    s->animAccum = 0.0f;
    s->frame++;

    if (e->state == ST_DYING)
    {
        if (s->frame > e->dieEnd - 1)
        {
            s->frame = e->dieEnd;
            e->dieEnded = true;
        }
    }
    else
    {
        if (s->frame > e->idleEnd - 1) s->frame = e->idleBegin;
    }
}

// ---------------------------------------------------------------------------
// Bullets

void BulletsUpdate(BulletPool *pool, float dt)
{
    for (int i = 0; i < pool->count; i++)
    {
        Bullet *b = &pool->items[i];
        b->pos.x += b->dir.x*dt*b->speed;
        b->pos.y += b->dir.y*dt*b->speed;

        if (b->kind == BULLET_ROTATE)
        {
            // fly_and_rotate: veer 15 degrees and speed up every half second
            b->elapsed += dt;
            if (b->elapsed > 0.5f)
            {
                b->dir = Vector2Normalize(Vector2Rotate(b->dir, DEG2RAD*15.0f));
                b->speed += 5.0f;
                b->elapsed = 0.0f;
            }
        }

        if (b->sprite.frameCount > 1) SpriteAnimate(&b->sprite, dt);
    }
}

// ---------------------------------------------------------------------------
// Player: 32x32 sprite, quadruple shot in a \||/ pattern every 0.2s of
// held fire; collision circle r=3 at offset (17,18)

static void PlayerEmit(World *w, Vector2 spawnOffset, Vector2 dir, Vector2 rotVec)
{
    Bullet *b = PoolAdd(&w->playerBullets);
    if (b == NULL) return;

    b->sprite = (Sprite){ .texture = TEX_BULLET_PLAYER, .frameCount = 1,
                          .frameW = 8, .animFps = 12 };
    b->sprite.rotation = ((rotVec.x == 0.0f) && (rotVec.y == 0.0f))?
                         0.0f : RAD2DEG*atan2f(rotVec.y, rotVec.x);
    b->pos = Vector2Add(w->playerPos, spawnOffset);
    b->dir = dir;    // upstream leaves the diagonal directions unnormalized
    b->speed = 100.0f;
    b->colCenter = (Vector2){ 5.0f, 5.0f };
    b->colRadius = 2.0f;
    b->collidable = false;
    b->damage = 1.0f;
    b->kind = BULLET_STRAIGHT;
}

void PlayerUpdate(World *w, float dt, bool left, bool right, bool up, bool down, bool shoot)
{
    const float speed = 50.0f;

    SpriteAnimate(&w->playerSprite, dt);

    if (left)  w->playerPos.x -= speed*dt;
    if (right) w->playerPos.x += speed*dt;
    if (up)    w->playerPos.y -= speed*dt;
    if (down)  w->playerPos.y += speed*dt;

    // Playdate addition: keep the 32x32 ship inside the visible playfield
    // (upstream lets it fly off-screen)
    w->playerPos = Vector2Clamp(w->playerPos,
                                (Vector2){ -8, -4 },
                                (Vector2){ FRAME_W - 24, FRAME_H - 28 });

    if (shoot)
    {
        if (w->playerEmission > 0.2)
        {
            w->playerEmission = 0.0;
            PlayerEmit(w, (Vector2){ 8, -4 },  (Vector2){ 0.0f, -1.0f },  (Vector2){ 0, 0 });
            PlayerEmit(w, (Vector2){ 16, -4 }, (Vector2){ 0.0f, -1.0f },  (Vector2){ 0, 0 });
            PlayerEmit(w, (Vector2){ 0, -4 },  (Vector2){ -0.3f, -1.0f }, (Vector2){ 1.0f, -0.3f });
            PlayerEmit(w, (Vector2){ 24, -4 }, (Vector2){ 0.3f, -1.0f },  (Vector2){ 1.0f, 0.3f });
        }
        // Upstream quirk kept: the timer only advances while fire is held,
        // so the very first press shoots after 0.2s of holding
        w->playerEmission += dt;
    }
}

// ---------------------------------------------------------------------------
// Enemies

// Tentacle: one aimed bullet per second while active
static void TentacleShoot(World *w, Enemy *e)
{
    Bullet *b = PoolAdd(&w->enemyBullets);
    if (b == NULL) return;

    b->sprite = (Sprite){ .texture = TEX_BULLET_TENTACLE, .frameCount = 1,
                          .frameW = 8, .animFps = 12 };
    b->pos = Vector2Add(e->pos, (Vector2){ 0.0f, 8.0f });
    // Aim at the player sprite's center (+16,+16), measured from a point
    // slightly inside the bullet — the exact upstream expression
    Vector2 toPlayer = Vector2Add(
        Vector2Subtract(w->playerPos, Vector2Add(b->pos, (Vector2){ 6.0f, 4.0f })),
        (Vector2){ 16.0f, 16.0f });
    b->dir = Vector2Normalize(toPlayer);
    b->speed = 40.0f;
    b->colCenter = (Vector2){ 4.5f, 4.5f };
    b->colRadius = 4.0f;
    b->collidable = false;
    b->damage = 30.0f;
    b->kind = BULLET_STRAIGHT;

    w->shotSoundQueued = true;
}

// Mosquito: a 12-bullet rose every second; bullets curve (fly_and_rotate)
static void MosquitoShoot(World *w, Enemy *e)
{
    for (int i = 1; i <= 12; i++)
    {
        Bullet *b = PoolAdd(&w->enemyBullets);
        if (b == NULL) break;

        // Upstream rotates by (180/3.14)*i in the degree-based raymath of
        // its day: ~57.3deg steps walking a scattered full circle
        float angle = DEG2RAD*(180.0f/3.14f)*(float)i;

        b->sprite = (Sprite){ .texture = TEX_BULLET_MOSQUITO, .frameCount = 6,
                              .frameW = 8, .animFps = 12 };
        b->pos = Vector2Add(Vector2Add(e->pos, (Vector2){ 4.0f, 4.0f }),
                            Vector2Rotate((Vector2){ 0.0f, 9.0f }, angle));
        b->dir = Vector2Normalize(Vector2Rotate((Vector2){ 0.0f, 1.0f }, angle));
        b->speed = 15.0f;
        b->colCenter = (Vector2){ 5.0f, 5.0f };
        b->colRadius = 2.0f;
        b->collidable = true;   // these can be shot down
        b->damage = 1.0f;
        b->kind = BULLET_ROTATE;
    }

    w->shotSoundQueued = true;
}

static void EnemyUpdate(World *w, Enemy *e, float dt)
{
    if (e->state == ST_GONE) return;

    EnemyAnimate(e, dt);
    e->elapsed += dt;

    switch (e->state)
    {
        case ST_INIT:
            if (e->elapsed > e->activateAfter)
            {
                e->active = true;
                e->state = ST_ARRIVAL;
            }
            break;

        case ST_ARRIVAL:
            if (e->pathIndex < e->pathCount)
            {
                Vector2 next = e->path[e->pathIndex];
                e->pos = Vector2MoveTowards(e->pos, next, e->speed*dt);
                if (Vector2Distance(next, e->pos) < 1.1f) e->pathIndex++;
            }
            else
            {
                e->state = ST_ALIVE;
            }
            break;

        case ST_ALIVE:
            break;

        case ST_DYING:
            if (e->dieEnded) e->state = ST_GONE;
            break;

        default:
            break;
    }

    // Enemies shoot as soon as they are active — including on approach
    if (e->active)
    {
        if (e->lastEmission > 1.0)
        {
            e->lastEmission = 0.0;
            if (e->kind == ENEMY_TENTACLE) TentacleShoot(w, e);
            else MosquitoShoot(w, e);
        }
        e->lastEmission += dt;
    }
}

void EnemiesUpdate(World *w, float dt)
{
    for (int i = 0; i < w->enemyCount; i++) EnemyUpdate(w, &w->enemies[i], dt);
}

// ---------------------------------------------------------------------------
// Level 1 timeline (converted from upstream assets/json/level1.json)

typedef struct {
    double time;
    Vector2 pos;
    EnemyKind kind;
    Vector2 path[PATH_MAX];
    int pathCount;
} Spawn;

static const Spawn kLevel1[] = {
    { 1.1,  {   0, -20 }, ENEMY_TENTACLE, { {  15, 15 } }, 1 },
    { 1.1,  { 140, -20 }, ENEMY_TENTACLE, { { 100, 15 } }, 1 },
    { 5.5,  {  70, -20 }, ENEMY_MOSQUITO, { {  50, 50 }, { 100, 100 }, { 30, 70 } }, 3 },
    { 11.1, { -10, -20 }, ENEMY_TENTACLE, { {  20, 35 } }, 1 },
    { 11.1, {  35, -20 }, ENEMY_TENTACLE, { {  45, 35 } }, 1 },
    { 11.1, {  70, -20 }, ENEMY_TENTACLE, { {  85, 35 } }, 1 },
    { 11.1, { 150, -20 }, ENEMY_TENTACLE, { { 120, 35 } }, 1 },
    { 15.5, {  70, -20 }, ENEMY_MOSQUITO, { {  50, 50 }, { 100, 100 }, { 30, 70 } }, 3 },
};

static Enemy MakeEnemy(const Spawn *s)
{
    Enemy e = { 0 };
    e.kind = s->kind;
    e.state = ST_INIT;
    e.pos = s->pos;
    e.activateAfter = s->time;
    e.pathCount = s->pathCount;
    for (int i = 0; i < s->pathCount; i++) e.path[i] = s->path[i];
    e.speed = 10.0f;
    e.colCenter = (Vector2){ 8.0f, 8.0f };
    e.colRadius = 8.0f;

    if (s->kind == ENEMY_TENTACLE)
    {
        e.sprite = (Sprite){ .texture = TEX_TENTACLE, .frameCount = 18,
                             .frameW = 16, .animFps = 12 };
        e.idleBegin = 0; e.idleEnd = 8;
        e.dieBegin = 9;  e.dieEnd = 17;
    }
    else
    {
        e.sprite = (Sprite){ .texture = TEX_MOSQUITO, .frameCount = 17,
                             .frameW = 16, .animFps = 12 };
        e.idleBegin = 0;  e.idleEnd = 10;
        e.dieBegin = 11;  e.dieEnd = 16;
    }
    e.sprite.frame = rand()%e.sprite.frameCount;

    return e;
}

void WorldReset(World *w)
{
    w->state = GAME_PLAY;

    w->bgY = 864.0f - FRAME_H;      // start at the strip's bottom frame
    w->bgAccum = 0.0f;

    w->playerPos = (Vector2){ 70.0f, 100.0f };
    w->playerSprite = (Sprite){ .texture = TEX_PLAYER, .frameCount = 7,
                                .frameW = 32, .animFps = 12 };
    w->playerSprite.frame = rand()%7;
    w->playerHp = 100.0f;
    w->playerEmission = 0.0;

    w->enemyCount = (int)(sizeof(kLevel1)/sizeof(kLevel1[0]));
    for (int i = 0; i < w->enemyCount; i++) w->enemies[i] = MakeEnemy(&kLevel1[i]);

    PoolClear(&w->enemyBullets, ENEMY_BULLET_MAX);
    PoolClear(&w->playerBullets, PLAYER_BULLET_MAX);

    w->kills = 0;
    w->shotSoundQueued = false;
}

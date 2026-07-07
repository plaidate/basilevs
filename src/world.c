// Basilevs — bullet pools, collision checks, cleanup

#include "game.h"
#include "raymath.h"
#include <stddef.h>

void PoolClear(BulletPool *pool, int max)
{
    pool->count = 0;
    pool->max = (max <= ENEMY_BULLET_MAX)? max : ENEMY_BULLET_MAX;
}

Bullet *PoolAdd(BulletPool *pool)
{
    if (pool->count >= pool->max) return NULL;
    Bullet *b = &pool->items[pool->count++];
    *b = (Bullet){ 0 };
    return b;
}

// Order inside a pool is irrelevant: swap-remove keeps it dense
void PoolRemoveAt(BulletPool *pool, int index)
{
    pool->count--;
    if (index != pool->count) pool->items[index] = pool->items[pool->count];
}

static Vector2 BulletCenter(const Bullet *b)
{
    return Vector2Add(b->pos, b->colCenter);
}

// Player bullets vs enemies: one hit sends an enemy into its destroyed
// animation (matching upstream, where hp is never actually decremented)
static void PlayerBulletsVsEnemies(World *w)
{
    for (int bi = w->playerBullets.count - 1; bi >= 0; bi--)
    {
        const Bullet *b = &w->playerBullets.items[bi];
        Vector2 bc = BulletCenter(b);
        bool hit = false;

        for (int ei = 0; ei < w->enemyCount; ei++)
        {
            Enemy *e = &w->enemies[ei];
            if (!e->active) continue;

            if (CheckCollisionCircles(bc, b->colRadius,
                                      Vector2Add(e->pos, e->colCenter), e->colRadius))
            {
                e->state = ST_DYING;
                e->active = false;
                e->sprite.frame = e->dieBegin;
                e->sprite.animAccum = 0.0f;
                w->kills++;
                hit = true;
                break;
            }
        }

        if (hit) PoolRemoveAt(&w->playerBullets, bi);
    }
}

// Player bullets vs collidable enemy bullets (the mosquito swarm): both die
static void PlayerBulletsVsEnemyBullets(World *w)
{
    for (int pi = w->playerBullets.count - 1; pi >= 0; pi--)
    {
        const Bullet *pb = &w->playerBullets.items[pi];
        Vector2 pc = BulletCenter(pb);
        bool hit = false;

        for (int ei = w->enemyBullets.count - 1; ei >= 0; ei--)
        {
            const Bullet *eb = &w->enemyBullets.items[ei];
            if (!eb->collidable) continue;

            if (CheckCollisionCircles(pc, pb->colRadius, BulletCenter(eb), eb->colRadius))
            {
                PoolRemoveAt(&w->enemyBullets, ei);
                hit = true;
                break;
            }
        }

        if (hit) PoolRemoveAt(&w->playerBullets, pi);
    }
}

// Enemy bullets vs the player: bullet dies, player takes its damage
static void EnemyBulletsVsPlayer(World *w)
{
    // Upstream player collision: r=3 circle at sprite offset (17,18)
    Vector2 playerCenter = Vector2Add(w->playerPos, (Vector2){ 17.0f, 18.0f });

    for (int bi = w->enemyBullets.count - 1; bi >= 0; bi--)
    {
        const Bullet *b = &w->enemyBullets.items[bi];

        if (CheckCollisionCircles(BulletCenter(b), b->colRadius, playerCenter, 3.0f))
        {
            w->playerHp -= b->damage;
            PoolRemoveAt(&w->enemyBullets, bi);
        }
    }
}

void CollisionChecks(World *w)
{
    PlayerBulletsVsEnemies(w);
    PlayerBulletsVsEnemyBullets(w);
    EnemyBulletsVsPlayer(w);
}

// Drop bullets once they leave the generous bounds rect around the playfield
static void CullPool(BulletPool *pool)
{
    const Rectangle bounds = { BOUND_X, BOUND_Y, BOUND_W, BOUND_H };

    for (int i = pool->count - 1; i >= 0; i--)
    {
        const Bullet *b = &pool->items[i];
        Rectangle r = { b->pos.x, b->pos.y, 8.0f, 8.0f };
        if (!CheckCollisionRecs(r, bounds)) PoolRemoveAt(pool, i);
    }
}

void CleanupBullets(World *w)
{
    CullPool(&w->enemyBullets);
    CullPool(&w->playerBullets);
}

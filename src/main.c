// Basilevs — Playdate entry point, rendering, and game states
//
// The world simulates at the upstream 160x144 playfield inside a render
// texture, which is scaled to 267x240 on the 400x240 LCD; the side gutters
// carry the HP bar and score. See game.h/README.md for provenance.
//
// Controls: d-pad move, A (space) shoot, B (enter) restart.
// Playdate additions over the upstream WIP: title screen (its basilevs.png
// title art was shipped but unused), HP/score gutters, a game-over state,
// and the bundled music.mp3 actually playing.

#include "game.h"
#include <stddef.h>

static const char *kTexturePaths[TEX_COUNT] = {
    [TEX_PLAYER]          = "resources/player.png",
    [TEX_BG]              = "resources/basilevs_bg_001.png",
    [TEX_BULLET_TENTACLE] = "resources/bullet8.png",
    [TEX_BULLET_PLAYER]   = "resources/bullet8-002.png",
    [TEX_TENTACLE]        = "resources/tentacle-0002.png",
    [TEX_MOSQUITO]        = "resources/mosquito-0001.png",
    [TEX_BULLET_MOSQUITO] = "resources/bullet8-003.png",
    [TEX_TITLE]           = "resources/basilevs.png",
};

static Texture2D textures[TEX_COUNT];
static Sound shotSound;
static Music music;
static World world;

static Rectangle SpriteRect(const Sprite *s, float frameH)
{
    return (Rectangle){ (float)s->frame*s->frameW, 0.0f, s->frameW, frameH };
}

static void RenderPlayfield(void)
{
    ClearBackground(COLOR_BG);

    // Background: a 160x144 window sliding up the 864px strip, drawn dimmed
    DrawTextureRec(textures[TEX_BG],
                   (Rectangle){ 0.0f, world.bgY, FRAME_W, FRAME_H },
                   (Vector2){ 0, 0 }, GRAY);

    // Player
    DrawTextureRec(textures[TEX_PLAYER], SpriteRect(&world.playerSprite, 32),
                   world.playerPos, WHITE);

    // Enemies (hidden while waiting to spawn and once fully destroyed)
    for (int i = 0; i < world.enemyCount; i++)
    {
        const Enemy *e = &world.enemies[i];
        if ((e->state == ST_INIT) || (e->state == ST_GONE)) continue;
        DrawTextureRec(textures[e->sprite.texture], SpriteRect(&e->sprite, 16),
                       e->pos, WHITE);
    }

    // Enemy bullets, then player bullets (player bullets rotate with their
    // diagonal directions, as upstream renders them)
    for (int i = 0; i < world.enemyBullets.count; i++)
    {
        const Bullet *b = &world.enemyBullets.items[i];
        DrawTextureRec(textures[b->sprite.texture], SpriteRect(&b->sprite, 8),
                       b->pos, WHITE);
    }
    for (int i = 0; i < world.playerBullets.count; i++)
    {
        const Bullet *b = &world.playerBullets.items[i];
        DrawTextureEx(textures[b->sprite.texture], b->pos,
                      b->sprite.rotation, 1.0f, WHITE);
    }
}

static void RenderTitle(void)
{
    ClearBackground(COLOR_BG);
    DrawTexture(textures[TEX_TITLE], 0, 0, WHITE);
    // Blink at ~1Hz
    if (((int)(GetTime()*2.0))%2 == 0)
    {
        DrawText("PRESS A", 56, 126, 10, COLOR_FG);
    }
}

static void RenderGutters(void)
{
    // Left gutter: HP bar
    DrawText("HP", 18, 16, 10, COLOR_FG);
    int hp = (world.playerHp > 0)? (int)world.playerHp : 0;
    int barH = 160*hp/100;
    DrawRectangleLines(20, 32, 12, 164, COLOR_FG);
    DrawRectangle(22, 34 + (160 - barH), 8, barH, COLOR_FG);

    // Right gutter: kills
    DrawText("HITS", 348, 16, 10, COLOR_FG);
    DrawText(TextFormat("%d", world.kills), 352, 32, 20, COLOR_FG);

    if (world.state == GAME_OVER)
    {
        DrawRectangle(VIEW_X + 20, 96, VIEW_W - 40, 48, COLOR_BG);
        DrawRectangleLines(VIEW_X + 20, 96, VIEW_W - 40, 48, COLOR_FG);
        DrawText("GAME OVER", VIEW_X + 78, 106, 20, COLOR_FG);
        DrawText("press B to retry", VIEW_X + 90, 128, 10, COLOR_FG);
    }
}

int main(void)
{
    InitWindow(SCREEN_W, SCREEN_H, "Basilevs");
    InitAudioDevice();

    for (int i = 0; i < TEX_COUNT; i++) textures[i] = LoadTexture(kTexturePaths[i]);

    shotSound = LoadSound("resources/bullet.wav");
    music = LoadMusicStream("resources/music.mp3");
    music.looping = true;

    RenderTexture2D target = LoadRenderTexture(FRAME_W, FRAME_H);

    world.state = GAME_TITLE;
    SetTargetFPS(50);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        switch (world.state)
        {
            case GAME_TITLE:
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
                {
                    WorldReset(&world);
                    PlayMusicStream(music);
                }
                break;

            case GAME_PLAY:
            {
                UpdateMusicStream(music);

                // Background scroll: ~1px per 12Hz tick until the strip top
                world.bgAccum += dt;
                if ((world.bgY > 0.0f) && (world.bgAccum >= 1.0f/12.0f))
                {
                    world.bgAccum = 0.0f;
                    world.bgY -= 1.0f;
                }

                PlayerUpdate(&world, dt,
                             IsKeyDown(KEY_LEFT), IsKeyDown(KEY_RIGHT),
                             IsKeyDown(KEY_UP), IsKeyDown(KEY_DOWN),
                             IsKeyDown(KEY_SPACE));
                EnemiesUpdate(&world, dt);
                BulletsUpdate(&world.enemyBullets, dt);
                BulletsUpdate(&world.playerBullets, dt);
                CollisionChecks(&world);
                CleanupBullets(&world);

                if (world.shotSoundQueued)
                {
                    world.shotSoundQueued = false;
                    PlaySound(shotSound);
                }

                // Upstream restarts on F10; B here. Death is a Playdate
                // addition (upstream hp underflows with no consequence)
                if (IsKeyPressed(KEY_ENTER)) WorldReset(&world);
                if (world.playerHp <= 0.0f)
                {
                    world.state = GAME_OVER;
                    StopMusicStream(music);
                }
                break;
            }

            case GAME_OVER:
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                {
                    WorldReset(&world);
                    PlayMusicStream(music);
                }
                break;
        }

        BeginTextureMode(target);
            if (world.state == GAME_TITLE) RenderTitle();
            else RenderPlayfield();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            // Render texture is OpenGL-style bottom-up: flip the source rect
            DrawTexturePro(target.texture,
                           (Rectangle){ 0, 0, FRAME_W, -FRAME_H },
                           (Rectangle){ VIEW_X, 0, VIEW_W, VIEW_H },
                           (Vector2){ 0, 0 }, 0.0f, WHITE);
            if (world.state != GAME_TITLE) RenderGutters();
        EndDrawing();
    }

    UnloadRenderTexture(target);
    for (int i = 0; i < TEX_COUNT; i++) UnloadTexture(textures[i]);
    UnloadSound(shotSound);
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}

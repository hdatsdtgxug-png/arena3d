#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <cmath>

struct Weapon {
    std::string name;
    float fireRate;
    float spread;
    Color color;
    int pellets;
    int damage;
    bool unlocked;
    int price;
};

struct Target {
    Vector3 position;
    bool alive;
    float radius;
};

struct Enemy {
    Vector3 position;
    bool alive;
    int health;
    float speed;
    double lastAttack;
};

struct Wall {
    Vector3 position;
    Vector3 size;
    Color color;
};

static bool CheckWallCollision(Vector3 pos, float radius, std::vector<Wall> &walls) {
    for (auto &w : walls) {
        float minX = w.position.x - w.size.x/2 - radius;
        float maxX = w.position.x + w.size.x/2 + radius;
        float minZ = w.position.z - w.size.z/2 - radius;
        float maxZ = w.position.z + w.size.z/2 + radius;
        if (pos.x > minX && pos.x < maxX && pos.z > minZ && pos.z < maxZ) return true;
    }
    return false;
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    const float MAP_LIMIT = 47.0f;
    const float PLAYER_RADIUS = 0.6f;

    InitWindow(screenWidth, screenHeight, "Arena 3D - Silah Oyunu");
    SetTargetFPS(60);
    DisableCursor();

    // ---------- OYUNCU ----------
    Vector3 playerPos = { 0.0f, 1.8f, 10.0f };
    float yaw = 180.0f * DEG2RAD;
    float pitch = 0.0f;
    int playerHealth = 100;
    int score = 0;
    int money = 0;
    bool gameOver = false;
    bool shopOpen = false;
    float damageFlash = 0.0f;

    Camera3D camera = { 0 };
    camera.position = playerPos;
    camera.up = (Vector3){ 0, 1, 0 };
    camera.fovy = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // ---------- HARİTA ----------
    std::vector<Wall> walls;
    walls.push_back({ (Vector3){0, 3, -50}, (Vector3){100, 6, 2}, BROWN });
    walls.push_back({ (Vector3){0, 3, 50},  (Vector3){100, 6, 2}, BROWN });
    walls.push_back({ (Vector3){-50, 3, 0}, (Vector3){2, 6, 100}, BROWN });
    walls.push_back({ (Vector3){50, 3, 0},  (Vector3){2, 6, 100}, BROWN });

    Color blockColors[4] = { GRAY, DARKGRAY, (Color){160,110,60,255}, (Color){110,130,140,255} };
    for (int i = 0; i < 16; i++) {
        float x = (float)(GetRandomValue(-42, 42));
        float z = (float)(GetRandomValue(-42, 42));
        if (Vector3Length((Vector3){x, 0, z}) < 10) continue;
        float w = (float)GetRandomValue(3, 9);
        float h = (float)GetRandomValue(3, 10);
        float d = (float)GetRandomValue(3, 9);
        walls.push_back({ (Vector3){x, h/2, z}, (Vector3){w, h, d}, blockColors[i % 4] });
    }

    // ---------- MARKET (dükkan) ----------
    Vector3 marketPos = { 30.0f, 2.0f, 30.0f };
    Vector3 marketSize = { 4.0f, 4.0f, 4.0f };
    float marketRadius = 5.0f;

    // ---------- SİLAHLAR ----------
    std::vector<Weapon> weapons = {
        { "Tabanca",    0.35f, 0.010f, (Color){255,204,51,255}, 1, 25, true,  0   },
        { "Pompali",    0.75f, 0.090f, (Color){255,85,51,255},  7, 12, true,  0   },
        { "Tufek",      0.11f, 0.030f, (Color){51,204,255,255},1, 15, true,  0   },
        { "Roket Atar", 0.90f, 0.000f, (Color){190,60,220,255},1, 100,false, 150 },
    };
    int currentWeapon = 0;
    float shotTimer = 0.0f;

    // ---------- HEDEFLER (sabit, egzersiz) ----------
    std::vector<Target> targets;
    auto spawnTarget = [&]() {
        Target t;
        t.radius = 0.6f;
        Vector3 p;
        int tries = 0;
        do {
            p = (Vector3){ (float)GetRandomValue(-42,42), 1.1f, (float)GetRandomValue(-42,42) };
            tries++;
        } while (CheckWallCollision(p, 1.0f, walls) && tries < 20);
        t.position = p;
        t.alive = true;
        targets.push_back(t);
    };
    for (int i = 0; i < 6; i++) spawnTarget();

    // ---------- DÜŞMANLAR ----------
    std::vector<Enemy> enemies;
    auto spawnEnemy = [&]() {
        Enemy e;
        Vector3 p;
        int tries = 0;
        do {
            p = (Vector3){ (float)GetRandomValue(-42,42), 1.0f, (float)GetRandomValue(-42,42) };
            tries++;
        } while ((CheckWallCollision(p, 1.0f, walls) || Vector3Distance(p, playerPos) < 15.0f) && tries < 30);
        e.position = p;
        e.alive = true;
        e.health = 40;
        e.speed = 2.2f;
        e.lastAttack = 0;
        enemies.push_back(e);
    };
    for (int i = 0; i < 5; i++) spawnEnemy();

    auto resetGame = [&]() {
        playerPos = (Vector3){0, 1.8f, 10.0f};
        yaw = 180.0f * DEG2RAD; pitch = 0.0f;
        playerHealth = 100; money = 0; gameOver = false; shopOpen = false;
        targets.clear(); for (int i = 0; i < 6; i++) spawnTarget();
        enemies.clear(); for (int i = 0; i < 5; i++) spawnEnemy();
        for (auto &w : weapons) if (w.price > 0) w.unlocked = false;
        currentWeapon = 0;
    };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        double now = GetTime();
        shotTimer -= dt;
        if (damageFlash > 0) damageFlash -= dt;

        bool padAvailable = IsGamepadAvailable(0);

        // ---------- MARKET KONTROLÜ ----------
        bool nearMarket = Vector3Distance(playerPos, marketPos) < marketRadius;
        if (!gameOver && nearMarket && IsKeyPressed(KEY_E)) shopOpen = !shopOpen;
        if (padAvailable && nearMarket && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) shopOpen = !shopOpen;

        if (gameOver) {
            if (IsKeyPressed(KEY_R)) resetGame();
        }
        else if (shopOpen) {
            // dükkanda silah satın alma
            for (int i = 0; i < (int)weapons.size(); i++) {
                bool keyPress = (i == 0 && IsKeyPressed(KEY_ONE)) || (i == 1 && IsKeyPressed(KEY_TWO)) ||
                                 (i == 2 && IsKeyPressed(KEY_THREE)) || (i == 3 && IsKeyPressed(KEY_FOUR));
                if (keyPress && !weapons[i].unlocked && money >= weapons[i].price) {
                    money -= weapons[i].price;
                    weapons[i].unlocked = true;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) shopOpen = false;
        }
        else {
            // ---------- BAKIŞ ----------
            Vector2 md = GetMouseDelta();
            yaw -= md.x * 0.003f;
            pitch -= md.y * 0.003f;
            if (padAvailable) {
                yaw -= GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X) * 2.2f * dt;
                pitch -= GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y) * 2.2f * dt;
            }
            pitch = Clamp(pitch, -1.3f, 1.3f);

            // ---------- HAREKET ----------
            float mf = 0, mr = 0;
            if (IsKeyDown(KEY_W)) mf += 1;
            if (IsKeyDown(KEY_S)) mf -= 1;
            if (IsKeyDown(KEY_D)) mr += 1;
            if (IsKeyDown(KEY_A)) mr -= 1;
            if (padAvailable) {
                mf -= GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
                mr += GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            }

            Vector3 forwardFlat = { sinf(yaw), 0, cosf(yaw) };
            Vector3 right = { cosf(yaw), 0, -sinf(yaw) };
            float speed = 6.0f * dt;

            Vector3 moveX = Vector3Add(playerPos, Vector3Scale(right, mr * speed));
            if (!CheckWallCollision((Vector3){moveX.x, 0, playerPos.z}, PLAYER_RADIUS, walls))
                playerPos.x = moveX.x;

            Vector3 moveZ = Vector3Add(playerPos, Vector3Scale(forwardFlat, mf * speed));
            if (!CheckWallCollision((Vector3){playerPos.x, 0, moveZ.z}, PLAYER_RADIUS, walls))
                playerPos.z = moveZ.z;

            playerPos.x = Clamp(playerPos.x, -MAP_LIMIT, MAP_LIMIT);
            playerPos.z = Clamp(playerPos.z, -MAP_LIMIT, MAP_LIMIT);

            Vector3 forward3D = { cosf(pitch)*sinf(yaw), sinf(pitch), cosf(pitch)*cosf(yaw) };
            camera.position = playerPos;
            camera.target = Vector3Add(playerPos, forward3D);

            // ---------- SİLAH DEĞİŞTİRME ----------
            if (IsKeyPressed(KEY_ONE) && weapons[0].unlocked) currentWeapon = 0;
            if (IsKeyPressed(KEY_TWO) && weapons[1].unlocked) currentWeapon = 1;
            if (IsKeyPressed(KEY_THREE) && weapons[2].unlocked) currentWeapon = 2;
            if (IsKeyPressed(KEY_FOUR) && weapons[3].unlocked) currentWeapon = 3;
            if (padAvailable && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) {
                int tries2 = 0;
                do { currentWeapon = (currentWeapon + 1) % (int)weapons.size(); tries2++; }
                while (!weapons[currentWeapon].unlocked && tries2 < 8);
            }

            // ---------- ATEŞ ----------
            bool firePressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
                                (padAvailable && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2));
            if (firePressed && shotTimer <= 0.0f) {
                Weapon &w = weapons[currentWeapon];
                shotTimer = w.fireRate;

                for (int p = 0; p < w.pellets; p++) {
                    Vector3 dir = forward3D;
                    dir.x += ((float)GetRandomValue(-100,100)/100.0f) * w.spread;
                    dir.y += ((float)GetRandomValue(-100,100)/100.0f) * w.spread;
                    dir = Vector3Normalize(dir);
                    Ray ray = { camera.position, dir };

                    float closestDist = 1000.0f;
                    int hitTargetIdx = -1, hitEnemyIdx = -1;

                    for (size_t i = 0; i < targets.size(); i++) {
                        if (!targets[i].alive) continue;
                        RayCollision col = GetRayCollisionSphere(ray, targets[i].position, targets[i].radius);
                        if (col.hit && col.distance < closestDist) { closestDist = col.distance; hitTargetIdx = (int)i; hitEnemyIdx = -1; }
                    }
                    for (size_t i = 0; i < enemies.size(); i++) {
                        if (!enemies[i].alive) continue;
                        RayCollision col = GetRayCollisionSphere(ray, Vector3Add(enemies[i].position, (Vector3){0,0.9f,0}), 0.8f);
                        if (col.hit && col.distance < closestDist) { closestDist = col.distance; hitEnemyIdx = (int)i; hitTargetIdx = -1; }
                    }

                    if (hitTargetIdx >= 0) {
                        targets[hitTargetIdx].alive = false;
                        score += 10; money += 10;
                        spawnTarget();
                    } else if (hitEnemyIdx >= 0) {
                        enemies[hitEnemyIdx].health -= w.damage;
                        if (enemies[hitEnemyIdx].health <= 0) {
                            enemies[hitEnemyIdx].alive = false;
                            score += 25; money += 25;
                            spawnEnemy();
                        }
                    }
                }
            }

            // ---------- DÜŞMAN YAPAY ZEKASI ----------
            for (auto &e : enemies) {
                if (!e.alive) continue;
                Vector3 toPlayer = Vector3Subtract(playerPos, e.position);
                toPlayer.y = 0;
                float dist = Vector3Length(toPlayer);
                if (dist > 2.2f) {
                    Vector3 dir = Vector3Normalize(toPlayer);
                    Vector3 newPos = Vector3Add(e.position, Vector3Scale(dir, e.speed * dt));
                    if (!CheckWallCollision(newPos, 0.8f, walls)) e.position = newPos;
                } else {
                    if (now - e.lastAttack > 1.0) {
                        playerHealth -= 8;
                        e.lastAttack = now;
                        damageFlash = 0.25f;
                        if (playerHealth <= 0) { playerHealth = 0; gameOver = true; }
                    }
                }
            }
        }

        // ---------- ÇİZİM ----------
        BeginDrawing();
        ClearBackground((Color){130, 198, 255, 255});

        BeginMode3D(camera);
        DrawPlane((Vector3){0,0,0}, (Vector2){100,100}, (Color){76,175,80,255});

        for (auto &w : walls) {
            DrawCube(w.position, w.size.x, w.size.y, w.size.z, w.color);
            DrawCubeWires(w.position, w.size.x, w.size.y, w.size.z, BLACK);
        }

        // market binası
        DrawCube(marketPos, marketSize.x, marketSize.y, marketSize.z, (Color){255,215,0,255});
        DrawCubeWires(marketPos, marketSize.x, marketSize.y, marketSize.z, BLACK);

        for (auto &t : targets) {
            if (!t.alive) continue;
            DrawSphere(t.position, t.radius, RED);
            DrawSphereWires(t.position, t.radius, 8, 8, MAROON);
        }

        for (auto &e : enemies) {
            if (!e.alive) continue;
            Vector3 bottom = { e.position.x, 0.2f, e.position.z };
            Vector3 top = { e.position.x, 1.7f, e.position.z };
            DrawCapsule(bottom, top, 0.4f, 8, 8, (Color){120,20,140,255});
        }

        // oyuncunun kendi vücudu (aşağı bakınca görünür)
        Vector3 bodyBottom = { playerPos.x, playerPos.y - 1.7f, playerPos.z };
        Vector3 bodyTop = { playerPos.x, playerPos.y - 0.5f, playerPos.z };
        DrawCapsule(bodyBottom, bodyTop, 0.35f, 8, 8, (Color){60,90,160,255});

        // elde tutulan silah (kameraya göre konumlanır)
        if (!gameOver) {
            Vector3 fwd = Vector3Subtract(camera.target, camera.position);
            fwd = Vector3Normalize(fwd);
            Vector3 rgt = { cosf(yaw), 0, -sinf(yaw) };
            Vector3 gunPos = Vector3Add(camera.position, Vector3Scale(fwd, 0.6f));
            gunPos = Vector3Add(gunPos, Vector3Scale(rgt, 0.28f));
            gunPos.y -= 0.22f;
            DrawCube(gunPos, 0.12f, 0.12f, 0.45f, weapons[currentWeapon].color);
            DrawCubeWires(gunPos, 0.12f, 0.12f, 0.45f, BLACK);
        }

        EndMode3D();

        // ---------- HUD ----------
        DrawText(TextFormat("Skor: %d", score), 12, 12, 24, WHITE);
        DrawText(TextFormat("Para: %d", money), 12, 40, 20, GOLD);
        DrawText(TextFormat("Silah: %s", weapons[currentWeapon].name.c_str()), 12, 66, 20, weapons[currentWeapon].color);

        // can barı
        DrawRectangle(12, 96, 204, 22, DARKGRAY);
        DrawRectangle(14, 98, (int)(200 * (playerHealth/100.0f)), 18, (playerHealth > 30) ? GREEN : RED);
        DrawText(TextFormat("Can: %d", playerHealth), 18, 99, 16, WHITE);

        DrawText("Sol tik: ates | Fare/Kumanda: bak | WASD/Sol Stik: hareket | 1-4: silah | E: market",
                  12, screenHeight - 26, 16, LIGHTGRAY);

        int cx = screenWidth/2, cy = screenHeight/2;
        DrawLine(cx-12, cy, cx+12, cy, SKYBLUE);
        DrawLine(cx, cy-12, cx, cy+12, SKYBLUE);

        if (damageFlash > 0) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(RED, damageFlash * 0.5f));
        }

        if (nearMarket && !shopOpen && !gameOver) {
            DrawText("Markete yaklastin - E'ye bas", screenWidth/2 - 160, screenHeight - 80, 22, GOLD);
        }

        if (shopOpen) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.7f));
            DrawText("MARKET", screenWidth/2 - 70, 60, 34, GOLD);
            DrawText(TextFormat("Paran: %d", money), screenWidth/2 - 70, 105, 22, WHITE);
            int y = 160;
            for (int i = 0; i < (int)weapons.size(); i++) {
                std::string status = weapons[i].unlocked ? "ALINDI" : TextFormat("%d para - [%d] tusuna bas", weapons[i].price, i+1);
                DrawText(TextFormat("%d) %s - %s", i+1, weapons[i].name.c_str(), status.c_str()),
                          screenWidth/2 - 220, y, 22, weapons[i].color);
                y += 40;
            }
            DrawText("Kapatmak icin ESC veya E", screenWidth/2 - 140, y + 20, 18, LIGHTGRAY);
        }

        if (gameOver) {
            DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.75f));
            DrawText("OYUN BITTI", screenWidth/2 - 120, screenHeight/2 - 40, 40, RED);
            DrawText(TextFormat("Skor: %d", score), screenWidth/2 - 60, screenHeight/2 + 10, 24, WHITE);
            DrawText("Yeniden baslamak icin R'ye bas", screenWidth/2 - 150, screenHeight/2 + 45, 20, LIGHTGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

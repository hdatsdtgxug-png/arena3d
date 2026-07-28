#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>

struct Weapon {
    std::string name;
    float fireRate;   // saniye
    float spread;
    Color color;
    int damage;
};

struct Target {
    Vector3 position;
    bool alive;
    float radius;
};

struct Wall {
    Vector3 position;
    Vector3 size;
    Color color;
};

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Arena 3D - Silah Oyunu");
    SetTargetFPS(60);
    DisableCursor();

    // ---------- KAMERA ----------
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 1.8f, 10.0f };
    camera.target   = (Vector3){ 0.0f, 1.8f, 0.0f };
    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy     = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // ---------- HARİTA ----------
    std::vector<Wall> walls;
    walls.push_back({ (Vector3){0, 3, -50}, (Vector3){100, 6, 2}, BROWN });
    walls.push_back({ (Vector3){0, 3, 50},  (Vector3){100, 6, 2}, BROWN });
    walls.push_back({ (Vector3){-50, 3, 0}, (Vector3){2, 6, 100}, BROWN });
    walls.push_back({ (Vector3){50, 3, 0},  (Vector3){2, 6, 100}, BROWN });

    Color blockColors[4] = { GRAY, DARKGRAY, (Color){160,110,60,255}, (Color){110,130,140,255} };
    for (int i = 0; i < 18; i++) {
        float x = (float)(GetRandomValue(-45, 45));
        float z = (float)(GetRandomValue(-45, 45));
        if (Vector3Length((Vector3){x, 0, z}) < 8) continue;
        float w = (float)GetRandomValue(3, 9);
        float h = (float)GetRandomValue(3, 10);
        float d = (float)GetRandomValue(3, 9);
        walls.push_back({ (Vector3){x, h/2, z}, (Vector3){w, h, d}, blockColors[i % 4] });
    }

    // ---------- SİLAHLAR ----------
    std::vector<Weapon> weapons = {
        { "Tabanca", 0.35f, 0.01f, (Color){255,204,51,255}, 10 },
        { "Pompali",  0.75f, 0.09f, (Color){255,85,51,255}, 8  },
        { "Tufek",    0.11f, 0.03f, (Color){51,204,255,255}, 6 },
    };
    int currentWeapon = 0;
    float shotTimer = 0.0f;

    // ---------- HEDEFLER ----------
    std::vector<Target> targets;
    auto spawnTarget = [&]() {
        Target t;
        t.radius = 0.6f;
        int tries = 0;
        Vector3 p;
        do {
            p = (Vector3){ (float)GetRandomValue(-45,45), 1.1f, (float)GetRandomValue(-45,45) };
            tries++;
        } while (tries < 20);
        t.position = p;
        t.alive = true;
        targets.push_back(t);
    };
    for (int i = 0; i < 8; i++) spawnTarget();

    int score = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        shotTimer -= dt;

        // silah değiştirme
        if (IsKeyPressed(KEY_ONE)) currentWeapon = 0;
        if (IsKeyPressed(KEY_TWO)) currentWeapon = 1;
        if (IsKeyPressed(KEY_THREE)) currentWeapon = 2;

        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        // ateş etme
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && shotTimer <= 0.0f) {
            Weapon &w = weapons[currentWeapon];
            shotTimer = w.fireRate;

            Vector3 dir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
            dir.x += (float)GetRandomValue(-100,100)/100.0f * w.spread;
            dir.y += (float)GetRandomValue(-100,100)/100.0f * w.spread;
            dir = Vector3Normalize(dir);

            Ray ray = { camera.position, dir };
            float closestDist = 1000.0f;
            int hitIndex = -1;
            for (size_t i = 0; i < targets.size(); i++) {
                if (!targets[i].alive) continue;
                RayCollision col = GetRayCollisionSphere(ray, targets[i].position, targets[i].radius);
                if (col.hit && col.distance < closestDist) {
                    closestDist = col.distance;
                    hitIndex = (int)i;
                }
            }
            if (hitIndex >= 0) {
                targets[hitIndex].alive = false;
                score += 10;
                spawnTarget();
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

        for (auto &t : targets) {
            if (!t.alive) continue;
            DrawSphere(t.position, t.radius, RED);
            DrawSphereWires(t.position, t.radius, 8, 8, MAROON);
        }
        EndMode3D();

        // HUD
        DrawText(TextFormat("Skor: %d", score), 12, 12, 26, WHITE);
        DrawText(TextFormat("Silah: %s  (1/2/3 ile degistir)", weapons[currentWeapon].name.c_str()), 12, 44, 20, weapons[currentWeapon].color);
        DrawText("Sol tik: ates | Fare: bak | WASD: hareket", 12, screenHeight - 30, 18, LIGHTGRAY);

        // nişangah
        int cx = screenWidth/2, cy = screenHeight/2;
        DrawLine(cx-12, cy, cx+12, cy, SKYBLUE);
        DrawLine(cx, cy-12, cx, cy+12, SKYBLUE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

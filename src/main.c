#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "world.h"

#define CHUNK_SIZE 1.0f

void UpdateCameraCustom(Camera3D *camera) {
    Vector3 move_vector = {
        .x = (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) ? 5.1f : 0.0f -
             (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) ? 5.1f : 0.0f,
        .y = (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) ? 5.1f : 0.0f -
             (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) ? 5.1f : 0.0f,
        .z = 0.0f,
    };

    Vector3 rotate_vector = {
        .x = GetMouseDelta().x * 0.05f,
        .y = GetMouseDelta().y * 0.05f,
        .z = 0.0f,
    };

    UpdateCameraPro(camera, move_vector, rotate_vector, 0);
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - 3d camera first person");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Camera3D camera = {
        .position = {0.1f, 10.0f, 0.1f},
        .target = {0.0f, 10.0f, 0.0f},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE,
    };

    DisableCursor();
    SetTargetFPS(60);

    InfinityWorld world = {12456};

    while (!WindowShouldClose()) {
        Area *localArea = world.GetArea(camera.position);
        if (localArea != NULL) {
          world.LoadNeighbours(localArea);
        }

        UpdateCameraCustom(&camera);

        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);

        world.EachArea(
            [](Area *area, void *data) {
                Vector3 position = {
                    .x = (float)area->location.x * Area_scaleOffset,
                    .y = -150.0f,
                    .z = (float)area->location.y * Area_scaleOffset,
                };
                DrawModel(*(area->model), position, CHUNK_SIZE, YELLOW);
            },
            NULL);

        EndMode3D();

        DrawFPS(80, 20);
        EndDrawing();

        world.UnloadFarAreas(camera.position);
    }

    CloseWindow();

    return 0;
}
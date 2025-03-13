#pragma once
#include "raylib.h"
#include "raymath.h"
#include "simplex.h"

#define LANDSCAPE_NOISE_SMALL           0.001f
#define LANDSCAPE_NOISE_MEDIUM          0.01f
#define LANDSCAPE_NOISE_BIG             0.00009f
#define CHUNK_SIZE                      100
#define CHUNK_TO_MESH_SCALE             5
#define FAR_AREA_LIMIT_DISTANCE         5

typedef struct ChunkNode {
    Vector2 location;
    Image heightMap;
    float heights[CHUNK_SIZE][CHUNK_SIZE];
    bool isLoaded;

    void (*Load)(struct ChunkNode*);
    void (*Unload)(struct ChunkNode*);
    float (*CalcHeight)(struct ChunkNode*, double, double, int, int);
} ChunkNode;

ChunkNode* CreateChunkNode(Vector2 location) {
    ChunkNode* node = (ChunkNode*)malloc(sizeof(ChunkNode));
    node->location = location;
    node->isLoaded = false;
    node->Load = LoadChunkNode;
    node->Unload = UnloadChunkNode;
    node->CalcHeight = CalcHeightChunkNode;
    return node;
}

void DestroyChunkNode(ChunkNode* node) {
    if (node->isLoaded) {
        node->Unload(node);
    }
    free(node);
}

float CalcHeightChunkNode(ChunkNode* node, double X, double Y, int min, int max) {
    float height = Clamp(
        SimplexNoise_Noise(X * LANDSCAPE_NOISE_SMALL, Y * LANDSCAPE_NOISE_SMALL) +
        SimplexNoise_Noise(X * LANDSCAPE_NOISE_MEDIUM, Y * LANDSCAPE_NOISE_MEDIUM) +
        SimplexNoise_Noise(X * LANDSCAPE_NOISE_BIG, Y * LANDSCAPE_NOISE_BIG),
        min, max
    );
    return height;
}

void LoadChunkNode(ChunkNode* node) {
    node->heightMap = GenImageColor(CHUNK_SIZE, CHUNK_SIZE, BLACK);
    int locationOffsetX = node->location.x * (CHUNK_SIZE - 1);
    int locationOffsetY = node->location.y * (CHUNK_SIZE - 1);

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            float val = node->CalcHeight(node, locationOffsetX + x, locationOffsetY + y, 0, 2);
            node->heights[x][y] = val;
            Color color = { (unsigned char)((val / 2) * 255), (unsigned char)((val / 2) * 255), (unsigned char)((val / 2) * 255), 255 };
            ImageDrawPixel(&node->heightMap, x, y, color);
        }
    }
    node->isLoaded = true;
}

void UnloadChunkNode(ChunkNode* node) {
    if (node->isLoaded) {
        UnloadImage(node->heightMap);
        node->isLoaded = false;
    }
}

typedef struct Area {
    Vector2 location;
    ChunkNode* chunkNode;
    Mesh* mesh;
    Model* model;
} Area;

typedef struct InfinityWorld {
    Area** areas;
    size_t areaCount;
    size_t areaCapacity;
} InfinityWorld;

InfinityWorld* CreateInfinityWorld(int seed) {
    InfinityWorld* world = (InfinityWorld*)malloc(sizeof(InfinityWorld));
    world->areas = NULL;
    world->areaCount = 0;
    world->areaCapacity = 0;

    SimplexNoise_SetSeed(seed); // Assuming a C-compatible simplex noise setter
    return world;
}

void DestroyInfinityWorld(InfinityWorld* world) {
    for (size_t i = 0; i < world->areaCount; i++) {
        UnloadArea(world, world->areas[i]);
        free(world->areas[i]);
    }
    free(world->areas);
    free(world);
}

void EachArea(InfinityWorld* world, void (*callback)(Area*)) {
    for (size_t i = 0; i < world->areaCount; i++) {
        callback(world->areas[i]);
    }
}

Area* GetAreaByLocalPos(InfinityWorld* world, Vector2 pos) {
    for (size_t i = 0; i < world->areaCount; i++) {
        Area* area = world->areas[i];
        if ((int)area->location.x == (int)pos.x && (int)area->location.y == (int)pos.y) {
            return area;
        }
    }
    return NULL;
}

Area* LoadOrGetAreaByLocation(InfinityWorld* world, Vector2 location) {
    Area* foundArea = GetAreaByLocalPos(world, location);
    if (foundArea) {
        return foundArea;
    }

    ChunkNode* chunk = CreateChunkNode(location);
    chunk->Load(chunk);

    Texture2D texture = LoadTextureFromImage(chunk->heightMap);
    Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
    *mesh = GenMeshHeightmap(chunk->heightMap, (Vector3){ CHUNK_TO_MESH_SCALE, 1, CHUNK_TO_MESH_SCALE });

    Model* model = (Model*)malloc(sizeof(Model));
    *model = LoadModelFromMesh(*mesh);
    model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;

    Area* area = (Area*)malloc(sizeof(Area));
    area->location = location;
    area->chunkNode = chunk;
    area->mesh = mesh;
    area->model = model;

    if (world->areaCount >= world->areaCapacity) {
        world->areaCapacity = world->areaCapacity == 0 ? 8 : world->areaCapacity * 2;
        world->areas = (Area**)realloc(world->areas, sizeof(Area*) * world->areaCapacity);
    }
    world->areas[world->areaCount++] = area;

    return area;
}

void UnloadArea(InfinityWorld* world, Area* area) {
    area->chunkNode->Unload(area->chunkNode);
    UnloadTexture(area->model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    UnloadModel(*area->model);
    free(area->mesh);
    free(area->model);
    DestroyChunkNode(area->chunkNode);
}

void UnloadFarAreas(InfinityWorld* world, Vector3 pos) {
    Vector2 currentLocation = {
        ceil(pos.x / (CHUNK_SIZE * CHUNK_TO_MESH_SCALE)) - 1,
        ceil(pos.z / (CHUNK_SIZE * CHUNK_TO_MESH_SCALE)) - 1
    };

    for (size_t i = 0; i < world->areaCount;) {
        Area* area = world->areas[i];
        if (Vector2Distance(currentLocation, area->location) > FAR_AREA_LIMIT_DISTANCE) {
            UnloadArea(world, area);
            world->areas[i] = world->areas[--world->areaCount];
        } else {
            i++;
        }
    }
}
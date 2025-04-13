#pragma once
#include "raylib.h"
#include "Misc.h"
#include "imgui.h"
#include <vector>
#include <iostream>
#include <string>
#include <cmath>

using namespace std;
class Block {
public:
    Vector3 position;
    Vector3 scale;
    Vector3 rotation;
    Color color = WHITE;
    Mesh cube;
    Material mat = LoadMaterialDefault();
    Matrix transform = MatrixIdentity();
    std::string name = "Block";
    int layer = 1;

    Block(Vector3 pos = { 0, 0, 0 }, Vector3 scl = { 1, 1, 1 }, Vector3 rot = { 0, 0, 0 },
        Color col = WHITE, std::string blockName = "Block", int lay = 1)
        : position(pos), scale(scl), rotation(rot), color(col), name(blockName), layer(lay), lastScale(scl) {
        cube = GenMeshCube(scale.x, scale.y, scale.z);
    }

    void Draw() {
        if (!Vector3Equals(scale, lastScale)) {
            if (cube.vertexCount > 0) UnloadMesh(cube);
            cube = GenMeshCube(scale.x, scale.y, scale.z);
            lastScale = scale;
        }

        mat.maps->color = color;
        transform = CreateTransformMatrix(position, rotation);
        DrawMesh(cube, mat, transform);
    }

private:
    Vector3 lastScale;
};
class Animator {
private:
    std::vector<ModelAnimation*> modelAnims;
    std::vector<int> animCounts;
    int currentAnimIndex = 0;
    int animFrame = 0;
    float animTime = 0.0f;
    float animSpeed = 60.0f;

public:
    Animator() = default;

    void LoadAnimations(const std::vector<const char*>& paths) {
        for (const auto& path : paths) {
            int count = 0;
            auto anims = LoadModelAnimations(path, &count);
            modelAnims.push_back(anims);
            animCounts.push_back(count);
            std::cout << "Animator loaded: " << path << " | Anim count: " << count << std::endl;
        }
    }

    void Unload() {
        for (int i = 0; i < modelAnims.size(); ++i) {
            if (modelAnims[i]) UnloadModelAnimations(modelAnims[i], animCounts[i]);
        }
        std::cout << "Animator resources unloaded.\n";
    }

    void UpdateAnimation(Model& model, int index) {
        if (index != currentAnimIndex) {
            currentAnimIndex = index;
            animFrame = 0;
            animTime = 0.0f;
        }

        if (modelAnims[index] && animCounts[index] > 0) {
            animTime += GetFrameTime();
            float frameDuration = 1.0f / animSpeed;

            if (animTime >= frameDuration) {
                animTime -= frameDuration;
                int totalFrames = modelAnims[index][0].frameCount;
                animFrame = (totalFrames > 0) ? (animFrame + 1) % totalFrames : 0;
            }

            UpdateModelAnimation(model, modelAnims[index][0], animFrame);
        }
    }

    void SetAnimationSpeed(float fps) {
        if (fps > 0) animSpeed = fps;
    }

    int GetCurrentAnimationIndex() const { return currentAnimIndex; }
    int GetAnimationCount() const { return animCounts[currentAnimIndex]; }
};

class PhysicsBody {
public:
    float mass = 1;
    float gravity = 9.81f;
    Vector3 velocity = { 0,0 };
    BoxCollider collider;
    void ApplyGravity(Vector3& position) {
        velocity.y -= gravity * GetFrameTime();
        position.y += velocity.y * GetFrameTime();
    }
    virtual void OnCollision(PhysicsBody& other) {
    }
};

class Player : public PhysicsBody {
private:
    std::vector<Model> models;
    Animator animator;
    bool isGrounded = false;
public:
    std::vector<const char*> modelPaths;
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    Color tint;
    float moveSpeed;
    int animIndex;
    Shader shader = LoadShader(0, 0);

    Player(std::vector<const char*> paths, Vector3 pos, Vector3 scl)
        : modelPaths(paths),
        position(pos),
        scale(scl),
        rotation({ 0, 0, 0 }),
        tint(WHITE),
        moveSpeed(4.0f),
        animIndex(0)
    {
        if (paths.empty()) {
            std::cerr << "ERROR: modelPaths is empty!" << std::endl;
            return;
        }

        for (const auto& path : paths) {
            models.push_back(LoadModel(path));
        }

        animator.LoadAnimations(paths);
    }

    ~Player() {
        for (Model& m : models) UnloadModel(m);
        animator.Unload();
        std::cout << "Player resources unloaded." << std::endl;
    }

    void Update( std::vector<Block>&blocks) {
        isGrounded = false;
        ApplyGravity(position);
        animator.UpdateAnimation(models[animIndex], animIndex);
        for (Block& block : blocks) {
            if (block.layer == 0) {
				OnCollision(block);
            }
           
        }

    }

    void Draw() {
        models[animIndex].materials[0].shader = shader;
        DrawModelEx(models[animIndex], position, { 0, 1, 0 }, rotation.y, scale, tint);
		
    }

    void PlayerController() {
        float dt = GetFrameTime();
        Vector3 moveDir = { 0 };
        bool moving = false;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { moveDir.z += 1.0f; moving = true; }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { moveDir.z -= 1.0f; moving = true; }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) { moveDir.x += 1.0f; moving = true; }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) { moveDir.x -= 1.0f; moving = true; }
        if (IsKeyPressed(KEY_SPACE) && isGrounded) {
            cout << "Player Jumped" << endl;
			isGrounded = false;
            velocity.y =gravity*2.0f;
        }

        
        animIndex = moving ? 1 : 0;

        float len = sqrtf(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
        if (len > 0.01f) {
            moveDir.x /= len;
            moveDir.z /= len;

            position.x += moveDir.x * moveSpeed * dt;
            position.z += moveDir.z * moveSpeed * dt;

            rotation.y = Lerp(rotation.y, atan2f(moveDir.x, moveDir.z) * RAD2DEG, 0.1f);
        }
    }

    void OnCollision(Block& block) {
		BoundingBox playerBox = GetTransformedBoundingBox(models[animIndex], position, scale);
        BoundingBox blockBox = {
            {block.position.x - block.scale.x / 2, block.position.y - block.scale.y / 2 , block.position.z - block.scale.z / 2},
            {block.position.x + block.scale.x / 2, block.position.y + block.scale.y / 2 , block.position.z + block.scale.z / 2}
        };

        if (CheckCollisionBoxes(playerBox, blockBox)) {
            // Basic landing logic — set player on top of block
            isGrounded = true;
            position.y = blockBox.max.y;
            velocity.y = 0;

            std::cout << "Player collided and landed on block.\n";
        }
    }
};




Block* onMouseCollision(vector<Block>& blocks, Ray ray, Camera camera) {
    for (Block& block : blocks) {
        BoundingBox box = {
            {block.position.x - block.scale.x/2, block.position.y - block.scale.y/2 , block.position.z - block.scale.z/2},
            {block.position.x + block.scale.x/2, block.position.y + block.scale.y/2 , block.position.z + block.scale.z/2}
        };
        BeginMode3D(camera);
        EndMode3D();
        RayCollision colData = GetRayCollisionMesh(ray, block.cube, block.transform);
        if (colData.hit) {
            cout << "Collision" << endl;
            return &block;
        }
    }
    return nullptr;
}

void ShowBlocksUI(Block* block) {
    if (block == nullptr) return;

    ImGui::Begin("Block Editor");

    ImGui::DragFloat3("Position", (float*)&block->position, 0.1f);
    ImGui::DragFloat3("Rotation", (float*)&block->rotation, 0.1f);
    ImGui::DragFloat3("Scale", (float*)&block->scale, 0.1f, 0.1f, 10.0f);

    Vector3 normalizedColor = {
        block->color.r / 255.0f,
        block->color.g / 255.0f,
        block->color.b / 255.0f
    };

    if (ImGui::ColorEdit3("Color", (float*)&normalizedColor)) {
        block->color.r = (unsigned char)(normalizedColor.x * 255.0f);
        block->color.g = (unsigned char)(normalizedColor.y * 255.0f);
        block->color.b = (unsigned char)(normalizedColor.z * 255.0f);
    }

    ImGui::End();
}

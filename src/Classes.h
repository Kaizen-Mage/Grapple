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
    Mesh mesh;
    Material mat = LoadMaterialDefault();
    Matrix transform = MatrixIdentity();
    std::string name = "Block";
    int layer = 1;

    Block(Vector3 pos = { 0, 0, 0 }, Vector3 scl = { 1, 1, 1 }, Vector3 rot = { 0, 0, 0 },
        Color col = WHITE, std::string blockName = "Block", int lay = 1)
        : position(pos), scale(scl), rotation(rot), color(col), name(blockName), layer(lay), lastScale(scl) {
        mesh = GenMeshCube(scale.x, scale.y, scale.z);
    }

    void Draw() {
        if (!Vector3Equals(scale, lastScale)) {
            if (mesh.vertexCount > 0) UnloadMesh(mesh);
            mesh = GenMeshCube(scale.x, scale.y, scale.z);
            lastScale = scale;
        }

        mat.maps->color = color;
        if (layer == 1) {
			transform = CreateTransformMatrix(position, rotation);
			DrawMesh(mesh, mat, transform);
        }
        if (layer == 0) {
            transform = CreateTransformMatrix({ position.x - scale.x/2,position.y,position.z - scale.z/2 }, rotation);
            DrawMesh(mesh, mat, transform);
        }
        
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
		static int lastFrame = -1;

		if (index != currentAnimIndex) {
			currentAnimIndex = index;
			animFrame = 0;
			animTime = 0.0f;
			lastFrame = -1; // force update on switch
		}

		if (modelAnims[index] && animCounts[index] > 0) {
			animTime += GetFrameTime();
			float frameDuration = 1.0f / animSpeed;

			int totalFrames = modelAnims[index][0].frameCount;
			int nextFrame = animFrame;

			if (animTime >= frameDuration) {
				animTime -= frameDuration;
				nextFrame = (totalFrames > 0) ? (animFrame + 1) % totalFrames : animFrame;
			}

			if (nextFrame != lastFrame) {
				animFrame = nextFrame;
				lastFrame = animFrame;
				UpdateModelAnimation(model, modelAnims[index][0], animFrame);
			}
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
    float gravity = 19.81f;
    Vector3 velocity = { 0,0 };
    BoxCollider collider;
    void ApplyGravity(Vector3& position) {
        velocity.y -= gravity * GetFrameTime();
        velocity.y = Clamp(velocity.y, -gravity * 2, gravity * 2);
        position.y += velocity.y * GetFrameTime();
    }
    virtual void OnCollision(PhysicsBody& other) {
    }
};

class Rope {
public:
    struct Point {
        Vector3 position;
        Vector3 oldPosition;
        bool locked = false;
        bool yLocked = false;
        float yLockHeight = 0.0f;
    };
    std::vector<Point> points;
    std::vector<std::pair<int, int>> constraints;
    int numPoints = 10;
    float segmentLength = 25.0f;
    float gravity = 0.05f;
    float currentTotalLength = 0.0f;
    float maxTensionFactor = 1.5f;
    bool IsTensionMaxed() {
        currentTotalLength = 0.0f;
        for (auto [a, b] : constraints) {
            Vector3 delta = Vector3Subtract(points[b].position, points[a].position);
            currentTotalLength += Vector3Length(delta);
        }

        float baseLength = segmentLength * (numPoints - 1);
        return currentTotalLength > baseLength * maxTensionFactor;
    }

    Vector3 GetRopeDirection() {
        return Vector3Normalize(Vector3Subtract(points[0].position, points[numPoints - 1].position));
    }
    void Init(int count, Vector3 start, Vector3 end) {
        points.clear();
        constraints.clear();
        numPoints = count;

        Vector3 delta = Vector3Subtract(end, start);
        for (int i = 0; i < numPoints; i++) {
            float t = (float)i / (numPoints - 1);
            Vector3 pos = Vector3Add(start, Vector3Scale(delta, t));
            points.push_back({ pos, pos });
        }

        points[0].locked = true; // Anchor to player
        points[numPoints - 1].locked = true; // Anchor to block

        for (int i = 0; i < numPoints - 1; i++) {
            constraints.push_back({ i, i + 1 });
        }

        segmentLength = Vector3Length(Vector3Subtract(start, end)) / (numPoints - 1);
    }
    void OnRopeCollision(const vector<Block>& blocks) {
        for (int i = 0; i < points.size(); i++) {
            Point& point = points[i];
            if (point.locked) continue;

            BoundingBox pointBox = {
                Vector3Subtract(point.position, Vector3{0.05f, 0.05f, 0.05f}),
                Vector3Add(point.position, Vector3{0.05f, 0.05f, 0.05f})
            };

            for (const Block& block : blocks) {
                BoundingBox blockBox = {
                    Vector3Subtract(block.position, Vector3Scale(block.scale, 0.5f)),
                    Vector3Add(block.position, Vector3Scale(block.scale, 0.5f))
                };

                if (CheckCollisionBoxes(pointBox, blockBox)) {
                    float surfaceY = blockBox.max.y;

                    // Snap rope point to the surface
                    point.position.y = surfaceY;
                    point.oldPosition.y = surfaceY;
                    point.yLocked = true;
                    point.yLockHeight = surfaceY;

                    break;
                }
                else {
                    // If no collision, unlock the y
                    point.yLocked = false;
                }
            }
        }

        // Optional: force neighbors into same Y if they are close and both yLocked
        for (int i = 1; i < points.size() - 1; i++) {
            if (points[i].yLocked && points[i - 1].yLocked) {
                float avgY = (points[i].yLockHeight + points[i - 1].yLockHeight) * 0.5f;
                points[i].position.y = avgY;
                points[i - 1].position.y = avgY;
            }
        }
    }


    void Update(Vector3 playerPos, Vector3 blockPos) {
        points[0].position = playerPos;
        points[numPoints - 1].position = blockPos;

        for (auto& p : points) {
            if (!p.locked) {
                Vector3 velocity = Vector3Subtract(p.position, p.oldPosition);
                p.oldPosition = p.position;
                p.position = Vector3Add(p.position, velocity);
                p.position.y -= gravity;
            }
        }

        for (int i = 0; i < 20; i++) {
            for (auto [a, b] : constraints) {
                Point& p1 = points[a];
                Point& p2 = points[b];
                Vector3 delta = Vector3Subtract(p2.position, p1.position);
                float dist = Vector3Length(delta);
                float diff = (dist - segmentLength) / dist;
                Vector3 offset = Vector3Scale(delta, 0.5f * diff);

                if (!p1.locked) p1.position = Vector3Add(p1.position, offset);
                if (!p2.locked) p2.position = Vector3Subtract(p2.position, offset);
            }
        }
    }

    void DrawRope() {
        int segmentsPerPair = 6; // the more, the smoother

        for (int i = 1; i < points.size() - 2; i++) {
            for (int j = 0; j < segmentsPerPair; j++) {
                float t1 = (float)j / segmentsPerPair;
                float t2 = (float)(j + 1) / segmentsPerPair;

                Vector3 pA = CatmullRom(
                    points[i - 1].position,
                    points[i].position,
                    points[i + 1].position,
                    points[i + 2].position,
                    t1
                );

                Vector3 pB = CatmullRom(
                    points[i - 1].position,
                    points[i].position,
                    points[i + 1].position,
                    points[i + 2].position,
                    t2
                );

                DrawLine3D(pA, pB, RED);
            }
        }
    }

};
class Player : public PhysicsBody {
private:
    std::vector<Model> models;
    Animator animator;
    bool isGrounded = false;
    float cachedGroundY = 0.0f;
    Vector2 lastCheckXZ = { -9999.0f, -9999.0f };
    BoundingBox playerBox;

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

	void Update(std::vector<Block>& blocks) {
        isGrounded = false;
		// Create a ray from above the player's feet
        Ray groundRay = {
            .position = { position.x, position.y + 2.0f, position.z },
            .direction = {0,-1,0}
		};

		// Check collision against blocks
		for (Block& block : blocks) {
			OnCollision(block, groundRay);
		}

		// Only apply gravity if not grounded
		if (!isGrounded) ApplyGravity(position);

		// Animate
		animator.UpdateAnimation(models[animIndex], animIndex);
	}


    void Draw() {
        models[animIndex].materials[0].shader = shader;
        DrawModelEx(models[animIndex], position, { 0, 1, 0 }, rotation.y, scale, tint);

    }

    void PlayerController(Rope& rope, Camera Camera) {
        float dt = GetFrameTime();
        Vector3 moveDir = { 0 };
        bool moving = false;

        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) { moveDir.z += 1.0f; moving = true; }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) { moveDir.z -= 1.0f; moving = true; }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) { moveDir.x += 1.0f; moving = true; }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) { moveDir.x -= 1.0f; moving = true; }

        if (IsKeyPressed(KEY_SPACE) && isGrounded) {
            isGrounded = false;
            velocity.y = gravity * 0.6f;
        }

        animIndex = moving ? 1 : 0;

        float len = sqrtf(moveDir.x * moveDir.x + moveDir.z * moveDir.z);
        if (len > 0.01f) {
            moveDir.x /= len;
            moveDir.z /= len;

            // Check rope tension and block movement if needed
            if (!(rope.IsTensionMaxed() && Vector3DotProduct(moveDir, rope.GetRopeDirection()) > 0.7f)) {
                Quaternion targetAngle = QuaternionFromVector3ToVector3(position, Camera.position);
                float camYaw = atan2f(Camera.target.x - Camera.position.x, Camera.target.z - Camera.position.z);
                Quaternion camRotation = QuaternionFromAxisAngle({ 0, 1, 0 }, camYaw);
                moveDir = Vector3RotateByQuaternion(moveDir, camRotation);
                Vector3 movement = Vector3Scale(moveDir, moveSpeed * dt);
                position = Vector3Add(position, movement);

            }

            rotation.y = LerpAngle(rotation.y, atan2f(moveDir.x, moveDir.z) * RAD2DEG, 0.1f);
        }

    }


 
	void OnCollision(Block& block, Ray ray) {
		if (block.layer > 0) {
			// Basic collision logic for blocks (layer > 0)
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

		if (block.layer == 0) {
            RayCollision col = GetRayCollisionMesh(ray, block.mesh, block.transform);
            if (col.hit) {
                float diff = position.y - (col.point.y -playerBox.min.y);
                if (diff < 0.0) {
                    position.y += Lerp(0, abs(diff), 0.5);
                    velocity.y = 0;
                    isGrounded = true;
                }
            }
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
        DrawBoundingBox(box, RED);
        EndMode3D();
        RayCollision colData = GetRayCollisionMesh(ray, block.mesh, block.transform);
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

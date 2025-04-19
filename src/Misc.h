#pragma once
#include <raylib.h>
#include <raymath.h>
#include <iostream>
#include <vector>
#include <string>
#include "rlgl.h"
using namespace std;
typedef struct BoxCollider {
	BoundingBox collider;

    void CheckCollision(BoxCollider& box1, BoxCollider& box2) {
		if (CheckCollisionBoxes(box1.collider, box2.collider)) {
			// Handle collision
			std::cout << "Collision detected between " << box1.collider.min.x << " and " << box2.collider.min.x << std::endl;
		}
    }
} Collider;
Vector3 CatmullRom(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;

    return Vector3{
        0.5f * ((2 * p1.x) + (-p0.x + p2.x) * t +
                (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * t2 +
                (-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * t3),

        0.5f * ((2 * p1.y) + (-p0.y + p2.y) * t +
                (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * t2 +
                (-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * t3),

        0.5f * ((2 * p1.z) + (-p0.z + p2.z) * t +
                (2 * p0.z - 5 * p1.z + 4 * p2.z - p3.z) * t2 +
                (-p0.z + 3 * p1.z - 3 * p2.z + p3.z) * t3),
    };
}

Matrix CreateTransformMatrix(Vector3 position, Vector3 rotation) {
    // Convert degrees to radians because Raylib uses radians in matrix functions
    float rx = DEG2RAD * rotation.x;
    float ry = DEG2RAD * rotation.y;
    float rz = DEG2RAD * rotation.z;
    rlPushMatrix();
    Matrix translation = MatrixTranslate(position.x, position.y, position.z);
    Matrix rotX = MatrixRotateX(rx);
    Matrix rotY = MatrixRotateY(ry);
    Matrix rotZ = MatrixRotateZ(rz);

    // Combine: M = T * Rz * Ry * Rx
    Matrix transform = MatrixMultiply(MatrixMultiply(rotZ, MatrixMultiply(rotY, rotX)), translation);
    rlPopMatrix();
    return transform;
}
void DrawSine(Vector3 lastPos, float dt, float speed,float amplitude,float phase_angle=1){
    for (int i = 0; i < GetScreenWidth(); i++) {
        float x = (float)i; // X coordinate
        float y = 0.0;
        float z = amplitude * sin(i / phase_angle * DEG2RAD + dt * speed); // Z coordinate for 3D depth

        // Draw a line in 3D space
        Vector3 currentPos = { x, y, z };
        DrawLine3D(lastPos, currentPos, RAYWHITE);

        lastPos = currentPos; // Update last position to current position
    }
}
std::string Vector3ToString(Vector3 vec) {
    return "{" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + "}";
}

BoundingBox GetTransformedBoundingBox(Model model, Vector3 position, Vector3 scale, bool recalculate = false) {
	static BoundingBox cachedBoundingBox = { {0, 0, 0}, {0, 0, 0} }; // Static cache

	// Only recalculate if needed
	if (recalculate) {
		cachedBoundingBox = GetModelBoundingBox(model);
	}

	// Apply scale and position transformations
	Vector3 scaledMin = Vector3Multiply(cachedBoundingBox.min, scale);
	Vector3 scaledMax = Vector3Multiply(cachedBoundingBox.max, scale);

	// Move the box to the correct position
	scaledMin = Vector3Add(scaledMin, position);
	scaledMax = Vector3Add(scaledMax, position);

	BoundingBox transformedBB = { scaledMin, scaledMax };
	return transformedBB;
}
float LerpAngle(float from, float to, float t) {
	float difference = fmodf(to - from + 540.0f, 360.0f) - 180.0f;
	return from + difference * t;
}
Mesh LoadCenteredHeightmap(Image heightmap, Vector3 position, Vector3 size)
{
	// Generate mesh from heightmap
	Mesh mesh = GenMeshHeightmap(heightmap, size);

	// Optionally transform vertices if you want to center it manually
	// Note: Mesh itself does not have a transform matrix, so you'd need to apply the offset to its vertices

	Vector3 offset = {
		size.x / 2.0f,
		0.0f,
		size.z / 2.0f
	};

	for (int i = 0; i < mesh.vertexCount; ++i)
	{
		mesh.vertices[i * 3 + 0] -= offset.x; // X
		mesh.vertices[i * 3 + 2] -= offset.z; // Z
	}

	UploadMesh(&mesh, false); // Re-upload mesh to GPU after modification
	return mesh;
}

Vector4 Vector4Transform(Vector4 v, Matrix m) {
	Vector4 result = {
		v.x * m.m0 + v.y * m.m4 + v.z * m.m8 + v.w * m.m12,
		v.x * m.m1 + v.y * m.m5 + v.z * m.m9 + v.w * m.m13,
		v.x * m.m2 + v.y * m.m6 + v.z * m.m10 + v.w * m.m14,
		v.x * m.m3 + v.y * m.m7 + v.z * m.m11 + v.w * m.m15
	};
	return result;
}


Vector3 ScreenToWorld(Vector2 pos, Matrix view, Matrix projection) {
	Matrix invView = MatrixInvert(view);
	Matrix invProj = MatrixInvert(projection);

	// Convert screen position to Normalized Device Coordinates (NDC)
	Vector4 startNDC = {
		(pos.x / (float)GetScreenWidth() - 0.5f) * 2.0f,
		-((pos.y / (float)GetScreenHeight() - 0.5f) * 2.0f),
		-1.0f,
		1.0f
	};

	Vector4 endNDC = startNDC;
	endNDC.z = 1.0f;

	// Transform to camera space
	Vector4 startCamera = Vector4Transform(startNDC, invProj);
	Vector4 endCamera = Vector4Transform(endNDC, invProj);

	// Perspective divide
	startCamera.x /= startCamera.w;
	startCamera.y /= startCamera.w;
	startCamera.z /= startCamera.w;
	startCamera.w = 1.0f;

	endCamera.x /= endCamera.w;
	endCamera.y /= endCamera.w;
	endCamera.z /= endCamera.w;
	endCamera.w = 1.0f;

	// Transform to world space
	Vector4 startWorld = Vector4Transform(startCamera, invView);
	Vector4 endWorld = Vector4Transform(endCamera, invView);

	// Build the ray
	Vector3 origin = { startWorld.x, startWorld.y, startWorld.z };
	return origin;
	
}
static inline float GetModelHeightFromWorld(Color* pixels, int mapWidth, int mapHeight, Vector3 pos, Vector3 size)
{
	// Convert world X,Z to image-space coordinates
	int x = (int)(pos.x * (mapWidth - 1) / size.x);
	int z = (int)(pos.z * (mapHeight - 1) / size.z);

	// Clamp to valid image range
	if (x < 0) x = 0; if (x >= mapWidth) x = mapWidth - 1;
	if (z < 0) z = 0; if (z >= mapHeight) z = mapHeight - 1;

	// Fetch grayscale from pixel
	Color c = pixels[x + z * mapWidth];
	float gray = (float)(c.r + c.g + c.b) / 3.0f;

	// Scale to world height
	return gray * (size.y / 255.0f);
}

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
} Collider;;
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
void DrawRope(Vector3 startPos,Vector3 endPos,int points) {
    Vector3 lastPos = startPos;
	for (int i = 0;i < points;i++) {
		float t = (float)i / (float)(points + 1);
		Vector3 pos = Vector3Add(startPos, Vector3Scale(Vector3Subtract(endPos, startPos), t));
		DrawSphere(pos,0.1f, BLUE);
        DrawLine3D(lastPos, pos, WHITE);
        lastPos = pos;
	}
}
BoundingBox GetTransformedBoundingBox(Model model, Vector3 position, Vector3 scale) {
    BoundingBox bb = GetModelBoundingBox(model);

    // Scale min and max from origin
    bb.min = Vector3Multiply(bb.min, scale);
    bb.max = Vector3Multiply(bb.max, scale);

    // Move box to correct position
    bb.min = Vector3Add(bb.min, position);
    bb.max = Vector3Add(bb.max, position);

    return bb;
}
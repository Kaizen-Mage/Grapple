#pragma once
#include "raylib.h"
#include "Misc.h"
#include "Classes.h"
extern float camdistance;  // Distance between camera and target
extern float yaw;  // Rotation around Y-axis (left/right)
extern float pitch;  // Rotation around X-axis (up/down)
extern float offsetY;

void MainCamControls(Camera& camera, float dt, Player& player, int mode = 0) {
    float camAngle = 0.0f;
    float targetAngle = 0.0f;
    if (mode == 0) {

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 mouseDelta = GetMouseDelta();
            yaw += mouseDelta.x * 0.1f;  // Rotate around Y-axis (yaw)
            pitch -= mouseDelta.y * 0.1f;  // Rotate around X-axis (pitch)

            // Constrain pitch so the camera doesn't flip upside down
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;

            // Update camera position based on yaw and pitch
            camera.position.x = camera.target.x - camdistance * cos(DEG2RAD * pitch) * sin(DEG2RAD * yaw);
            camera.position.y = camera.target.y - camdistance * sin(DEG2RAD * pitch);
            camera.position.z = camera.target.z - camdistance * cos(DEG2RAD * pitch) * cos(DEG2RAD * yaw);
        }
        // Zoom in and out with mouse scroll
        float scroll = GetMouseWheelMove();
        camdistance -= scroll * 2.0f;  // Adjust the zoom speed (you can change the multiplier)

        // Constrain the distance to avoid zooming too close or too far
        if (camdistance < 0.01f) camdistance = 0.01f;
        if (camdistance > 100.0f) camdistance = 100.0f;
        Vector3 direction = Vector3Normalize(Vector3Subtract(camera.position, camera.target));
        camera.position = Vector3Add(camera.target, Vector3Scale(direction, camdistance));
        if (IsKeyPressed(KEY_F)) {
            ToggleFullscreen();
        }
        if (Vector3Length(Vector3Subtract(camera.position, camera.target)) < 10.) {
            Vector3 direction = Vector3Normalize(Vector3Subtract(camera.position, camera.target));
            camera.position = Vector3Add(camera.target, Vector3Scale(direction, 10.0f));

        }
    }
    if (mode == 1) {
        // --- Third-Person Camera Logic ---
        float dt = GetFrameTime(); // Get frame time

        // 1. Determine if the player is trying to move (check keys directly)
        bool isPlayerMoving = false;
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP) ||
            IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN) ||
            IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT) ||
            IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            isPlayerMoving = true;
        }

        // 2. Update Yaw and Pitch
        if (isPlayerMoving) {
            // --- Auto-Follow Camera Logic ---
            // Use the player's current rotation as the target yaw for the camera
            float targetYaw = player.rotation.y;

            // Handle angle wrapping for smooth Lerp
            float angleDiff = targetYaw - yaw;
            while (angleDiff <= -180.0f) angleDiff += 360.0f;
            while (angleDiff > 180.0f) angleDiff -= 360.0f;

            // Smoothly interpolate camera yaw towards the player's facing direction
            yaw += angleDiff * 8.0f * dt; // Adjust lerp speed (8.0f) as needed

            // Keep pitch controlled by mouse
            Vector2 mouseDelta = GetMouseDelta();
            pitch -= mouseDelta.y * 0.1f;

        }
        else {
            // --- Manual Orbit Camera Logic (when not moving) ---
            Vector2 mouseDelta = GetMouseDelta();
            yaw += mouseDelta.x * 0.1f;
            pitch -= mouseDelta.y * 0.1f;
        }

        // Constrain pitch (apply in both cases)
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -10.0f) pitch = -10.0f;

        // 3. Update Camera Target to Player Position
        Vector3 targetPos = player.position;
        // targetPos.y += 1.0f; // Optional: Look slightly above player base
        camera.target = Vector3Lerp(camera.target, targetPos, 10.0f * dt);

        // 4. Calculate Ideal Camera Position
        Vector3 idealPosition;
        idealPosition.x = camera.target.x - camdistance * cos(DEG2RAD * pitch) * sin(DEG2RAD * yaw);
        idealPosition.y = camera.target.y - camdistance * sin(DEG2RAD * pitch)+offsetY;
        idealPosition.z = camera.target.z - camdistance * cos(DEG2RAD * pitch) * cos(DEG2RAD * yaw);

        // 5. Smoothly Move Camera Position
        camera.position = Vector3Lerp(camera.position, idealPosition, 15.0f * dt);

        // 6. Handle Zoom
        float scroll = GetMouseWheelMove();
        camdistance -= scroll * 2.0f;
        if (camdistance < 2.0f) camdistance = 2.0f;
        if (camdistance > 20.0f) camdistance = 20.0f;
    }
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

Ray ScreenToWorld(Vector2 pos, Matrix view, Matrix projection) {
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
    Vector3 end = { endWorld.x,   endWorld.y,   endWorld.z };
    Vector3 direction = Vector3Normalize(Vector3Subtract(end, origin));

    Ray ray = { 0 };
    ray.position = origin;
    ray.direction = direction;
    return ray;
}

#include <iostream>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "vector"
#include "string"
#include "imgui.h"
#include "rlImGui.h"
#include "src/Camera.h"
#include "src/Misc.h"
#include "src/Classes.h"


using namespace std;
// Global parameters
float camdistance = 5.0f;  // Distance between camera and target
float yaw = 0.0f;  // Rotation around Y-axis (left/right)
float pitch = 0.0f;  // Rotation around X-axis (up/down)
float offsetY = 5.0f;
float thickness = 10.0f;
int main() {
    InitWindow(1366, 800, "Grapple");

    // Model stuff
    Model arrow = LoadModel("resources/models/arrow/arrow.gltf"); // Load model
    RenderTexture2D target = LoadRenderTexture(GetScreenWidth(),GetScreenHeight()); // Create render texture
    //Shader stuff
    Shader outline = LoadShader("src/outline.vert", "src/Hblur.frag");
	
    
    
    //Player stuff
    vector<const char*>playerAnims;
    playerAnims.push_back("resources/models/player/Vampire/Idle.glb");
    playerAnims.push_back("resources/models/player/Vampire/Run.glb");
    Player player1 = Player(playerAnims, { 0,0,0 }, {0.01f,0.01f,0.01f });
    player1.rotation = { 0,0,0 };
    player1.moveSpeed = 10.0f;
    player1.animIndex = 0;


    // Camera and ImGui setup
    rlImGuiSetup(true);

    // Define the camera to look into our 3D world
    Camera3D camera = { 0 };
    camera.position = Vector3{ 0.0f, 10.0f, -10.0f }; // Camera position
    camera.target = player1.position;     // Camera looking at point
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };         // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;                            // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    Matrix model = MatrixIdentity();
    Matrix view = GetCameraMatrix(camera);
    Matrix projection = MatrixPerspective(camera.fovy * DEG2RAD,
        (float)GetScreenWidth() / GetScreenHeight(),
        nearPlane,
        farPlane);

    int cameraMode = 0;// Camera projection type
    Block ground ={{0,-1,0},{100,1,100}};
	ground.layer = 0;
	ground.color = DARKGRAY;
	ground.name = "Ground";
	Block wall2 = { {0,0,-10},{1,2,1} };
	Block wall3 = { {10,0,10},{1,2,1} };
	vector<Block> blocks;
	blocks.push_back(wall2);
	blocks.push_back(wall3);
	blocks.push_back(ground);
    Block* selectedBlock=nullptr;
    SetTargetFPS(120);
    while (!WindowShouldClose()) {
        //shader
        SetShaderValueMatrix(outline, GetShaderLocation(outline, "model"), model);
        SetShaderValueMatrix(outline, GetShaderLocation(outline, "view"), view);
        SetShaderValueMatrix(outline, GetShaderLocation(outline, "projection"), projection);
        SetShaderValue(outline, GetShaderLocation(outline, "outlineThickness"), &thickness, SHADER_UNIFORM_FLOAT);
        //player        
        player1.Update(blocks);
        player1.PlayerController();
        player1.shader = outline;


        if (IsKeyPressed(KEY_KP_ADD)) {
            cameraMode = 1;
        }
        if (IsKeyPressed(KEY_KP_SUBTRACT)) {
            cameraMode = 0;
        }

	   	MainCamControls(camera, GetFrameTime(),player1,cameraMode);
        BeginTextureMode(target); // Activate render texture
        BeginMode3D(camera);
        ClearBackground(PURPLE);  // Clear texture background
        Vector3 lastPos = { 0,0 };
		DrawGrid(10, 1); // Draw a grid
        for (int i = 0; i < blocks.size(); i++) {
            blocks[i].Draw(); // Draw blocks
        }
        if (selectedBlock != nullptr) {
            DrawRope(player1.position, selectedBlock->position, 20);
        }
       
		player1.Draw(); // Draw player
        selectedBlock = onMouseCollision(blocks,GetScreenToWorldRay(GetMousePosition(),camera), camera);
        EndMode3D();
        EndTextureMode(); // End render texture mode
        // Begin Drawing (apply postprocessing)
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTextureRec(target.texture, { 0, 0, (float)target.texture.width, (float)-target.texture.height },  { 0, 0 }, WHITE); // Draw the render texture with applied shade
        DrawSphere(player1.position, 5, RED);
		if (selectedBlock != nullptr) {
            DrawRope(player1.position, selectedBlock->position, 20);
			if (IsKeyPressed(KEY_E)) {
				camera.target = { selectedBlock->position.x,selectedBlock->position.y,selectedBlock->position.z };
			}
		}
        rlImGuiBegin();
		ImGui::Begin("Blocks");
        ImGui::SliderFloat("Camera FOV", &camera.fovy, 1.0f, 100.0f);
		ImGui::DragFloat3("Player Position", (float*)&player1.position, 0.1f);
        ImGui::InputInt("Player Animation Index", &player1.animIndex);
		ImGui::DragFloat3("Camera.position", (float*)&camera.position, 0.1f);
		ImGui::End();
        ShowBlocksUI(selectedBlock); // Show blocks UI
        ImGui::Begin("Camera");
        ImGui::SliderFloat("Offset Y", &offsetY, -10.0, 10.0);
        ImGui::End();
        rlImGuiEnd();
		DrawText(TextFormat("Camera Mode:%d", cameraMode), 10, 40, 20, WHITE); // Draw camera mode
		DrawFPS(10, 10); // Draw FPS
        EndDrawing();
    }
	UnloadRenderTexture(target); 
	UnloadModel(arrow);
    // Cleanup
    CloseWindow(); // Close window and OpenGL context
    return 0;
}

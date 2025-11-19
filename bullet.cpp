#include "raylib.h"
#include <btBulletDynamicsCommon.h>
#include <iostream>

int main() {
    // ---------------------------------------------------------
    // 1. Raylib 初始化 (图形部分)
    // ---------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Bullet3 + Raylib Example");
    
    // 定义摄像机
    Camera3D camera = { 0 };
    camera.position = Vector3{ 10.0f, 10.0f, 10.0f }; // 相机位置
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };      // 相机看向原点
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };          // Y轴向上
    camera.fovy = 45.0f;                                // 视野
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60); // 锁定 60 帧

    // ---------------------------------------------------------
    // 2. Bullet 初始化 (物理部分)
    // ---------------------------------------------------------
    btDefaultCollisionConfiguration* collisionConfig = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfig);
    btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
    btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfig);
    
    dynamicsWorld->setGravity(btVector3(0, -10, 0));

    // --- 创建地面 ---
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0); // Y=0 处的平面
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo groundBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
    btRigidBody* groundBody = new btRigidBody(groundBodyCI);
    dynamicsWorld->addRigidBody(groundBody);

    // --- 创建盒子 ---
    // 注意：Raylib 的 DrawCube 参数是全长，Bullet 的 BoxShape 参数是半长
    btCollisionShape* boxShape = new btBoxShape(btVector3(1, 1, 1)); // 2x2x2 的盒子
    btDefaultMotionState* boxMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 10, 0)));
    btScalar mass = 1;
    btVector3 boxInertia(0, 0, 0);
    boxShape->calculateLocalInertia(mass, boxInertia);
    btRigidBody::btRigidBodyConstructionInfo boxBodyCI(mass, boxMotionState, boxShape, boxInertia);
    btRigidBody* boxBody = new btRigidBody(boxBodyCI);
    dynamicsWorld->addRigidBody(boxBody);

    // ---------------------------------------------------------
    // 3. 主循环
    // ---------------------------------------------------------
    while (!WindowShouldClose()) {
        // --- A. 物理模拟步进 ---
        dynamicsWorld->stepSimulation(1.0f / 60.0f, 10);

        // --- B. 用户输入 (重置) ---
        if (IsKeyPressed(KEY_SPACE)) {
            // 重置位置到高空
            btTransform tr;
            tr.setIdentity();
            tr.setOrigin(btVector3(0, 10, 0));
            boxBody->setWorldTransform(tr);
            boxBody->getMotionState()->setWorldTransform(tr);
            // 清除线速度和角速度，否则它会带着之前的速度瞬移
            boxBody->setLinearVelocity(btVector3(0, 0, 0));
            boxBody->setAngularVelocity(btVector3(0, 0, 0));
            // 清除受力缓存
            boxBody->clearForces(); 
        }

        // --- C. 获取物理数据用于渲染 ---
        btTransform trans;
        boxBody->getMotionState()->getWorldTransform(trans);
        float x = trans.getOrigin().getX();
        float y = trans.getOrigin().getY();
        float z = trans.getOrigin().getZ();

        // --- D. 渲染绘制 ---
        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                // 绘制地面 (Raylib Grid)
                DrawGrid(20, 1.0f);
                
                // 绘制盒子 (位置来自 Bullet)
                // 大小是 2.0f，因为 Bullet 的半长是 1
                DrawCube(Vector3{x, y, z}, 2.0f, 2.0f, 2.0f, RED); 
                DrawCubeWires(Vector3{x, y, z}, 2.0f, 2.0f, 2.0f, MAROON);

            EndMode3D();

            DrawText("Press SPACE to reset box", 10, 10, 20, DARKGRAY);
            DrawText(TextFormat("Height: %.2f", y), 10, 40, 20, DARKGRAY);

        EndDrawing();
    }

    // ---------------------------------------------------------
    // 4. 清理
    // ---------------------------------------------------------
    dynamicsWorld->removeRigidBody(boxBody);
    delete boxBody->getMotionState();
    delete boxBody;
    delete boxShape;

    dynamicsWorld->removeRigidBody(groundBody);
    delete groundBody->getMotionState();
    delete groundBody;
    delete groundShape;

    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfig;

    CloseWindow();

    return 0;
}
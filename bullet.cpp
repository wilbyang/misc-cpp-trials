#include <iostream>
#include <btBulletDynamicsCommon.h>

int main() {
    // --- 1. 初始化物理世界配置 ---
    
    // 碰撞配置：用于管理内存和碰撞算法
    btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
    
    // 碰撞调度器：处理碰撞对
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
    
    // 宽相（Broadphase）接口：快速检测潜在的碰撞对（这里使用动态包围体层次结构 Dbvt）
    btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();
    
    // 约束求解器：解决接触、关节等约束
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
    
    // 创建动态世界
    btDiscreteDynamicsWorld* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    
    // 设置重力 (Y轴向下 -10)
    dynamicsWorld->setGravity(btVector3(0, -10, 0));

    // --- 2. 创建地面 (静态刚体) ---
    
    // 创建形状：静态平面 (法线向上 (0,1,0)，距离原点 0)
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    
    // 运动状态：默认位置和旋转 (Identity)
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -1, 0)));
    
    // 构建刚体信息：质量为 0 表示它是静态的（不动的）
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape, btVector3(0, 0, 0));
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    
    // 将地面添加到世界中
    dynamicsWorld->addRigidBody(groundRigidBody);

    // --- 3. 创建下落的盒子 (动态刚体) ---
    
    // 创建形状：盒子 (半长宽高均为 1)
    btCollisionShape* fallShape = new btBoxShape(btVector3(1, 1, 1));
    
    // 运动状态：初始位置在 Y=50 的高度
    btDefaultMotionState* fallMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 50, 0)));
    
    btScalar mass = 1;
    btVector3 fallInertia(0, 0, 0);
    // 动态物体必须计算局部惯性
    fallShape->calculateLocalInertia(mass, fallInertia);
    
    // 构建刚体
    btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass, fallMotionState, fallShape, fallInertia);
    btRigidBody* fallRigidBody = new btRigidBody(fallRigidBodyCI);
    
    // 将盒子添加到世界中
    dynamicsWorld->addRigidBody(fallRigidBody);

    // --- 4. 模拟循环 ---
    
    std::cout << "Start Simulation..." << std::endl;
    
    // 模拟 300 步 (假设 60Hz，大约 5 秒)
    for (int i = 0; i < 300; i++) {
        // 步进模拟：时间步长 1/60 秒
        dynamicsWorld->stepSimulation(1.0f / 60.0f, 10);

        // 获取盒子当前的变化矩阵
        btTransform trans;
        fallRigidBody->getMotionState()->getWorldTransform(trans);

        // 输出高度 (Y坐标)
        std::cout << "Step " << i << " : Height = " << trans.getOrigin().getY() << std::endl;
        
        // 当物体接近地面时（地面在 -1，盒子半径 1，中心接近 0），它应该停止下降
    }

    // --- 5. 清理内存 (非常重要) ---
    
    // 移除刚体
    dynamicsWorld->removeRigidBody(fallRigidBody);
    delete fallRigidBody->getMotionState();
    delete fallRigidBody;
    delete fallShape;

    dynamicsWorld->removeRigidBody(groundRigidBody);
    delete groundRigidBody->getMotionState();
    delete groundRigidBody;
    delete groundShape;

    // 删除核心组件
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;

    return 0;
}
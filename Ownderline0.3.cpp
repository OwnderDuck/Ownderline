#include <raylib.h>
#include <raymath.h>
#include <bits/stdc++.h>
#include <rlgl.h>
#pragma GCC optimize(3)
#define min(_a,_b) (_a)<(_b)?(_a):(_b)
#define max(_a,_b) (_a)>(_b)?(_a):(_b)
using namespace std;
// 地形参数
const int n = 512, m = 512;
float a[514][514];
float w[514][514];
float l[514][514];
float neww[514][514];
float scale = 0.005;
const int octaves = 3;
float BeAbleSee=48;

float flowRate = 0.8f;      // 水流速率
float waterViscosity = 0.15f; // 水流粘滞系数
float inertia[514][514][2];// 水流惯性矢量场

// 玩家参数
float playerX = n/ 2.0f;
float playerZ = m/ 2.0f;
float playerRotation = 0.0f;      // 玩家水平旋转角度
float moveSpeed; 

float QXX = n/ 2.0f;
float QXZ = m/ 2.0f;

float lastWPressedTime = 0.0;      // 上次按下W的时间
int wPressCount = 0;                // W键连续按压计数
bool isSprinting = false;           // 是否处于疾跑状态
const float floatPressThreshold = 0.7; // 双击判定时间阈值（秒）

// 摄像机参数
float cameraDistance = 10.0f;      // 摄像机跟随距离
float cameraHeight = 2.5f;        // 摄像机高度
float mouseSensitivity = 0.3f;    // 鼠标灵敏度
float cameraPitch = 0.0f;          // 新增垂直视角角度
const float MAX_PITCH = 89.0f;     // 最大俯仰角度
bool isFirstPerson = 1;

bool HideUI=0;
float Time=7;

//float hun=0.005;
float hun=0;
float PlayerHeight=1.8f;

bool isDoubleClickHeld = false;
bool shiftPressed = false;

enum MyCameraMode { FIRST_PERSON, THIRD_PERSON, ISOMETRIC };
MyCameraMode cameraMode = FIRST_PERSON;
float isoDistance = 25.0f;     // 等轴视角距离
float isoAngle = 45.0f;        // 等轴视角角度
char QX[40][29]={
	"        ##      ####",//0
	"     ###########....###",
	"    #.......##..#.....#",//2
	"   #...#...##......###",
	"   ##...####.#######",//4
	"    ####............##",
	"    #................#",//6
	"   #.................#",
	"  #..................#",//8
	" ############........#",
	" ###.................#",//10
	"    ##...............##",
	"    ##...............###",//12
	"   # #...............# ##",
	"  #  #................# #",//14
	" #   #................#  #",
	"##   #................#  ##",//16
	" #   #................#  #",
	"  #  #................# ##",//18
	"  # #.................##",
	"   ##.................#",//20
	"   ##.................#",
	"    #.................#",//22
	"    #.................#",
	"    #..................#",//24
	"    #..................#",
	"    #..................#",//26
	"   #..................#",
	"   #..................#",//28
	"   ####################",
	"   #                   #",//30
	"   #                   #",
	"   #                   #",//32
	"   #                   #",
	"   #                   #",//34
	"   #                   #",
	"   #                   #",//36
	"   #                  #",
	"   #                  #",//38
	"  ##                  ###"
};
struct DoubleClickDetector {
	unordered_map<int, float> lastPressTime;  // 按键最后按压时间
	unordered_map<int, int> pressCount;       // 连续按压计数
	
	bool Check(int key, float threshold = 0.3f) {
		if (IsKeyPressed(key)) {
			const float currentTime = GetTime();
			
			// 初始化新按键记录
			if (!lastPressTime.count(key)) {
				lastPressTime[key] = 0.0f;
				pressCount[key] = 0;
			}
			
			// 计算时间差并更新状态
			if (currentTime - lastPressTime[key] < threshold) {
				pressCount[key]++;
			} else {
				pressCount[key] = 1;  // 超时重新计数
			}
			
			lastPressTime[key] = currentTime;
			
			// 当达到双击时重置并返回true
			if (pressCount[key] >= 2) {
				pressCount[key] = 0;
				return true;
			}
		}
		return false;
	}
	
	// 可选：外部手动重置状态
	void Reset(int key) {
		lastPressTime[key] = 0.0f;
		pressCount[key] = 0;
	}
}g_doubleClick;
// 在全局变量区添加生存状态结构体
struct SurvivalStats {
	float health = 100;
	float hunger = 100;
	float bodyTemp = 37.0f;
	float ambientTemp = 25.0f;
	
	void Update(float deltaTime) {
		static float timer = 0;
		timer += deltaTime;
		
		// 更新一次状态
		if(timer >= 0.01f) {
			timer = 0;
			
			// 基础消耗
			hunger = max(hunger-hun,0);
			
			// 体温调节
			//bodyTemp += (ambientTemp - bodyTemp)/k  ;
			
			// 健康检测
			if(hunger<=0) health = max(health - 0.5, 0);
			//if(bodyTemp > 39.0f || bodyTemp < 35.0f) health = max(health - 0.04, 0);
		}
	}
}g_stats;

void flow();
/*废弃*/bool LoadHeightmapFromPNG(const char* filename);
void build_PerlinNoise();/*生成地形*/
void Playerdo();/*玩家操作*/
void DrawHUD();
/*+---------------------------------------------+*/
/*|祖传屎山，build_PerlinNoise()需要用的，勿动！|*/class PerlinNoise {private:int permutation[512];public:PerlinNoise(unsigned int seed) {/*初始化排列表*/iota(permutation, permutation + 256, 0);shuffle(permutation, permutation + 256, default_random_engine(seed));/* 重复排列表以避免越界*/for (int i = 0; i < 256; ++i)permutation[256 + i] = permutation[i];}/*生成二维柏林噪声*/float noise(float x, float y) {/*确定网格单元*/int X = (int)floor(x) & 255;int Y = (int)floor(y) & 255;/*网格内相对坐标*/x -= floor(x);y -= floor(y);/*计算缓和曲线*/float u = fade(x);float v = fade(y);/*哈希索引周围四个点*/ int A = permutation[X] + Y;int AA = permutation[A & 255];int AB = permutation[(A + 1) & 255];int B = permutation[X + 1] + Y;int BA = permutation[B & 255];int BB = permutation[(B + 1) & 255];/*混合梯度贡献*/ float gradAA = grad(AA, x, y);float gradBA = grad(BA, x - 1, y);float gradAB = grad(AB, x, y - 1);float gradBB = grad(BB, x - 1, y - 1);/* 双线性插值*/float lerp1 = lerp(u, gradAA, gradBA);float lerp2 = lerp(u, gradAB, gradBB);return lerp(v, lerp1, lerp2);}private:/*缓和曲线：6t^5 - 15t^4 + 10t^3*/ static float fade(float t) {return t * t * t * (t * (t * 6 - 15) + 10);}/*线性插值*/static float lerp(float t, float a, float b) {return a + t * (b - a);}/*计算梯度值（使用预定义的四个方向）*/static float grad(int hash, float x, float y) {int h = hash & 3;switch (h) {case 0: return x + y;    /*右上方*/case 1: return -x + y;   /*左上方*/case 2: return x - y;    /*右下方*/case 3: return -x - y;   /*左下方*/default: return 0; /* 不会执行*/}}};
/*+---------------------------------------------+*/
int main() {
	build_PerlinNoise();
	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI );
	
	InitWindow(640, 480, "Ownderline");
	SetTargetFPS(120);
	DisableCursor();
	
	g_stats.ambientTemp = a[(int)round(playerX)][(int)round(playerZ)] * 0.5f;// 初始化生存状态
	Camera3D camera = { 0 };
	camera.fovy = 60.0f;
	camera.projection = CAMERA_PERSPECTIVE;
	camera.up = {0, 1, 0};
	w[256][256]=1;
	for (int i = 0+QXX-20; i < 40+QXX+20; i++) {
		for (int j = 0+QXZ-20; j < 27+QXZ+20; j++) {
			a[i][j]=1;
		}
	}
	while (!WindowShouldClose()) {
		moveSpeed=g_stats.hunger*0.06f+1;
		moveSpeed=moveSpeed/1.8f*PlayerHeight;
		Time+=0.02;
		//w[256][256]+=max(10.0/w[256][256],0.01);
		/*
		for(int i = 1; i <= n; i++) 
		for(int j = 1; j <= m; j++) 
		if(w[i][j]<max(Time-a[i][j],0))w[i][j]+=max(Time-a[i][j],0);
		*/
		/*
		int i=600,j=600;
		w[i][j]=max(Time*100-a[i][j],0);
		*/
		//g_stats.ambientTemp = a[(int)playerX][(int)playerZ];// 更新环境温度
		g_stats.Update(GetFrameTime());
		
		// 鼠标控制
		Vector2 mouseDelta = GetMouseDelta();
		playerRotation -= mouseDelta.x * mouseSensitivity;  // 水平旋转
		cameraPitch += mouseDelta.y * mouseSensitivity * 0.5f; // 垂直视角（降低灵敏度）
		cameraPitch = Clamp(cameraPitch, -MAX_PITCH, MAX_PITCH); // 限制角度
		
		// 鼠标滚轮控制
		float wheel = GetMouseWheelMove();
		if (cameraMode == THIRD_PERSON) {
			cameraDistance = cameraDistance - wheel * 2.0f;
		}
		
		if ((IsKeyDown(KEY_LEFT_CONTROL)||IsKeyDown(KEY_RIGHT_CONTROL))&&(IsKeyDown(KEY_KP_0)||IsKeyDown(KEY_ZERO))&&!isFirstPerson) cameraDistance=10.0f;
		
		Vector3 playerPos = {
			playerX,
			a[(int)round(playerX)][(int)round(playerZ)],
			playerZ
		};
		
		switch (cameraMode) {
			case FIRST_PERSON: {
				camera.position = {
					playerPos.x,
					playerPos.y + PlayerHeight/1.8f*1.6f,
					playerPos.z
				};
				
				float yaw = DEG2RAD * playerRotation;
				float pitch = -DEG2RAD * cameraPitch;
				Vector3 front = {
					sinf(yaw) * cosf(pitch),
					sinf(pitch),
					cosf(yaw) * cosf(pitch)
				};
				front = Vector3Normalize(front);
				camera.target = {
					camera.position.x + front.x,
					camera.position.y + front.y,
					camera.position.z + front.z
				};
				camera.projection = CAMERA_PERSPECTIVE;
				break;
			}
			case THIRD_PERSON: {
				camera.position = {
					playerPos.x - cameraDistance * cosf(DEG2RAD*cameraPitch) * sinf(DEG2RAD*playerRotation),
					playerPos.y + cameraDistance * sinf(DEG2RAD*cameraPitch) + cameraHeight,
					playerPos.z - cameraDistance * cosf(DEG2RAD*cameraPitch) * cosf(DEG2RAD*playerRotation)
				};
				camera.target = playerPos;
				camera.projection = CAMERA_PERSPECTIVE;
				break;
			}
			case ISOMETRIC: {
				const float baseHeight = 5.0f;
				camera.position = {
					playerPos.x + isoDistance * cosf(DEG2RAD*(isoAngle)),
					playerPos.y + baseHeight + isoDistance,
					playerPos.z + isoDistance * sinf(DEG2RAD*(isoAngle))
				};
				camera.target = playerPos;
				camera.up = (Vector3){0, 1, 0};
				camera.projection = CAMERA_ORTHOGRAPHIC;
				camera.fovy = 50.0f;
				break;
			}
		}
		
		if (cameraMode != ISOMETRIC) {
			Vector2 mouseDelta = GetMouseDelta();
			playerRotation -= mouseDelta.x * mouseSensitivity;
			cameraPitch += mouseDelta.y * mouseSensitivity * 0.5f;
			cameraPitch = Clamp(cameraPitch, -MAX_PITCH, MAX_PITCH);
		}
		Playerdo();
		static float simAccumulator = 0.0f;
		simAccumulator += GetFrameTime();
		if (simAccumulator >= 1.0f/24.0f) { // 30Hz物理更新
			flow();
			simAccumulator -= 1.0f/24.0f;
		}
		BeginDrawing();
		ClearBackground(SKYBLUE);
		BeginMode3D(camera);
		BeginScissorMode(0,0,GetScreenWidth(),GetScreenHeight());
		// 绘制地形
		BeginBlendMode(BLEND_ALPHA);
		
		for (int i = 0; i < 40; i++) {
			for (int j = 0; j < 27; j++) {
				Vector3 pos = { (float)i+QXX, 2, (float)j+QXZ };
				if(QX[i][j]=='#') DrawCubeV(pos, {1.0f, 1.0f, 1.0f}, BLACK);
				else if(QX[i][j]=='.') DrawCubeV(pos, {1.0f, 1.0f, 1.0f}, WHITE);
			}
		}
		for (int i = playerX-BeAbleSee; i <= playerX+BeAbleSee; i++) {
			for (int j = playerZ-BeAbleSee; j <= playerZ+BeAbleSee; j++) {
				
				Vector3 pos = { (float)i, a[i][j] * 0.5f, (float)j };
				DrawCubeV(pos, {1.0f, a[i][j], 1.0f}, Color{ 100, (unsigned char)a[i][j]*5, 100, 255 });
				
				if (w[i][j] > 0.01f) {
					Vector3 waterPos = { 
						(float)i, 
						a[i][j] + w[i][j] * 0.5f,  // 精确的水体位置
						(float)j 
					};
					
					// 动态颜色：水深影响颜色和透明度
					const float depth = w[i][j];
					const unsigned char alpha = (unsigned char)Clamp(80 + depth * 20, 100, 200);
					const Color waterColor = {
						30,  // R
						(unsigned char)Clamp(150 - depth * 5, 50, 200),  // G
						(unsigned char)Clamp(200 + depth * 10, 200, 250),// B
						alpha  // A
					};
					
					DrawCubeV(waterPos, 
						Vector3{1.0f, w[i][j], 1.0f}, 
						waterColor
						);
				}
				
			}
		}
		
		if(cameraMode != FIRST_PERSON){
			// 计算立方体中心位置（考虑玩家高度）
			Vector3 cubeCenter = {
				playerPos.x,
				playerPos.y + PlayerHeight/2.0f, // 将立方体垂直居中
				playerPos.z
			};
			
			// 应用旋转和平移变换
			rlPushMatrix();
			rlTranslatef(cubeCenter.x, cubeCenter.y, cubeCenter.z);
			rlRotatef(playerRotation, 0.0f, 1.0f, 0.0f); // 绕Y轴旋转（注意符号）
			
			// 绘制旋转后的立方体（原点位于中心）
			DrawCube(Vector3Zero(), 0.8f, PlayerHeight, 0.8f, RED);
			DrawCubeWiresV(Vector3Zero(), (Vector3){0.8f, PlayerHeight, 0.8f}, DARKGRAY);
			
			rlPopMatrix();
		}
		
		EndScissorMode();
		EndMode3D();
		if(!HideUI)DrawHUD();
		
		EndDrawing();
	}
	
	CloseWindow();
	return 0;
}

void DrawHUD(){
	
	DrawRectangle(0,GetScreenHeight()-20, g_stats.health / 100*GetScreenWidth()/2, 20, RED);
	DrawText("Health:", 0+10, GetScreenHeight()-20, 20, WHITE);// 生命值
	
	
	DrawRectangle(GetScreenWidth()/2, GetScreenHeight()-20, g_stats.hunger / 100*GetScreenWidth()/2, 20, ORANGE);
	DrawText("Hunger:",GetScreenWidth()/2+10, GetScreenHeight()-20, 20, WHITE);// 饱食度
	/*
	char tempStr[20];// 体温
	sprintf(tempStr, "Temp: %.1fC", g_stats.bodyTemp);
	Color tempColor = (g_stats.bodyTemp >38 || g_stats.bodyTemp <36) ? RED : SKYBLUE;
	DrawText(tempStr, hudX, hudY+120, 20, tempColor);
	//其他
	*/
	
	DrawText(TextFormat("%.1f,%.1f,%.1f", playerX, a[(int)round(playerX)][(int)round(playerZ)],playerZ), 10, 10, 20, BLACK);// 调试信息
	DrawFPS(10, 40);
	
}

void Playerdo() {
	if (IsKeyPressed(KEY_F1)) HideUI = !HideUI;
	if (IsKeyPressed(KEY_F2)) {
		time_t now = time(NULL);
		struct tm *tm = localtime(&now);
		char filename[256];
		strftime(filename, sizeof(filename), "screenshots_%Y%m%d_%H%M%S.png", tm);
		TakeScreenshot(filename); 
	}
	
	// F5切换视角模式
	if (IsKeyPressed(KEY_F5)) {
		cameraMode = static_cast<MyCameraMode>((static_cast<int>(cameraMode) + 1));
		if(static_cast<int>(cameraMode) > ISOMETRIC) cameraMode = FIRST_PERSON;
		
		if(cameraMode == ISOMETRIC) {
			isoDistance = 25.0f;
			isoAngle = 45.0f;
		}
	}
	
	// 等轴视角滚轮控制
	if (cameraMode == ISOMETRIC) {
		float wheel = GetMouseWheelMove();
		isoDistance = Clamp(isoDistance - wheel * 2.0f, 10.0f, 100.0f);
	}
	if (g_doubleClick.Check(KEY_LEFT_SHIFT)) {
		isDoubleClickHeld = true;
	}
	
	// 按下状态处理
	if (IsKeyDown(KEY_LEFT_SHIFT)) {
		if (isDoubleClickHeld) {
			PlayerHeight = 0.4f;  // 双击后按住
		} else {
			PlayerHeight = 0.9f;   // 普通按住
		}
		shiftPressed = true;
	} 
	// 释放状态处理
	else if (IsKeyReleased(KEY_LEFT_SHIFT)) {
		PlayerHeight = 1.8f;
		isDoubleClickHeld = false;
		shiftPressed = false;
	}
	
	// 防止在未双击时保持状态
	if (!shiftPressed) {
		isDoubleClickHeld = false;
	}
	if (IsKeyReleased(KEY_LEFT_SHIFT)) PlayerHeight=1.8f;
	if (IsKeyDown(KEY_SPACE)) w[(int)round(playerX)][(int)round(playerZ)] += 10.0;
	if (IsKeyDown(KEY_T)) {
		for (int dx = -5; dx <= 5; ++dx)
			for (int dy = -5; dy <= 5; ++dy)
				w[(int)round(playerX)+dx][(int)round(playerZ)+dy] = 0;
	}
	if (IsKeyDown(KEY_Q)) {
		for (int dx = -2; dx <= 2; ++dx)
			for (int dy = -2; dy <= 2; ++dy)
				a[(int)round(playerX)+dx][(int)round(playerZ)+dy] -=0.25;
	}
	if (IsKeyDown(KEY_E)) {
		for (int dx = -2; dx <= 2; ++dx)
			for (int dy = -2; dy <= 2; ++dy)
				a[(int)round(playerX)+dx][(int)round(playerZ)+dy] +=0.25;
	}
	if (g_doubleClick.Check(KEY_W))isSprinting = true;// 检测双击W逻辑
	if (IsKeyReleased(KEY_W)) isSprinting = false;// 松开W时重置疾跑状态
	Vector2 inputDir = {0};
	if (IsKeyDown(KEY_W)) {inputDir.y = 1;g_stats.hunger = max(g_stats.hunger - hun*2, 0);}
	if (IsKeyDown(KEY_S)) {inputDir.y = -1;g_stats.hunger = max(g_stats.hunger - hun*2, 0);}
	if (IsKeyDown(KEY_A)) {inputDir.x = 1;g_stats.hunger = max(g_stats.hunger - hun*2, 0);}
	if (IsKeyDown(KEY_D)) {inputDir.x = -1;g_stats.hunger = max(g_stats.hunger -hun*2, 0);}
	float currentSpeed = moveSpeed;
	if (isSprinting && inputDir.y == 1) { // 只有向前时加速
		currentSpeed *= 2.0f; // 疾跑速度翻倍
		// 这里可以添加体力消耗逻辑
		g_stats.hunger = max(g_stats.hunger - 2.5*hun, 0);
	}
	
	// 修复方向计算
	float cosTheta = cosf(DEG2RAD * playerRotation);
	float sinTheta = sinf(DEG2RAD * playerRotation);
	Vector3 moveDir = {
		inputDir.x * cosTheta + inputDir.y * sinTheta,  // 修复方向符号
		0.0f,
		inputDir.y * cosTheta - inputDir.x * sinTheta   // 修复方向符号
	};
	// 应用速度参数和帧时间
	float deltaTime = GetFrameTime();
	playerX = Clamp(playerX + moveDir.x * currentSpeed * deltaTime, 1.0f, n);
	playerZ = Clamp(playerZ + moveDir.z * currentSpeed * deltaTime, 1.0f, m);
	
	
	
	// 交互操作
	
}
/*------------------*/
void build_PerlinNoise() {
	PerlinNoise pn(time(0));
	
	// 噪声参数配置
	const float baseScale = 0.03f;    // 基础噪声尺度
	const int octaves = 5;            // 噪声层数
	const float persistence = 0.45f;  // 振幅衰减率
	const float lacunarity = 2.0f;    // 频率倍增率
	
	
	// 第一阶段：生成基础地形
	for (int i = 1; i <= n; i++) {
		for (int j = 1; j <= m; j++) {
			float x = i * baseScale;
			float y = j * baseScale;
			
			float total = 0.0;
			float amplitude = 1.0;
			float frequency = 1.0;
			float maxAmplitude = 0.0;
			
			// 多层噪声叠加
			for (int k = 0; k < octaves; ++k) {
				total += pn.noise(x * frequency, y * frequency) * amplitude;
				maxAmplitude += amplitude;
				amplitude *= persistence;
				frequency *= lacunarity;
			}
			
			// 高度归一化与曲线调整
			float height = (total / maxAmplitude) * 0.5 + 0.5; // [-1,1] -> [0,1]
			height = pow(height, 1.5f);  // 增强中间过渡
			
			// 基础高度映射
			a[i][j] = height * 50.0f;    // 最大高度50单位
		}
	}
	
	// 第二阶段：地形平滑（5x5高斯模糊）
	for(int s = 0; s < 3; ++s) {
		for(int i = 3; i < n-3; ++i) {
			for(int j = 3; j < m-3; ++j) {
				float sum = 
				a[i-2][j] * 0.05f + a[i-1][j] * 0.1f +
				a[i+1][j] * 0.1f + a[i+2][j] * 0.05f +
				a[i][j-2] * 0.05f + a[i][j-1] * 0.1f +
				a[i][j+1] * 0.1f + a[i][j+2] * 0.05f +
				a[i][j] * 0.4f;
				a[i][j] = sum;
			}
		}
	}
	
	// 第三阶段：侵蚀模拟
	const float erosionStrength = 0.3f;
	const int erosionRadius = 2;
	for(int i = erosionRadius; i < n-erosionRadius; ++i) {
		for(int j = erosionRadius; j < m-erosionRadius; ++j) {
			float maxSlope = 0;
			
			// 检测周围最大坡度
			for(int dx = -1; dx <= 1; ++dx) {
				for(int dz = -1; dz <= 1; ++dz) {
					if(dx == 0 && dz == 0) continue;
					float slope = a[i+dx][j+dz] - a[i][j];
					maxSlope = max(maxSlope, slope);
				}
			}
			
			// 坡度侵蚀处理
			if(maxSlope > 1.5f) {
				float erosion = maxSlope * erosionStrength;
				for(int dx = -erosionRadius; dx <= erosionRadius; ++dx) {
					for(int dz = -erosionRadius; dz <= erosionRadius; ++dz) {
						float weight = 1.0f / (1 + abs(dx) + abs(dz));
						a[i+dx][j+dz] -= erosion * weight;
					}
				}
			}
		}
	}
	
	// 第四阶段：添加细节噪声
	const float detailScale = 0.2f;
	for(int i = 1; i <= n; i++) {
		for(int j = 1; j <= m; j++) {
			float x = i * detailScale;
			float y = j * detailScale;
			
			// 生成细节噪声（范围-2.5~+2.5）
			float detail = pn.noise(x, y) * 2.5f; 
			
			// 根据地势调整细节强度
			float heightFactor = 1.0f - (a[i][j] / 50.0f);
			detail *= heightFactor * 0.8f;  // 低处细节更强
			
			a[i][j] += detail;
		}
	}
	
	// 最终高度限制
	for(int i = 1; i <= n; i++) {
		for(int j = 1; j <= m; j++) {
			a[i][j] = Clamp(a[i][j], 0.0f, 55.0f);
			w[i][j]=max(15-a[i][j],0);
		}
	}
}
/*|-------废物部分-------|*/
/*| ！！！屎山部分！！！ |*/


bool LoadHeightmapFromPNG(const char* filename) {
	// 加载图像
	Image img = LoadImage(filename);
	if (!img.data) {
		TraceLog(LOG_WARNING, "无法加载高度图文件: %s", filename);
		return false;
	}
	
	// 转换为灰度格式（确保单通道）
	ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
	
	// 检查尺寸是否匹配
	if (img.width != n || img.height != m) {
		TraceLog(LOG_WARNING, "图像尺寸不匹配，需要 %dx%d，实际 %dx%d", 
			n, m, img.width, img.height);
		UnloadImage(img);
		return false;
	}
	
	// 提取灰度数据
	unsigned char *grayData = (unsigned char*)img.data;
	for (int y = 1; y < m; ++y) {
		for (int x = 1; x < n; ++x) {
			// 图像Y轴需要翻转（图像原点在左上，地形原点在左下）
			int imgY = m - 1 - y;
			a[x+1][y+1] = static_cast<float>(grayData[imgY * n + x]);
		}
	}
	
	UnloadImage(img);
	return true;
}
void flow() {
	static const int dx[] = {-1, 0, 1, 0}; // 静态常量方向数组
	static const int dy[] = {0, -1, 0, 1};
	
	// 计算有效遍历范围
	const int start_i = max(1, playerX - BeAbleSee);
	const int end_i = min(n, playerX + BeAbleSee);
	const int start_j = max(1, playerZ - BeAbleSee);
	const int end_j = min(m, playerZ + BeAbleSee);
	
	memset(neww, 0, sizeof(neww)); // 清空水流变化数组
	
	// 第一遍遍历：计算水流变化
	for(int i = start_i; i <= end_i; ++i) {
		for(int j = start_j; j <= end_j; ++j) {
			const float w_ij = w[i][j];
			if (w_ij < 0.001f) continue; // 忽略无水单元格
			if (w_ij < 0.001f)
				// 边界消减（每次减少0.1，不依赖水位）
				if (i == 1 || i == n || j == 1 || j == m) {
					neww[i][j] -= 0.1f;
				}
			
			// 缓存地形高度和水位总和
			const float currentHeight = a[i][j] + w_ij;
			
			// 四方向流动处理
			for(int k = 0; k < 4; ++k) {
				const int ni = i + dx[k];
				const int nj = j + dy[k];
				
				if(ni >= 1 && ni <= n && nj >= 1 && nj <= m) {
					const float neighborHeight = a[ni][nj] + w[ni][nj];
					
					if(currentHeight > neighborHeight) {
						const float heightDiff = currentHeight - neighborHeight;
						const float flowAmount = max(min(w_ij, heightDiff * 0.2f), 0.0f);
						
						neww[i][j] -= flowAmount;
						neww[ni][nj] += flowAmount;
					}
				}
			}
		}
	}
	
	// 第二遍遍历：应用水流变化
	for(int i = start_i; i <= end_i; ++i) {
		for(int j = start_j; j <= end_j; ++j) {
			w[i][j] = max(w[i][j] + neww[i][j], 0.0f);
			if(a[i][j]<=0) w[i][j]=max(w[i][j]+0.005*a[i][j],0);
		}
	}
}

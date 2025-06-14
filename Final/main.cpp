#define _USE_MATH_DEFINES
#include <iostream>
#include <math.h>
#include <cmath>
#include <stdlib.h>
#include <vector>
#include <GL/freeglut.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <chrono>
#include <random>

#include "objReader.h"

using namespace std;

// 確保 M_PI 在所有編譯環境下都被定義為 float
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif


// Forward declarations
void DrawObject(ObjModel* model, int);
float GetRandomRange(float min, float max);
void SetTriangleNormal(vector<float> p1, vector<float> p2, vector<float> p3);
// 統一將參數命名為 radiusTop (錐體Y=0處的半徑) 和 radiusBottom (錐體Y=-height處的半徑)
void DrawSuctionBeam(float radiusTop, float radiusBottom, float height, float segments, float alpha, float pulseFactor);


// Constants
const float cameraMoveSpeed = 12.5f, cameraRotateSpeed = 3.5f;
const int TEXTURE_NUMBER = 8; // 增加一個紋理槽給星星、地球和劍 (之前是7，現在是8)
const int bluedotTextureIndex = 0, reddotTextureIndex = 1, greendotTextureIndex = 2, UFOTextureIndex = 3, starTextureIndex = 4, earthTextureIndex = 5, swordTextureIndex = 6; // 新增星星紋理索引、地球紋理索引和劍紋理索引

const int DOT_NUMBER = 20; // 增加點的數量以營造更多效果
const int UFO_NUMBER = 1;
const int STAR_NUMBER = 50; // 星星的數量
const int SWORD_NUMBER = 4; // 劍的數量

bool isPlaying = false;
float blendFactor = 0.0f;
GLuint skyTextures[2];

int windowWidth = 800, windowHeight = 600;

// *** 修改: 調整環境光、漫射光和背景色以呈現魔幻藍紫色調 ***
// Re-typed these lines carefully to remove any hidden characters (U+00A0)
GLfloat ambientLight[] = { 0.1f, 0.05f, 0.2f, 1.0f }; // 深紫色環境光
GLfloat diffuseLight[] = { 0.5f, 0.4f, 0.8f, 1.0f }; // 淺紫色漫射光
GLfloat specular[] = { 0.5f, 0.5f, 1.0f, 1.0f }; // 較亮的藍色高光
GLfloat lightPos[] = { 1000, 5000, 1000, 1 }; // 這裡也檢查一下，雖然沒報錯，但最好也重打

GLfloat mat_a[] = { 0.8f, 0.8f, 0.8f, 1.0 };
GLfloat mat_d[] = { 0.3f, 0.3f, 0.3f, 1.0 };
GLfloat mat_s[] = { 0.3f, 0.3f, 0.3f, 1.0 };
GLfloat low_sh[] = { 0.35f };

GLuint textures[TEXTURE_NUMBER];

float cameraPos[] = { -90, -10, -50 }, cameraRotate[] = { 5, 130, 0 }, cameraDistance = 240;
float groundHeight = 3.0f, groundSize = 3000;

ObjModel dotBlue, dotRed, dotGreen, ufo; // Assuming ufo here will be used for scaling reference
ObjModel starModel; // 星星模型
ObjModel swordModel; // 劍模型

std::random_device rd;
std::mt19937 gen(rd());


struct UFO
{
	float position[3] = { 0, 0, 0 }, rotateDegree = 0;
	float t = 0;
	float A = 200, B = 100, height = 80;
	float orbitSpeed = 1.0f;
	float verticalOscillationSpeed = 2.0f;
	float verticalAmplitude = 5.0f;
	float beamPulseTime = 0.0f; // for beam pulsation
	float ufoScale = 5.0f; // UFO模型縮放比例

	float ufoBottomLocalY = -15.0f;

	// *** 修改: 吸光束參數調整，讓尖端更細，底部更寬，長度可以更長 ***
	float beamRadiusAtUFO = 0.5f; // 吸光特效在UFO下方（尖端）的半徑，更細
	float beamRadiusAtGround = 25.0f; // 吸光特效在地面處（寬口）的半徑，更寬
	float beamLength = 70.0f; // 吸光特效的長度，更長

	void DrawSelf() {
		glPushMatrix();
		glTranslatef(position[0], groundHeight + position[1], position[2]);
		glRotatef(rotateDegree, 0, 1, 0);
		glScalef(ufoScale, ufoScale, ufoScale); // 使用ufoScale成員
		glColor3f(1.0f, 1.0f, 1.0f);
		DrawObject(&ufo, UFOTextureIndex);
		glPopMatrix();

		// --- 繪製吸光特效 ---
		glPushMatrix();
		// 將坐標系平移到UFO的中心位置 (世界座標)
		glTranslatef(position[0], groundHeight + position[1], position[2]);

		// 進一步平移，使得吸光特效的「尖端」（錐體的局部Y=0位置）對齊UFO模型的底部。
		// ufoBottomLocalY * ufoScale 是UFO底部相對於UFO中心點的實際Y偏移
		// 所以，錐體的尖端會被移動到 UFO 模型的底部。
		// *** 修改: 讓吸光束從UFO底部下方一點開始，形成一點點的間隙感 ***
		glTranslatef(0.0f, 0.0f, 0.0f);

		// 計算吸光特效的脈動因子
		// 讓脈動更明顯
		float pulseFactor = (sin(beamPulseTime * 8.0f) * 0.5f + 0.5f) * 0.7f + 0.3f; // 脈動範圍 0.3 到 1.0
		float beamAlpha = 0.6f * pulseFactor; // 最大透明度約 0.6，比原來更亮

		// 調用繪製吸光特效的函式
		DrawSuctionBeam(beamRadiusAtUFO, // 尖端半徑 (靠近UFO)
			beamRadiusAtGround * pulseFactor, // 寬口半徑 (靠近地面，脈動更明顯)
			beamLength,
			20, // 分段數
			beamAlpha,
			pulseFactor);
		glPopMatrix();
	}
	void Update(float deltaTime) {
		t += orbitSpeed * deltaTime;
		position[0] = A * sin(t);
		position[2] = B * sin(t) * cos(t);
		position[1] = height + sin(t * verticalOscillationSpeed) * verticalAmplitude;
		rotateDegree += 100.0f * deltaTime;
		beamPulseTime += deltaTime; // 更新吸光特效脈動時間
	}
};

UFO ufoArray[UFO_NUMBER];


// 在 Earth 結構體中
struct Earth {
	float position[3] = { 0, -200.0f, 0 }; // 地球的中心位置，這裡作為公轉的參考中心
	float radius = 150.0f; // 地球半徑
	float rotateDegree = 0; // 自轉角度
	float rotationSpeed = 10.0f; // 自轉速度

	// 新增公轉參數
	float orbitRadius = 800.0f; // 公轉半徑，地球離中心點的距離
	float orbitDegree = 0;
	float orbitSpeed = 5.0f;
	float orbitCenter[3] = { 0.0f, 0.0f, 0.0f }; // 公轉中心點，可以設定為場景原點或其他物體的位置

	void DrawSelf() {
		glPushMatrix();

		// 應用公轉的平移
		// 計算地球在公轉軌道上的當前位置
		// *** 修正: 移除 glm::radians 並使用手動轉換，同時顯式轉換為 float ***
		float currentOrbitX = orbitCenter[0] + orbitRadius * cos(orbitDegree * M_PI / 180.0f);
		float currentOrbitZ = orbitCenter[2] + orbitRadius * sin(orbitDegree * M_PI / 180.0f);

		// 將地球的 Y 軸位置獨立出來，保持其相對於公轉中心的垂直位置
		glTranslatef(currentOrbitX, position[1], currentOrbitZ);

		glRotatef(rotateDegree, 0, 1, 0); // 自轉 (繞自己的Y軸)

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textures[earthTextureIndex]); // 綁定地球紋理

		GLUquadric* quad = gluNewQuadric();
		gluQuadricDrawStyle(quad, GLU_FILL);
		gluQuadricNormals(quad, GLU_SMOOTH);
		gluQuadricTexture(quad, GL_TRUE); // 啟用紋理生成

		glColor3f(1.0f, 1.0f, 1.0f); // 確保地球模型不受頂層顏色影響
		gluSphere(quad, radius, 50, 50); // 繪製球體
		gluDeleteQuadric(quad);

		glDisable(GL_TEXTURE_2D);
		glPopMatrix();
	}

	void Update(float deltaTime) {
		rotateDegree += rotationSpeed * deltaTime; // 更新自轉角度
		orbitDegree += orbitSpeed * deltaTime;
		if (orbitDegree >= 360.0f) {
			orbitDegree -= 360.0f; // 保持角度在 0-360 之間
		}
	}
};

Earth earth; // 宣告一個地球實例

// --- 新增 Earth 結構體結束 ---


struct Dot
{
	float position[3] = { 0, 0, 0 }, rotateDegree = 0;
	float spawnTime = 0.0f;
	float dotScale = 0.01f;
	float dotAlpha = 0.0f;
	int textureIndex = 0;
	ObjModel* model = nullptr;
	float fadeInDuration = 2.0f;
	float scaleUpDuration = 2.0f;
	float initialScale = 0.01f;
	float targetScale = 0.61f;
	float floatSpeed = 0.5f;
	float floatAmplitude = 5.0f;
	float initialY = 0.0f;

	float targetPosition[3] = { 0, 0, 0 };
	float followSpeed = 0.05f;
	int assignedUFOIndex = 0;

	// *** NEW: 用於發光效果的顏色，可以調整為魔幻色系 ***
	float emitColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f }; // 預設白色，可在 InitModel 中依 textureIndex 改變

	void DrawDot() {
		glPushMatrix();
		glTranslatef(position[0], groundHeight + position[1], position[2]);
		glScalef(dotScale, dotScale, dotScale);

		// *** 修改: 將點點的顏色設定為帶有發光感的顏色，並啟用 GL_ADD 混合模式 ***
		// GL_ADD 混合模式可以讓物體顏色疊加，模擬發光效果
		// 注意：這會讓點點變得非常亮，可能需要調整 emitColor 和 alpha
		// 也可以嘗試 GL_ONE, GL_ONE 混合模式，它會讓顏色更強烈
		glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 模擬發光混合模式

		// 將點的顏色設定為其發光色，並應用 alpha
		glColor4f(emitColor[0], emitColor[1], emitColor[2], dotAlpha * emitColor[3]);

		if (model != nullptr)
			DrawObject(model, textureIndex);

		glPopMatrix();
	}

	void Update(float currentTime, float deltaTime) {
		float elapsed = currentTime - spawnTime;

		if (elapsed < fadeInDuration) {
			float t = elapsed / fadeInDuration;
			dotAlpha = t * t * (3.0f - 2.0f * t);
			dotScale = initialScale + (targetScale - initialScale) * (t * t * (3.0f - 2.0f * t));
		}
		else {
			dotAlpha = 1.0f;
			dotScale = targetScale;
		}

		targetPosition[0] = ufoArray[assignedUFOIndex].position[0];
		targetPosition[1] = ufoArray[assignedUFOIndex].position[1];
		targetPosition[2] = ufoArray[assignedUFOIndex].position[2];

		// 讓點點加速飛向UFO，增加動感
		float distanceToUFO = sqrt(pow(targetPosition[0] - position[0], 2) +
			pow(targetPosition[1] - position[1], 2) +
			pow(targetPosition[2] - position[2], 2));
		float dynamicFollowSpeed = followSpeed * (1.0f + (100.0f / (distanceToUFO + 10.0f))); // 距離越近，速度越快

		position[0] += (targetPosition[0] - position[0]) * dynamicFollowSpeed * deltaTime;

		float targetBaseY = targetPosition[1];
		position[1] += (targetBaseY - position[1]) * dynamicFollowSpeed * deltaTime;
		float bobbingOffset = sin(currentTime * floatSpeed) * floatAmplitude;
		position[1] += bobbingOffset;

		position[2] += (targetPosition[2] - position[2]) * dynamicFollowSpeed * deltaTime;
	}
};

Dot dotArray[DOT_NUMBER];

// --- 新增 Star 結構體 ---

struct Star {
	float position[3];
	float size;
	float flickerSpeed;
	float flickerPhase; // 錯開閃爍時間
	float currentAlpha;
	ObjModel* model;

	void Draw() {
		glPushMatrix();
		glTranslatef(position[0], position[1], position[2]);

		// 新增：Billboarding - 讓星星永遠面對攝影機
		// 這裡應用攝影機旋轉角度的負值，以抵消攝影機本身的旋轉
		// 旋轉順序要和 RenderScene 中攝影機旋轉的逆序相反
		// 攝影機是先繞X，再繞Y，再繞Z旋轉的 (cameraRotate[0], cameraRotate[1], cameraRotate[2])
		// 所以這裡需要先抵消Z，再抵消Y，最後抵消X
		glRotatef(-cameraRotate[2], 0, 0, 1); // 抵消 Z 軸旋轉 (Roll)
		glRotatef(-cameraRotate[1], 0, 1, 0); // 抵消 Y 軸旋轉 (Yaw)
		glRotatef(-cameraRotate[0], 1, 0, 0); // 抵消 X 軸旋轉 (Pitch)

		glScalef(size, size, size);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 發光混合模式
		glColor4f(1.0f, 1.0f, 1.0f, currentAlpha); // 星星發白光，透明度受 currentAlpha 控制

		if (model != nullptr)
			DrawObject(model, starTextureIndex);

		glPopMatrix();
	}

	void Update(float currentTime) {
		// 讓星星亮度在一個範圍內變化，模擬閃爍
		currentAlpha = 0.5f + 0.5f * sin(currentTime * flickerSpeed + flickerPhase); // 亮度範圍 0.0 到 1.0
	}
};

Star starArray[STAR_NUMBER]; // 星星陣列

// --- 新增 Sword 結構體 ---
struct Sword {
	float position[3]; // 世界座標位置
	float orbitRadius; // 圍繞地球的半徑
	float orbitAngle;  // 當前軌道角度
	float orbitSpeed;  // 軌道速度
	float selfRotateSpeed; // 自轉速度
	float currentSelfRotateAngle; // 當前自轉角度
	ObjModel* model;
	float scale;
	int textureIndex;

	void DrawSelf() {
		glPushMatrix();
		// 首先，移動到地球的位置
		glTranslatef(earth.orbitCenter[0] + earth.orbitRadius * cos(earth.orbitDegree * M_PI / 180.0f),
			earth.position[1],
			earth.orbitCenter[2] + earth.orbitRadius * sin(earth.orbitDegree * M_PI / 180.0f));

		// 然後，相對於地球中心，移動到劍的軌道位置
		glTranslatef(orbitRadius * cos(orbitAngle * M_PI / 180.0f),
			0.0f, // 保持在地球的Y平面上公轉
			orbitRadius * sin(orbitAngle * M_PI / 180.0f));

		// 移除這一行，或者將其註釋掉
		// glRotatef(orbitAngle + 90.0f, 0, 1, 0); // 讓劍的尖端指向公轉方向外側

		// 如果劍模型本身是水平的，你可能需要一個初始旋轉使其垂直。
		// 假設你的 sword.obj 模型默認是水平的 (例如，沿X軸或Z軸)，
		// 為了讓它垂直，你可能需要一個固定的 X 或 Z 軸旋轉。
		// 如果劍的「向上」方向在模型中是X軸，則需要繞Z軸旋轉-90度。
		// 如果劍的「向上」方向在模型中是Z軸，則需要繞X軸旋轉90度。
		// 請根據你的 sword.obj 模型的原始方向進行調整。
		// 這裡假設劍模型的尖端指向Z軸正方向，你需要繞X軸旋轉90度使其垂直。
		glRotatef(90.0f, 1, 0, 0); // 固定旋轉使劍垂直 (假設模型尖端朝Z軸)

		// 劍的自轉
		glRotatef(currentSelfRotateAngle, 0, 0, 1); // 讓劍繞自己的Z軸旋轉，模擬劍身旋轉

		glScalef(scale, scale, scale);
		glColor3f(1.0f, 1.0f, 1.0f); // 確保劍模型顏色正確

		if (model != nullptr) {
			DrawObject(model, textureIndex);
		}
		glPopMatrix();
	}

	void Update(float deltaTime) {
		orbitAngle += orbitSpeed * deltaTime;
		if (orbitAngle >= 360.0f) {
			orbitAngle -= 360.0f;
		}
		currentSelfRotateAngle += selfRotateSpeed * deltaTime;
		if (currentSelfRotateAngle >= 360.0f) {
			currentSelfRotateAngle -= 360.0f;
		}
	}
};

Sword swordArray[SWORD_NUMBER]; // 劍陣列
// --- 新增 Sword 結構體結束 ---


void NormalizeTriangle(vector<float>* v) {
	float len = sqrt(v->at(0) * v->at(0) + v->at(1) * v->at(1) + v->at(2) * v->at(2));
	for (int i = 0; i <= 2; i++) {
		v->at(i) /= len;
	}
}

void SetTriangleNormal(vector<float> p1, vector<float> p2, vector<float> p3) {
	float d1[] = { p1[0] - p2[0], p1[1] - p2[1] , p1[2] - p2[2] },
		d2[] = { p2[0] - p3[0], p2[1] - p3[1] , p3[2] - p3[2] };
	vector<float> normal = {
		d1[1] * d2[2] - d1[2] * d2[1],
		d1[2] * d2[0] - d1[0] * d2[2],
		d1[0] * d2[1] - d1[1] * d2[0]
	};
	glNormal3f(normal[0], normal[1], normal[2]);
}

void DrawObject(ObjModel* model, int textureIndex) {
	if (model == nullptr) return;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[textureIndex]);

	glBegin(GL_TRIANGLES);

	for (int i = 0; i < model->face.size(); i++) {
		vector<float> p1 = model->vertex[model->face[i][0]];
		vector<float> p2 = model->vertex[model->face[i][1]];
		vector<float> p3 = model->vertex[model->face[i][2]];

		SetTriangleNormal(p1, p2, p3);

		if (!model->faceTex.empty() && model->faceTex.size() > i) {
			glTexCoord2f(model->texture[model->faceTex[i][0]][0], model->texture[model->faceTex[i][0]][1]);
		}
		glVertex3f(p1[0], p1[1], p1[2]);

		if (!model->faceTex.empty() && model->faceTex.size() > i) {
			glTexCoord2f(model->texture[model->faceTex[i][1]][0], model->texture[model->faceTex[i][1]][1]);
		}
		glVertex3f(p2[0], p2[1], p2[2]);

		if (!model->faceTex.empty() && model->faceTex.size() > i) {
			glTexCoord2f(model->texture[model->faceTex[i][2]][0], model->texture[model->faceTex[i][2]][1]);
		}
		glVertex3f(p3[0], p3[1], p3[2]);
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

// *** 修正後的 DrawSuctionBeam 函式：增加顏色漸變 ***
void DrawSuctionBeam(float radiusTip, float radiusBase, float height, float segments, float alpha, float pulseFactor) {
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // GL_ONE 混合模式讓光束更亮，類似發光

	// 調整光束顏色，可以嘗試不同的顏色漸變
	// 從UFO端（亮藍色）到地面（亮紫色或洋紅色）
	float colorTip[4] = { 0.5f, 0.7f, 1.0f, alpha }; // 亮藍色
	float colorBase[4] = { 0.8f, 0.4f, 1.0f, alpha }; // 亮紫色/洋紅色

	// 繪製錐體尖端圓盤 (位於局部Y=0)
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(colorTip[0], colorTip[1], colorTip[2], colorTip[3]); // 尖端顏色
	glVertex3f(0.0f, 0.0f, 0.0f); // 圓盤中心在局部Y=0
	for (int i = 0; i <= segments; ++i) {
		float angle = 2.0f * M_PI * i / segments;
		float x = radiusTip * cos(angle);
		float z = radiusTip * sin(angle);
		glVertex3f(x, 0.0f, z); // 圓盤上的點 (尖端)
	}
	glEnd();

	// 繪製錐體側面 (從Y=0尖端到Y=-height寬口)
	glShadeModel(GL_SMOOTH); // 啟用平滑著色，用於顏色漸變
	glBegin(GL_TRIANGLE_STRIP);
	for (int i = 0; i <= segments; ++i) {
		float angle = 2.0f * M_PI * i / segments;
		float x_tip = radiusTip * cos(angle);
		float z_tip = radiusTip * sin(angle);
		float x_base = radiusBase * cos(angle);
		float z_base = radiusBase * sin(angle);

		// 尖端點 (局部Y=0) - 應用尖端顏色
		glColor4f(colorTip[0], colorTip[1], colorTip[2], colorTip[3]);
		glVertex3f(x_tip, 0.0f, z_tip);

		// 寬口點 (局部Y=-height) - 應用寬口顏色
		glColor4f(colorBase[0], colorBase[1], colorBase[2], colorBase[3]);
		glVertex3f(x_base, -height, z_base);
	}
	glEnd();
	glShadeModel(GL_FLAT); // 恢復預設，避免影響其他繪製

	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING); // 重新啟用光照
}


float GetRandomRange(float min, float max) {
	std::uniform_real_distribution<float> dist(min, max);
	return dist(gen);
}

void ResetRender() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// *** 修改: 將背景清除顏色調整為深藍或深紫，營造夜間或魔幻氣氛 ***
	glClearColor(0.05f, 0.0f, 0.1f, 1); // 深藍紫
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	// gluLookAt(0, 0, cameraDistance, 0, 0, 0, 0, 1, 0); // Moved to RenderScene for per-view setup
}

// Modified to accept a texture ID for the base background
void DrawFullScreenQuad(GLuint textureID, float alpha = 1.0f) {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, windowWidth, 0, windowHeight);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	// *** 修改: 背景混合模式，讓兩種天空紋理疊加，創造更豐富的夜空效果 ***
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 嘗試 GL_ONE, GL_ONE_MINUS_SRC_ALPHA，或 GL_ONE_MINUS_SRC_COLOR

	glBindTexture(GL_TEXTURE_2D, textureID);
	glColor4f(1.0f, 1.0f, 1.0f, alpha);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 1); glVertex2f(0, 0);
	glTexCoord2f(1, 1); glVertex2f(windowWidth, 0);
	glTexCoord2f(1, 0); glVertex2f(windowWidth, windowHeight);
	glTexCoord2f(0, 0); glVertex2f(0, windowHeight);
	glEnd();

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

// Updated DrawBackground to take the primary sky texture index
void DrawBackground(int primarySkyIndex) {
	glDisable(GL_LIGHTING);

	// Draw the primary background (either dark or bright sky)
	DrawFullScreenQuad(skyTextures[primarySkyIndex], 1.0f);

	// Determine the secondary background to blend with
	int secondarySkyIndex = (primarySkyIndex == 0) ? 1 : 0; // If primary is dark (0), secondary is bright (1), and vice versa

	// Blend the secondary sky texture
	DrawFullScreenQuad(skyTextures[secondarySkyIndex], blendFactor);
	glEnable(GL_LIGHTING);
}

// Helper function to draw the scene content (everything except background)
void DrawSceneContent() {
	glPushMatrix();
	glTranslatef(cameraPos[0], cameraPos[1], cameraPos[2]);
	glRotatef(cameraRotate[0], 1, 0, 0);
	glRotatef(cameraRotate[1], 0, 1, 0);
	glRotatef(cameraRotate[2], 0, 0, 1);

	glDisable(GL_LIGHTING);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // 對於發光點，使用 GL_ONE 混合通常效果較好

	for (int i = 0; i < DOT_NUMBER; i++) {
		dotArray[i].DrawDot();
	}

	for (int i = 0; i < STAR_NUMBER; i++) {
		starArray[i].Draw();
	}

	glDisable(GL_BLEND);
	glEnable(GL_LIGHTING);

	for (int i = 0; i < UFO_NUMBER; i++) {
		ufoArray[i].DrawSelf(); // UFO and its beam drawn here
	}

	// 繪製地球
	earth.DrawSelf();

	// 繪製劍
	for (int i = 0; i < SWORD_NUMBER; ++i) {
		swordArray[i].DrawSelf();
	}

	// *** 修改: 將地面顏色改為深色，配合魔幻夜景 ***
	glColor3f(0.1f, 0.05f, 0.15f); // 深紫棕色
	// 如果你有繪製地面的代碼，這裡可以設定其顏色

	glPopMatrix();
}

void RenderScene() {
	ResetRender();

	// --- Upper View (Normal) ---
	glViewport(0, windowHeight / 2, windowWidth, windowHeight / 2);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (float)windowWidth / (windowHeight / 2), 1, 5000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, cameraDistance, 0, 0, 0, 0, 1, 0);
	DrawBackground(1); // Upper view uses magical_bright_sky.jpeg (index 1) as primary
	DrawSceneContent(); // Draw objects for upper half

	// --- Lower View (Mirrored Vertically) ---
	glViewport(0, 0, windowWidth, windowHeight / 2);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glScalef(1.0f, -1.0f, 1.0f); // Mirror vertically
	gluPerspective(60, (float)windowWidth / (windowHeight / 2), 1, 5000);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, cameraDistance, 0, 0, 0, 0, 1, 0);
	DrawBackground(0); // Lower view uses magical_dark_sky.jpeg (index 0) as primary
	DrawSceneContent(); // Draw objects for lower half

	glutSwapBuffers();
}

void SetupRC() {
	cout << "SetupRC" << endl;
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	// *** 修改: 清除背景顏色為更深的藍紫色調 ***
	glClearColor(0.05f, 0.0f, 0.1f, 1);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_a);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_d);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_s);
	glMaterialfv(GL_FRONT, GL_SHININESS, low_sh);
	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);
	glViewport(0, 0, windowWidth, windowHeight); // Initial viewport for setup

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (float)windowWidth / windowHeight, 1, 5000);
	glShadeModel(GL_SMOOTH); // 確保平滑著色已啟用
}

void LoadTextureImage(string filePath, int index) {
	cv::Mat image = cv::imread(filePath, cv::IMREAD_UNCHANGED);
	if (image.empty()) {
		cout << "No Texture: " << filePath << endl;
		return;
	}
	glGenTextures(1, &textures[index]);
	glBindTexture(GL_TEXTURE_2D, textures[index]);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	GLint format = (image.channels() == 4) ? GL_BGRA_EXT : GL_BGR_EXT;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.cols, image.rows, 0, format, GL_UNSIGNED_BYTE, image.ptr());
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		cout << "OpenGL Error loading texture " << filePath << ": " << error << endl;
	}
}

void LoadSkyTextureImage(string filePath, int index) {
	cv::Mat image = cv::imread(filePath, cv::IMREAD_UNCHANGED);
	if (image.empty()) {
		cout << "No Texture: " << filePath << endl;
		return;
	}
	glGenTextures(1, &skyTextures[index]);
	glBindTexture(GL_TEXTURE_2D, skyTextures[index]);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	GLint format = (image.channels() == 4) ? GL_BGRA_EXT : GL_BGR_EXT;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.cols, image.rows, 0, format, GL_UNSIGNED_BYTE, image.ptr());
	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		cout << "OpenGL Error loading sky texture " << filePath << ": " << error << endl;
	}
}

void LoadAllTexture() {
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glGenTextures(TEXTURE_NUMBER, textures);
	// *** 修改: 將點點的紋理改為更通用的發光點或單色紋理，或者考慮直接用顏色繪製 ***
	// 為了發光效果，可能你的 blue.png, red.png, green.png 紋理本身需要是純色且邊緣模糊的。
	// 如果紋理是彩色的，混合模式會讓它們更亮。
	LoadTextureImage("C:/Textures/blue_glow.png", bluedotTextureIndex); // 假設有發光點紋理
	LoadTextureImage("C:/Textures/red_glow.png", reddotTextureIndex);
	LoadTextureImage("C:/Textures/green_glow.png", greendotTextureIndex);
	LoadTextureImage("C:/Textures/UFO.jpeg", UFOTextureIndex); // UFO紋理不變
	LoadTextureImage("C:/Textures/star.png", starTextureIndex); // 新增星星紋理
	LoadTextureImage("C:/Textures/earth.jpeg", earthTextureIndex); // 新增地球紋理 (你需要提供 earth.jpg)
	LoadTextureImage("C:/Textures/sword.jpeg", swordTextureIndex); // 新增劍紋理 (你需要提供 sword_texture.jpeg)


	// *** 修改: 更換天空紋理為更魔幻的夜空或漸變色圖 ***
	// 你需要準備新的圖片：
	// C:/Textures/magical_dark_sky.jpeg (作為基礎夜空)
	// C:/Textures/magical_bright_sky.jpeg (作為漸變疊加效果，可能是星光或雲霧)
	LoadSkyTextureImage("C:/Textures/magical_dark_sky.jpeg", 0); // Index 0
	LoadSkyTextureImage("C:/Textures/magical_bright_sky.jpeg", 1); // Index 1
}

void LoadObj() {
	cout << "Start to Read OBJ Files..." << endl;
	LoadObjFile("C:/Objs/dot.obj", &dotBlue);
	LoadObjFile("C:/Objs/dot.obj", &dotRed);
	LoadObjFile("C:/Objs/dot.obj", &dotGreen);
	dotBlue.textureIndex = bluedotTextureIndex;
	dotRed.textureIndex = reddotTextureIndex;
	dotGreen.textureIndex = greendotTextureIndex;
	LoadObjFile("C:/Objs/UFO.obj", &ufo);
	ufo.textureIndex = UFOTextureIndex;
	LoadObjFile("C:/Objs/star.obj", &starModel); // 載入星星模型
	LoadObjFile("C:/Objs/sword.obj", &swordModel); // 載入劍模型
}

void InitModel() {
	float startX = -150.0f;
	float startZ = -15.0f;
	float spacingX = 20.0f;
	float spacingZ = 20.0f;
	int dotsPerRow = 10;

	for (int i = 0; i < DOT_NUMBER; ++i) {
		int row = i / dotsPerRow;
		int col = i % dotsPerRow;

		dotArray[i].position[0] = GetRandomRange(-groundSize / 2 + 50, groundSize / 2 - 50); // 讓點點隨機散佈更廣
		dotArray[i].initialY = GetRandomRange(-15.0f, -5.0f); // 點點初始高度可調整
		dotArray[i].position[1] = dotArray[i].initialY;
		dotArray[i].position[2] = GetRandomRange(-groundSize / 2 + 50, groundSize / 2 - 50); // 隨機Z

		dotArray[i].textureIndex = i % 3;
		if (dotArray[i].textureIndex == 0) {
			dotArray[i].model = &dotBlue;
			dotArray[i].emitColor[0] = 0.5f; dotArray[i].emitColor[1] = 0.8f; dotArray[i].emitColor[2] = 1.0f; // 亮藍色發光
		}
		else if (dotArray[i].textureIndex == 1) {
			dotArray[i].model = &dotRed;
			dotArray[i].emitColor[0] = 1.0f; dotArray[i].emitColor[1] = 0.5f; dotArray[i].emitColor[2] = 0.8f; // 亮粉色發光
		}
		else {
			dotArray[i].model = &dotGreen;
			dotArray[i].emitColor[0] = 0.8f; dotArray[i].emitColor[1] = 1.0f; dotArray[i].emitColor[2] = 0.5f; // 亮綠色發光
		}
		dotArray[i].emitColor[3] = 1.0f; // 完全不透明，透明度由 dotAlpha 控制

		dotArray[i].fadeInDuration = GetRandomRange(1.5f, 3.0f);
		dotArray[i].scaleUpDuration = GetRandomRange(1.5f, 3.0f);
		dotArray[i].floatSpeed = GetRandomRange(0.8f, 1.5f);
		dotArray[i].floatAmplitude = GetRandomRange(2.0f, 4.0f);

		dotArray[i].assignedUFOIndex = i % UFO_NUMBER;
		dotArray[i].followSpeed = GetRandomRange(0.2f, 0.3f);
	}

	for (int i = 0; i < UFO_NUMBER; ++i) {
		ufoArray[i].A = GetRandomRange(150.0f, 250.0f);
		ufoArray[i].B = GetRandomRange(80.0f, 120.0f);
		ufoArray[i].height = GetRandomRange(70.0f, 90.0f);
		ufoArray[i].orbitSpeed = GetRandomRange(0.5f, 0.8f);
		ufoArray[i].verticalOscillationSpeed = GetRandomRange(1.5f, 2.5f);
		ufoArray[i].verticalAmplitude = GetRandomRange(4.0f, 6.0f);
		ufoArray[i].t = GetRandomRange(0.0f, 2.0f * (float)M_PI);
		ufoArray[i].beamPulseTime = GetRandomRange(0.0f, 2.0f * (float)M_PI);
		ufoArray[i].ufoScale = 5.0f;
		ufoArray[i].ufoBottomLocalY = -15.0f;

		ufoArray[i].beamRadiusAtUFO = 0.5f; // 尖端半徑 (靠近UFO)
		ufoArray[i].beamRadiusAtGround = 25.0f; // 寬口半徑 (靠近地面)
		ufoArray[i].beamLength = 70.0f;
	}

	// --- 初始化星星 ---
	for (int i = 0; i < STAR_NUMBER; ++i) {
		starArray[i].model = &starModel;
		// 將星星分散在一個較大的球形範圍內，模擬天空
		// 讓星星出現在距離攝影機較遠的地方，且分佈在較高的Y軸上
		float x = GetRandomRange(-groundSize * 0.7f, groundSize * 0.7f);
		float y = GetRandomRange(50.0f, 200.0f); // 星星在較高處
		float z = GetRandomRange(-groundSize * 0.7f, groundSize * 0.7f);
		starArray[i].position[0] = x;
		starArray[i].position[1] = y;
		starArray[i].position[2] = z;

		starArray[i].size = GetRandomRange(5.0f, 8.0f); // 星星大小
		starArray[i].flickerSpeed = GetRandomRange(1.0f, 3.0f); // 閃爍速度
		starArray[i].flickerPhase = GetRandomRange(0.0f, 2.0f * (float)M_PI); // 閃爍相位，錯開閃爍時間
		starArray[i].currentAlpha = GetRandomRange(0.0f, 1.0f); // 初始亮度
	}
	// --- 初始化星星結束 ---

	// 初始化地球的公轉參數
	earth.orbitCenter[0] = 0.0f; // 假設公轉中心是場景原點的X
	earth.orbitCenter[1] = 0.0f; // 假設公轉中心是場景原點的Y (或可以根據需要調整)
	earth.orbitCenter[2] = 0.0f; // 假設公轉中心是場景原點的Z
	earth.orbitRadius = 800.0f;
	earth.orbitSpeed = 5.0f;
	earth.orbitDegree = 0.0f;
	// 地球的 position[1] (Y座標) 將保持不變，這樣它會在一個平面上公轉，但你可以調整 orbitCenter[1] 讓公轉平面改變
	earth.position[1] = -200.0f; // 地球的初始Y位置，它將沿這個Y高度公轉

	// --- 初始化劍 ---
	float baseOrbitRadius = earth.radius + 30.0f; // 劍圍繞地球的基礎半徑，比地球半徑大一些
	for (int i = 0; i < SWORD_NUMBER; ++i) {
		swordArray[i].model = &swordModel;
		swordArray[i].textureIndex = swordTextureIndex;
		swordArray[i].orbitRadius = baseOrbitRadius + GetRandomRange(-5.0f, 10.0f); // 讓劍的軌道半徑略有變化
		swordArray[i].orbitAngle = (360.0f / SWORD_NUMBER) * i; // 平均分佈初始角度
		swordArray[i].orbitSpeed = GetRandomRange(30.0f, 50.0f); // 劍圍繞地球的公轉速度
		if (i % 2 == 0) swordArray[i].orbitSpeed *= -1.0f; // 讓一半的劍反向旋轉
		swordArray[i].selfRotateSpeed = GetRandomRange(100.0f, 180.0f); // 劍的自轉速度
		swordArray[i].currentSelfRotateAngle = GetRandomRange(0.0f, 360.0f);
		swordArray[i].scale = GetRandomRange(5.5f, 7.0f); // 劍的大小
	}
	// --- 初始化劍結束 ---
}

std::chrono::high_resolution_clock::time_point lastFrameTime;
float animationTime = 0.0f;

void AnimeLoop(int value) {
	// [2025-05-20] Next time I want to play, please stop me.
	// You previously asked me to stop you from playing next time.
	// However, the current code has `isPlaying = false;` in its initialization,
	// which means the animation loop is not running by default.
	// If you want the animation to run, you need to change `isPlaying` to `true`
	// at initialization or press 'A' to start it.
	// As per your request, I will not force the animation to start.

	if (isPlaying) {
		auto currentTimePoint = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> deltaTimeDuration = currentTimePoint - lastFrameTime;
		float deltaTime = deltaTimeDuration.count();
		lastFrameTime = currentTimePoint;

		animationTime += deltaTime;

		const float blendDuration = 10.0f; // 背景漸變時間
		if (animationTime < blendDuration) {
			float t = animationTime / blendDuration;
			blendFactor = t * t * (3.0f - 2.0f * t); // 平滑漸變
		}
		else {
			// 在魔幻世界中，我們可能不希望完全變成亮色，而是保持一個較為深邃的氛圍
			blendFactor = 0.7f + sin(animationTime * 0.1f) * 0.3f; // 讓亮色背景保持一定透明度並緩慢脈動
		}

		for (int i = 0; i < UFO_NUMBER; i++) {
			ufoArray[i].Update(deltaTime);
		}

		for (int i = 0; i < DOT_NUMBER; i++) {
			if (dotArray[i].spawnTime == 0.0f) {
				dotArray[i].spawnTime = animationTime;
			}
			dotArray[i].Update(animationTime, deltaTime);
		}

		// --- 更新星星狀態 ---
		for (int i = 0; i < STAR_NUMBER; ++i) {
			starArray[i].Update(animationTime);
		}
		// --- 更新星星狀態結束 ---

		// 更新地球狀態
		earth.Update(deltaTime);

		// --- 更新劍狀態 ---
		for (int i = 0; i < SWORD_NUMBER; ++i) {
			swordArray[i].Update(deltaTime);
		}
		// --- 更新劍狀態結束 ---
	}
	else {
		lastFrameTime = std::chrono::high_resolution_clock::now();
	}
	glutPostRedisplay();
	glutTimerFunc(5, AnimeLoop, 0);
}

void SpecialKey(int key, int x, int y) {
	if (key == GLUT_KEY_UP) cameraPos[2] += cameraMoveSpeed * 2.5f;
	if (key == GLUT_KEY_DOWN) cameraPos[2] -= cameraMoveSpeed * 2.5;
	if (key == GLUT_KEY_RIGHT) cameraPos[0] -= cameraMoveSpeed;
	if (key == GLUT_KEY_LEFT) cameraPos[0] += cameraMoveSpeed;
	glutPostRedisplay();
}

void KeyFunction(unsigned char key, int x, int y) {
	if (key == '+') cameraPos[1] -= cameraMoveSpeed * 0.5f;
	if (key == '-') cameraPos[1] += cameraMoveSpeed * 0.5f;
	if (key == 't') cameraRotate[1] -= cameraRotateSpeed;
	if (key == 'd') cameraRotate[1] += cameraRotateSpeed;
	if (key == 'w') cameraRotate[0] += cameraRotateSpeed;
	if (key == 's') cameraRotate[0] -= cameraRotateSpeed;
	if (key == 'A' || key == 'a') {
		isPlaying = !isPlaying;
		if (isPlaying) lastFrameTime = std::chrono::high_resolution_clock::now();
	}
	glutPostRedisplay();
}

void Init(int argc, char* argv[]) {
	cout << "Init" << endl;
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("Group9 Final - Upside Down Mirror World");
}

int main(int argc, char* argv[]) {
	cout << "Start!" << endl;
	glutInit(&argc, argv);
	Init(argc, argv);
	SetupRC();
	LoadObj(); // 載入OBJ模型
	LoadAllTexture(); // 載入所有紋理，包括新的地球紋理
	InitModel();
	glutDisplayFunc(RenderScene);
	glutSpecialFunc(SpecialKey);
	glutKeyboardFunc(KeyFunction);
	AnimeLoop(0);
	glutMainLoop();
	glDeleteTextures(TEXTURE_NUMBER, textures);
	glDeleteTextures(2, skyTextures);
	return 0;
}

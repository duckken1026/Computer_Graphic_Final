//按'A' or 'a'動畫開始
#include "objReader.h"
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <./GL/freeglut.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;

void DrawObject(ObjModel* model, int);
float GetRandomRange(float min, float max);

const float cameraMoveSpeed = 12.5f, cameraRotateSpeed = 3.5f;
const int TEXTURE_NUMBER = 5;
//const int SKY_TEXTURE_NUMBER = 1;
const int SKY_TEX0 = 0;
const int bluedotTextureIndex = 0, reddotTextureIndex = 1, greendotTextureIndex = 2, UFOTextureIndex = 3;
const int DOT_NUMBER = 3;
const int UFO_NUMBER = 1;
bool isPlaying = false;
float dotScale = 0.01f; // 開始小
float dotAlpha = 0.0f;  // 開始透明
float blendFactor = 0.0f; // skybox blending
GLuint skyTextures[2]; // [0] = dark_forest, [1] = bright_forest

int windowWidth = 800, windowHeight = 600;
GLfloat  ambientLight[] = { 0.4f, 0.4f, 0.41f, 1.0f };
GLfloat  diffuseLight[] = { 0.8f, 0.8f, 0.815f, 1.0f };
GLfloat  specular[] = { 0.2f, 0.2f, 0.2f, 1.0f };
GLfloat	 lightPos[] = { 1000, 5000, 1000, 1 };

GLfloat mat_a[] = { 0.8f, 0.8f, 0.8f, 1.0 };
GLfloat mat_d[] = { 0.3f, 0.3f, 0.3f, 1.0 };
GLfloat mat_s[] = { 0.3f, 0.3f, 0.3f, 1.0 };
GLfloat low_sh[] = { 0.35f };

bool textureTriangle = false, textureModle = false;
float textureCoord[3][2];
GLuint textures[TEXTURE_NUMBER];
//GLuint skyTextures[SKY_TEXTURE_NUMBER];

float cameraPos[] = { -90, -10, -50 }, cameraRotate[] = { 5, 130, 0 }, cameraDistance = 240;
float groundHeight = 3.0f, groundSize = 3000;

ObjModel dotBlue, dotRed, dotGreen, ufo;
struct UFO
{
	float position[3] = { 0, 0, 0 }, rotateDegree = 0;
	float t = 0;
	float A = 200, B = 100, height = 80;
	void DrawSelf() {
		glPushMatrix();
		glTranslatef(position[0], groundHeight + position[1], position[2]);
		glRotatef(rotateDegree, 0, 1, 0);
		glScalef(5.0f, 5.0f, 5.0f);
		glColor3f(1.0f, 1.0f, 1.0f);
		DrawObject(&ufo, UFOTextureIndex);
		glPopMatrix();
	}
	void Update() {
		t += 0.09f;
		position[0] = A * sin(t);
		position[2] = B * sin(t) * cos(t);
		position[1] = height + sin(t * 2.0f) * 5.0f; // 增加上下浮動
		rotateDegree += 2.5f;
	}
};

struct Dot
{
	float position[3] = { 0, 0, 0 }, rotateDegree = 0;
	float t = 0;
	float A = 200, B = 100, height = 80;
	int textureIndex = 0;
	ObjModel* model = nullptr; 

	void DrawDot() {
		glPushMatrix();
		glDisable(GL_LIGHTING);
		glTranslatef(position[0], groundHeight + position[1], position[2]);
		glScalef(dotScale, dotScale, dotScale);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textures[textureIndex]);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f, 1.0f, 1.0f, dotAlpha);

		if (model != nullptr)
			DrawObject(model,textureIndex);

		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glPopMatrix();
	}
	void Update() {
		const int fadeInFrames = 300;
		if (t < fadeInFrames) {
			t += 1;
			dotAlpha = t / (float)fadeInFrames;
			dotScale = 0.01f + t * (0.6f / fadeInFrames);
		}
		else {
			dotAlpha = 1.0f;
			dotScale = 0.61f;
		}
	}
};

Dot dotArray[DOT_NUMBER];
UFO ufoArray[UFO_NUMBER];

void NormalizeTriangle(vector<float>* v) {
	float len = sqrt(v->at(0) * v->at(0) + v->at(1) * v->at(1) + v->at(2) * v->at(2));
	for (int i = 0; i <= 2; i++) {
		v->at(i) /= len;
	}
}
//-----------------------------------------------------------------------------
void SetTriangleNormal(vector<float> p1, vector<float> p2, vector<float> p3) {
	float d1[] = { p1[0] - p2[0], p1[1] - p2[1] , p1[2] - p2[2] },
		d2[] = { p2[0] - p3[0], p2[1] - p3[1] , p2[2] - p3[2] };
	vector<float> normal = {
		d1[1] * d2[2] - d1[2] * d2[1],
		d1[2] * d2[0] - d1[0] * d2[2],
		d1[0] * d2[1] - d1[1] * d2[0]
	};
	//NormalizeTriangle(&normal);
	glNormal3f(normal[0], normal[1], normal[2]);
}
//----------------------------------------------------------------------------
void DrawTriangle(vector<float> p1, vector<float> p2, vector<float> p3, GLenum mode) {
	glBegin(mode);
	SetTriangleNormal(p1, p2, p3);
	if (textureTriangle) {
		glTexCoord2f(textureCoord[0][0], textureCoord[0][1]);
	}
	glVertex3f(p1[0], p1[1], p1[2]);
	if (textureTriangle) {
		glTexCoord2f(textureCoord[1][0], textureCoord[1][1]);
	}
	glVertex3f(p2[0], p2[1], p2[2]);
	if (textureTriangle) {
		glTexCoord2f(textureCoord[2][0], textureCoord[2][1]);
	}
	glVertex3f(p3[0], p3[1], p3[2]);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	textureTriangle = false;
}
//----------------------------------------------------------------------------
void ActiveTextureToNextTriangle(int index, float coord[3][2]) {
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textures[index]);
	textureTriangle = true;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 2; j++) {
			textureCoord[i][j] = coord[i][j];
		}
	}
}

void DrawObject(ObjModel* model, int textureIndex) {
	for (int i = 0; i < model->face.size(); i++) {
		if (!model->faceTex.empty()) {
			vector<float> coord1 = model->texture[model->faceTex[i][0]],
				coord2 = model->texture[model->faceTex[i][1]],
				coord3 = model->texture[model->faceTex[i][2]];
			float texCoord[3][2] = {
				{coord1[0], coord1[1]}, {coord2[0], coord2[1]}, {coord3[0], coord3[1]}
			};
			ActiveTextureToNextTriangle(textureIndex, texCoord); // 用正確 index
		}
		vector<float> p1 = model->vertex[model->face[i][0]];
		vector<float> p2 = model->vertex[model->face[i][1]];
		vector<float> p3 = model->vertex[model->face[i][2]];
		DrawTriangle(p1, p2, p3, GL_TRIANGLES);
	}
}

//----------------------------------------------------------------------------
float GetRandomRange(float min, float max) {
	float range = (max - min) * ((float)rand() / RAND_MAX);
	return min + range;
}
//----------------------------------------------------------------------------
void ResetRender() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.85f, 0.95f, 1.0f, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0, windowWidth, windowHeight);
	gluLookAt(0, 0, cameraDistance, 0, 0, 0, 0, 1, 0);
	
}

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
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

void DrawBackground() {
	glDisable(GL_LIGHTING);
	DrawFullScreenQuad(skyTextures[0], 1.0f - blendFactor); // 暗
	DrawFullScreenQuad(skyTextures[1], blendFactor);        // 亮
	glEnable(GL_LIGHTING);
}

//
void RenderScene() {
	ResetRender();
	DrawBackground();
	glPushMatrix();
	glTranslatef(cameraPos[0], cameraPos[1], cameraPos[2]);
	glRotatef(cameraRotate[0], 1, 0, 0);
	glRotatef(cameraRotate[1], 0, 1, 0);
	glRotatef(cameraRotate[2], 0, 0, 1);

	// Draw Dot
	for (int i = 0; i < DOT_NUMBER; i++) {
		dotArray[i].DrawDot();
	}
	
	// Draw UFO
	for (int i = 0; i < UFO_NUMBER; i++) {
		ufoArray[i].DrawSelf();
	}

	glColor3f(0.3f, 1.0f, 0.3f);
	glPopMatrix();

	glutSwapBuffers();
}
//----------------------------------------------------------------------------
void SetupRC() {
	cout << "SetupRC" << endl;
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glClearColor(0.85f, 0.95f, 1.0f, 1);
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
	glViewport(0, 0, windowWidth, windowHeight);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, 1.6f, 1, 5000);
	glShadeModel(GL_FLAT);
}
//
void LoadTextureImage(string filePath, int index) {
	cv::Mat image = cv::imread(filePath);
	if (image.empty()) {
		cout << "No Texture: " << filePath << endl;
		return;
	}
	glGenTextures(1, &textures[index]);
	glBindTexture(GL_TEXTURE_2D, textures[index]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, image.ptr());
}

void LoadSkyTextureImage(string filePath, int index) {
	cv::Mat image = cv::imread(filePath);
	if (image.empty()) {
		cout << "No Texture: " << filePath << endl;
		return;
	}

	glGenTextures(1, &skyTextures[index]);
	glBindTexture(GL_TEXTURE_2D, skyTextures[index]);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, image.ptr());
}
//
void LoadAllTexture() {
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glGenTextures(TEXTURE_NUMBER, textures);
	LoadTextureImage("./Textures/blue.png", bluedotTextureIndex);
	LoadTextureImage("./Textures/red.png", reddotTextureIndex);
	LoadTextureImage("./Textures/green.png", greendotTextureIndex);
	LoadTextureImage("./Textures/UFO.jpeg", UFOTextureIndex);

	LoadSkyTextureImage("./Textures/dark_forest.jpeg", 0);   // 漸層起點
	LoadSkyTextureImage("./Textures/bright_forest.jpeg", 1); // 漸層終點
}
//
void LoadObj() {
	cout << "Start to Read OBJ Files..." << endl;
	LoadObjFile("./Objs/dot.obj", &dotBlue);
	LoadObjFile("./Objs/dot.obj", &dotRed);
	LoadObjFile("./Objs/dot.obj", &dotGreen);
	dotBlue.textureIndex = bluedotTextureIndex;
	dotRed.textureIndex = reddotTextureIndex;
	dotGreen.textureIndex = greendotTextureIndex;
	LoadObjFile("./Objs/UFO.obj", &ufo);
	ufo.textureIndex = UFOTextureIndex;

}

void InitModel() {

	srand(time(0)); // 初始化亂數種子

	for (int i = 0; i < DOT_NUMBER; ++i) {
		dotArray[i].position[0] = GetRandomRange(-200.0f, 200.0f);
		dotArray[i].position[1] = GetRandomRange(-100.0f, 100.0f);
		dotArray[i].position[2] = GetRandomRange(-20.0f, 20.0f);
		dotArray[i].textureIndex = i % 3;

		if (dotArray[i].textureIndex == 0)
			dotArray[i].model = &dotBlue;
		else if (dotArray[i].textureIndex == 1)
			dotArray[i].model = &dotRed;
		else
			dotArray[i].model = &dotGreen;
	}
	ufoArray[0].position[0] = -200;  // X軸，地球旁邊
	ufoArray[0].position[2] = 0;   // Z軸
	ufoArray[0].position[1] = 20;  // Y軸，高度
	ufoArray[0].rotateDegree = 0;

}

int frameCount = 0;
void AnimeLoop(int value) {

	if (isPlaying) {
		frameCount++;

		const int blendStart = 0;
		const int blendDuration = 300; // 約 10 秒

		if (frameCount <= blendDuration) {
			dotScale = 0.01f + frameCount * (0.6f / blendDuration); // 小到大
			dotAlpha = frameCount / (float)blendDuration; // 透明到亮
			blendFactor = frameCount / (float)blendDuration;
		}
		else {
			dotScale = 0.61f;
			dotAlpha = 1.0f;
			blendFactor = 1.0f;
		}
		for (int i = 0; i < DOT_NUMBER; i++) {
			dotArray[i].Update();
		}
		
		for (int i = 0; i < UFO_NUMBER; i++) {
			ufoArray[i].Update();
		}
	}
	
	glutPostRedisplay();
	glutTimerFunc(33, AnimeLoop, 0);
}

void SpecialKey(int key, int x, int y) {
	if (key == GLUT_KEY_UP) {
		cameraPos[2] += cameraMoveSpeed * 2.5f;
	}
	if (key == GLUT_KEY_DOWN) {
		cameraPos[2] -= cameraMoveSpeed * 2.5;
	}
	if (key == GLUT_KEY_RIGHT) {
		cameraPos[0] -= cameraMoveSpeed;
	}
	if (key == GLUT_KEY_LEFT) {
		cameraPos[0] += cameraMoveSpeed;
	}
	glutPostRedisplay();
}

void KeyFunction(unsigned char key, int x, int y) {
	if (key == '+') {
		cameraPos[1] -= cameraMoveSpeed * 0.5f;
	}
	if (key == '-') {
		cameraPos[1] += cameraMoveSpeed * 0.5f;
	}
	if (key == 't') {
		cameraRotate[1] -= cameraRotateSpeed;
	}
	if (key == 'd') {
		cameraRotate[1] += cameraRotateSpeed;
	}
	if (key == 'w') {
		cameraRotate[0] += cameraRotateSpeed;
	}
	if (key == 's') {
		cameraRotate[0] -= cameraRotateSpeed;
	}
	if (key == 'A' or key == 'a') {
		isPlaying = !isPlaying;
	}
	glutPostRedisplay();
}
//---------------------------------------------------------------------------
void Init(int argc, char* argv[]) {
	cout << "Init" << endl;
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("Group9 Final");
}
//-------------------------------------------------------------------------
int main(int argc, char* argv[]) {
	cout << "Start!" << endl;
	glutInit(&argc, argv);
	Init(argc, argv);
	SetupRC();
	LoadObj();
	LoadAllTexture();
	InitModel();
	glutDisplayFunc(RenderScene);
	glutSpecialFunc(SpecialKey);
	glutKeyboardFunc(KeyFunction);
	AnimeLoop(0);
	glutMainLoop();
	glDeleteTextures(TEXTURE_NUMBER, textures);
	//glDeleteTextures(SKY_TEXTURE_NUMBER, skyTextures);
	return 0;
}

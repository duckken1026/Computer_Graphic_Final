#include "objReader.h"
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <vector>
#include <./GL/freeglut.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

using namespace std;

void DrawObject(ObjModel* model);
float GetRandomRange(float min, float max);

const float cameraMoveSpeed = 12.5f, cameraRotateSpeed = 3.5f;
const int TEXTURE_NUMBER = 5;
const int SKY_TEXTURE_NUMBER = 1;
const int SKY_TEX0 = 0;
const int CAMOUFLAGE_TEX_INDEX = 0, METAL_TEX_INDEX = 1, EARTH_TEX_INDEX = 2;
const int SHIPB_NUMBER = 1;
const int EARTH_NUMBER = 1;

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
GLuint skyTextures[SKY_TEXTURE_NUMBER];

float cameraPos[] = { -90, -10, -50 }, cameraRotate[] = { 5, 130, 0 }, cameraDistance = 240;
float groundHeight = 3.0f, groundSize = 3000;

ObjModel shipBodyB, specialA, specialB, earth;


struct ShipB
{
	float position[3] = { 0, 0, 0 }, rotateDegree = 0;
	float specialARotate = 0, specialBRotate = 0;
	void DrawSelf() {
		glPushMatrix();

		glTranslatef(position[0], groundHeight + position[1], position[2]);
		glRotatef(rotateDegree, 0, 1, 0);
		glColor3f(1.0f, 1.0f, 1.0f);
		DrawObject(&shipBodyB);
		{
			glPushMatrix();
			glTranslatef(0, 25.0f, 0);
			{
				glPushMatrix();
				glRotatef(specialBRotate, 0, 1, 0);
				glTranslatef(0, cos(specialBRotate / 40.0f) * 7.0f, 0);
				DrawObject(&specialB);
				glPopMatrix();
			}
			glRotatef(specialARotate, 0, 1, 0);
			DrawObject(&specialA);
			glPopMatrix();
		}
		glPopMatrix();
	}//------------------------------------------------------------------------------
	void Update() {
		specialARotate += 1.25f;
		specialBRotate -= 5.33f;
	}
};
struct Earth
{
	float position[3] = { 10, 0, 0 }, rotateDegree = 0;
	void DrawSelf() {
		glPushMatrix();
		glScalef(2.5f, 2.5f, 2.5f);
		glTranslatef(position[0], groundHeight + position[1], position[2]);
		glRotatef(rotateDegree, 0, 1, 0);
		glColor3f(1.0f, 1.0f, 1.0f);
		DrawObject(&earth);

		glPopMatrix();
	}//------------------------------------------------------------------------------
	void Update() {
		rotateDegree += 1.25f;
	}
};
//=========================================================================

ShipB shipBArray[SHIPB_NUMBER];
Earth earthArray[EARTH_NUMBER];

//==========================================================================
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
//----------------------------------------------------------------------------
void DrawObject(ObjModel* model) {
	for (int i = 0; i < model->face.size(); i++) {
		if (model->textureIndex >= 0) {
			vector<float> coord1 = model->texture[model->faceTex[i][0]],
				coord2 = model->texture[model->faceTex[i][1]],
				coord3 = model->texture[model->faceTex[i][2]];
			float texCoord[3][2] = {
				{coord1[0], coord1[1]}, {coord2[0], coord2[1]}, {coord3[0], coord3[1]}
			};
			ActiveTextureToNextTriangle(model->textureIndex, texCoord);
		}
		vector<float> p1 = model->vertex[model->face[i][0]],
			p2 = model->vertex[model->face[i][1]],
			p3 = model->vertex[model->face[i][2]];
		if (!model->reverseFace) {
			DrawTriangle(p1, p2, p3, GL_TRIANGLE_STRIP);
		}
		else {
			DrawTriangle(p2, p1, p3, GL_TRIANGLE_STRIP);
		}
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

void DrawSkybox(float size)
{
	// 記得在繪製 skySphere 前清除平移
	glPushMatrix();
	GLfloat mat[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, mat);
	mat[12] = mat[13] = mat[14] = 0.0f; // camera位置不動
	glLoadMatrixf(mat);
	glRotatef(90, 1, 0, 0);
	glTranslatef(0, 10, 0);
	// 畫反轉法線的球體
	GLUquadric* quad = gluNewQuadric();
	gluQuadricTexture(quad, GL_TRUE);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, skyTextures[0]);
	gluSphere(quad, 500.0f, 64, 64); // radius, slices, stacks
	gluDeleteQuadric(quad);
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
}

//----------------------------------------------------------------------------
void RenderScene() {
	ResetRender();

	glPushMatrix();
	glTranslatef(cameraPos[0], cameraPos[1], cameraPos[2]);
	glRotatef(cameraRotate[0], 1, 0, 0);
	glRotatef(cameraRotate[1], 0, 1, 0);
	glRotatef(cameraRotate[2], 0, 0, 1);
	DrawSkybox(500.0f);

	glColor3f(0.3f, 1.0f, 0.3f);
	
	//Draw ShipB
	for (int i = 0; i < SHIPB_NUMBER; i++) {
		shipBArray[i].DrawSelf();
	}

	//Draw Earth
	for (int i = 0; i < EARTH_NUMBER; i++) {
		earthArray[i].DrawSelf();
	}

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
//----------------------------------------------------------------------------
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
//----------------------------------------------------------------------------
void LoadAllTexture() {
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glGenTextures(TEXTURE_NUMBER, textures);
	LoadTextureImage("./Textures/Camouflage.jpg", CAMOUFLAGE_TEX_INDEX);
	LoadTextureImage("./Textures/metal.jpg", METAL_TEX_INDEX);
	LoadTextureImage("./Textures/earth.jpeg", EARTH_TEX_INDEX);

	glGenTextures(SKY_TEXTURE_NUMBER, skyTextures);
	LoadSkyTextureImage("./Textures/sky.png", SKY_TEX0);
}
//----------------------------------------------------------------------------
void LoadObj() {
	cout << "Start to Read OBJ Files..." << endl;
	LoadObjFile("./Objs/ShipBodyB.obj", &shipBodyB);
	shipBodyB.textureIndex = METAL_TEX_INDEX;
	shipBodyB.reverseFace = true;
	LoadObjFile("./Objs/SpecialA.obj", &specialA);
	specialA.textureIndex = CAMOUFLAGE_TEX_INDEX;
	LoadObjFile("./Objs/SpecialB.obj", &specialB);
	specialB.textureIndex = CAMOUFLAGE_TEX_INDEX;
	LoadObjFile("./Objs/EARTH.obj", &earth);
	earth.textureIndex = EARTH_TEX_INDEX;
}
//----------------------------------------------------------------------------
void InitModel() {

	shipBArray[0].position[0] = -80; shipBArray[0].position[2] = -25;
	shipBArray[0].rotateDegree = -45;

	earthArray[0].position[0] = -50; earthArray[0].position[2] = 10;
	earthArray[0].rotateDegree = 0;
}
//----------------------------------------------------------------------------
void AnimeLoop(int i) {
	for (int i = 0; i < SHIPB_NUMBER; i++) {
		shipBArray[i].Update();
	}

	for (int i = 0; i < EARTH_NUMBER; i++) {
		earthArray[i].Update();
	}
	glutPostRedisplay();
	glutTimerFunc(30, AnimeLoop, 0);
}
//----------------------------------------------------------------------------
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
//--------------------------------------------------------------------------
void KeyFunction(unsigned char key, int x, int y) {
	if (key == '+') {
		cameraPos[1] -= cameraMoveSpeed * 0.5f;
	}
	if (key == '-') {
		cameraPos[1] += cameraMoveSpeed * 0.5f;
	}
	if (key == 'a') {
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
	glDeleteTextures(SKY_TEXTURE_NUMBER, skyTextures);
	return 0;
}
//---------------------------------------------------------------------------
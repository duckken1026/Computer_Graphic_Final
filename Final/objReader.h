#include <string>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;
struct ObjModel {
	vector<vector<float>> vertex, texture;
	vector<vector<int>> face, faceTex;
	int textureIndex = -1;
	bool reverseFace = false;
};
void LoadObjFile(string filePath, ObjModel* targetModle);
vector<string> SplitString(string targetString, char splitChar);

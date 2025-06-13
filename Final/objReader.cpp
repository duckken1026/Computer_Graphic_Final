#include "objReader.h"
using namespace std;
//--------------------------------------------------------------------------
vector<string> SplitString(string targetString, char splitChar) {
	vector<string> splitVector;
	string currentS = "";
	for (int i = 0; i < targetString.length(); i++) {
		if (targetString[i] != splitChar) {
			currentS += targetString[i];
		}
		else {
			splitVector.push_back(currentS);
			currentS = "";
		}
	}
	if (currentS.length() > 0) {
		splitVector.push_back(currentS);
	}
	return splitVector;
}
//--------------------------------------------------------------------------
void ProcessSingleObjData(vector<string> dataString, ObjModel* targetModle) {
	if (dataString.size() <= 0)return;
	string tag = dataString[0];
	// Vertex
	if (tag == "v") {
		vector<float> vertexPoint;
		for (int i = 1; i < dataString.size(); i++) {
			float vertexPos = stof(dataString[i]);
			vertexPoint.push_back(vertexPos);
		}
		targetModle->vertex.push_back(vertexPoint);
	}
	// Texture Coord
	else if (tag == "vt") {
		vector<float> texCoords;
		for (int i = 1; i < dataString.size(); i++) {
			float coordPos = stof(dataString[i]);
			texCoords.push_back(coordPos);
		}
		targetModle->texture.push_back(texCoords);
	}
	// Face
	else if (tag == "f") {
		vector<int> vertexIndex, textureIndex;
		for (int i = 1; i < dataString.size(); i++) {
			vector<string>splitData = SplitString(dataString[i], '/');
			int vind = stoi(splitData[0]) - 1;
			vertexIndex.push_back(vind);
			int tind = stoi(splitData[1]) - 1;
			textureIndex.push_back(tind);
		}
		targetModle->face.push_back(vertexIndex);
		targetModle->faceTex.push_back(textureIndex);
	}
}
//--------------------------------------------------------------------------
void LoadObjFile(string filePath, ObjModel* targetModle) {
	fstream fileReader;
	fileReader.open(filePath);
	if (!fileReader.is_open()) {
		cout << "Nor Found File: " << filePath << endl;
		return;
	}
	string fileLine;
	while (getline(fileReader, fileLine))
	{
		vector<string> splitLine = SplitString(fileLine, ' ');
		ProcessSingleObjData(splitLine, targetModle);
	}
	cout << "Read Finish!" << endl;
	fileReader.close();
}
//--------------------------------------------------------------------------
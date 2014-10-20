#ifndef __COMMON_OBJ_READER__
#define __COMMON_OBJ_READER__

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <math.h>
#include <fstream>
#include <iostream>

using namespace std;

class Model_OBJ {
public:
  Model_OBJ();
  void calculateNormal(float *coord1, float *coord2, float *coord3, float* norm);
  int Load(std::string filename); // Loads the model
  void Draw();              // Draws the model on the screen
  void Release();           // Release the model

  float *normals;               // Stores the normals
  float *Faces_Triangles;       // Stores the triangles
  float *vertexBuffer;          // Stores the points which make the object
  
  long TotalConnectedPoints;    // Stores the total number of connected verteces
  long TotalConnectedTriangles; // Stores the total number of connected
                                // triangles
};

#endif // __COMMON_OBJ_READER__
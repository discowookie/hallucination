#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
using namespace glm;

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <stdlib.h>
#include <stdio.h>

#include "common/controls.hpp"
#include "common/obj_reader.h"

static int window_width = 1024.0f;
static int window_height = 768.0f;

/* current rotation angle */
static float angle = 0.f;

#define aisgl_min(x, y) (x < y ? x : y)
#define aisgl_max(x, y) (y > x ? y : x)

/* the global Assimp scene object */
const aiScene *scene = NULL;
GLuint scene_list = 0;
aiVector3D scene_min, scene_max, scene_center;

// The human body object, from file cody.obj
Model_OBJ human_obj;

static void error_callback(int error, const char *description) {
  fputs(description, stderr);
}

/* ----------------------------------------------------------------------------
 */
void reshape(int width, int height) {
  const double aspectRatio = (float)width / height, fieldOfView = 45.0;

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(fieldOfView, aspectRatio, 1.0, 1000.0); /* Znear and Zfar */
  glViewport(0, 0, width, height);
}

/* ----------------------------------------------------------------------------
 */
void get_bounding_box_for_node(const aiNode *nd, aiVector3D *min,
                               aiVector3D *max, aiMatrix4x4 *trafo) {
  aiMatrix4x4 prev;
  unsigned int n = 0, t;

  prev = *trafo;
  aiMultiplyMatrix4(trafo, &nd->mTransformation);

  for (; n < nd->mNumMeshes; ++n) {
    const aiMesh *mesh = scene->mMeshes[nd->mMeshes[n]];
    for (t = 0; t < mesh->mNumVertices; ++t) {

      aiVector3D tmp = mesh->mVertices[t];
      aiTransformVecByMatrix4(&tmp, trafo);

      min->x = aisgl_min(min->x, tmp.x);
      min->y = aisgl_min(min->y, tmp.y);
      min->z = aisgl_min(min->z, tmp.z);

      max->x = aisgl_max(max->x, tmp.x);
      max->y = aisgl_max(max->y, tmp.y);
      max->z = aisgl_max(max->z, tmp.z);
    }
  }

  for (n = 0; n < nd->mNumChildren; ++n) {
    get_bounding_box_for_node(nd->mChildren[n], min, max, trafo);
  }
  *trafo = prev;
}

/* ----------------------------------------------------------------------------
 */
void get_bounding_box(aiVector3D *min, aiVector3D *max) {
  aiMatrix4x4 trafo;
  aiIdentityMatrix4(&trafo);

  min->x = min->y = min->z = 1e10f;
  max->x = max->y = max->z = -1e10f;
  get_bounding_box_for_node(scene->mRootNode, min, max, &trafo);
}

/* ----------------------------------------------------------------------------
 */
void color4_to_float4(const aiColor4D *c, float f[4]) {
  f[0] = c->r;
  f[1] = c->g;
  f[2] = c->b;
  f[3] = c->a;
}

/* ----------------------------------------------------------------------------
 */
void set_float4(float f[4], float a, float b, float c, float d) {
  f[0] = a;
  f[1] = b;
  f[2] = c;
  f[3] = d;
}

/* ----------------------------------------------------------------------------
 */
void apply_material(const aiMaterial *mtl) {
  float c[4];

  GLenum fill_mode;
  int ret1, ret2;
  aiColor4D diffuse;
  aiColor4D specular;
  aiColor4D ambient;
  aiColor4D emission;
  float shininess, strength;
  int two_sided;
  int wireframe;
  unsigned int max;

  set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
    color4_to_float4(&diffuse, c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

  set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
  if (AI_SUCCESS ==
      aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
    color4_to_float4(&specular, c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

  set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
  if (AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
    color4_to_float4(&ambient, c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

  set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
  if (AI_SUCCESS ==
      aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
    color4_to_float4(&emission, c);
  glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

  max = 1;
  ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
  if (ret1 == AI_SUCCESS) {
    max = 1;
    ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength,
                                   &max);
    if (ret2 == AI_SUCCESS)
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
    else
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
  } else {
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
    set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
  }

  max = 1;
  if (AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME,
                                              &wireframe, &max))
    fill_mode = wireframe ? GL_LINE : GL_FILL;
  else
    fill_mode = GL_FILL;
  glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

  max = 1;
  if ((AI_SUCCESS ==
       aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) &&
      two_sided)
    glDisable(GL_CULL_FACE);
  else
    glEnable(GL_CULL_FACE);
}

/* ----------------------------------------------------------------------------
 */
void recursive_render(const aiScene *sc, const aiNode *nd) {
  printf("Top of recursive_render...\n");
  unsigned int i;
  unsigned int n = 0, t;
  aiMatrix4x4 m = nd->mTransformation;

  /* update transform */
  aiTransposeMatrix4(&m);
  glPushMatrix();
  glMultMatrixf((float *)&m);

  /* draw all meshes assigned to this node */
  for (; n < nd->mNumMeshes; ++n) {
    const aiMesh *mesh = scene->mMeshes[nd->mMeshes[n]];

    apply_material(sc->mMaterials[mesh->mMaterialIndex]);

    if (mesh->mNormals == NULL) {
      glDisable(GL_LIGHTING);
    } else {
      glEnable(GL_LIGHTING);
    }

    for (t = 0; t < mesh->mNumFaces; ++t) {
      const aiFace *face = &mesh->mFaces[t];
      GLenum face_mode;

      switch (face->mNumIndices) {
      case 1:
        face_mode = GL_POINTS;
        break;
      case 2:
        face_mode = GL_LINES;
        break;
      case 3:
        face_mode = GL_TRIANGLES;
        break;
      default:
        face_mode = GL_POLYGON;
        break;
      }

      glBegin(face_mode);

      for (i = 0; i < face->mNumIndices; i++) {
        int index = face->mIndices[i];
        if (mesh->mColors[0] != NULL)
          glColor4fv((GLfloat *)&mesh->mColors[0][index]);
        if (mesh->mNormals != NULL)
          glNormal3fv(&mesh->mNormals[index].x);
        glVertex3fv(&mesh->mVertices[index].x);
      }

      glEnd();
    }
  }

  /* draw all children */
  for (n = 0; n < nd->mNumChildren; ++n) {
    recursive_render(sc, nd->mChildren[n]);
  }

  glPopMatrix();
}

/* ----------------------------------------------------------------------------
 */
void do_motion(void) {
  static GLint prev_time = 0;
  static GLint prev_fps_time = 0;
  static int frames = 0;

  // TODO(wcraddock): make sure this still works in glfw
  int time = glfwGetTime();
  angle += (time - prev_time) * 0.01;
  prev_time = time;

  frames += 1;
  if ((time - prev_fps_time) > 1000) /* update every seconds */
  {
    int current_fps = frames * 1000 / (time - prev_fps_time);
    printf("%d fps\n", current_fps);
    frames = 0;
    prev_fps_time = time;
  }
}

#define TOTAL_FLOATS_IN_TRIANGLE 9

void draw_random_hairs() {
  srand(time(NULL));

  const int hairs_to_draw = 2400;
  int hairs_drawn = 0;

  while (hairs_drawn < hairs_to_draw) {
    // Pick a random face
    int total_faces = human_obj.TotalConnectedTriangles / 9;
    int face_number = rand() % total_faces;

    // Choose the first vertex of the face
    float *vertex = &human_obj.Faces_Triangles[face_number * TOTAL_FLOATS_IN_TRIANGLE];
    glm::vec3 top_center(vertex[0], vertex[1], vertex[2]);

    // Get the normal for that vertex.
    float *normal_f = &human_obj.normals[face_number * TOTAL_FLOATS_IN_TRIANGLE];
    glm::vec3 normal = glm::normalize(
        glm::vec3(normal_f[0], normal_f[1], normal_f[2]));
    printf("normal from file: %f %f %f\n", normal.x, normal.y, normal.z);

    glm::vec3 A(vertex[0], vertex[1], vertex[2]);
    glm::vec3 B(vertex[3], vertex[4], vertex[5]);
    glm::vec3 C(vertex[6], vertex[7], vertex[8]);

    normal = glm::normalize(glm::cross(B - A, C - A));
    printf("computed normal: %f %f %f\n", normal.x, normal.y, normal.z);

    // Cross the normal with the "straight down" direction to get a vector
    // that points left, along the width of the hair.
    glm::vec3 hair_left =
        glm::normalize(glm::cross(normal, glm::vec3(0.0f, -1.0f, 0.0f)));

    // Cross the left vector with the normal to obtain a vector along the
    // length of the hair.
    glm::vec3 hair_down = glm::normalize(glm::cross(hair_left, normal));

    const float hair_width = 0.0127f;  // 0.5 inch
    const float hair_height = 0.0762f; // 3 inches

    glm::vec3 top_left = top_center - (hair_width / 2.0f) * hair_left;
    glm::vec3 bottom_left = top_left + hair_height * hair_down;
    glm::vec3 bottom_right = bottom_left + hair_width * hair_left;
    glm::vec3 top_right = bottom_right - hair_height * hair_down;

    printf("top_center %f %f %f\n", top_center.x, top_center.y, top_center.z);
    printf("normal %f %f %f\n", normal.x, normal.y, normal.z);
    printf("hair_left %f %f %f\n", hair_left.x, hair_left.y, hair_left.z);
    printf("hair_down %f %f %f\n", hair_down.x, hair_down.y, hair_down.z);
    printf("top_left %f %f %f\n", top_left.x, top_left.y, top_left.z);
    printf("bottom_left %f %f %f\n", bottom_left.x, bottom_left.y,
           bottom_left.z);
    printf("bottom_right %f %f %f\n", bottom_right.x, bottom_right.y,
           bottom_right.z);
    printf("top_right %f %f %f\n", top_right.x, top_right.y, top_right.z);

    double illumination = ((double)rand() / (RAND_MAX));
    // glColor3f(illumination, illumination, illumination);

    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(top_center.x, top_center.y, top_center.z);
    glm::vec3 end = top_center + normal;
    printf("end point %f %f %f\n", end.x, end.y, end.z);
    glVertex3f(end.x, end.y, end.z);
    glEnd();

    // TODO(wcraddock): Make sure each hair is not too close to another
    // TODO(wcraddock): Get normal at that point
    // TODO(wcraddock): Add lighting

    glBegin(GL_QUADS);
    // glVertex3f(vertex[0], vertex[1], vertex[2]);               // top left
    // glVertex3f(vertex[0], vertex[1] - hair_height, vertex[2]); // bottom left
    // glVertex3f(vertex[0] + hair_width, vertex[1] - hair_height,
    //            vertex[2]);                                    // bottom right
    // glVertex3f(vertex[0] + hair_width, vertex[1], vertex[2]); // top right

    glVertex3f(top_left.x, top_left.y, top_left.z);
    glVertex3f(bottom_left.x, bottom_left.y, bottom_left.z);
    glVertex3f(bottom_right.x, bottom_right.y, bottom_right.z);
    glVertex3f(top_right.x, top_right.y, top_right.z);

    glEnd();

    hairs_drawn += 1;
  }
}

void display(void) {
  float tmp;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* center the model */
  glTranslatef(-scene_center.x, -scene_center.y, -scene_center.z);

  /* if the display list has not been made yet, create a new one and
     fill it with scene contents */
  if (scene_list == 0) {
    printf("Creating display list...\n");
    scene_list = glGenLists(1);
    glNewList(scene_list, GL_COMPILE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor3f(1.0f, 0.86f, 0.69f);
    human_obj.Draw();

    draw_random_hairs();

    glEndList();
  }

  glCallList(scene_list);
}

typedef struct {
  int width;
  int height;
  char *title;

  float field_of_view_angle;
  float z_near;
  float z_far;
} glutWindow;

void initialize() {
  GLint width = 1024;
  GLint height = 768;
  char *title = "Disco Wookie";
  GLfloat field_of_view_angle = 45;
  GLfloat z_near = 1.0f;
  GLfloat z_far = 500.0f;

  // Set up z-buffering
  glShadeModel(GL_SMOOTH);
  glClearColor(0.0f, 0.1f, 0.0f, 0.5f);
  glClearDepth(1.0f);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  // Set up lighting
  GLfloat amb_light[] = { 0.1, 0.1, 0.1, 1.0 };
  GLfloat diffuse[] = { 0.6, 0.6, 0.6, 1 };
  GLfloat specular[] = { 0.7, 0.7, 0.3, 1 };
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb_light);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);
  glShadeModel(GL_SMOOTH);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
}

int main(void) {
  printf("Calling obj.Load()...\n");
  human_obj.Load("models/tshirt_long.obj");

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwSetErrorCallback(error_callback);

  glfwWindowHint(GLFW_SAMPLES, 4);

  printf("Creating OpenGL window...\n");
  GLFWwindow *window = glfwCreateWindow(window_width, window_height,
                                        "Simple example", NULL, NULL);
  if (!window) {
    printf("Oh noes!\n");
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);

  glfwSetCursorPosCallback(window, cursor_position_callback);
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

  printf("Calling initialize()...\n");
  initialize();

  printf("Entering main loop...\n");
  while (!glfwWindowShouldClose(window)) {
    // Compute the MVP matrix from keyboard and mouse input
    computeMatricesFromInputs(window);
    glm::mat4 ProjectionMatrix = getProjectionMatrix();
    glm::mat4 ViewMatrix = getViewMatrix();
    glm::mat4 ModelMatrix = glm::mat4(1.0);
    glm::mat4 MV = ViewMatrix * ModelMatrix;

    // Set the MVP matrix
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(&ProjectionMatrix[0][0]);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(&MV[0][0]);

    display();
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  exit(EXIT_SUCCESS);
}

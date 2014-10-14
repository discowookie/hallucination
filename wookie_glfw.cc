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

void draw_random_hairs() {
  srand(time(NULL));

  const int hairs_to_draw = 1000;
  int hairs_drawn = 0;

  while (hairs_drawn < hairs_to_draw) {
    int total_vertices = human_obj.TotalConnectedPoints / 3;
    int vertex_number = rand() % total_vertices;
    float *vertex = &human_obj.vertexBuffer[vertex_number * 3];

    printf("Selected vertex # %d out of %d\n", vertex_number, total_vertices);

    // Make sure the point is on the torso
    if (vertex[1] > 1.5f || vertex[1] < 1.0f
     || fabs(vertex[0]) > 0.25f || fabs(vertex[2]) > 0.25f) {
      printf("Vertex was %f %f %f; ignoring...\n", vertex[0], vertex[1], vertex[2]);
      continue;
    }

    glColor3f(1.0f, 0.0f, 0.0f);

    // TODO(wcraddock): Get normal at that point
    
    const float hair_width = 0.0127f;
    const float hair_height = 0.0762f;

    glBegin(GL_QUADS);
    glVertex3f(vertex[0], vertex[1], vertex[2]); // top left
    glVertex3f(vertex[0], vertex[1] - hair_height, vertex[2]); // bottom left
    glVertex3f(vertex[0] + hair_width, vertex[1] - hair_height, vertex[2]); // bottom right
    glVertex3f(vertex[0] + hair_width, vertex[1], vertex[2]); // top right
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
    glColor3f(1.0f, 1.0f, 1.0f);
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
  GLint width = 640;
  GLint height = 480;
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
  human_obj.Load("cody_simple.obj");

  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwSetErrorCallback(error_callback);

  glfwWindowHint(GLFW_SAMPLES, 4);

  printf("Creating OpenGL window...\n");
  GLFWwindow *window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
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

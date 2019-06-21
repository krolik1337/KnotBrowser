#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>

/* GLUT include file*/
#include <GL/glut.h>

/* assimp include files. These three are usually needed. */
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

/* AntTweakBar include files*/
#include <AntTweakBar.h>

/*Rotation*/
float gRotation[3] = { 0.f,1.f,0.f };
int gAutoRotate = 1;
float rotationSpeed = 0.1f;
float zoom;

/*Background Color*/
float bgColor[3] = { 0.2f,0.3f,0.5f };

/* the global Assimp scene object */
const struct aiScene* scene = NULL;
GLuint scene_list = 0;
struct aiVector3D scene_min, scene_max, scene_center;

/* current rotation angle */
static float angle = 45.f;
float lightMultiplier;
float lightDirection[3];
float matAmbient[] = { 0.5f, 0.0f, 0.0f, 1.0f };
float matDiffuse[] = { 1.0f, 1.0f, 0.0f, 1.0f };

/* to load meshes and models */
GLuint Mesh = 3;
GLuint newMesh = 3;

GLuint Model = 2;
GLuint newModel = 2;

#define aisgl_min(x,y) ((x)<(y)?(x):(y))
#define aisgl_max(x,y) ((y)>(x)?(y):(x))

/* ---------------------------------------------------------------------------- */
void reshape(int width, int height)
{
	const double aspectRatio = (float)width / height, fieldOfView = 45.0;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fieldOfView, aspectRatio, 1.0, 1000.0);  /* Znear and Zfar */
	glViewport(0, 0, width, height);
	TwWindowSize(width, height);
}

/* ---------------------------------------------------------------------------- */
void get_bounding_box_for_node(const struct aiNode* nd, struct aiVector3D* min, struct aiVector3D* max, struct aiMatrix4x4* trafo) {
	struct aiMatrix4x4 prev;
	unsigned int n = 0, t;

	prev = *trafo;
	aiMultiplyMatrix4(trafo, &nd->mTransformation);

	for (; n < nd->mNumMeshes; ++n) {
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
		for (t = 0; t < mesh->mNumVertices; ++t) {

			struct aiVector3D tmp = mesh->mVertices[t];
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

/* ---------------------------------------------------------------------------- */
void get_bounding_box(struct aiVector3D* min, struct aiVector3D* max) {
	struct aiMatrix4x4 trafo;
	aiIdentityMatrix4(&trafo);

	min->x = min->y = min->z = 1e10f;
	max->x = max->y = max->z = -1e10f;
	get_bounding_box_for_node(scene->mRootNode, min, max, &trafo);
}

/* ---------------------------------------------------------------------------- */
void color4_to_float4(const struct aiColor4D *c, float f[4]) {
	f[0] = c->r;
	f[1] = c->g;
	f[2] = c->b;
	f[3] = c->a;
}

/* ---------------------------------------------------------------------------- */
void set_float4(float f[4], float a, float b, float c, float d) {
	f[0] = a;
	f[1] = b;
	f[2] = c;
	f[3] = d;
}

/* ---------------------------------------------------------------------------- */
void recursive_render(const struct aiScene *sc, const struct aiNode* nd) {
	unsigned int i;
	unsigned int n = 0, t;
	struct aiMatrix4x4 m = nd->mTransformation;

	/* update transform */
	aiTransposeMatrix4(&m);
	glPushMatrix();
	glMultMatrixf((float*)&m);

	/* draw all meshes assigned to this node */
	for (; n < nd->mNumMeshes; ++n) {
		const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

		if (mesh->mNormals == NULL) {
			glDisable(GL_LIGHTING);
		}
		else {
			glEnable(GL_LIGHTING);
		}

		for (t = 0; t < mesh->mNumFaces; ++t) {
			const struct aiFace* face = &mesh->mFaces[t];
			GLenum face_mode;

			switch (Mesh) {
			case 1: face_mode = GL_POINTS; break;
			case 2: face_mode = GL_LINES; break;
			case 3: face_mode = GL_TRIANGLES; break;
			default: face_mode = GL_POLYGON; break;
			}

			glBegin(face_mode);

			for (i = 0; i < face->mNumIndices; i++) {
				int index = face->mIndices[i];
				if (mesh->mColors[0] != NULL)
					glColor4fv((GLfloat*)&mesh->mColors[0][index]);
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

/* ---------------------------------------------------------------------------- */
int loadasset(Model) {
	/* we are taking one of the postprocessing presets to avoid
	   spelling out 20+ single postprocessing flags here. */
	const char * path ="";

	switch (Model)
	{
		case 0: path = "D:\\Git\\KnotBrowser\\KnotBrowser\\Debug\\Knots\\knot2.obj"; break;
		case 1: path = "D:\\Git\\KnotBrowser\\KnotBrowser\\Debug\\Knots\\rope.obj"; break;
		case 2: path = "D:\\Git\\KnotBrowser\\KnotBrowser\\Debug\\Knots\\knot1.obj"; break;
		case 3: path = "D:\\Git\\KnotBrowser\\KnotBrowser\\Debug\\Knots\\wezelszrotowy.obj"; break;
		case 4: path = "D:\\Git\\KnotBrowser\\KnotBrowser\\Debug\\Knots\\wezelplaski.obj"; break;
		case 5: path = "D:\\Git\\KnotBrowser\\KnotBrowser\\Debug\\Knots\\wezelrzutkowy.obj"; break;
		case 6: path = "D:\\Git\\KnotBrowser\\KnotBrowser\\Debug\\Knots\\wyblinka.obj"; break;	
	}

	scene = aiImportFile(path, aiProcessPreset_TargetRealtime_MaxQuality);

	if (scene) {
		get_bounding_box(&scene_min, &scene_max);
		scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
		scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
		scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
		return 0;
	}
	return 1;
}

/* Reload model after model or mesh change*/
void ModelReload(GLuint newMesh, GLuint newModel) {
	Mesh = newMesh;
	Model = newModel;
	aiReleaseImport(scene);
	aiDetachAllLogStreams();
	scene_list = 0;
	loadasset(Model);
}

/* ---------------------------------------------------------------------------- */
void do_motion(void) {
	static GLint prev_time = 0;
	static GLint prev_fps_time = 0;
	static int frames = 0;

	int time = glutGet(GLUT_ELAPSED_TIME);
	angle += (time - prev_time)*rotationSpeed;
	prev_time = time;

	frames += 1;
	if ((time - prev_fps_time) > 1000) /* update every seconds */
	{
		int current_fps = frames * 1000 / (time - prev_fps_time);
		printf("%d fps\n", current_fps);
		frames = 0;
		prev_fps_time = time;
	}

	if (Mesh != newMesh || Model != newModel) ModelReload(newMesh,newModel);

	glutPostRedisplay();
}

/* ---------------------------------------------------------------------------- */
void display(void) {
	float tmp;
	float v[4];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.f, 0.f, 3.f, 0.f, 0.f, -5.f, 0.f, 1.f, 0.f);

	/* rotate it around the y axis */
	glRotatef(angle, gRotation[0], gRotation[1], gRotation[2]);

	/* scale the whole asset to fit into our view frustum */
	tmp = scene_max.x - scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y, tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z, tmp);
	tmp = 1.f / tmp;
	glScalef(tmp, tmp, tmp);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	v[0] = v[1] = v[2] = lightMultiplier * 0.4f; v[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_AMBIENT, v);
	v[0] = v[1] = v[2] = lightMultiplier * 0.8f; v[3] = 1.0f;
	glLightfv(GL_LIGHT0, GL_DIFFUSE, v);
	v[0] = -lightDirection[0]; v[1] = -lightDirection[1]; v[2] = -lightDirection[2]; v[3] = 0.0f;
	glLightfv(GL_LIGHT0, GL_POSITION, v);

	/*Sets background color*/
	glClearColor(bgColor[0],bgColor[1],bgColor[2], 0.f);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
	/* center the model */
	glTranslatef(-scene_center.x, -scene_center.y, -scene_center.z);

	/* if the display list has not been made yet, create a new one and
	   fill it with scene contents */
	if (scene_list == 0) {
		scene_list = glGenLists(1);
		glNewList(scene_list, GL_COMPILE);
		/* now begin at the root node of the imported data and traverse
		   the scenegraph by multiplying subsequent local transforms
		   together on GL's matrix stack. */
		recursive_render(scene, scene->mRootNode);
		glEndList();
	}

	glScalef(zoom, zoom, zoom);
	glCallList(scene_list);

	TwDraw();

	glutSwapBuffers();

	do_motion();
}


// Function called at exit
void Terminate(void) {
	TwTerminate();
}

/* Sets autoratate flag */
void TW_CALL AutoRotateCB(void *p) {
	if (gAutoRotate) rotationSpeed = 0.f;
	else rotationSpeed = 0.1f;
	gAutoRotate = !gAutoRotate;
}

/* ---------------------------------------------------------------------------- */
int main(int argc, char **argv) {
	struct aiLogStream stream;

	glutInitWindowSize(1280, 720);
	glutInitWindowPosition(100, 100);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInit(&argc, argv);
	glutCreateWindow("Knot Browser - Check your knot!");

	/* Turn off Ant Tweak Bar after glutMainLoop ends */
	atexit(Terminate);

	zoom = 1;
	matAmbient[0] = 0.0f; matAmbient[1] = matAmbient[2] = 0.2f; matAmbient[3] = 1;
	matDiffuse[0] = 0.0f; matDiffuse[1] = 1; matDiffuse[2] = 0; matDiffuse[3] = 1;
	lightMultiplier = 1;
	lightDirection[0] = lightDirection[1] = lightDirection[2] = -0.57735f;

	// Initialize AntTweakBar
	TwInit(TW_OPENGL, NULL);
	TwBar* bar = TwNewBar("Knot Browser");

	// Array of drop down items
	TwEnumVal Meshes[] = { {0, "Polygon"}, {1, "Points"}, {2, "Lines"}, {3, "Triangles"} };
	// ATB identifier for the array
	TwType MeshTwType = TwDefineEnum("MeshType", Meshes, 4);

	// Array of drop down items
	TwEnumVal Models[] = { {0, "Lina 1"}, {1, "Lina 2"}, {2, "Osemka"}, {3, "Szrotowy"}, {4, "Plaski"}, {5, "Rzutkowy"}, {6, "Wyblinka"} };
	// ATB identifier for the array
	TwType ModelTwType = TwDefineEnum("ModelType", Models, 7);

	// TweakBar Menu
	TwAddVarRW(bar, "Mesh", MeshTwType, &newMesh, NULL);
	TwAddVarRW(bar, "Model", ModelTwType, &newModel, NULL);
	TwAddSeparator(bar, "", NULL);

	TwAddVarRW(bar, "Zoom", TW_TYPE_FLOAT, &zoom, " min=0.01 max=2.5 step=0.01");
	TwAddVarRW(bar, "Multiplier", TW_TYPE_FLOAT, &lightMultiplier, " label='Light booster' min=0.1 max=4 step=0.02 ");
	TwAddVarRW(bar, "Rotation Speed", TW_TYPE_FLOAT, &rotationSpeed, " min=0 max=5 step=0.05 keyIncr=+ keyDecr=- help='Rotation speed (turns/second)' ");
	TwAddButton(bar, "AutoRotate", AutoRotateCB, NULL, " label='Auto rotate' ");
	TwAddSeparator(bar, "", NULL);

	TwAddVarRW(bar, "Rotation", TW_TYPE_DIR3F, &gRotation, " axisz=-z ");
	TwAddVarRW(bar, "LightDir", TW_TYPE_DIR3F, &lightDirection, " label='Light direction'");
	TwAddVarRW(bar, "BgColor", TW_TYPE_COLOR3F, &bgColor, "label='Background color'");
	TwAddVarRW(bar, "Ambient", TW_TYPE_COLOR3F, &matAmbient, " group='Material' ");
	TwAddVarRW(bar, "Diffuse", TW_TYPE_COLOR3F, &matDiffuse, " group='Material' ");

	// after GLUT initialization
	// directly redirect GLUT events to AntTweakBar
	glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
	glutMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT);
	glutPassiveMotionFunc((GLUTmousemotionfun)TwEventMouseMotionGLUT); // same as MouseMotion
	glutKeyboardFunc((GLUTkeyboardfun)TwEventKeyboardGLUT);
	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);

	// send the ''glutGetModifers'' function pointer to AntTweakBar
	TwGLUTModifiersFunc(glutGetModifiers);

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	/* get a handle to the predefined STDOUT log stream and attach
	   it to the logging system. It remains active for all further
	   calls to aiImportFile(Ex) and aiApplyPostProcessing. */
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT, NULL);
	aiAttachLogStream(&stream);

	/* ... same procedure, but this stream now writes the
	   log messages to assimp_log.txt */
	stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE, "assimp_log.txt");
	aiAttachLogStream(&stream);

	/* load default model */
	loadasset(Model);

	glEnable(GL_DEPTH_TEST);

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_NORMALIZE);

	/* XXX docs say all polygons are emitted CCW, but tests show that some aren't. */
	if (getenv("MODEL_IS_BROKEN"))
		glFrontFace(GL_CW);

	glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

	glutGet(GLUT_ELAPSED_TIME);
	glutMainLoop();

	/* cleanup - calling 'aiReleaseImport' is important, as the library
	   keeps internal resources until the scene is freed again. Not
	   doing so can cause severe resource leaking. */
	aiReleaseImport(scene);

	/* We added a log stream to the library, it's our job to disable it
	   again. This will definitely release the last resources allocated
	   by Assimp.*/
	aiDetachAllLogStreams();

	return 0;
}
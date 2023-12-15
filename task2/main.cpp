#include"GL/glew.h"
#include"GL/freeglut.h"
#include"SOIL.h"
#include<vector>
#include<glm/glm.hpp>
#include<string>
#include<sstream>
#include<fstream>
#include<map>
#include<ctime>
#include<random>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>

GLuint texture;

//вершины объекта
std::vector<glm::vec3> indexed_vertices;
//текстурные координаты для каждой вершины объекта
std::vector<glm::vec2> indexed_uvs;
//нормали для каждой вершины объекта
std::vector<glm::vec3> indexed_normals;
//индексы вершин дл¤ соединения вершин в грани
std::vector<unsigned short> indices;

GLuint vertexbuffer;
GLuint uvbuffer;
GLuint normalbuffer;
GLuint elementbuffer;

const double pi = 3.14159265358979323846;
int light_num = 0;
std::vector<float> light_angle{ 0, 90, 180 };
std::vector<float> light_pos{ 5, 5, 5 };
std::vector<float> light_rad{ 10, 10, 10 };
float model_angle = 0;
int is_ahead = 0;
int is_up = 0;
int is_down = 0;
int is_left = 0;
int is_right = 0;
int is_back = 0;
int width = 0, height = 0;
float x = 0; float z = 0; float y = 0;
float SPEED = 0.1;
// Переменные для хранения положения камеры
float cameraX = 0.0f;
float cameraY = 0.0f;
float cameraZ = 5.0f;
float kk = 0.0;

struct VertexData
{
	glm::vec3 vertex;
	glm::vec2 uv;
	glm::vec3 normal;
	bool operator<(const VertexData that) const
	{
		return memcmp((void*)this, (void*)&that, sizeof(VertexData)) > 0;
	};
};

void indexVBO(std::vector<glm::vec3>& in_vertices, std::vector<glm::vec2>& in_uvs, std::vector<glm::vec3>& in_normals,
	std::vector<unsigned short>& out_indices, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs,
	std::vector<glm::vec3>& out_normals)
{
	std::map<VertexData, unsigned short> VertexToOutIndex;

	for (unsigned int i = 0; i < in_vertices.size(); i++)
	{
		VertexData packed = { in_vertices[i], in_uvs[i], in_normals[i] };

		unsigned short index;
		auto it = VertexToOutIndex.find(packed);
		if (it != VertexToOutIndex.end()) // check if vertex already exists
			out_indices.push_back(it->second);
		else
		{ // If not, it needs to be added in the output data.
			out_vertices.push_back(in_vertices[i]);
			out_uvs.push_back(in_uvs[i]);
			out_normals.push_back(in_normals[i]);
			unsigned short newindex = (unsigned short)out_vertices.size() - 1;
			out_indices.push_back(newindex);
			VertexToOutIndex[packed] = newindex;
		}
	}
}
void processMesh(aiMesh* mesh, const aiScene* scene, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals)
{
	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		glm::vec3 vertex;
		vertex.x = mesh->mVertices[i].x;
		vertex.y = mesh->mVertices[i].y;
		vertex.z = mesh->mVertices[i].z;
		out_vertices.push_back(vertex);

		glm::vec2 uv;
		if (mesh->mTextureCoords[0])
		{
			uv.x = mesh->mTextureCoords[0][i].x;
			uv.y = mesh->mTextureCoords[0][i].y;
		}
		else
		{
			uv.x = 0.0f;
			uv.y = 0.0f;
		}
		out_uvs.push_back(uv);

		glm::vec3 normal;
		normal.x = mesh->mNormals[i].x;
		normal.y = mesh->mNormals[i].y;
		normal.z = mesh->mNormals[i].z;
		out_normals.push_back(normal);
	}
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}
}
void processNode(aiNode* node, const aiScene* scene, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals)
{
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(mesh, scene, out_vertices, out_uvs, out_normals);
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		processNode(node->mChildren[i], scene, out_vertices, out_uvs, out_normals);
	}
}
void readObj(const std::string& path, std::vector<glm::vec3>& out_vertices, std::vector<glm::vec2>& out_uvs, std::vector<glm::vec3>& out_normals)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
		return;
	}

	processNode(scene->mRootNode, scene, out_vertices, out_uvs, out_normals);
}


void init(void)
{
	glClearColor(0.529, 0.808, 0.922, 1.0);  // Устанавливаем цвет фона главного окна в голубой
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_CULL_FACE);

	const GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1 };
	const GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
	const GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);

	glLightfv(GL_LIGHT2, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light_specular);

	glLightfv(GL_LIGHT3, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT3, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT3, GL_SPECULAR, light_specular);

	glEnable(GL_DEPTH_TEST);
	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT1);
	//glEnable(GL_LIGHT2);
	//glEnable(GL_LIGHT3);

	texture = SOIL_load_OGL_texture
	(
		"Mickey Mouse.jpg",
		SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID,
		SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
	);
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	readObj("tt.obj", vertices, uvs, normals);
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);
}

double gr_cos(float angle) noexcept
{
	return cos(angle / 180 * pi);
}

double gr_sin(float angle) noexcept
{
	return sin(angle / 180 * pi);
}

void set_lights()
{
	double x = light_rad[0] * gr_cos(light_angle[0]);
	double z = light_rad[0] * gr_sin(light_angle[0]);
	GLfloat position[] = { 0, 0, 0, 1 };
	GLfloat light[] = { 1, 1, 1, 1 };
	GLfloat no_light[] = { 0, 0, 0, 1 };
	if (glIsEnabled(GL_LIGHT1))
	{
		glPushMatrix();
		glTranslatef(x, light_pos[0], z);
		glLightfv(GL_LIGHT1, GL_POSITION, position);
		glMaterialfv(GL_FRONT, GL_EMISSION, light);
		glutSolidSphere(0.5, 10, 10);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_light);
		glPopMatrix();
	}

	if (glIsEnabled(GL_LIGHT2))
	{
		glPushMatrix();
		x = light_rad[1] * gr_cos(light_angle[1]);
		z = light_rad[1] * gr_sin(light_angle[1]);
		glTranslatef(x, light_pos[1], z);
		glLightfv(GL_LIGHT2, GL_POSITION, position);
		glMaterialfv(GL_FRONT, GL_EMISSION, light);
		glutSolidSphere(0.5, 10, 10);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_light);
		glPopMatrix();
	}

	if (glIsEnabled(GL_LIGHT3))
	{
		glPushMatrix();
		x = light_rad[2] * gr_cos(light_angle[2]);
		z = light_rad[2] * gr_sin(light_angle[2]);
		glTranslatef(x, light_pos[2], z);
		glLightfv(GL_LIGHT3, GL_POSITION, position);
		glMaterialfv(GL_FRONT, GL_EMISSION, light);
		glutSolidSphere(0.5, 10, 10);
		glMaterialfv(GL_FRONT, GL_EMISSION, no_light);
		glPopMatrix();
	}
}

void reshape(int w, int h)
{
	width = w; height = h;
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, w / h, 1.0, 100.0);
	gluLookAt(0, 10, 30, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void prepare_buffers()
{
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
}

void set_vertices_attributes()
{
	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// 2nd attribute buffer : UVs
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glTexCoordPointer(2, GL_FLOAT, 0, 0);

	// 3rd attribute buffer : normals
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glNormalPointer(GL_FLOAT, 0, 0); // Normal start position address

	// Index buffer
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
}

void prepare_texture_buffer()
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
}

void cleanup_buffers()
{
	glDisable(GL_TEXTURE_2D);
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

void draw_head()
{
	prepare_buffers();
	prepare_texture_buffer();
	set_vertices_attributes();
	//draw triangles
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, 0);
	cleanup_buffers();
}

void draw_model()
{
	glPushMatrix();
	glTranslatef(x, y, z);
	glRotatef(model_angle, 0, 1, 0);
	//glColor3f(0.8, 0.8, 0.8);
	draw_head();
	glPopMatrix();
}

void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW); // Выбор матрицы моделирования-вида
	glLoadIdentity(); // Сброс матрицы
	kk += SPEED;
	//gluLookAt(0+kk, 10, 30,  // Положение камеры
	//	0.0f+kk, 0.0f, 0.0f,          // Координаты цели, куда смотрит камера (в данном случае в начало координат)
	//	0.0f, 1.0f, 0.0f);         // Направление вертикальной оси камеры
	if (is_ahead)
	{
		x += SPEED * gr_sin(model_angle);
		z += SPEED * gr_cos(model_angle);
		is_ahead = 0;
	}
	if (is_up)
	{
		x += SPEED * gr_sin(model_angle);
		y += SPEED * gr_cos(model_angle);
		is_up = 0;
	}
	if (is_down)
	{
		x -= SPEED * gr_sin(model_angle);
		y -= SPEED * gr_cos(model_angle);
		is_down = 0;
	}
	if (is_right)
	{
		//x += SPEED * gr_sin(model_angle);
		x += SPEED * gr_cos(model_angle);
		is_right = 0;
	}
	if (is_left)
	{
		//y += SPEED * gr_sin(model_angle);
		x -= SPEED * gr_cos(model_angle);
		is_left = 0;
	}
	if (is_back)
	{
		x -= SPEED * gr_sin(model_angle);
		z -= SPEED * gr_cos(model_angle);
		is_back = 0;
	}
	set_lights();
	draw_model();
	glutSwapBuffers();
}

//void specialKeys(int key, int x, int y)
//{
//	switch (key)
//	{
//	case GLUT_KEY_UP: light_pos[light_num] += 0.5; break;
//	case GLUT_KEY_DOWN: light_pos[light_num] -= 0.5; break;
//	case GLUT_KEY_RIGHT: light_angle[light_num] -= 3; break;
//	case GLUT_KEY_LEFT: light_angle[light_num] += 3; break;
//	case GLUT_KEY_PAGE_UP: light_rad[light_num] -= 0.5; break;
//	case GLUT_KEY_PAGE_DOWN: light_rad[light_num] += 0.5; break;
//	case GLUT_KEY_F1:
//		light_num = 0;
//		break;
//	case GLUT_KEY_F2:
//		light_num = 1;
//		break;
//	case GLUT_KEY_F3:
//		light_num = 2;
//		break;
//	}
//	glutPostRedisplay();
//}

void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case '1':
		if (glIsEnabled(GL_LIGHT1))
			glDisable(GL_LIGHT1);
		else
			glEnable(GL_LIGHT1);
		break;
	case '2':
		if (glIsEnabled(GL_LIGHT2))
			glDisable(GL_LIGHT2);
		else
			glEnable(GL_LIGHT2);
		break;
	case '3':
		if (glIsEnabled(GL_LIGHT3))
			glDisable(GL_LIGHT3);
		else
			glEnable(GL_LIGHT3);
		break;
	case 'q':
	case 'Q':
		model_angle += 5;
		break;
	case 'e':
	case 'E':
		model_angle -= 5;
		break;
	case 'w':
	case 'W':
		is_ahead = 1;
		//cameraZ -= SPEED; // Движение вперед
		break;
	case 'x':
	case 'X':
		is_up = 1;
		break;
	case 'z':
	case 'Z':
		is_down = 1;
		break;
	case 'a':
	case 'A':
		is_left = 1;
		cameraX -= SPEED/8; // Движение влево
		break;
	case 'd':
	case 'D':
		is_right = 1;
		break;
	case 's':
	case 'S':
		is_back = 1;
		break;
	}
	glutPostRedisplay();
}

void disable_all()
{
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

void scale_model(float scale_factor)
{
	for (unsigned int i = 0; i < indexed_vertices.size(); i++)
	{
		indexed_vertices[i].x *= scale_factor;
		indexed_vertices[i].y *= scale_factor;
		indexed_vertices[i].z *= scale_factor;
	}
}



int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(800, 800);
	glutInitWindowPosition(10, 10);
	glutCreateWindow("Lab13/task2");
	glewInit();
	init();
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	//glutSpecialFunc(specialKeys);
	glutKeyboardFunc(keyboard);
	//scale_model(0.0125 * 10);
	scale_model(10);
	glutMainLoop();

	//disable_all();
	return 0;
}
#define _CRT_SECURE_NO_WARNINGS //--- 프로그램 맨 앞에 선언할 것

#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h> 
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>
#include "dtd.h" 
#define SIZE 600
// 함수 선언
void MakeVertexShaders();
void MakeFragmentShaders();
GLuint MakeShaderProgram();
void InitBuffers();
GLvoid DrawScene();
GLvoid Reshape(int w, int h);
void Mouse(int button, int state, int x, int y);
void Keyboard(unsigned char key, int x, int y);
void SpecialKeyboard(int key, int x, int y); // 특수 키(화살표) 처리 함수 선언
void LoadOBJ(const char* filename);
void InitBuffer();
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
void TimerFunction(int value);
GLchar* filetobuf(const char* file);
// 전역 변수
GLint width = 600, height = 600;
GLuint shaderProgramID, vertexShader, fragmentShader;
GLuint VAO, VBO, EBO;

random_device rd;
mt19937 gen(rd());
uniform_real_distribution<float> rcolor(0.0f, 1.0f);
uniform_int_distribution<int> Rint(1, 9);

bool checkbox[10] = { false, };

typedef struct {
	float x, y, z;
} Vertex;
typedef struct {
	unsigned int v1, v2, v3;
} Face;
typedef struct {
	Vertex* vertices;
	size_t vertex_count;
	Face* faces;
	size_t face_count;
} Model;
class Shape {
public:
	vector<GLfloat> vertexData;
	vector<GLuint> indices;
	glm::vec3 t = { 0.0f,0.0f,0.0f };
	glm::vec3 s = { 1.0f,1.0f,1.0f };
	glm::vec3 r = { 0.0f,0.0f,0.0f };
	glm::vec3 colors;
	glm::mat4 modelMat = glm::mat4(1.0f);
	glm::vec3 orimodelMat = glm::vec3(0.0f);
	int shape_num;
	float angle = 0.0f;
	Shape(Model model, int i) {
		vertexData.clear();
		InitBuffer(model);
		ColorRandom(model);
		shape_num = i;
	}

	void InitBuffer(Model model) {
		vertexData.clear();
		// 모든 face의 꼭짓점 좌표를 중복 포함해서 vertexData에 추가
		for (size_t i = 0; i < model.face_count; ++i) {
			Face f = model.faces[i];
			Vertex v1 = model.vertices[f.v1];
			Vertex v2 = model.vertices[f.v2];
			Vertex v3 = model.vertices[f.v3];
			// 첫 번째 꼭짓점
			vertexData.push_back(v1.x);
			vertexData.push_back(v1.y);
			vertexData.push_back(v1.z);
			// 두 번째 꼭짓점
			vertexData.push_back(v2.x);
			vertexData.push_back(v2.y);
			vertexData.push_back(v2.z);
			// 세 번째 꼭짓점
			vertexData.push_back(v3.x);
			vertexData.push_back(v3.y);
			vertexData.push_back(v3.z);
		}
		// 인덱스는 필요 없으므로 비워둡니다
		indices.clear();
	}
	void Update(Model model) {
	}
	void ColorRandom(Model model) {
		colors = { 0.0f,0.0f,0.0f };

		float r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float g = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		float b = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
		colors = glm::vec3(r, g, b);

	}


};
vector<Shape>shape;
void read_newline(char* str) {
	char* pos;
	if ((pos = strchr(str, '\n')) != NULL)
		*pos = '\0';
}

Model read_obj_file(const char* filename) {
	FILE* file;
	Model model{};

	// 파일 열기 (fopen_s를 사용하므로 Visual Studio 환경에 적합)
	fopen_s(&file, filename, "r");
	if (!file) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	// 임시 벡터를 사용하여 동적으로 정점 및 면 데이터를 저장합니다.
	std::vector<Vertex> temp_vertices;
	std::vector<Face> temp_faces;

	char line[365];
	while (fgets(line, sizeof(line), file)) {
		read_newline(line);

		// v 라인 처리
		if (line[0] == 'v' && line[1] == ' ') {
			Vertex v;
			if (sscanf_s(line + 2, "%f %f %f", &v.x, &v.y, &v.z) == 3) {
				temp_vertices.push_back(v);
			}
		}
		// f 라인 처리 - N각형을 삼각 분할(Triangulation)합니다.
		else if (line[0] == 'f' && line[1] == ' ') {
			char face_str[365];
			// 원본 라인을 훼손하지 않기 위해 복사 (strtok 사용을 위함)
			// '제목 없음.obj' 파일이 'f v1 v2 v3 v4...' 형식이라고 가정합니다.
			if (strlen(line + 2) >= sizeof(face_str)) {
				// 버퍼 오버플로우 방지 (필요 시 더 큰 버퍼 사용)
				continue;
			}
			strcpy_s(face_str, sizeof(face_str), line + 2);

			// strtok를 사용하여 공백으로 분리된 각 정점 데이터를 가져옵니다.
			char* token = strtok(face_str, " ");
			std::vector<unsigned int> face_indices; // 면을 이루는 정점 인덱스 저장

			// 토큰(정점 인덱스)을 읽습니다.
			while (token != NULL) {
				unsigned int v_idx;

				// v/vt/vn 형태일 경우, 첫 '/' 이전의 v만 파싱
				if (strchr(token, '/') != NULL) {
					// v/vt/vn 형태일 경우, 첫 '/' 이전의 v만 파싱
					if (sscanf_s(token, "%u", &v_idx) == 1) {
						face_indices.push_back(v_idx - 1); // 1-based index를 0-based index로 변환
					}
				}
				else {
					// v 형태일 경우, v만 파싱
					if (sscanf_s(token, "%u", &v_idx) == 1) {
						face_indices.push_back(v_idx - 1); // 1-based index를 0-based index로 변환
					}
				}
				token = strtok(NULL, " ");
			}

			// N각형(N >= 3) 면을 삼각 분할(Fan Triangulation)하여 저장합니다.
			if (face_indices.size() >= 3) {
				// 0번 인덱스를 중심으로 삼각형을 만듭니다. (0, 1, 2), (0, 2, 3), (0, 3, 4)...
				for (size_t i = 1; i < face_indices.size() - 1; ++i) {
					Face f;
					f.v1 = face_indices[0]; // 중심점
					f.v2 = face_indices[i];
					f.v3 = face_indices[i + 1];
					temp_faces.push_back(f);
				}
			}
		}
	}
	fclose(file);

	// 최종적으로 모델 구조체에 동적 할당 및 복사합니다.
	model.vertex_count = temp_vertices.size();
	model.face_count = temp_faces.size();

	// 기존 코드가 malloc/free를 사용하므로 이 방식을 유지합니다.
	model.vertices = (Vertex*)malloc(model.vertex_count * sizeof(Vertex));
	model.faces = (Face*)malloc(model.face_count * sizeof(Face));

	if (model.vertex_count > 0) {
		std::copy(temp_vertices.begin(), temp_vertices.end(), model.vertices);
	}
	if (model.face_count > 0) {
		std::copy(temp_faces.begin(), temp_faces.end(), model.faces);
	}

	return model;
}


vector<Model> model;
int facenum = -1;
int modelType = 0; // 0: Cube, 1: Cone
bool allFaceDraw = true; // true: 전체 면 그리기, false: 한 면씩 그리기

//커비
glm::vec3 cameraPos = glm::vec3(5.0f, 5.0f, 5.0f);
float cameraAngleori = 90.0f;
float cameraAngle = glm::atan(glm::radians(cameraAngleori)); // 초기 각도 (45도 라디안)

// 도형(모델) 이동 및 회전 관련 추가 전역 변수
glm::vec3 modelPos = glm::vec3(0.0f, 0.0f, 0.0f);  // 도형 위치 (X, Y, Z 이동량)
float modelRotY = 0.0f;  // Y축 회전 각도 (라디안)
float modelRotX = 0.0f;  // X축 회전 각도 (라디안)

int shape_check = 0;

bool silver = false;
bool solid = false;

glm::vec3 camera_move = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // 카메라가 바라보는 목표점
float w1 = 0.0f;
float w2 = 0.0f;
int w = 15;


glm::vec3 center1;
glm::vec3 center2;
glm::vec3 center3;
glm::vec3 center4;


float rotate_count = 0.0f;

int scale_time = 0;

int move_num = 0;

glm::vec3 center = { 0.0f,0.0f,0.0f };
glm::vec3 rotate1 = glm::vec3(0, 0.707f, -0.707f);
glm::vec3 rotate2 = glm::vec3(0, 1.0f, 0.0f);
glm::vec3 rotate3 = glm::vec3(0, 0.707f, 0.707f);

float _z = 0.0f;
int z_mode = 0;

//커비
void InitData() {
	shape.clear();
	for (size_t i = 0; i < 8; ++i) {
		shape.push_back(Shape(model[0], i));// Shape 생성 시 model 정보 전달

	}

}
void Update() {
	vector<GLfloat> vertexData;
	vector<GLuint> indices;

	for (size_t i = 0; i < shape.size(); ++i) {
		shape[i].Update(model[0]);
		vertexData.insert(vertexData.end(), shape[i].vertexData.begin(), shape[i].vertexData.end());

		// 인덱스 오프셋 보정 필요 (여러 모델 합칠 경우)
		GLuint offset = i == 0 ? 0 : (GLuint)(vertexData.size() / 3 - shape[i].vertexData.size() / 3);
		for (auto idx : shape[i].indices) indices.push_back(idx + offset);
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(GLfloat), vertexData.data(), GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);
}
int main(int argc, char** argv) {
	srand(static_cast<unsigned>(time(0))); // 랜덤 시드 초기화
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH); // GLUT_DEPTH 추가
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("tung tung tung tung tung tung tung tung tung sours");

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패" << std::endl;
		return -1;
	}

	MakeVertexShaders();
	MakeFragmentShaders();
	shaderProgramID = MakeShaderProgram();
	if (shaderProgramID == 0) {
		std::cerr << "셰이더 프로그램 생성 실패" << std::endl;
		return -1;
	}
	model.push_back(read_obj_file("cube.obj"));
	model.push_back(read_obj_file("pyramid.obj"));
	model.push_back(read_obj_file("sphere.obj"));
	
	glm::vec3 v1 = { -0.5,-0.5,-0.5 };
	glm::vec3 v2 = { -0.5,-0.5,0.5 };
	glm::vec3 v3 = { -0.5,0.5,-0.5 };
	glm::vec3 v4 = { -0.5,0.5,0.5 };
	glm::vec3 v5 = { 0.5,-0.5,-0.5 };
	glm::vec3 v6 = { 0.5,-0.5,0.5 };
	glm::vec3 v7 = { 0.5,0.5,-0.5 };
	glm::vec3 v8 = { 0.5,0.5,0.5 };

	vector<glm::vec3> vertices;

	//정육면체 모델

	//앞면
	vertices.push_back(v2); vertices.push_back(v6); vertices.push_back(v8);
	vertices.push_back(v2); vertices.push_back(v8); vertices.push_back(v4);
	model.push_back({ vertices });

	vertices.clear();

	//왼쪽면
	vertices.push_back(v1); vertices.push_back(v4); vertices.push_back(v3);
	vertices.push_back(v1); vertices.push_back(v2); vertices.push_back(v4);
	model.push_back({ vertices });

	vertices.clear();

	//오른쪽면
	vertices.push_back(v5); vertices.push_back(v7); vertices.push_back(v8);
	vertices.push_back(v5); vertices.push_back(v8); vertices.push_back(v6);
	model.push_back({ vertices });

	vertices.clear();

	//뒷면
	vertices.push_back(v1); vertices.push_back(v7); vertices.push_back(v5);
	vertices.push_back(v1); vertices.push_back(v3); vertices.push_back(v7);
	model.push_back({ vertices });

	vertices.clear();


	//아래면
	vertices.push_back(v1); vertices.push_back(v5); vertices.push_back(v6);
	vertices.push_back(v1); vertices.push_back(v6); vertices.push_back(v2);
	model.push_back({ vertices });

	vertices.clear();

	//윗면
	vertices.push_back(v3); vertices.push_back(v8); vertices.push_back(v7);
	vertices.push_back(v3); vertices.push_back(v4); vertices.push_back(v8);
	model.push_back({ vertices });

	vertices.clear();


	//피라미드 모델
	v1 = { -0.5,-0.2,-0.5 };
	v2 = { 0.5,-0.2,-0.5 };
	v3 = { 0.5,-0.2,0.5 };
	v4 = { -0.5,-0.2,0.5 };
	v5 = { 0,0.8,0 };

	//앞면
	vertices.push_back(v5); vertices.push_back(v4); vertices.push_back(v3);
	model.push_back({ vertices });

	vertices.clear();

	//왼쪽면
	vertices.push_back(v5); vertices.push_back(v1); vertices.push_back(v4);
	model.push_back({ vertices });

	vertices.clear();

	//오른쪽면
	vertices.push_back(v5); vertices.push_back(v3); vertices.push_back(v2);
	model.push_back({ vertices });

	vertices.clear();
	//뒷면
	vertices.push_back(v5); vertices.push_back(v2); vertices.push_back(v1);
	model.push_back({ vertices });

	vertices.clear();



	//아래면
	vertices.push_back(v3); vertices.push_back(v1); vertices.push_back(v4);
	vertices.push_back(v3); vertices.push_back(v2); vertices.push_back(v1);
	model.push_back({ vertices });

	vertices.clear();

	InitBuffers();
	InitData();
	glutTimerFunc(10, TimerFunction, 1);
	glutDisplayFunc(DrawScene);
	glutReshapeFunc(Reshape);
	glutMouseFunc(Mouse);
	glutKeyboardFunc(Keyboard);
	glutSpecialFunc(SpecialKeyboard); // 특수 키(화살표) 함수 등록

	glutMainLoop();
	return 0;
}



void MakeVertexShaders() {
	GLchar* vertexSource = filetobuf("vertex.glsl");
	if (!vertexSource) {
		std::cerr << "ERROR: vertex.glsl." << std::endl;
		return;
	}

	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result) {
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		std::cerr << "ERROR: vertex shader\n" << errorLog << std::endl;
	}
	free(vertexSource);
}

void MakeFragmentShaders() {
	GLchar* fragmentSource = filetobuf("fragment.glsl");
	if (!fragmentSource) {
		std::cerr << "ERROR: fragment.glsl." << std::endl;
		return;
	}

	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);

	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result) {
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: fragment shader\n" << errorLog << std::endl;
	}
	free(fragmentSource);
}

GLuint MakeShaderProgram() {
	GLuint shaderID = glCreateProgram();
	glAttachShader(shaderID, vertexShader);
	glAttachShader(shaderID, fragmentShader);
	glLinkProgram(shaderID);

	GLint result;
	GLchar errorLog[512];
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program\n" << errorLog << std::endl;
		return 0;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	glUseProgram(shaderID);
	return shaderID;
}

void InitBuffers() {
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);



	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);



	glBindVertexArray(0);
}

void DrawScene() {

	glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	//glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화
	//glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	////glEnable(GL_CULL_FACE); // 은면 제거 활성화
	//glCullFace(GL_BACK); // 뒷면 제거 

	if (silver) {
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // 뒷면 제거
	}
	else {
		glDisable(GL_CULL_FACE);
	}


	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // 깊이 버퍼 클리어 추가

	glUseProgram(shaderProgramID);
	glBindVertexArray(VAO);

	Update();

	if (!solid)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);


	// Uniform 매트릭스 매핑
	GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
	GLint projLoc = glGetUniformLocation(shaderProgramID, "proj");
	GLint faceColorLoc = glGetUniformLocation(shaderProgramID, "faceColor");


	//--------------------------------------------------------------------------


	GLuint offset = 0;
	for (int i = 0; i < shape.size(); ++i) {
		for (int j = 0; j < shape[i].vertexData.size() / 9; ++j) { // colors 개수 == face 개수
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(shape[i].modelMat));
			glUniform3f(faceColorLoc, shape[i].colors[0], shape[i].colors[1], shape[i].colors[2]);
			glDrawArrays(GL_TRIANGLES, offset, 3);
			offset += 3;
		}
	}

	//--------------------------------------------------------------------------
	// Camera (View) 및 Projection 매트릭스 설정
	glm::vec3 camera_ = cameraPos + camera_move;
	glm::mat4 view = glm::lookAt(camera_, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f)); // 뷰 매트릭스
	glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f); // 프로젝션 매트릭스

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));


	glm::mat4 axisModelMat = glm::mat4(1.0f);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(axisModelMat));

	//---------------------------------------------------------------------------
	// XYZ 축 
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(axisModelMat));

	std::vector<GLfloat> axesData = {
		// X축
		-5.0f,0.0f,0.0f,  // 시작
		5.0f,0.0f,0.0f,  // 끝

		// Y축
		0.0f,-5.0f,0.0f,
		0.0f,5.0f,0.0f,

		// Z축
		0.0f,0.0f,-5.0f,
		0.0f,0.0f,5.0f
	};

	// 축용 VBO 업로드 (위치만, 색상 uniform 사용)
	glBufferData(GL_ARRAY_BUFFER, axesData.size() * sizeof(GLfloat), axesData.data(), GL_DYNAMIC_DRAW);

	// X축
	glUniform3f(faceColorLoc, 1.0f, 0.0f, 0.0f);
	glDrawArrays(GL_LINES, 0, 2);

	// Y축 
	glUniform3f(faceColorLoc, 0.0f, 0.0f, 1.0f);
	glDrawArrays(GL_LINES, 2, 2);

	// Z축 
	glUniform3f(faceColorLoc, 0.0f, 1.0f, 0.0f);
	glDrawArrays(GL_LINES, 4, 2);

	glBindVertexArray(0);
	glutSwapBuffers();
}


void Reshape(int w, int h) {
	glViewport(0, 0, w, h);
	width = w;
	height = h;
}

void Mouse(int button, int state, int x, int y) {

}

void Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	
	case 'q':
		exit(0);
		break;
	}

	glutPostRedisplay();
}
//ㅇㅅㅇ 11/5
// 특수 키(화살표) 처리 함수: 도형 회전
void SpecialKeyboard(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP:
		
		break;
	case GLUT_KEY_DOWN:
		
		break;
	case GLUT_KEY_LEFT:
		
		break;
	case GLUT_KEY_RIGHT:
		
		break;
	default:
		break;
	}

	glutPostRedisplay();  // 재렌더링 요청
}

void TimerFunction(int value)
{

	for (size_t i = 0; i < shape.size(); ++i) {
		
	
	}


	glutTimerFunc(10, TimerFunction, 1);
	glutPostRedisplay();
}

GLchar ch[256]{};
GLchar* filetobuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;
	fptr = fopen(file, "rb"); // Open file for reading
	if (!fptr) // Return NULL on failure
		return NULL;
	fseek(fptr, 0, SEEK_END); // Seek to the end of the file
	length = ftell(fptr); // Find out how many bytes into the file we are
	buf = (char*)malloc(length + 1); // Allocate a buffer for the entire length of the file and a null terminator
	fseek(fptr, 0, SEEK_SET); // Go back to the beginning of the file
	fread(buf, length, 1, fptr); // Read the contents of the file in to the buffer
	fclose(fptr); // Close the file
	buf[length] = 0; // Null terminator
	return buf; // Return the buffer
}
//버텍스 세이더 객체 만들기
void make_vertexShaders()
{
	GLchar* vertexSource;

	//버텍스 세이더 읽어 저장하고 컴파일 하기
	//filetobuf : 사용자정의 함수로 텍스트를 읽어서 문자열에 저장하는 함수
	vertexSource = filetobuf("vertex.glsl");
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
		cerr << "ERROR: vertex shader \n" << errorLog << std::endl;
		return;
	}
}

//프래그먼트 세이더 객체 만들기
void make_fragmentShaders()
{
	GLchar* fragmentSource;
	//프래그먼트 세이더 읽어 저장하고 컴파일하기
	fragmentSource = filetobuf("fragment.glsl"); // 프래그세이더 읽어오기
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
		std::cerr << "ERROR: frag_shader \n" << errorLog << std::endl;
		return;
	}
}

//세이더 프로그램 만들고 세이더 객체 링크하기
GLuint make_shaderProgram()
{
	GLuint shaderID;
	shaderID = glCreateProgram(); //세이더 프로그램 만들기
	glAttachShader(shaderID, vertexShader); //세이더 프로그램에 버텍스 세이더 붙이기
	glAttachShader(shaderID, fragmentShader); //세이더 프로그램에 프래그먼트 세이더 붙이기
	glLinkProgram(shaderID); //세이더 프로그램 링크하기
	glDeleteShader(vertexShader); //세이더 객체를 세이더 프로그램에 링크했음으로 세이더 객체 자체는 삭제 가능
	glDeleteShader(fragmentShader);
	GLint result;
	GLchar errorLog[512];
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);// 세이더가 잘 연결되었는지 체크하기
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program  \n" << errorLog << std::endl;
		return false;
	}
	glUseProgram(shaderID); //만들어진 세이더 프로그램 사용하기
	//여러 개의 세이더프로그램 만들 수 있고,그 중 한개의 프로그램을 사용하려면
	//glUseProgram 함수를 호출하여 사용 할 특정 프로그램을 지정한다
	//사용하기 직전에 호출할 수 있다
	return shaderID;
}



#define _CRT_SECURE_NO_WARNINGS //--- 프로그램 맨 앞에 선언할 것
#define MAX_LINE_LENGTH 256

#define BOX_SIZE  2.0f


#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include <gl/freeglut_ext.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <random>
#include <vector>
#include <string.h>
#include <algorithm> 
#include <stack>     

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<float> rdcolor(0.0f, 1.0f);
std::uniform_real_distribution<float> rdmaxheight(BOX_SIZE*2, BOX_SIZE*3);
std::uniform_real_distribution<float> rdspeed(0.01f, 0.05f);

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid Keyboard(unsigned char key, int x, int y);
GLvoid Mouse(int button, int state, int x, int y);
GLvoid Timer(int value);
void InitBuffer();
void LoadOBJ(const char* filename, int object_num);
void reset_bool();

//--- 필요한 변수 선언
GLint width, height;
GLuint shaderProgramID; //--- 세이더 프로그램 이름
GLuint vertexShader; //--- 버텍스 세이더 객체
GLuint fragmentShader; //--- 프래그먼트 세이더 객체
GLuint vao, vbo;

//
bool depth_on = true;
bool proj_on = true;

float x_cam = 0.0f;
float y_cam = 0.0f;
float z_cam = 0.0f;
float x_at = 0.0f;
float y_at = 0.0f;
float z_at = 0.0f;
float x_up = 0.0f;
float y_up = 1.0f;
float z_up = 0.0f;
glm::vec3 cam_locate = glm::vec3(x_cam, y_cam, z_cam);
glm::vec3 cam_at = glm::vec3(x_at, y_at, z_at);
glm::vec3 cam_up = glm::vec3(x_up, y_up, z_up);

int maze_width = 0, maze_length = 0;
bool start_anime = true;
bool ism = false;
bool isv = false;
bool isr = false;

bool is_mouse_down = false;
int last_mouse_x = 0;      
int last_mouse_y = 0;      
float cam_radius = 0.0f;   // cam_at과 cam_locate 사이의 거리


//


struct OBB {
    glm::vec3 center = glm::vec3(0.0f);     // OBB의 중심 (월드 좌표계)
    glm::vec3 u[3];                         // OBB의 세 정규직교 축 (u[0]=x축, u[1]=y축, u[2]=z축)
    glm::vec3 half_length = glm::vec3(0.0f); // 각 축을 따른 중심으로부터의 반치수 (Local Space)
};

std::vector<GLfloat> allVertices;
struct SHAPE {
    std::vector<GLfloat> vertex;
    glm::mat4 model = glm::mat4(1.0f);
	glm::vec3 reset = glm::vec3(1.0f);

    int face_count;
    int object_num;

	bool r_increasing = true;
    bool g_increasing = true;
    bool b_increasing = true;

	float height = BOX_SIZE/2;
	float max_height = rdmaxheight(mt);
	float speed = rdspeed(mt);
	bool is_moving_up = true;
    
	bool draw = true;

    // OBB 관련 추가 항목
    OBB local_obb;  // 모델링 시점의 로컬 OBB (Model Matrix가 Identity일 때)
    OBB world_obb;  // 현재 프레임의 월드 OBB (Model Matrix 적용 후)
    bool is_colliding = false; // 충돌 상태
};
std::vector<SHAPE> shapes;

GLvoid MouseMove(int x, int y);
void update_world_obb(SHAPE& shape);
bool check_obb_collision(const SHAPE& shapeA, const SHAPE& shapeB);
bool is_separated(const OBB& a, const OBB& b, const glm::vec3& axis);
void init_maze();
void drawMiniMap(int w, int h);
void reset_c();
void update_camera();

//
int GRID_HEIGHT;
int GRID_WIDTH ;

enum CellType {
    PATH = 0,
    WALL = 1  
};

std::vector<std::vector<int>> maze;

void printMaze();
void printMaze() {
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            if (maze[y][x] == WALL) {
                std::cout << "■";
            }
            else {
                std::cout << " ";
            }
        }
        std::cout << std::endl;
    }
}

void generateMaze(int cx, int cy);
void generateMaze(int cx, int cy) {
    maze[cy][cx] = PATH;

    std::vector<std::pair<int, int>> directions = {
        {0, -2}, 
        {0, 2},  
        {-2, 0}, 
        {2, 0}   
    };

    std::shuffle(directions.begin(), directions.end(), mt);

    for (const auto& dir : directions) {
        int nx = cx + dir.first;
        int ny = cy + dir.second;
        
        if (nx > 0 && nx < GRID_WIDTH  && ny > 0 && ny < GRID_HEIGHT  && maze[ny][nx] == WALL) {

            if (ny == GRID_HEIGHT - 1) {
                ny--;
				maze[ny][nx] = PATH;
                continue;
            }
            if( nx == GRID_WIDTH - 1) {
                nx--;
				maze[ny][nx] = PATH;
                continue;
			}
            
            int wall_x = cx + dir.first / 2;
            int wall_y = cy + dir.second / 2;
            maze[wall_y][wall_x] = PATH;

            generateMaze(nx, ny);
        }
    }
}

char* filetobuf(const char* file)
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

void main(int argc, char** argv) //--- 윈도우 출력하고 콜백함수 설정
{
    do {
        std::cout << "길이(x방향) 너비(z방향) ex)20 15\n입력하시오: ";
        std::cin >> maze_width >> maze_length;
    } while (!(maze_width >= 5 && maze_width <= 35) || !(maze_length >= 5 && maze_length <= 35));
	std::cout << maze_width << " " << maze_length << std::endl;

    GRID_HEIGHT = maze_width;
    GRID_WIDTH = maze_length;

    std::vector<std::vector<int>> a(GRID_HEIGHT, std::vector<int>(GRID_WIDTH, WALL));
    maze = a;

    y_cam = BOX_SIZE * (maze_width + maze_width)/2+ 10.0f; // 카메라 초기 위치 설정
	z_cam = maze_width + maze_width; // 카메라 초기 위치 설정
	maze[0][1] = PATH;
    generateMaze(1, 1);
    for(int i = GRID_WIDTH -1; i > 0; i--) {
        if(maze[GRID_HEIGHT-2][i] == PATH) {
            maze[GRID_HEIGHT-1][i] = PATH;
            break;
        }
	}
    printMaze();

    width = 1200;
    height = 800;

    //--- 윈도우 생성하기
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(width, height);
    glutCreateWindow("Example1");

    //--- GLEW 초기화하기
    glewExperimental = GL_TRUE;
    glewInit();

    //--- 세이더 읽어와서 세이더 프로그램 만들기: 사용자 정의함수 호출
    make_vertexShaders(); //--- 버텍스 세이더 만들기
    make_fragmentShaders(); //--- 프래그먼트 세이더 만들기
    shaderProgramID = make_shaderProgram();	//--- 세이더 프로그램 만들기

    // 버퍼(VAO/VBO/EBO) 초기화 (셰이더 프로그램 생성 후 호출)
    InitBuffer();
    for(int i = 0; i<maze_length*maze_width; i++) LoadOBJ("cube.obj", i);
	init_maze();

    update_camera();
    cam_radius = glm::length(cam_locate - cam_at);
    if (cam_radius < 1.0f) cam_radius = 1.0f; // 최소 거리 설정

    glutDisplayFunc(drawScene); //--- 출력 콜백 함수
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutMotionFunc(MouseMove);
    glutTimerFunc(1000 / 60, Timer, 1); //--- 타이머 콜백함수 지정 (60 FPS)

    glutMainLoop();
}

void make_vertexShaders()
{
    GLchar* vertexSource;

    //--- 버텍스 세이더 읽어 저장하고 컴파일 하기
    //--- filetobuf: 사용자정의 함수로 텍스트를 읽어서 문자열에 저장하는 함수

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
        std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

//--- 프래그먼트 세이더 객체 만들기
void make_fragmentShaders()
{
    GLchar* fragmentSource;

    //--- 프래그먼트 세이더 읽어 저장하고 컴파일하기
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
        std::cerr << "ERROR: frag_shader 컴파일 실패\n" << errorLog << std::endl;
        return;
    }
}

GLuint make_shaderProgram()
{
    GLint result;
    GLchar* errorLog = NULL;
    GLuint shaderID;
    shaderID = glCreateProgram(); //--- 세이더 프로그램 만들기
    glAttachShader(shaderID, vertexShader); //--- 세이더 프로그램에 버텍스 세이더 붙이기
    glAttachShader(shaderID, fragmentShader); //--- 세이더 프로그램에 프래그먼트 세이더 붙이기
    glLinkProgram(shaderID); //--- 세이더 프로그램 링크하기
    glDeleteShader(vertexShader); //--- 세이더 객체를 세이더 프로그램에 링크했음으로, 세이더 객체 자체는 삭제 가능
    glDeleteShader(fragmentShader);
    glGetProgramiv(shaderID, GL_LINK_STATUS, &result); // ---세이더가 잘 연결되었는지 체크하기
    if (!result) {
        glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
        std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
        return false;
    }
    glUseProgram(shaderID); //--- 만들어진 세이더 프로그램 사용하기
    //--- 여러 개의 세이더프로그램 만들 수 있고, 그 중 한개의 프로그램을 사용하려면
    //--- glUseProgram 함수를 호출하여 사용 할 특정 프로그램을 지정한다.
    //--- 사용하기 직전에 호출할 수 있다.
    return shaderID;
}


void UpdateBuffer()
{
    allVertices.clear();

    for (int i = 0; i < shapes.size(); i++)	allVertices.insert(allVertices.end(), shapes[i].vertex.begin(), shapes[i].vertex.end());

    // 합쳐진 데이터로 VBO 업데이트
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(GLfloat), allVertices.data(), GL_DYNAMIC_DRAW);
}

GLvoid drawScene() {
    if (depth_on)
        glEnable(GL_DEPTH_TEST); // 은면제거
    else
        glDisable(GL_DEPTH_TEST);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glm::mat4 view = glm::lookAt(cam_locate, cam_at, cam_up);

    glm::mat4 projection;
    if (proj_on) {
        projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
    }
    else {
        float aspect = (float)width / (float)height;
        float ortho_size = 30.0f;
        projection = glm::ortho(-ortho_size * aspect, ortho_size * aspect, -ortho_size, ortho_size, 0.1f, 100.0f);
    }

    GLuint modelLoc = glGetUniformLocation(shaderProgramID, "uModel");
    GLuint viewLoc = glGetUniformLocation(shaderProgramID, "uView");
    GLuint projLoc = glGetUniformLocation(shaderProgramID, "uProj");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

    if (viewLoc == -1 || projLoc == -1) {
        std::cerr << "ERROR: View or Proj uniform location not found!" << std::endl;
    }

    glBindVertexArray(vao);
    GLint first = 0;
    for (int i = 0; i < shapes.size(); i++) {
        glm::mat4 model = shapes[i].model;

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

        if (modelLoc == -1) {
            std::cerr << "ERROR: Model uniform location not found!" << std::endl;
        }

        int vertexCount = shapes[i].vertex.size() / 6;
        if( isr && shapes[i].draw || !isr)glDrawArrays(GL_TRIANGLES, first, vertexCount);
        first += vertexCount;
    }
    glBindVertexArray(0);

    drawMiniMap(width, height);

    glutSwapBuffers();
}

GLvoid Reshape(int w, int h) //--- 콜백 함수: 다시 그리기 콜백 함수
{
    glViewport(0, 0, w, h);
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 'o':
        proj_on = false;
        break;
	case 'p':
        proj_on = true;
		break;
    case 'z':
        z_cam += 0.5f;
		z_at += 0.5f;
        break;
    case 'Z':
        z_cam -= 0.5f;
		z_at -= 0.5f;
        break;
    case 'w':
		y_at += 0.1f;
		break;
	case 'a':
		x_at -= 0.1f;
        break;
    case 's':
		y_at -= 0.1f;
		break;
	case 'd':
        x_at += 0.1f;
		break;
    case 'y': {
        glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        
        // 1. cam_locate 공전 계산
        glm::vec3 relative_pos = cam_locate - cam_at;
        glm::vec4 rotated_relative_pos = rotation_mat * glm::vec4(relative_pos, 1.0f);
        glm::vec3 new_cam_locate = cam_at + glm::vec3(rotated_relative_pos);
        
        // 2. cam_up 벡터를 월드 Y축으로 재설정 (핵심 수정)
        glm::vec3 new_cam_up = glm::vec3(0.0f, 1.0f, 0.0f); 

        // 3. 전역 변수에 결과 반영
        x_cam = new_cam_locate.x;
        y_cam = new_cam_locate.y;
        z_cam = new_cam_locate.z;
        
        x_up = new_cam_up.x;
        y_up = new_cam_up.y;
        z_up = new_cam_up.z;

        cam_radius = glm::length(cam_locate - cam_at);
        break;
    }
    case 'Y': {
        glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::radians(-15.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // 1. cam_locate 공전 계산
        glm::vec3 relative_pos = cam_locate - cam_at;
        glm::vec4 rotated_relative_pos = rotation_mat * glm::vec4(relative_pos, 1.0f);
        glm::vec3 new_cam_locate = cam_at + glm::vec3(rotated_relative_pos);

        // 2. cam_up 벡터를 월드 Y축으로 재설정 (핵심 수정)
        glm::vec3 new_cam_up = glm::vec3(0.0f, 1.0f, 0.0f);

        // 3. 전역 변수에 결과 반영
        x_cam = new_cam_locate.x;
        y_cam = new_cam_locate.y;
        z_cam = new_cam_locate.z;

        x_up = new_cam_up.x;
        y_up = new_cam_up.y;
        z_up = new_cam_up.z;

        cam_radius = glm::length(cam_locate - cam_at);
        break;
    }
	case 'r':
		isr = !isr;
        break;
    case 'm':
        if(!start_anime) ism = true;
        break;
    case 'M':
        if (!start_anime) ism = false;
		break;
    case 'v':
		isv = !isv;
        break;
    case '=':
        for (int i = 0; i < shapes.size();i++) {
            if(shapes[i].speed < 0.1f) shapes[i].speed += 0.01f;
        }
        break;
    case '-':
        for (int i = 0; i < shapes.size();i++) {
            if (shapes[i].speed > 0.01f) shapes[i].speed -= 0.01f;
        }
        break;
    case'c':
        reset_c();
        break;
    case 'h':
        depth_on = !depth_on;
        break;
    case 'q':
        exit(0);
        break;
    }
    update_camera();
    glutPostRedisplay(); // 다시 그리기 요청
}

GLvoid Timer(int value) //--- 콜백 함수: 타이머 콜백 함수
{
    for (int i = 0; i < shapes.size(); i++) {

        for (int j = 0; j < shapes[i].vertex.size(); j++) {
            if (j % 6 == 3) {
                if (shapes[i].vertex[j] >= 1.0f) shapes[i].r_increasing = false;
				else if (shapes[i].vertex[j] <= 0.0f) shapes[i].r_increasing = true;
                if(shapes[i].r_increasing) shapes[i].vertex[j] += 1.0f / (float)(maze_width * maze_length);
                else shapes[i].vertex[j] -= 1.0f / (float)(maze_width * maze_length);
            }
            if (j % 6 == 4) {
                if (shapes[i].vertex[j] <= 0.0f) shapes[i].g_increasing = true;
                else if(shapes[i].vertex[j] >= 1.0f) shapes[i].g_increasing = false;
                if (shapes[i].g_increasing) shapes[i].vertex[j] += 1.0f / (float)(maze_width * maze_length);
                else shapes[i].vertex[j] -= 1.0f / (float)(maze_width * maze_length);
            }
            if (j % 6 == 5) {
                if (shapes[i].vertex[j] <= 0.0f) shapes[i].b_increasing = true;
                else if (shapes[i].vertex[j] >= 1.0f) shapes[i].b_increasing = false;
                if (shapes[i].b_increasing) shapes[i].vertex[j] += 1.0f / (float)(maze_width * maze_length);
                else shapes[i].vertex[j] -= 1.0f / (float)(maze_width * maze_length);
            }
        }

        if (start_anime) {
            if (shapes[i].height >= shapes[i].max_height) {
                shapes[i].is_moving_up = false;
				start_anime = false;
            }
            else shapes[i].height += shapes[i].speed;
        }
        if (isv) {
            shapes[i].is_moving_up = false;
            if (shapes[i].height <= BOX_SIZE/2) shapes[i].is_moving_up = true;
			else shapes[i].height -= shapes[i].speed;
        }
        else {
            if (ism) {
                if (shapes[i].is_moving_up && (shapes[i].height >= shapes[i].max_height)) shapes[i].is_moving_up = false;
                else if (!shapes[i].is_moving_up && (shapes[i].height <= BOX_SIZE / 2)) shapes[i].is_moving_up = true;
                if (shapes[i].is_moving_up) shapes[i].height += shapes[i].speed;
                else shapes[i].height -= shapes[i].speed;
            }
        }


		shapes[i].model = glm::mat4(1.0f);
		shapes[i].model = glm::translate(shapes[i].model, shapes[i].reset);
        shapes[i].model = glm::translate(shapes[i].model, glm::vec3(0.0f, shapes[i].height, 0.0f));
		shapes[i].model = glm::scale(shapes[i].model, glm::vec3(1.0f, shapes[i].height, 1.0f));
        update_world_obb(shapes[i]);
    }
    UpdateBuffer();
    glutPostRedisplay(); // 다시 그리기 요청
    glutTimerFunc(1000 / 60, Timer, 1); //--- 타이머 콜백함수 지정 (60 FPS)
}

GLvoid Mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            is_mouse_down = true;
            last_mouse_x = x;
            last_mouse_y = y;

            // cam_radius가 초기화되지 않았을 경우 다시 계산
            if (cam_radius < 1e-6) {
                cam_radius = glm::length(cam_locate - cam_at);
                if (cam_radius < 1.0f) cam_radius = 1.0f;
            }
        }
        else {
            is_mouse_down = false;
        }
    }
}

GLvoid MouseMove(int x, int y) {
    if (is_mouse_down) {
        float dx = (float)(x - last_mouse_x);
        float dy = (float)(y - last_mouse_y);

        // 회전 속도 조절 (Sensitivity)
        float sensitivity = 0.005f;

        // 1. 카메라의 현재 벡터 (시선 방향) 계산
        glm::vec3 view_dir = glm::normalize(cam_locate - cam_at);

        // 2. Yaw (수평 회전) 적용 (월드 Y축 기준)
        glm::mat4 yaw_rot = glm::rotate(glm::mat4(1.0f), -dx * sensitivity, glm::vec3(0.0f, 1.0f, 0.0f));
        view_dir = glm::vec3(yaw_rot * glm::vec4(view_dir, 0.0f));

        // 3. Pitch (수직 회전) 적용 (카메라의 오른쪽 벡터 기준)
        // 오른쪽 벡터: 시선 벡터와 업 벡터의 외적
        glm::vec3 right_vector = glm::normalize(glm::cross(view_dir, cam_up));
        glm::mat4 pitch_rot = glm::rotate(glm::mat4(1.0f), dy * sensitivity, right_vector);
        view_dir = glm::vec3(pitch_rot * glm::vec4(view_dir, 0.0f));

        // 4. 새로운 cam_at 위치 계산
        // cam_at = cam_locate - (회전된 시선 벡터 * 반지름)
        glm::vec3 new_cam_at = cam_locate - view_dir * cam_radius;

        x_at = new_cam_at.x;
        y_at = new_cam_at.y;
        z_at = new_cam_at.z;

        last_mouse_x = x;
        last_mouse_y = y;

        update_camera();
        glutPostRedisplay();
    }
}

void InitBuffer()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // 초기에는 빈 버퍼
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    // --- 위치 속성 (location = 0, vec3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // --- 색상 속성 (location = 1, vec3)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


struct Vertex {
    float x, y, z;
};

struct Face {
    unsigned int v1, v2, v3;
};

struct Model {
    Vertex* vertices;
    size_t vertex_count;
    Face* faces;
    size_t face_count;
};
std::vector<Model> models;
Model read_obj_file(const char* filename);

void read_newline(char* str) {
    char* pos;
    if ((pos = strchr(str, '\n')) != NULL)
        *pos = '\0';
}

Model read_obj_file(const char* filename) {
    Model model;
    FILE* file;
    fopen_s(&file, filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    char line[MAX_LINE_LENGTH];
    model.vertex_count = 0;
    model.face_count = 0;

    while (fgets(line, sizeof(line), file)) {
        read_newline(line);
        if (line[0] == 'v' && line[1] == ' ')
            model.vertex_count++;
        else if (line[0] == 'f' && line[1] == ' ')
            model.face_count++;
    }

    fseek(file, 0, SEEK_SET);
    model.vertices = (Vertex*)malloc(model.vertex_count * sizeof(Vertex));
    model.faces = (Face*)malloc(model.face_count * sizeof(Face));
    size_t vertex_index = 0; size_t face_index = 0;
    while (fgets(line, sizeof(line), file)) {
        read_newline(line);
        if (line[0] == 'v' && line[1] == ' ') {
            int result = sscanf_s(line + 2, "%f %f %f",
                &model.vertices[vertex_index].x,
                &model.vertices[vertex_index].y,
                &model.vertices[vertex_index].z);
            vertex_index++;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            unsigned int v1, v2, v3;
            int result = sscanf_s(line + 2, "%u %u %u", &v1, &v2, &v3);
            model.faces[face_index].v1 = v1 - 1; // OBJ indices start at 1
            model.faces[face_index].v2 = v2 - 1;
            model.faces[face_index].v3 = v3 - 1;
            face_index++;
        }
    }
    fclose(file);
    return model;
}

void LoadOBJ(const char* filename, int object_num)
{
    models.clear();
    models.push_back(read_obj_file(filename));

    // --- OBB 계산을 위한 로컬 좌표계의 최소/최대값 초기화
    glm::vec3 min_v = glm::vec3(FLT_MAX);
    glm::vec3 max_v = glm::vec3(-FLT_MAX);

    static float r=0.0f, g=1.0f, b=1.0f;
    r += 1.0f / (float)(maze_width * maze_length);
    g -= 1.0f / (float)(maze_width * maze_length);
    b -= 1.0f / (float)(maze_width * maze_length);

    // 큐브, 구체 등 object_num이 1 이상인 도형 (단일 객체 로딩)

    SHAPE object_shape;
    object_shape.object_num = object_num;

    // --- AABB 계산 및 정점 데이터 합치기 ---
    for (auto& m : models) {
        for (size_t i = 0; i < m.face_count; i++) {
            Face f = m.faces[i];
            Vertex v1 = m.vertices[f.v1];
            Vertex v2 = m.vertices[f.v2];
            Vertex v3 = m.vertices[f.v3];

            // AABB 계산 (모든 정점을 대상으로)
            for (const Vertex* v : { &v1, &v2, &v3 }) {
                min_v.x = glm::min(min_v.x, v->x);
                min_v.y = glm::min(min_v.y, v->y);
                min_v.z = glm::min(min_v.z, v->z);
                max_v.x = glm::max(max_v.x, v->x);
                max_v.y = glm::max(max_v.y, v->y);
                max_v.z = glm::max(max_v.z, v->z);
            }

            // 색상 설정 (object_num 기준)

            object_shape.vertex.insert(object_shape.vertex.end(), {
                v1.x, v1.y, v1.z, r, g, b,
                v2.x, v2.y, v2.z, r, g, b,
                v3.x, v3.y, v3.z, r, g, b
                });
            object_shape.face_count++;
        }
    }

    // 2. **[OBB 설정]**
    glm::vec3 local_center = (min_v + max_v) * 0.5f;
    glm::vec3 local_half_length = (max_v - min_v) * 0.5f;

    object_shape.local_obb.center = local_center;
    object_shape.local_obb.half_length = local_half_length; // OBJ 파일의 로컬 Half-Length (큐브/구체는 약 1.0)
    object_shape.local_obb.u[0] = glm::vec3(1.0f, 0.0f, 0.0f);
    object_shape.local_obb.u[1] = glm::vec3(0.0f, 1.0f, 0.0f);
    object_shape.local_obb.u[2] = glm::vec3(0.0f, 0.0f, 1.0f);

    shapes.push_back(object_shape);

    UpdateBuffer();
}

void update_world_obb(SHAPE& shape) {
    // 모델 행렬의 회전/스케일 부분 (3x3) 추출
    glm::mat3 rotation_scale_mat = glm::mat3(shape.model);

    // 1. 월드 중심 변환
    glm::vec4 local_center_h = glm::vec4(shape.local_obb.center, 1.0f);
    shape.world_obb.center = glm::vec3(shape.model * local_center_h);

    // 2. 월드 축 변환
    for (int i = 0; i < 3; i++) {
        shape.world_obb.u[i] = glm::normalize(rotation_scale_mat * shape.local_obb.u[i]);
    }

    // 3. **[핵심 수정]** 반치수 설정: 스케일 벡터를 추출하여 local_obb에 적용
    // (모델 행렬의 각 축 길이가 스케일 팩터를 포함함)
    glm::vec3 scale_factors = glm::vec3(
        glm::length(rotation_scale_mat[0]), // X축 스케일
        glm::length(rotation_scale_mat[1]), // Y축 스케일
        glm::length(rotation_scale_mat[2])  // Z축 스케일
    );

    // 로컬 반치수에 스케일 팩터를 곱하여 월드 반치수 설정
    // (주의: 여기서는 비균일 스케일을 처리하기 위해 각 축의 스케일을 개별적으로 곱합니다.)
    shape.world_obb.half_length = shape.local_obb.half_length * scale_factors;
}

bool is_separated(const OBB& a, const OBB& b, const glm::vec3& axis) {
    // 축의 길이가 너무 작으면 무시 (부동소수점 오차 처리)
    if (glm::length(axis) < 1e-6) return false;

    // 두 OBB 중심 사이의 벡터
    glm::vec3 T = b.center - a.center;

    // 1. 중심 간 거리 투영
    float distance_proj = glm::abs(glm::dot(T, axis));

    // 2. OBB A의 '반지름' 투영 (각 로컬 축의 투영 길이의 합)
    float radius_a =
        glm::abs(glm::dot(a.half_length.x * a.u[0], axis)) +
        glm::abs(glm::dot(a.half_length.y * a.u[1], axis)) +
        glm::abs(glm::dot(a.half_length.z * a.u[2], axis));

    // 3. OBB B의 '반지름' 투영
    float radius_b =
        glm::abs(glm::dot(b.half_length.x * b.u[0], axis)) +
        glm::abs(glm::dot(b.half_length.y * b.u[1], axis)) +
        glm::abs(glm::dot(b.half_length.z * b.u[2], axis));

    // 중심 간 거리가 두 반지름의 합보다 크면 분리 (충돌 아님)
    return distance_proj > (radius_a + radius_b);
}

bool check_obb_collision(const SHAPE& shapeA, const SHAPE& shapeB) {
    const OBB& a = shapeA.world_obb;
    const OBB& b = shapeB.world_obb;

    // 1. OBB A의 축 3개 검사
    for (int i = 0; i < 3; i++) {
        if (is_separated(a, b, a.u[i])) return false;
    }

    // 2. OBB B의 축 3개 검사
    for (int i = 0; i < 3; i++) {
        if (is_separated(a, b, b.u[i])) return false;
    }

    // 3. 교차 축 (A_i x B_j) 9개 검사
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec3 cross_axis = glm::cross(a.u[i], b.u[j]);
            if (is_separated(a, b, cross_axis)) return false;
        }
    }

    // 15개 축 모두에서 분리되지 않았다면 충돌!
    return true;
}

void init_maze() {
    int cube_idx = 0;

    for (int i = 0; i < GRID_HEIGHT; i++) {   
        for (int j = 0; j < GRID_WIDTH; j++) {

            if (maze[i][j] == WALL) {
                if (cube_idx >= shapes.size()) {
                    std::cerr << "ERROR: Too many walls for pre-loaded shapes!" << std::endl;
                    return;
                }

                float x_pos = BOX_SIZE / 2 + (BOX_SIZE * j) - ((BOX_SIZE * (float)GRID_WIDTH) / 2);
                float z_pos = BOX_SIZE / 2 + (BOX_SIZE * i) - ((BOX_SIZE * (float)GRID_HEIGHT) / 2); // Z축 좌표계에 맞게 수정

                shapes[cube_idx].reset = glm::vec3(x_pos, 0.0f, z_pos);
                shapes[cube_idx].draw = true;
            }
            else if (maze[i][j] == PATH) {
                shapes[cube_idx].draw = false;
                float x_pos = BOX_SIZE / 2 + (BOX_SIZE * j) - ((BOX_SIZE * (float)GRID_WIDTH) / 2);
                float z_pos = BOX_SIZE / 2 + (BOX_SIZE * i) - ((BOX_SIZE * (float)GRID_HEIGHT) / 2); // Z축 좌표계에 맞게 수정

                shapes[cube_idx].reset = glm::vec3(x_pos, 0.0f, z_pos);
            }
            cube_idx++;
        }
    }

    // (중요) 사용되지 않은 큐브(shape) 제거
    // 만약 모든 shape을 LoadOBJ로 생성했다면, 벽이 아닌 곳의 shape은 숨기거나 제거해야 합니다.
    // 가장 쉬운 방법은 '벽 개수'만큼만 큐브를 LoadOBJ하는 것입니다.
    // 또는, 'reset' 위치를 아주 멀리 두거나, 렌더링 목록에서 제외합니다.

    // 예: 사용한 큐브(벽) 수만큼만 남기고 나머지를 제거
    // shapes.resize(cube_idx); 
    // UpdateBuffer(); // 버퍼도 업데이트 필요
}

void drawMiniMap(int w, int h)
{
    glViewport(w * 3 / 4, h * 3 / 4, w / 4, h / 4);

    GLuint modelLoc = glGetUniformLocation(shaderProgramID, "uModel");
    GLuint viewLoc = glGetUniformLocation(shaderProgramID, "uView");
    GLuint projLoc = glGetUniformLocation(shaderProgramID, "uProj");

    float map_width = BOX_SIZE * maze_length;
    float map_height = BOX_SIZE * maze_width;

    float maxrange = std::max(map_width, map_height) / 2.0f + BOX_SIZE * 2.0f; // 약간의 여백 추가

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, maxrange * 2.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),           
        glm::vec3(0.0f, 0.0f, -1.0f)           
    );

    glm::mat4 proj = glm::ortho(-maxrange, maxrange, -maxrange, maxrange, 0.1f, maxrange * 4.0f);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(vao);
    GLint first = 0;
    for (size_t i = 0; i < shapes.size(); i++) {
        glm::mat4 model = shapes[i].model;

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

        int vertexCount = shapes[i].vertex.size() / 6;
        if (isr && shapes[i].draw || !isr) glDrawArrays(GL_TRIANGLES, first, vertexCount);
        first += vertexCount;
    }
    glBindVertexArray(0);

    glViewport(0, 0, w, h);
}

void reset_c() {
	x_cam = 0.0f;
    y_cam = BOX_SIZE * (maze_width + maze_width) / 2 + 10.0f; // 카메라 초기 위치 설정
    z_cam = maze_width * 2; // 카메라 초기 위치 설정

    x_at = 0.0f;
    y_at = 0.0f;
    z_at = 0.0f;

    x_up = 0.0f;
    y_up = 1.0f;
    z_up = 0.0f;
     
    update_camera();
    cam_radius = glm::length(cam_locate - cam_at);
    if (cam_radius < 1.0f) cam_radius = 1.0f;

    float r = 0.0f, g = 1.0f, b = 1.0f;
    for(int i = 0; i < shapes.size(); i++) {
        shapes[i].height = BOX_SIZE / 2;
        shapes[i].is_moving_up = true;

        r += 1.0f / (float)(maze_width * maze_length);
        g -= 1.0f / (float)(maze_width * maze_length);
        b -= 1.0f / (float)(maze_width * maze_length);
        for (int j = 0; j < shapes[i].vertex.size(); j++) {
            if (j % 6 == 3) shapes[i].vertex[j] = r;
            if (j % 6 == 4) shapes[i].vertex[j] = g;
            if (j % 6 == 5) shapes[i].vertex[j] = b;
        }
	}
    UpdateBuffer();

    depth_on = true;
    proj_on = true;
	start_anime = true;
    ism = false;
    isv = false;
}

void update_camera() {
    cam_locate = glm::vec3(x_cam, y_cam, z_cam);
    cam_at = glm::vec3(x_at, y_at, z_at);
	cam_up = glm::vec3(x_up, y_up, z_up);
}
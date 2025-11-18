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
std::uniform_int_distribution<int> rdmaze(0, 1);

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
GLvoid Keyboard(unsigned char key, int x, int y);
GLvoid SpecialKey(int key, int x, int y);
GLvoid SpecialUpKey(int key, int x, int y);
GLvoid Mouse(int button, int state, int x, int y);
GLvoid MouseMove(int x, int y);
GLvoid Timer(int value);
void InitBuffer();
void LoadOBJ(const char* filename, int object_num);
void reset_bool();

//--- 필요한 변수 선언
GLint width = 1200, height = 800;
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
bool iss = false;
bool is1 = false;

bool is_mouse_down = false;
int last_mouse_x = 0;      
int last_mouse_y = 0;      
float cam_radius = 0.0f;   // cam_at과 cam_locate 사이의 거리

int player_object_num = -1;
float p_x, p_y, p_z;
int p_x_move = 0; // -1: left, 1: right
int p_z_move = 0; // -1: down, 1: up

int ground_object_num = -1;
int goal_cnt = 0;
//


struct OBB {
    glm::vec3 center = glm::vec3(0.0f);     
    glm::vec3 u[3];                         // 세 정규직교 축 (u[0]=x축, u[1]=y축, u[2]=z축)
    glm::vec3 half_length = glm::vec3(0.0f); 
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

    // OBB
    OBB local_obb;  // 로컬 OBB
    OBB world_obb;  // 월드 OBB 
    bool is_colliding = false;
};
std::vector<SHAPE> shapes;

void update_world_obb(SHAPE& shape);
bool check_obb_collision(const SHAPE& shapeA, const SHAPE& shapeB);
bool is_separated(const OBB& a, const OBB& b, const glm::vec3& axis);
void drawMiniMap(int w, int h);
void reset_c();
void update_camera();
void init_maze();
void setMaze();
void retouchMaze();
void generateMaze(int cx, int cy);
void printMaze();

enum CellType {
    PATH = 0,
    WALL = 1  
};

std::vector<std::vector<int>> maze;

void printMaze() {
    for (int y = 0; y < maze_width; ++y) {
        for (int x = 0; x < maze_length; ++x) {
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
        
        if (nx > 0 && nx < maze_length-1  && ny > 0 && ny < maze_width-1  && maze[ny][nx] == WALL) {
            
            int wall_x = cx + dir.first / 2;
            int wall_y = cy + dir.second / 2;
            maze[wall_y][wall_x] = PATH;

            generateMaze(nx, ny);
        }
    }
}

void retouchMaze() {

    if (maze_width % 2 == 0) {  // ㅣ
        for (int i = 1; i < maze_length - 1; i++) {
            if (rdmaze(mt) == 1) maze[i][maze_width - 2] = PATH;
            else maze[i][maze_width - 2] = WALL;
        }
    }
    if (maze_length % 2 == 0) { // ㅡ
        for (int i = 1; i < maze_width - 1; i++) {
            if (rdmaze(mt) == 1) maze[maze_length - 2][i] = PATH;
            else maze[maze_length - 2][i] = WALL;
        }
    }

	if (maze_width % 2 == 0) { // ㅣ
        for (int i = 1; i < maze_length - 1; i++) {
            if (maze[i][maze_width - 2] == PATH && maze[i-1][maze_width - 2] == WALL && maze[i + 1][maze_width - 2] == WALL && maze[i][maze_width - 3] == WALL) {
                if(i == 1) maze[i + 1][maze_width - 2] = PATH;
                else maze[i - 1][maze_width - 2] = PATH;
			}
        }
    }
	if (maze_length % 2 == 0) { // ㅡ
        for (int i = 1; i < maze_width - 1; i++) {
            if (maze[maze_length - 2][i] == PATH && maze[maze_length - 2][i - 1] == WALL && maze[maze_length - 2][i + 1] == WALL && maze[maze_length - 3][i] == WALL) {
                if (i == 1) maze[maze_length - 2][i + 1] = PATH;
                else maze[maze_length - 2][i - 1] = PATH;
            }
        }
    }

    for (int i = maze_width - 1; i > 0; i--) {  // 출구
        if (maze[maze_length - 2][i] == PATH) {
            maze[maze_length - 1][i] = PATH;
            break;
        }
		goal_cnt++;
    }
}

void setMaze() {
    do {
        std::cout << "길이(z방향) 너비(x방향) ex)20 15\n입력하시오: ";
        std::cin >> maze_length >> maze_width;
    } while (!(maze_width >= 5 && maze_width <= 35) || !(maze_length >= 5 && maze_length <= 35));
    std::cout << maze_width << " " << maze_length << std::endl;

    std::vector<std::vector<int>> a(maze_width, std::vector<int>(maze_length, WALL));
    maze = a;

    y_cam = BOX_SIZE * (maze_width + maze_width) / 2 + 10.0f; // 카메라 초기 위치 설정
    z_cam = maze_width + maze_width; // 카메라 초기 위치 설정
    maze[0][1] = PATH;
    generateMaze(1, 1);
    retouchMaze();
    printMaze();
}

void init_maze() {
    int cube_idx = 0;

    for (int i = 0; i < maze_length; i++) {
        for (int j = 0; j < maze_width; j++) {

            float x_pos = BOX_SIZE / 2 + (BOX_SIZE * j) - ((BOX_SIZE * (float)maze_width) / 2);
            float z_pos = BOX_SIZE / 2 + (BOX_SIZE * i) - ((BOX_SIZE * (float)maze_length) / 2);
            shapes[cube_idx].reset = glm::vec3(x_pos, 0.0f, z_pos);
            if (cube_idx == 1) {
                p_x = x_pos;
                p_y = BOX_SIZE / 2;
				p_z = z_pos;
			}

            if (maze[i][j] == WALL) shapes[cube_idx].draw = true;
            else if (maze[i][j] == PATH) shapes[cube_idx].draw = false;
            cube_idx++;
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
    setMaze();

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

	player_object_num = maze_length * maze_width;
    LoadOBJ("cube.obj", player_object_num);
    shapes[player_object_num].draw = false;
    
	ground_object_num = player_object_num + 1;
    LoadOBJ("cube.obj", ground_object_num);   //전체 바닥
    LoadOBJ("cube.obj", ground_object_num+1);   //시작 바닥
	shapes[ground_object_num + 1].reset = shapes[1].reset;
    LoadOBJ("cube.obj", ground_object_num+2);   //도착 바닥
    shapes[ground_object_num + 2].reset = shapes[(maze_length * maze_width) - goal_cnt - 1].reset;

    update_camera();
    cam_radius = glm::length(cam_locate - cam_at);
    if (cam_radius < 1.0f) cam_radius = 1.0f;

    glutDisplayFunc(drawScene);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutSpecialFunc(SpecialKey);
    glutSpecialUpFunc(SpecialUpKey);
    glutMouseFunc(Mouse);
    glutMotionFunc(MouseMove);
    glutTimerFunc(1000 / 60, Timer, 1); 

    glutMainLoop();
}

void make_vertexShaders()
{
    GLchar* vertexSource;

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

void make_fragmentShaders()
{
    GLchar* fragmentSource;

    fragmentSource = filetobuf("fragment.glsl");
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
    shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);
    glLinkProgram(shaderID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
    if (!result) {
        glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
        std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
        return false;
    }
    glUseProgram(shaderID);
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

    glBindVertexArray(vao);
    GLint first = 0;
    for (int i = 0; i < shapes.size(); i++) {
        glm::mat4 model = shapes[i].model;

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

        int vertexCount = shapes[i].vertex.size() / 6;
        if (i == player_object_num) {
            if(shapes[i].draw) glDrawArrays(GL_TRIANGLES, first, vertexCount);
        }
        else if (i == ground_object_num || i == ground_object_num + 1 || i == ground_object_num + 2) {
            if (isr) glDrawArrays(GL_TRIANGLES, first, vertexCount);
        }
        else {
            if ((isr && shapes[i].draw) || !isr)glDrawArrays(GL_TRIANGLES, first, vertexCount);
        }
        first += vertexCount;
    }
    glBindVertexArray(0);

    drawMiniMap(width, height);

    glutSwapBuffers();
}

void drawMiniMap(int w, int h)
{
    glViewport(w * 3 / 4, h * 3 / 4, w / 4, h / 4);

    GLuint modelLoc = glGetUniformLocation(shaderProgramID, "uModel");
    GLuint viewLoc = glGetUniformLocation(shaderProgramID, "uView");
    GLuint projLoc = glGetUniformLocation(shaderProgramID, "uProj");

    float map_width = BOX_SIZE * maze_length;
    float map_height = BOX_SIZE * maze_width;
    float maxrange = std::max(map_width, map_height) / 2.0f + BOX_SIZE * 2.0f;

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
        if (i == player_object_num) {
            if (isr && iss) glDrawArrays(GL_TRIANGLES, first, vertexCount);
        }
        else if(i == ground_object_num || i == ground_object_num +1 || i == ground_object_num +2){
            if(isr) glDrawArrays(GL_TRIANGLES, first, vertexCount);
		}
        else {
            if (isr && shapes[i].draw || !isr) glDrawArrays(GL_TRIANGLES, first, vertexCount);
        }
        first += vertexCount;
    }
    glBindVertexArray(0);

    glViewport(0, 0, w, h);
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
        if (proj_on) {
            z_cam += 0.5f;
            z_at += 0.5f;
        }
        break;
    case 'Z':
        if (proj_on) {
            z_cam -= 0.5f;
            z_at -= 0.5f;
        }
        break;
    case 'x':
        if (proj_on) {
            x_cam += 0.5f;
            x_at += 0.5f;
        }
        break;
    case 'X':
        if (proj_on) {
            x_cam -= 0.5f;
            x_at -= 0.5f;
        }
        break;
    case 'y': {
        glm::mat4 rotation_mat = glm::rotate(glm::mat4(1.0f), glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        
        glm::vec3 relative_pos = cam_locate - cam_at;
        glm::vec4 rotated_relative_pos = rotation_mat * glm::vec4(relative_pos, 1.0f);
        glm::vec3 new_cam_locate = cam_at + glm::vec3(rotated_relative_pos);
        
        glm::vec3 new_cam_up = glm::vec3(0.0f, 1.0f, 0.0f); 

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

        glm::vec3 relative_pos = cam_locate - cam_at;
        glm::vec4 rotated_relative_pos = rotation_mat * glm::vec4(relative_pos, 1.0f);
        glm::vec3 new_cam_locate = cam_at + glm::vec3(rotated_relative_pos);

        glm::vec3 new_cam_up = glm::vec3(0.0f, 1.0f, 0.0f);

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
		isr = true;
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
    case 's':
        if (isr) {
            iss = true;
            shapes[player_object_num].draw = true;
        }
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
    case '1':
        if (iss) {
            is1 = true;
            shapes[player_object_num].draw = false;
            cam_radius = 100.0f;
        }
        break;
    case '3':
        if (is1) {
            is1 = false;

            x_cam = 0.0f;
            y_cam = BOX_SIZE * (maze_width + maze_width) / 2 + 10.0f;
            z_cam = maze_width * 2;

            x_at = 0.0f;
            y_at = 0.0f;
            z_at = 0.0f;

            x_up = 0.0f;
            y_up = 1.0f;
            z_up = 0.0f;

            update_camera();
            cam_radius = glm::length(cam_locate - cam_at);
            if (cam_radius < 1.0f) cam_radius = 1.0f;
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

GLvoid SpecialKey(int key, int x, int y) {
    switch (key) {
    case GLUT_KEY_UP:
		p_z_move = 1;
        break;
    case GLUT_KEY_DOWN:
        p_z_move = -1;
        break;
    case GLUT_KEY_LEFT:
        p_x_move = -1;
        break;
    case GLUT_KEY_RIGHT:
        p_x_move = 1;
        break;
    }
}

GLvoid SpecialUpKey(int key, int x, int y){ // 키 업
    switch (key) {
    case GLUT_KEY_UP:
		if (p_z_move == 1) p_z_move = 0;
        break;
    case GLUT_KEY_DOWN:
		if (p_z_move == -1) p_z_move = 0;
        break;
    case GLUT_KEY_LEFT:
		if (p_x_move == -1) p_x_move = 0;
        break;
    case GLUT_KEY_RIGHT:
		if (p_x_move == 1) p_x_move = 0;
        break;
    }
}

GLvoid Timer(int value) //--- 콜백 함수: 타이머 콜백 함수
{
    for (int i = 0; i < shapes.size(); i++) {
        if (i < player_object_num) {

            for (int j = 0; j < shapes[i].vertex.size(); j++) {
                if (j % 6 == 3) {
                    if (shapes[i].vertex[j] >= 1.0f) shapes[i].r_increasing = false;
                    else if (shapes[i].vertex[j] <= 0.0f) shapes[i].r_increasing = true;
                    if (shapes[i].r_increasing) shapes[i].vertex[j] += 1.0f / (float)(maze_width * maze_length);
                    else shapes[i].vertex[j] -= 1.0f / (float)(maze_width * maze_length);
                }
                if (j % 6 == 4) {
                    if (shapes[i].vertex[j] <= 0.0f) shapes[i].g_increasing = true;
                    else if (shapes[i].vertex[j] >= 1.0f) shapes[i].g_increasing = false;
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
                if (shapes[i].height <= 0.1f) shapes[i].is_moving_up = true;
                else shapes[i].height -= shapes[i].speed;
            }
            else {
                if (ism) {
                    if (shapes[i].is_moving_up && (shapes[i].height >= shapes[i].max_height)) shapes[i].is_moving_up = false;
                    else if (!shapes[i].is_moving_up && (shapes[i].height <= BOX_SIZE / 3)) shapes[i].is_moving_up = true;
                    if (shapes[i].is_moving_up) shapes[i].height += shapes[i].speed;
                    else shapes[i].height -= shapes[i].speed;
                }
            }

            shapes[i].model = glm::mat4(1.0f);
            shapes[i].model = glm::translate(shapes[i].model, shapes[i].reset);
            shapes[i].model = glm::translate(shapes[i].model, glm::vec3(0.0f, shapes[i].height, 0.0f));
            shapes[i].model = glm::scale(shapes[i].model, glm::vec3(1.0f, shapes[i].height, 1.0f));
        }
        else if(i == player_object_num) {
			float new_p_x = p_x, new_p_z = p_z;
			bool is_collision = false;

            if(p_x_move == -1) new_p_x = p_x - 0.05f;
			else if (p_x_move == 1) new_p_x = p_x + 0.05f;
            if (p_z_move == -1) new_p_z = p_z + 0.05f;
			else if (p_z_move == 1) new_p_z = p_z - 0.05f;

			shapes[i].reset = glm::vec3(new_p_x, p_y, new_p_z);
            shapes[i].model = glm::mat4(1.0f);
            shapes[i].model = glm::translate(shapes[i].model, glm::vec3(shapes[i].reset));
            shapes[i].model = glm::scale(shapes[i].model, glm::vec3(0.2f, 0.2f, 0.2f));
            update_world_obb(shapes[i]);

            for(int j = 0; j < player_object_num; j++) {
                if (j != i && check_obb_collision(shapes[i], shapes[j]) && shapes[j].draw) {
                    is_collision = true;
                }
                if (check_obb_collision(shapes[i], shapes[maze_length * maze_width - goal_cnt - 1])) {
					exit(0);
                }
			}

            if(!is_collision) {
                p_x = new_p_x;
                p_z = new_p_z;
            }

            shapes[i].reset = glm::vec3(p_x, p_y, p_z);
            shapes[i].model = glm::mat4(1.0f);
            shapes[i].model = glm::translate(shapes[i].model, glm::vec3(shapes[i].reset));
            shapes[i].model = glm::scale(shapes[i].model, glm::vec3(0.2f, 0.2f, 0.2f));
		}
        else if(i == ground_object_num) {
            shapes[i].model = glm::mat4(1.0f);
            shapes[i].model = glm::scale(shapes[i].model, glm::vec3(maze_width, 0.01f, maze_length));
		}
        else if (i == ground_object_num+1) {
            shapes[i].model = glm::mat4(1.0f);
            shapes[i].model = glm::translate(shapes[i].model, glm::vec3(shapes[i].reset));
            shapes[i].model = glm::scale(shapes[i].model, glm::vec3(1.0f, 0.02f, 1.0f));
        }
        else if (i == ground_object_num+2) {
            shapes[i].model = glm::mat4(1.0f);
            shapes[i].model = glm::translate(shapes[i].model, glm::vec3(shapes[i].reset));
            shapes[i].model = glm::scale(shapes[i].model, glm::vec3(1.0f, 0.02f, 1.0f));
        }
        update_world_obb(shapes[i]);
    }

    if (is1) {
        x_cam = p_x;
		y_cam = p_y + 0.1f;
        z_cam = p_z;

		//static float new_x_at = p_x, new_z_at = p_z;
        //if (p_x_move == -1 && p_z_move == -1) {
        //    new_x_at = p_x - 0.05f;
        //    new_z_at = p_z + 0.05f;
        //}
        //else if (p_x_move == -1 && p_z_move == 1) {
        //    new_x_at = p_x - 0.05f;
        //    new_z_at = p_z - 0.05f;
        //}
        //else if (p_x_move == 1 && p_z_move == -1) {
        //    new_x_at = p_x + 0.05f;
        //    new_z_at = p_z + 0.05f;
        //}
        //else if (p_x_move == 1 && p_z_move == 1) {
        //    new_x_at = p_x + 0.05f;
        //    new_z_at = p_z - 0.05f;
		//}
        //else if (p_x_move == -1) {
        //    new_x_at = p_x - 0.05f;
        //    new_z_at = p_z;
        //}
        //else if (p_x_move == 1) {
        //    new_x_at = p_x + 0.05f;
        //    new_z_at = p_z;
        //}
        //else if(p_z_move == -1) {
        //    new_x_at = p_x;
        //    new_z_at = p_z + 0.05f;
        //}
        //else if (p_z_move == 1) {
        //    new_x_at = p_x;
        //    new_z_at = p_z - 0.05f;
		//}
        //
		//x_at = new_x_at;
		//z_at = new_z_at;
        //y_at = y_cam;

        x_up = 0.0f;
        y_up = 1.0f;
        z_up = 0.0f;

        update_camera();
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

        float sensitivity = 0.005f;

        glm::vec3 view_dir = glm::normalize(cam_locate - cam_at);

        glm::mat4 yaw_rot = glm::rotate(glm::mat4(1.0f), -dx * sensitivity, glm::vec3(0.0f, 1.0f, 0.0f));
        view_dir = glm::vec3(yaw_rot * glm::vec4(view_dir, 0.0f));

        glm::vec3 right_vector = glm::normalize(glm::cross(view_dir, cam_up));
        glm::mat4 pitch_rot = glm::rotate(glm::mat4(1.0f), dy * sensitivity, right_vector);
        view_dir = glm::vec3(pitch_rot * glm::vec4(view_dir, 0.0f));

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

    glm::vec3 min_v = glm::vec3(FLT_MAX);
    glm::vec3 max_v = glm::vec3(-FLT_MAX);

    static float wall_r=rdcolor(mt), wall_g = rdcolor(mt), wall_b = rdcolor(mt);
    float r=0.0f, g = 0.0f, b = 0.0f;

    if (object_num >= 0 && object_num < maze_length*maze_width) {
        wall_r += (1.0f - wall_r) / (float)(maze_width * maze_length);
        wall_g += (1.0f - wall_g) / (float)(maze_width * maze_length);
        wall_b += (1.0f - wall_b) / (float)(maze_width * maze_length);
		r = wall_r; g = wall_g; b = wall_b;
    }
    else if(object_num == player_object_num) {
        r = 1.0f; g = 0.41f; b = 0.71f;
	}
    else if (object_num == ground_object_num) {
        r = 0.2f; g = 0.2f; b = 0.2f;
    }
    else if (object_num == ground_object_num+1) {
        r = 0.6f; g = 0.3f; b = 0.2f;
    }
    else if (object_num == ground_object_num+2) {
        r = 0.5f; g = 0.6f; b = 0.3f;
    }

    SHAPE object_shape;
    object_shape.object_num = object_num;

    for (auto& m : models) {
        for (size_t i = 0; i < m.face_count; i++) {
            Face f = m.faces[i];
            Vertex v1 = m.vertices[f.v1];
            Vertex v2 = m.vertices[f.v2];
            Vertex v3 = m.vertices[f.v3];

            for (const Vertex* v : { &v1, &v2, &v3 }) {
                min_v.x = glm::min(min_v.x, v->x);
                min_v.y = glm::min(min_v.y, v->y);
                min_v.z = glm::min(min_v.z, v->z);
                max_v.x = glm::max(max_v.x, v->x);
                max_v.y = glm::max(max_v.y, v->y);
                max_v.z = glm::max(max_v.z, v->z);
            }

            object_shape.vertex.insert(object_shape.vertex.end(), {
                v1.x, v1.y, v1.z, r, g, b,
                v2.x, v2.y, v2.z, r, g, b,
                v3.x, v3.y, v3.z, r, g, b
                });
            object_shape.face_count++;
        }
    }

    glm::vec3 local_center = (min_v + max_v) * 0.5f;
    glm::vec3 local_half_length = (max_v - min_v) * 0.5f;

    object_shape.local_obb.center = local_center;
    object_shape.local_obb.half_length = local_half_length;
    object_shape.local_obb.u[0] = glm::vec3(1.0f, 0.0f, 0.0f);
    object_shape.local_obb.u[1] = glm::vec3(0.0f, 1.0f, 0.0f);
    object_shape.local_obb.u[2] = glm::vec3(0.0f, 0.0f, 1.0f);

    shapes.push_back(object_shape);

    UpdateBuffer();
}

void update_world_obb(SHAPE& shape) {
    glm::mat3 rotation_scale_mat = glm::mat3(shape.model);

    glm::vec4 local_center_h = glm::vec4(shape.local_obb.center, 1.0f);
    shape.world_obb.center = glm::vec3(shape.model * local_center_h);

    for (int i = 0; i < 3; i++) {
        shape.world_obb.u[i] = glm::normalize(rotation_scale_mat * shape.local_obb.u[i]);
    }

    glm::vec3 scale_factors = glm::vec3(
        glm::length(rotation_scale_mat[0]), // X축 스케일
        glm::length(rotation_scale_mat[1]), // Y축 스케일
        glm::length(rotation_scale_mat[2])  // Z축 스케일
    );

    shape.world_obb.half_length = shape.local_obb.half_length * scale_factors;
}

bool is_separated(const OBB& a, const OBB& b, const glm::vec3& axis) {
    if (glm::length(axis) < 1e-6) return false;

    glm::vec3 T = b.center - a.center;

    float distance_proj = glm::abs(glm::dot(T, axis));

    float radius_a =
        glm::abs(glm::dot(a.half_length.x * a.u[0], axis)) +
        glm::abs(glm::dot(a.half_length.y * a.u[1], axis)) +
        glm::abs(glm::dot(a.half_length.z * a.u[2], axis));

    float radius_b =
        glm::abs(glm::dot(b.half_length.x * b.u[0], axis)) +
        glm::abs(glm::dot(b.half_length.y * b.u[1], axis)) +
        glm::abs(glm::dot(b.half_length.z * b.u[2], axis));

    return distance_proj > (radius_a + radius_b);
}

bool check_obb_collision(const SHAPE& shapeA, const SHAPE& shapeB) {
    const OBB& a = shapeA.world_obb;
    const OBB& b = shapeB.world_obb;

    for (int i = 0; i < 3; i++) {
        if (is_separated(a, b, a.u[i])) return false;
    }

    for (int i = 0; i < 3; i++) {
        if (is_separated(a, b, b.u[i])) return false;
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            glm::vec3 cross_axis = glm::cross(a.u[i], b.u[j]);
            if (is_separated(a, b, cross_axis)) return false;
        }
    }

    return true;
}

void reset_c() {
	x_cam = 0.0f;
    y_cam = BOX_SIZE * (maze_width + maze_width) / 2 + 10.0f; 
    z_cam = maze_width * 2; 

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
	}
    UpdateBuffer();

    depth_on = true;
    proj_on = true;
	start_anime = true;
    ism = false;
    isv = false;
    iss = false;
	isr = false;
    is1 = false;

	shapes[player_object_num].draw = false;
    p_x = shapes[1].reset.x;
    p_y = BOX_SIZE / 2;
    p_z = shapes[1].reset.z;
    p_x_move = 0;
	p_z_move = 0;

}

void update_camera() {
    cam_locate = glm::vec3(x_cam, y_cam, z_cam);
    cam_at = glm::vec3(x_at, y_at, z_at);
	cam_up = glm::vec3(x_up, y_up, z_up);
}
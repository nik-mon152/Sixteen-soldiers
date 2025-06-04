#include <GL/freeglut.h>

#include <windows.h>
#include <utility>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <cstdlib>  // Äëÿ srand() è rand()
#include <ctime>    // Äëÿ time()

#define INF     INT_MAX
#define NONE    0
#define RED     1
#define BLUE    2
#define TIE     3
#define EASY    4
#define MEDIUM  5
#define HARD    6
using namespace std;

// used to create the suggestion vectors
struct st_suggestions
{
    int suggest_x, suggest_y;
    bool suggest_is_captured;
};
// stores the 4 coordinates for each of the 3 menu rectangles (Easy, Medium, Hard)
vector<pair<int, int> > quad[] = { { {-2, 5}, {-2, 4}, {1, 4}, {1, 5} },
                                   { {-2, 3}, {-2, 2}, {1, 2}, {1, 3} },
                                   { {-2, 1}, {-2, 0}, {1, 0}, {1, 1} } };
// starting menu text array
string menu_string[] = { "Select difficulty", "Easy", "Medium", "Hard" };

// starting menu text coordinates
vector<pair<float, float> > menu_text_coordinate = { {-1.8, 6}, {-1, 4.3}, {-1.2, 2.3}, {-1, 0.3} };

// stores the 4 coordinates for play again rectangle
vector<pair<int, int> > play_again = { {-2, 10}, {-2, 9}, {2, 9}, {2, 10} };

// play again string text coordinate
pair<float, float> play_again_text_coordinate = { -2.2, 9.3 };

vector<pair<int, int> > points;                         // stores all valid points that make the board
vector<pair<int, int> > line_start, line_end;           // line_start stores coordinates of lines' starting points && line_end stores ending points respectively
map<int, map<int, bool> > is_valid_point;               // is_valid_point[x][y] returns true if (x,y) coordinate is a valid point inside the board
map<int, map<int, map<int, map<int, bool> > > > adj;    // adjacency matrix - adj[][][][] = boolean, stores the connections between the adjacent points
map<int, map<int, int> > mark;                          // 0 for none, 1 for red, 2 for blue
vector<pair<int, int> > red_soldiers, blue_soldiers;    // stores the alive soldiers' coordinates
vector<struct st_suggestions> suggestions;              // stores the possible moves for the selected soldier

int x, y;                                               // stores the coordinate of soldier to move
int difficulty;                                         // stores the difficulty level of the game
int fromX, fromY, toX, toY;                             // used to draw lines from (fromX, fromY) to (toX, toY)
int text_x = -2, text_y = -9;                           // turning texts will be printed at this coordinate
int winHeight = 900, winWidth = 1200;                   // console window size
int partial_area = 8;                                   // increasing this value decreases the partial area of soldier selection coordinate
int turn;                                               // indicates whose turn it is
int game_won;                                           // indicates who won the game, initially none
int hX, hY;                                             // highlighted coordinate
int count_blue_soldier, count_red_soldier;              // number of alive soldiers
int minimax_depth_limit;                                // stores the depth limit of forward checking of minimax algorithm
bool is_red_moving;                                     // indicates if moving indicator needs to be drawn
bool is_check_game_status;                              // indicates if checking game status is required
bool is_highlighted;                                    // stores the highlighted status
bool is_captured;                                       // stores the captured status
bool is_game_started = false;                           // used to select difficulty level

int  reCalculate(int, int);
void mouse(int, int, int, int);
void initialize();
void select_difficulty();
void select_play_again();
void display();
void reshape(int, int);
void shuffle_vector(bool, bool);
void suggest_moves(int, int, int, bool);
void user_move();
int  minimax(bool, int, int, bool, int, int, int, int);
void opponent_move();
void check_game_status();
void draw_difficulty_section();
void draw_points();
void draw_lines();
void draw_move_indicator_line();
void draw_team_red();
void draw_team_blue();
void draw_highlighted_point();
void draw_suggested_points();
void draw_text();
void draw_game_over();

int main(int argc, char* argv[]) {

    /* initialize GLUT, using any commandline parameters passed to the
       program */
    glutInit(&argc, argv);

    /* setup the size, position, and display mode for new windows */
    glutInitWindowSize(winWidth, winHeight);
    glutInitWindowPosition(500, 50);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);

    /* create and set up a window */
    glutCreateWindow("Sixteen soldiers");

    glutMouseFunc(mouse);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);

    glClearColor(0.0f, 0.5f, 1.0f, 1.0f);
    /* tell GLUT to wait for events */
    glutMainLoop();
}

int reCalculate(int a, int sz)
{
    float b1;
    int mid, b2, area;
    mid = sz / 10 / 2;
    area = winWidth / 10 / partial_area;

    b1 = (float)a / mid;
    //b2 = round(b1);
    b2 = static_cast<int>(round(b1));
    if (abs(b2 * mid - a) <= area) return b2;
    return INF;
}

void mouse(int button, int state, int mousex, int mousey)
{
    int tempx, tempy;

    // during the difficulty level selection we need PROJECTION mode coordinate
    // so conversion is not needed here
    if (!is_game_started || turn == NONE)
    {
        x = mousex;
        y = mousey;
        return;
    }

    // during the game we need MODELVIEW mode coordinate
    // so conversion is needed here
    if (turn == BLUE && button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        tempx = mousex - (winWidth / 2);
        tempy = (winHeight / 2) - mousey;

        // convert the mousex, mousey coordinate into MODELVIEW matrix mode
        x = reCalculate(tempx, winWidth);
        y = reCalculate(tempy, winHeight);
    }
}

void initialize()
{
    srand(time(NULL));
    //srand(static_cast<unsigned int>(time(NULL)));


    red_soldiers.clear();
    blue_soldiers.clear();
    mark.clear();

    turn = BLUE;
    game_won = NONE;
    count_blue_soldier = 16;
    count_red_soldier = 16;
    is_red_moving = false;
    is_check_game_status = false;
    is_highlighted = false;
    is_captured = false;

    points.push_back({ -4, 8 });
    points.push_back({ 0, 8 });
    points.push_back({ 4, 8 });

    points.push_back({ -2, 6 });
    points.push_back({ 0, 6 });
    points.push_back({ 2, 6 });

    points.push_back({ -4, 4 });
    points.push_back({ -2, 4 });
    points.push_back({ 0, 4 });
    points.push_back({ 2, 4 });
    points.push_back({ 4, 4 });

    points.push_back({ -4, 2 });
    points.push_back({ -2, 2 });
    points.push_back({ 0, 2 });
    points.push_back({ 2, 2 });
    points.push_back({ 4, 2 });

    points.push_back({ -4, 0 });
    points.push_back({ -2, 0 });
    points.push_back({ 0, 0 });
    points.push_back({ 2, 0 });
    points.push_back({ 4, 0 });

    points.push_back({ -4, -2 });
    points.push_back({ -2, -2 });
    points.push_back({ 0, -2 });
    points.push_back({ 2, -2 });
    points.push_back({ 4, -2 });

    points.push_back({ -4, -4 });
    points.push_back({ -2, -4 });
    points.push_back({ 0, -4 });
    points.push_back({ 2, -4 });
    points.push_back({ 4, -4 });

    points.push_back({ -2, -6 });
    points.push_back({ 0, -6 });
    points.push_back({ 2, -6 });

    points.push_back({ -4, -8 });
    points.push_back({ 0, -8 });
    points.push_back({ 4, -8 });

    for (auto& i : points)
        is_valid_point[i.first][i.second] = true;

    line_start.push_back({ -4, 8 });
    line_end.push_back({ 0, 8 });

    line_start.push_back({ -4, 8 });
    line_end.push_back({ -2, 6 });

    line_start.push_back({ 0, 8 });
    line_end.push_back({ 4, 8 });

    line_start.push_back({ 0, 8 });
    line_end.push_back({ 0, 6 });

    line_start.push_back({ 4, 8 });
    line_end.push_back({ 2, 6 });

    line_start.push_back({ -2, 6 });
    line_end.push_back({ 0, 6 });

    line_start.push_back({ 0, 6 });
    line_end.push_back({ 2, 6 });

    line_start.push_back({ -2, 6 });
    line_end.push_back({ 0, 6 });

    line_start.push_back({ 0, 6 });
    line_end.push_back({ 2, 6 });

    line_start.push_back({ -2, 6 });
    line_end.push_back({ 0, 4 });

    line_start.push_back({ 0, 6 });
    line_end.push_back({ 0, 4 });

    line_start.push_back({ 2, 6 });
    line_end.push_back({ 0, 4 });

    line_start.push_back({ -4, 4 });
    line_end.push_back({ -2, 4 });

    line_start.push_back({ -2, 4 });
    line_end.push_back({ 0, 4 });

    line_start.push_back({ 0, 4 });
    line_end.push_back({ 2, 4 });

    line_start.push_back({ 2, 4 });
    line_end.push_back({ 4, 4 });

    line_start.push_back({ -4, 4 });
    line_end.push_back({ -4, 2 });

    line_start.push_back({ -4, 4 });
    line_end.push_back({ -2, 2 });

    line_start.push_back({ -2, 4 });
    line_end.push_back({ -2, 2 });

    line_start.push_back({ 0, 4 });
    line_end.push_back({ -2, 2 });

    line_start.push_back({ 0, 4 });
    line_end.push_back({ 0, 2 });

    line_start.push_back({ 0, 4 });
    line_end.push_back({ 2, 2 });

    line_start.push_back({ 2, 4 });
    line_end.push_back({ 2, 2 });

    line_start.push_back({ 4, 4 });
    line_end.push_back({ 2, 2 });

    line_start.push_back({ 4, 4 });
    line_end.push_back({ 4, 2 });

    line_start.push_back({ -4, 2 });
    line_end.push_back({ -2, 2 });

    line_start.push_back({ -2, 2 });
    line_end.push_back({ 0, 2 });

    line_start.push_back({ 0, 2 });
    line_end.push_back({ 2, 2 });

    line_start.push_back({ 2, 2 });
    line_end.push_back({ 4, 2 });

    line_start.push_back({ -4, 2 });
    line_end.push_back({ -4, 0 });

    line_start.push_back({ -2, 2 });
    line_end.push_back({ -4, 0 });

    line_start.push_back({ -2, 2 });
    line_end.push_back({ -2, 0 });

    line_start.push_back({ -2, 2 });
    line_end.push_back({ 0, 0 });

    line_start.push_back({ 0, 2 });
    line_end.push_back({ 0, 0 });

    line_start.push_back({ 2, 2 });
    line_end.push_back({ 0, 0 });

    line_start.push_back({ 2, 2 });
    line_end.push_back({ 2, 0 });

    line_start.push_back({ 2, 2 });
    line_end.push_back({ 4, 0 });

    line_start.push_back({ 4, 2 });
    line_end.push_back({ 4, 0 });

    line_start.push_back({ -4, 0 });
    line_end.push_back({ -2, 0 });

    line_start.push_back({ -2, 0 });
    line_end.push_back({ 0, 0 });

    line_start.push_back({ 0, 0 });
    line_end.push_back({ 2, 0 });

    line_start.push_back({ 2, 0 });
    line_end.push_back({ 4, 0 });

    line_start.push_back({ -4,-2 });
    line_end.push_back({ -4, 0 });

    line_start.push_back({ -2,-2 });
    line_end.push_back({ -4, 0 });

    line_start.push_back({ -2,-2 });
    line_end.push_back({ -2, 0 });

    line_start.push_back({ -2,-2 });
    line_end.push_back({ 0, 0 });

    line_start.push_back({ 0,-2 });
    line_end.push_back({ 0, 0 });

    line_start.push_back({ 2,-2 });
    line_end.push_back({ 0, 0 });

    line_start.push_back({ 2,-2 });
    line_end.push_back({ 2, 0 });

    line_start.push_back({ 2,-2 });
    line_end.push_back({ 4, 0 });

    line_start.push_back({ 4,-2 });
    line_end.push_back({ 4, 0 });

    line_start.push_back({ -4, 0 });
    line_end.push_back({ -2, 0 });

    line_start.push_back({ -2, 0 });
    line_end.push_back({ 0, 0 });

    line_start.push_back({ 0, 0 });
    line_end.push_back({ 2, 0 });

    line_start.push_back({ 2, 0 });
    line_end.push_back({ 4, 0 });

    line_start.push_back({ -4,-4 });
    line_end.push_back({ -4,-2 });

    line_start.push_back({ -4,-4 });
    line_end.push_back({ -2,-2 });

    line_start.push_back({ -2,-4 });
    line_end.push_back({ -2,-2 });

    line_start.push_back({ 0,-4 });
    line_end.push_back({ -2,-2 });

    line_start.push_back({ 0,-4 });
    line_end.push_back({ 0,-2 });

    line_start.push_back({ 0,-4 });
    line_end.push_back({ 2,-2 });

    line_start.push_back({ 2,-4 });
    line_end.push_back({ 2,-2 });

    line_start.push_back({ 4,-4 });
    line_end.push_back({ 2,-2 });

    line_start.push_back({ 4,-4 });
    line_end.push_back({ 4,-2 });

    line_start.push_back({ -4,-2 });
    line_end.push_back({ -2,-2 });

    line_start.push_back({ -2,-2 });
    line_end.push_back({ 0,-2 });

    line_start.push_back({ 0,-2 });
    line_end.push_back({ 2,-2 });

    line_start.push_back({ 2,-2 });
    line_end.push_back({ 4,-2 });

    line_start.push_back({ -2,-6 });
    line_end.push_back({ 0,-6 });

    line_start.push_back({ 0,-6 });
    line_end.push_back({ 2,-6 });

    line_start.push_back({ -2,-6 });
    line_end.push_back({ 0,-4 });

    line_start.push_back({ 0,-6 });
    line_end.push_back({ 0,-4 });

    line_start.push_back({ 2,-6 });
    line_end.push_back({ 0,-4 });

    line_start.push_back({ -4,-4 });
    line_end.push_back({ -2,-4 });

    line_start.push_back({ -2,-4 });
    line_end.push_back({ 0,-4 });

    line_start.push_back({ 0,-4 });
    line_end.push_back({ 2,-4 });

    line_start.push_back({ 2,-4 });
    line_end.push_back({ 4,-4 });

    line_start.push_back({ -4,-8 });
    line_end.push_back({ 0,-8 });

    line_start.push_back({ -4,-8 });
    line_end.push_back({ -2,-6 });

    line_start.push_back({ 0,-8 });
    line_end.push_back({ 4,-8 });

    line_start.push_back({ 0,-8 });
    line_end.push_back({ 0,-6 });

    line_start.push_back({ 4,-8 });
    line_end.push_back({ 2,-6 });

    line_start.push_back({ -2,-6 });
    line_end.push_back({ 0,-6 });

    line_start.push_back({ 0,-6 });
    line_end.push_back({ 2,-6 });

    for (int i = 0; i < (int)line_start.size(); i++)
    {
        adj[line_start[i].first][line_start[i].second][line_end[i].first][line_end[i].second] = true;
        adj[line_end[i].first][line_end[i].second][line_start[i].first][line_start[i].second] = true;
    }

    red_soldiers.push_back({ -4, 8 });
    red_soldiers.push_back({ 0, 8 });
    red_soldiers.push_back({ 4, 8 });

    red_soldiers.push_back({ -2, 6 });
    red_soldiers.push_back({ 0, 6 });
    red_soldiers.push_back({ 2, 6 });

    red_soldiers.push_back({ -4, 4 });
    red_soldiers.push_back({ -2, 4 });
    red_soldiers.push_back({ 0, 4 });
    red_soldiers.push_back({ 2, 4 });
    red_soldiers.push_back({ 4, 4 });

    red_soldiers.push_back({ -4, 2 });
    red_soldiers.push_back({ -2, 2 });
    red_soldiers.push_back({ 0, 2 });
    red_soldiers.push_back({ 2, 2 });
    red_soldiers.push_back({ 4, 2 });

    for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
        mark[it->first][it->second] = RED;

    blue_soldiers.push_back({ -4, -2 });
    blue_soldiers.push_back({ -2, -2 });
    blue_soldiers.push_back({ 0, -2 });
    blue_soldiers.push_back({ 2, -2 });
    blue_soldiers.push_back({ 4, -2 });

    blue_soldiers.push_back({ -4, -4 });
    blue_soldiers.push_back({ -2, -4 });
    blue_soldiers.push_back({ 0, -4 });
    blue_soldiers.push_back({ 2, -4 });
    blue_soldiers.push_back({ 4, -4 });

    blue_soldiers.push_back({ -2, -6 });
    blue_soldiers.push_back({ 0, -6 });
    blue_soldiers.push_back({ 2, -6 });

    blue_soldiers.push_back({ -4, -8 });
    blue_soldiers.push_back({ 0, -8 });
    blue_soldiers.push_back({ 4, -8 });

    for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
        mark[it->first][it->second] = BLUE;
}

void select_difficulty()
{
    int x_min, x_max, y_min, y_max, diff, height, width;
    width = winWidth / 10 / 2;
    height = winHeight / 10 / 2;

    for (int i = 0; i < 3; i++)
    {
        x_min = quad[i][0].first, x_max = quad[i][2].first;
        y_min = quad[i][3].second, y_max = quad[i][1].second;
        x_min = (winWidth / 2) + (x_min * width);
        x_max = (winWidth / 2) + (x_max * width);
        y_min = (winHeight / 2) - (y_min * height);
        y_max = (winHeight / 2) - (y_max * height);

        if (x_min <= x && x <= x_max && y_min <= y && y <= y_max)
        {
            diff = i + 4;
            if (diff == 4) difficulty = EASY, minimax_depth_limit = 1;
            if (diff == 5) difficulty = MEDIUM, minimax_depth_limit = 2;
            if (diff == 6) difficulty = HARD, minimax_depth_limit = 4;

            is_game_started = true;
            break;
        }
    }
    if (is_game_started)
    {
        glutPostRedisplay();
    }
}

void select_play_again()
{
    int x_min, x_max, y_min, y_max, height, width;
    width = winWidth / 10 / 2;
    height = winHeight / 10 / 2;

    x_min = play_again[0].first, x_max = play_again[2].first;
    y_min = play_again[3].second, y_max = play_again[1].second;
    x_min = (winWidth / 2) + (x_min * width);
    x_max = (winWidth / 2) + (x_max * width);
    y_min = (winHeight / 2) - (y_min * height);
    y_max = (winHeight / 2) - (y_max * height);

    if (x_min <= x && x <= x_max && y_min <= y && y <= y_max)
        is_game_started = false;
}

void display() {

    /* clear window */
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    winHeight = glutGet(GLUT_WINDOW_HEIGHT);
    winWidth = glutGet(GLUT_WINDOW_WIDTH);

    if (!is_game_started)
    {
        // choose difficulty level
        draw_difficulty_section();
        select_difficulty();
        if (is_game_started) initialize();
    }
    else
    {
        // difficulty level is selected. Main game will run here

        if (turn == BLUE)
        {
            user_move();
            if (is_check_game_status) check_game_status();
        }
        else if (turn == RED && !is_red_moving)
        {
            opponent_move();
            if (is_check_game_status) check_game_status();
            
        }

        draw_points();
        draw_lines();
        draw_team_red();
        draw_team_blue();
        if (is_red_moving)
        {
            draw_move_indicator_line();
            is_red_moving = false;
        }
        draw_text();
        if (is_highlighted) draw_highlighted_point();
        if (is_highlighted && turn == BLUE) draw_suggested_points();
        if (turn == NONE)
        {
            draw_game_over();
            select_play_again();
        }
    }
    /* flush drawing routines to the window */
    
    glFlush();

}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-10, 10, -10, 10);
    glMatrixMode(GL_MODELVIEW);
}

void shuffle_vector(bool shuffle_red_soldiers, bool shuffle_suggestions)
{
    int a, b, sz = (int)suggestions.size();
    if (shuffle_red_soldiers)
    {
        for (int i = 0; i < count_red_soldier; i++)
        {
            a = rand() % count_red_soldier;
            b = rand() % count_red_soldier;
            swap(red_soldiers[a], red_soldiers[b]);
        }
    }
    if (shuffle_suggestions)
    {
        for (int i = 0; i < sz; i++)
        {
            a = rand() % sz;
            b = rand() % sz;
            swap(suggestions[a], suggestions[b]);
        }
    }
}

void suggest_moves(int x1, int y1, int opposite, bool capture)
{
    // last 2 direction values are for uppermost and lowermost row
   // these 2 values doesn't effect other rows
   // adj matrix handles this case
    int fx[] = { 2, -2, 0,  0, 2,  2, -2, -2, 4, -4 };
    int fy[] = { 0,  0, 2, -2, 2, -2, -2,  2, 0,  0 };
    int tx1, tx2, ty1, ty2;

    suggestions.clear();

    for (int i = 0; i < 10; i++)
    {
        tx1 = x1 + fx[i];
        ty1 = y1 + fy[i];
        tx2 = tx1 + fx[i];
        ty2 = ty1 + fy[i];

        // works only when it's soldier's first move of a new turn
        // suggests a normal move. can't capture opponent's soldier
        if (!capture)
        {
            if (mark[tx1][ty1] == NONE && adj[x1][y1][tx1][ty1])
                suggestions.push_back({ tx1, ty1, 0 });
        }

        // works both in first move and further
        // suggests a possible capturing move
        if (mark[tx2][ty2] == NONE
            && adj[x1][y1][tx1][ty1] && adj[tx1][ty1][tx2][ty2]
            && mark[tx1][ty1] == opposite)
            suggestions.push_back({ tx2, ty2, 1 });
    }
}

void user_move()
{
    int midx, midy;
    bool is_coordinate_found_in_suggestion_list = false;
    bool is_capturing_move;

    if (!is_captured && mark[x][y] == BLUE)
    {
        // highlights this coordinate
        hX = x, hY = y;
        is_highlighted = true;
        suggest_moves(x, y, RED, is_captured);
    }
    else
    {
        // check if clicked coordinate is included in the suggestion list
        for (auto it = suggestions.begin(); it != suggestions.end(); it++)
        {
            if (it->suggest_x == x && it->suggest_y == y)
            {
                is_coordinate_found_in_suggestion_list = true;
                is_capturing_move = it->suggest_is_captured;
                break;
            }
        }

        if (is_coordinate_found_in_suggestion_list)
        {
            if (!is_capturing_move)
            {
                /// normal move

                // shift the highlighted soldier to new coordinate
                for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
                {
                    if (it->first == hX && it->second == hY)
                    {
                        it->first = x, it->second = y;
                        break;
                    }
                }
                mark[hX][hY] = NONE;
                mark[x][y] = BLUE;

                is_highlighted = false;
                is_check_game_status = true;
                turn = RED;
                glutPostRedisplay();
            }
            else
            {
                /// capturing move

                // shift highlighted soldier to new coordinate
                for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
                {
                    if (it->first == hX && it->second == hY)
                    {
                        it->first = x, it->second = y;
                        break;
                    }
                }
                mark[hX][hY] = NONE;
                mark[x][y] = BLUE;

                // delete the opponent soldier from the middle
                midx = (x + hX) / 2;
                midy = (y + hY) / 2;
                mark[midx][midy] = NONE;
                for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
                {
                    if (it->first == midx && it->second == midy)
                    {
                        red_soldiers.erase(it);
                        break;
                    }
                }
                hX = x, hY = y;
                count_red_soldier--;
                is_captured = true;

                suggest_moves(x, y, RED, is_captured);
                if (!(int)suggestions.size())
                {
                    is_captured = false;
                    is_highlighted = false;
                    is_check_game_status = true;
                    turn = RED;
                    glutPostRedisplay();
                    
                }
            }
        }
    }
    
}

int minimax(bool is_first_iteration, int level, int turn, bool capture, int xcord, int ycord, int dead_red, int dead_blue)
{
    if (level == minimax_depth_limit) return (dead_blue - dead_red * 2);
    if (!count_blue_soldier) return 100;
    if (!count_red_soldier) return -100;

    int res{}, ans, deleted_position, oldX, oldY, newX = 0, newY = 0, midX, midY;

    if (turn == RED)             // RED tries to maximize
    {
        res = -INF;

        for (auto& i : red_soldiers)
        {
            if (capture && (i.first != xcord || i.second != ycord)) continue;

            suggest_moves(i.first, i.second, BLUE, capture);
            // shuffle suggestions vector
            shuffle_vector(false, true);
            vector<struct st_suggestions> copy_suggestions = suggestions;

            if (capture && !(int)copy_suggestions.size())
            {
                ans = minimax(false, level + 1, BLUE, false, newX, newY, dead_red, dead_blue);           /// minimax call
                if (ans >= res)
                {
                    res = ans;
                    if (is_first_iteration)
                    {
                        toX = INF;
                        is_captured = true;
                    }
                }
            }

            for (auto& j : copy_suggestions)
            {
                oldX = i.first, oldY = i.second;
                newX = j.suggest_x, newY = j.suggest_y;

                if (j.suggest_is_captured)
                {
                    /// for capturing move

                    midX = (oldX + newX) / 2, midY = (oldY + newY) / 2;
                    mark[oldX][oldY] = NONE;
                    mark[midX][midY] = NONE;
                    mark[newX][newY] = RED;

                    // shift (oldX, oldY) to (newX, newY)
                    for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
                    {
                        if (it->first == oldX && it->second == oldY)
                        {
                            it->first = newX;
                            it->second = newY;
                            break;
                        }
                    }

                    // delete (midX, midY) from blue_soldiers vector
                    deleted_position = 0;
                    for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
                    {
                        if (it->first == midX && it->second == midY)
                        {
                            blue_soldiers.erase(it);
                            break;
                        }
                        deleted_position++;
                    }
                    count_blue_soldier--;

                    ans = minimax(false, level, RED, true, newX, newY, dead_red, dead_blue + 1);        /// minimax call
                    if (ans >= res)
                    {
                        res = ans;
                        if (is_first_iteration)
                        {
                            fromX = oldX, fromY = oldY;
                            toX = newX, toY = newY;
                            is_captured = true;
                        }
                    }

                    mark[oldX][oldY] = RED;
                    mark[midX][midY] = BLUE;
                    mark[newX][newY] = NONE;

                    // shift (newX, newY) to (oldX, oldY)
                    for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
                    {
                        if (it->first == newX && it->second == newY)
                        {
                            it->first = oldX;
                            it->second = oldY;
                            break;
                        }
                    }

                    // put (midX, midY) back in the blue_soldiers vector
                    blue_soldiers.insert(blue_soldiers.begin() + deleted_position, { midX, midY });
                    count_blue_soldier++;
                }
                else
                {
                    /// for non capturing move

                    mark[oldX][oldY] = NONE;
                    mark[newX][newY] = RED;

                    // shift (oldX, oldY) to (newX, newY)
                    for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
                    {
                        if (it->first == oldX && it->second == oldY)
                        {
                            it->first = newX;
                            it->second = newY;
                            break;
                        }
                    }

                    ans = minimax(false, level + 1, BLUE, false, newX, newY, dead_red, dead_blue);        /// minimax call
                    if (ans > res)
                    {
                        res = ans;
                        if (is_first_iteration)
                        {
                            fromX = oldX, fromY = oldY;
                            toX = newX, toY = newY;
                            is_captured = false;
                        }
                    }


                    mark[oldX][oldY] = RED;
                    mark[newX][newY] = NONE;

                    // shift (newX, newY) to (oldX, oldY)
                    for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
                    {
                        if (it->first == newX && it->second == newY)
                        {
                            it->first = oldX;
                            it->second = oldY;
                            break;
                        }
                    }
                }
            }
        }

        // in case of TIE
        if (res == -INF) res = (dead_blue - dead_red * 2);
    }

    if (turn == BLUE)            // BLUE tries to minimize
    {
        res = INF;

        for (auto& i : blue_soldiers)
        {
            if (capture && (i.first != xcord || i.second != ycord)) continue;

            suggest_moves(i.first, i.second, RED, capture);
            vector<struct st_suggestions> copy_suggestions = suggestions;

            if (capture && !(int)copy_suggestions.size())
            {
                ans = minimax(false, level + 1, RED, false, newX, newY, dead_red, dead_blue);           /// minimax call
                if (ans <= res) res = ans;
            }

            for (auto& j : copy_suggestions)
            {
                oldX = i.first, oldY = i.second;
                newX = j.suggest_x, newY = j.suggest_y;

                if (j.suggest_is_captured)
                {
                    /// for capturing move

                    midX = (oldX + newX) / 2, midY = (oldY + newY) / 2;
                    mark[oldX][oldY] = NONE;
                    mark[midX][midY] = NONE;
                    mark[newX][newY] = BLUE;

                    // shift (oldX, oldY) to (newX, newY)
                    for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
                    {
                        if (it->first == oldX && it->second == oldY)
                        {
                            it->first = newX;
                            it->second = newY;
                            break;
                        }
                    }

                    // delete (midX, midY) from red vector
                    deleted_position = 0;
                    for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
                    {
                        if (it->first == midX && it->second == midY)
                        {
                            red_soldiers.erase(it);
                            break;
                        }
                        deleted_position++;
                    }
                    count_red_soldier--;

                    ans = minimax(false, level, BLUE, true, newX, newY, dead_red + 1, dead_blue);        /// minimax call
                    if (ans <= res) res = ans;

                    mark[oldX][oldY] = BLUE;
                    mark[midX][midY] = RED;
                    mark[newX][newY] = NONE;

                    // shift (newX, newY) to (oldX, oldY)
                    for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
                    {
                        if (it->first == newX && it->second == newY)
                        {
                            it->first = oldX;
                            it->second = oldY;
                            break;
                        }
                    }

                    // put (midX, midY) back in the red_soldiers vector
                    red_soldiers.insert(red_soldiers.begin() + deleted_position, { midX, midY });
                    count_red_soldier++;
                }
                else
                {
                    /// for non capturing move

                    mark[oldX][oldY] = NONE;
                    mark[newX][newY] = BLUE;

                    // shift (oldX, oldY) to (newX, newY)
                    for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
                    {
                        if (it->first == oldX && it->second == oldY)
                        {
                            it->first = newX;
                            it->second = newY;
                            break;
                        }
                    }

                    ans = minimax(false, level + 1, RED, false, newX, newY, dead_red, dead_blue);        /// minimax call
                    if (ans < res) res = ans;

                    mark[oldX][oldY] = BLUE;
                    mark[newX][newY] = NONE;

                    // shift (newX, newY) to (oldX, oldY)
                    for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
                    {
                        if (it->first == newX && it->second == newY)
                        {
                            it->first = oldX;
                            it->second = oldY;
                            break;
                        }
                    }
                }
            }
        }

        // in case of TIE
        if (res == INF) res = (dead_blue - dead_red * 2);
    }

    return res;
}

void opponent_move()
{
    int midX, midY;

    // shuffle red_soldiers vector
    shuffle_vector(true, false);
    minimax(true, 0, RED, is_captured, x, y, 0, 0);         /// minimax call

    if (!is_captured)
    {
        /// non capturing move

        mark[fromX][fromY] = NONE;
        mark[toX][toY] = RED;

        is_red_moving = true;
        display();

        // shift (fromX, fromY) to (toX, toY)
        for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
        {
            if (it->first == fromX && it->second == fromY)
            {
                it->first = toX;
                it->second = toY;
                break;
            }
        }

        is_check_game_status = true;
        turn = BLUE;
    }
    else if (is_captured)
    {
        /// capturing move

        if (toX == INF)
        {
            is_captured = false;
            is_check_game_status = true;
            turn = BLUE;
            return;
        }

        midX = (fromX + toX) / 2;
        midY = (fromY + toY) / 2;

        mark[fromX][fromY] = NONE;
        mark[midX][midY] = NONE;
        mark[toX][toY] = RED;

        is_red_moving = true;
        display();

        // shift (fromX, fromY) to (toX, toY)
        for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
        {
            if (it->first == fromX && it->second == fromY)
            {
                it->first = toX;
                it->second = toY;
                break;
            }
        }

        // delete (midX, midY) from blue_soldiers vector
        for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
        {
            if (it->first == midX && it->second == midY)
            {
                blue_soldiers.erase(it);
                break;
            }
        }
        count_blue_soldier--;

        // next minimax will start from (x, y)
        x = toX, y = toY;
        toX = toY = INF;
    }

    Sleep(1000);
    glutPostRedisplay();
}

void check_game_status()
{
    bool red_ok, blue_ok;
    red_ok = blue_ok = false;

    // check if any move is possible for RED
    for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
    {
        suggest_moves(it->first, it->second, BLUE, false);
        if ((int)suggestions.size())
        {
            red_ok = true;
            break;
        }
    }

    // check if any move is possible for BLUE
    for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
    {
        suggest_moves(it->first, it->second, RED, false);
        if ((int)suggestions.size())
        {
            blue_ok = true;
            break;
        }
    }
    suggestions.clear();

    if (turn == RED)
    {
        if (!count_red_soldier)
        {
            turn = NONE;
            game_won = BLUE;
        }
        else if (!red_ok)
        {
            turn = NONE;
            game_won = TIE;
        }
    }
    if (turn == BLUE)
    {
        if (!count_blue_soldier)
        {
            turn = NONE;
            game_won = RED;
        }
        else if (!blue_ok)
        {
            turn = NONE;
            game_won = TIE;
        }
    }

    is_check_game_status = false;
    x = y = -1;
}

void draw_difficulty_section()
{
    int len;

    glColor3f(0.0, 0.0, 1.0);
    glBegin(GL_QUADS);

    // draw the empty rectangles
    for (int i = 0; i < 3; i++)
    {
        for (auto it = quad[i].begin(); it != quad[i].end(); it++)
            glVertex2f(static_cast<GLfloat>(it->first), static_cast<GLfloat>(it->second));
    }
    glEnd();

    glColor3f(1.0, 1.0, 1.0);
    // print "menu_string" array strings
    for (int i = 0; i < 4; i++)
    {
        len = (int)menu_string[i].size();
        glRasterPos2f(menu_text_coordinate[i].first, menu_text_coordinate[i].second);

        //loop to display character by character
        for (int j = 0; j < len; j++)
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, menu_string[i][j]);
    }
}

void draw_points()
{
    glPointSize(2.0);
    glBegin(GL_POINTS);
    glColor3f(1,1,1);
    for (auto it = points.begin(); it != points.end(); it++)
        glVertex2f(static_cast<GLfloat>(it->first), static_cast<GLfloat>(it->second));
    glEnd();
}

void draw_lines()
{
    glBegin(GL_LINES);
    //glColor3f(1.0, 1.0, 1.0);
    glColor3f(1, 1, 1);
    for (int i = 0; i < (int)line_start.size(); i++)
    {
        glVertex2f(static_cast<GLfloat>(line_start[i].first), static_cast<GLfloat>(line_start[i].second));
        glVertex2f(static_cast<GLfloat>(line_end[i].first), static_cast<GLfloat>(line_end[i].second));

    }
    glEnd();
}

void draw_move_indicator_line()
{
    glBegin(GL_LINES);
    glColor3f(1, 0, 0);
    glVertex2f(static_cast<GLfloat>(fromX), static_cast<GLfloat>(fromY));
    glVertex2f(static_cast<GLfloat>(toX), static_cast<GLfloat>(toY));
    glEnd();
}

void draw_team_red()
{
    glPointSize(20.0);
    glBegin(GL_POINTS);
    glColor3f(1, 0, 0);
    for (auto it = red_soldiers.begin(); it != red_soldiers.end(); it++)
        glVertex2f(static_cast<GLfloat>(it->first), static_cast<GLfloat>(it->second));
    glEnd();
}

void draw_team_blue()
{
    glPointSize(20.0);
    glBegin(GL_POINTS);
    glColor3f(0, 0, 1);
    for (auto it = blue_soldiers.begin(); it != blue_soldiers.end(); it++)
        glVertex2f(static_cast<GLfloat>(it->first), static_cast<GLfloat>(it->second));
    glEnd();
}

void draw_highlighted_point()
{
    glPointSize(20.0);
    glBegin(GL_POINTS);
    glColor3f(static_cast<GLfloat>(0.5), 0, 1);
    glVertex2f(static_cast<GLfloat>(hX), static_cast<GLfloat>(hY));
    glEnd();
}

void draw_suggested_points()
{
    glPointSize(10.0);
    glBegin(GL_POINTS);
    glColor3f(0.5, 0.0, 1.0);
    for (auto it = suggestions.begin(); it != suggestions.end(); it++)
        glVertex2f(static_cast<GLfloat>(it->suggest_x), static_cast<GLfloat>(it->suggest_y));
    glEnd();
}

void draw_text()
{
    int len;
    string str;

    if (turn == BLUE)
    {
        glColor3f(0.0, 0.0, 1.0);
        str = "            Your Turn...";
    }
    else if (turn == RED)
    {
        glColor3f(1.0, 0.0, 0.0);
        str = "       Opponent's Turn...";
    }
    glRasterPos2f(static_cast<GLfloat>(text_x), static_cast<GLfloat>(text_y));
    len = (int)str.size();

    //loop to display character by character
    for (int i = 0; i < len; i++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str[i]);
}

void draw_game_over()
{
    int len;
    string str1, str2;
    str1 = "          PLAY AGAIN";

    glColor3f(1.0, 1.0, 1.0);
    glBegin(GL_QUADS);

    for (int i = 0; i < 4; i++)
        glVertex2f(static_cast<GLfloat>(play_again[i].first), static_cast<GLfloat>(play_again[i].second));
    glEnd();

    glColor3f(0.0, 0.0, 0.0);
    glRasterPos2f(play_again_text_coordinate.first, play_again_text_coordinate.second);
    len = (int)str1.size();

    //loop to display character by character
    for (int i = 0; i < len; i++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str1[i]);

    glColor3f(0.0, 1.0, 0.0);

    if (game_won == RED)
        str2 = "           YOU LOSE";
    if (game_won == BLUE)
        str2 = "           YOU WIN";
    if (game_won == TIE)
        str2 = "                 TIE";

    glRasterPos2f(static_cast<GLfloat>(text_x), static_cast<GLfloat>(text_y));
    len = (int)str2.size();

    //loop to display character by character
    for (int i = 0; i < len; i++)
        glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, str2[i]);
}

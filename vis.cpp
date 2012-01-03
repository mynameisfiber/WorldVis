#include <math.h>
#include <iostream>
#include "json/json.h"

#include <vector>
#include <map>

#include <GL/gl.h>
#include <GL/freeglut.h>

#include "elements.h"

#define VERSION "0.5.1"

#define JSON_GET_STR(json_obj, field) (json_object_get_string(json_object_object_get(json_obj, field)))
#define JSON_FREE(json_obj) (json_object_put(json_obj))

//////////// PARAMETERS ////////////////
static int MIN_KEYWORD_COUNT   = 5;
static double KW_R             = 175;
static double KW_DTHETA        = M_PI/16 * sqrt(2);
static const double MIN_ALPHA  = 1e-6;
////////////////////////////////////////

void ReadInput();
void InitGraphics(int argc, char **argv);

std::map<std::string, Keyword>   keywords;
std::vector<Click>               clicks;
std::vector<Explosion>           explosions;
std::vector<Link>                links;

static int MAX_COUNT           = 1;
static double KW_THETA         = 0;
static std::string LAST_DECODE = "";

int main(int argc, char **argv)
{
    InitGraphics(argc, argv);

    return 0;
}

struct json_object *parse_json(std::string json_str)
{
    if (json_str.length() < 3) {
        fprintf(stderr, "ERR: empty json\n");
        return NULL;
    }

    return json_tokener_parse(json_str.c_str());
}


void ReadInput()
{
    if (!std::cin) return;

    std::string input;
    std::getline(std::cin, input);
    struct json_object *decode = parse_json(input);

    if (decode != NULL)
    {
        std::string cur_decode = std::string(JSON_GET_STR(decode, "i"));
        if (cur_decode != LAST_DECODE) {
            LAST_DECODE = cur_decode;

            const char *ll_str = JSON_GET_STR(decode, "ll");
            if (ll_str != NULL) {
                double x, y;
                sscanf(ll_str, "[ %lf, %lf ]", &y, &x);

                Click newclick;
                newclick.x      = {x, y};
                newclick.alpha  = 1.0;
                newclick.dalpha = 2.5e-4;
                clicks.push_back(newclick);

                Explosion newexp;
                newexp.x      = {x,y};
                newexp.r      = 0.0;
                newexp.dr     = 0.05;
                newexp.alpha  = 0.7;
                newexp.dalpha = 0.01;
                explosions.push_back(newexp);

                std::string cur_kw = JSON_GET_STR(decode, "e");
                if (keywords.find(cur_kw) != keywords.end()) {
                    double s, c;
                    sincos(keywords[cur_kw].theta, &s, &c);

                    keywords[cur_kw].count ++;
                    if (keywords[cur_kw].count > MAX_COUNT)
                        MAX_COUNT = keywords[cur_kw].count;
                    keywords[cur_kw].alpha = 1.0;
                    keywords[cur_kw].x     = { s * KW_R, 
                                               c * KW_R };
                } else {
                    double s, c;
                    sincos(KW_THETA, &s, &c);

                    Keyword keywd;
                    keywd.kw     = cur_kw;
                    keywd.x      = { s * KW_R, c * KW_R };
                    keywd.theta  = KW_THETA;
                    keywd.alpha  = 1.0;
                    keywd.dalpha = 2.5e-3;
                    keywd.count  = 1;
                    keywords[cur_kw] = keywd;

                    KW_THETA += KW_DTHETA;
                }
                keywords[cur_kw].dr = fmin( 0.5 * MAX_COUNT / keywords[cur_kw].count,
                                           10.0 );

                Link li;
                li.click  = &clicks.back();
                li.kw     = &keywords[cur_kw];
                li.alpha  = 0.25;
                li.dalpha = 0.01;
                links.push_back(li);
            }
        }
        JSON_FREE(decode);
    }
}



// Visualization procedures go here
// -----------------------------------------------------------------------------

static void DrawGLScene();
static void ResizeGLScene(int Width, int Height);
static void MousePressed(int button, int state, int x, int y);
static void KeyPressed(unsigned char key, int x, int y);
static void SpecialKeyPressed(int key, int x, int y);

static int WindowID;
static int WindowWidth    = 768;
static int WindowHeight   = 512;
static int FullScreenMode = 0;
static int IsRunning      = 0;
static int mouseButtons   = 0;
static int mouseOldX;
static int mouseOldY;

static float xTranslate = 0.0;
static float yTranslate = 0.0;
static float zTranslate = 4.5;
static float xRotate = 0.0;
static float yRotate = 0.0;
static float currentScale = 0.01;


void InitGraphics(int argc, char **argv)
{
    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
    glutInitWindowSize(WindowWidth, WindowHeight);
    glutInitWindowPosition(80, 80);
    WindowID = glutCreateWindow("World Vis");

    glutDisplayFunc       (DrawGLScene);
    glutIdleFunc          (DrawGLScene);
    glutReshapeFunc       (ResizeGLScene);
    glutKeyboardFunc      (KeyPressed);
    glutSpecialFunc       (SpecialKeyPressed);
    glutMouseFunc         (MousePressed);
    glEnable              (GL_BLEND);
    glBlendFunc           (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    glClearColor(0.2, 0.2, 0.1, 0.0);
    glClearDepth(1.0);
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)WindowWidth/(GLfloat)WindowHeight, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
    glScaled(currentScale, currentScale, 1);


    if (FullScreenMode) {
        glutFullScreen();
    }

    glutMainLoop();
}

static void DrawCircle(double x[2], double r, double alpha=1)
{
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0, 1.0, 1.0, alpha);
    glVertex3f(x[0], x[1], 0.0);
    for(double theta=0; theta<=2*M_PI; theta+=M_PI/24.)
    {
        double s, c;
        sincos(theta, &s, &c);
        glVertex3f(x[0] + s * r, x[1] + c * r, 0.0);
    }
    glEnd();
}

static void DrawTorus(double x[2], double r, double alpha=1)
{
    glBegin(GL_LINE_LOOP);
    glColor4f(1.0, 1.0, 1.0, alpha);
    for(double theta=0; theta<=2*M_PI; theta+=M_PI/24.)
    {
        double s, c;
        sincos(theta, &s, &c);
        glVertex3f(x[0] + s * r, x[1] + c * r, 0.0);
    }
    glEnd();
}

static void DrawText(std::string kw, double x[2], double theta, double alpha=1)
{
    glColor4f(1.0, 1.0, 1.0, alpha);
    glRasterPos3f(x[0], x[1], 0.0);
    glutBitmapString(GLUT_BITMAP_HELVETICA_12, (const unsigned char*) kw.c_str());

}

static void DrawLine(double x0[2], double x1[2], double alpha=1)
{
    glBegin(GL_LINES);
    glLineWidth(1);
    glColor4f(1.0, 1.0, 1.0, alpha);
    glVertex3f(x0[0], x0[1], 0.0);
    glVertex3f(x1[0], x1[1], 0.0);
    glEnd();
}

static void DrawPoint(double x[2], double alpha=1)
{
    DrawCircle(x, .35, alpha );
    //glBegin(GL_POINT);
    //glColor4f(1.0, 1.0, 1.0, alpha);
    //glVertex2f(x[0], x[1]);
    //glEnd();
}

static void DrawGLScene()
{
    ReadInput();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(-xTranslate, -yTranslate, -zTranslate);
    glScaled(currentScale, currentScale, currentScale);
    glRotatef(xRotate, 1, 0, 0);
    glRotatef(yRotate, 0, 1, 0);

    // First we draw the clicks
    for( auto click = clicks.begin(); !clicks.empty() && click != clicks.end(); click++)
    {
        if (click->alpha < MIN_ALPHA) {
            clicks.erase(click);
            continue;
        } else {
            DrawPoint( click->x, click->alpha );
            click->alpha -= click->dalpha;
        }
    }

    // Now the explosions
    for( auto explosion = explosions.begin() ; !explosions.empty() && explosion != explosions.end(); explosion++)
    {
        if (explosion->alpha < MIN_ALPHA) {
            explosions.erase(explosion);
            continue;
        } else {
            DrawTorus( explosion->x, explosion->r, explosion->alpha );
            explosion->alpha -= explosion->dalpha;
            explosion->r += explosion->dr;
        }
    }

    // Keyword time
    int new_max_count = 0;
    for ( auto kw_pair=keywords.begin() ; !keywords.empty() && kw_pair != keywords.end(); kw_pair++ ) 
    {
        auto kw = &(kw_pair->second);
        if (kw->alpha < MIN_ALPHA) {
            keywords.erase(kw_pair);
            continue;
        } else {
            if (kw->count >= MIN_KEYWORD_COUNT)
                DrawText( kw->kw, kw->x, kw->theta, kw->alpha );

            double s, c;
            sincos(kw->theta, &s, &c);
            kw->x[0] += kw->dr * s * MAX_COUNT / kw->count;
            kw->x[1] += kw->dr * c * MAX_COUNT / kw->count;

            kw->alpha -= kw->dalpha;

            new_max_count = (new_max_count < kw->count ? kw->count : new_max_count);
        }
    }
    MAX_COUNT = new_max_count;

    // Finally, we draw the lines!
    for ( auto li=links.begin() ; !links.empty() && li != links.end(); li++ ) 
    {
        if (li->alpha < MIN_ALPHA || li->click == NULL || li->kw == NULL) {
            links.erase(li);
            continue;
        } else {
            if (li->kw->count >= MIN_KEYWORD_COUNT)
                DrawLine(li->click->x, li->kw->x, li->alpha);

            li->alpha -= li->dalpha;
        }
    }

    glFlush();
    glutSwapBuffers();
}


void ResizeGLScene(int Width, int Height)
{
    if (Height==0) Height=1;
    glViewport(0, 0, Width, Height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(45.0f, (GLfloat)Width/(GLfloat)Height, 0.1f, 100.0f);
    glMatrixMode(GL_MODELVIEW);
}

void SpecialKeyPressed(int key, int x, int y)
{
    if (key == GLUT_KEY_LEFT) {
        xTranslate -= 5e-2;
    } else if (key == GLUT_KEY_RIGHT) {
        xTranslate += 5e-2;
    }

    else if (key == GLUT_KEY_UP) {
        yTranslate += 5e-2;
    } else if (key == GLUT_KEY_DOWN) {
        yTranslate -= 5e-2;
    }

    glutPostRedisplay();
}

void MousePressed(int button, int state, int x, int y)
{
    if      (state == GLUT_DOWN) mouseButtons |= 1<<button;
    else if (state == GLUT_UP  ) mouseButtons  = 0;

    if (mouseButtons == 0) {
        xRotate = mouseOldX - x;
        yRotate = mouseOldY - y;
    }

    mouseOldX = x;
    mouseOldY = y;
    glutPostRedisplay();
}


void KeyPressed(unsigned char key, int x, int y)
{
    static const int ESCAPE = 27;
    const int keynum = atoi((char*)&key);

    switch (key) {
        case ESCAPE:
        case 'q':
            glutDestroyWindow(WindowID);
            exit(0);
            break;

        case 'f':
            if (FullScreenMode) {
                glutReshapeWindow(WindowWidth, WindowHeight);
                glutPositionWindow(0,0);
                FullScreenMode = 0;
            }
            else {
                glutFullScreen();
                FullScreenMode = 1;
            }
            break;

        case '+':
            currentScale += 5e-4;
            break;

        case '-':
            currentScale = fabs(currentScale - 5e-4);
            break;

        case ',':
            MIN_KEYWORD_COUNT = (MIN_KEYWORD_COUNT == 0 ? 0 : MIN_KEYWORD_COUNT-1);
            printf("MIN_KEYWORD_COUNT: %d\n", MIN_KEYWORD_COUNT);
            break;

        case '.':
            MIN_KEYWORD_COUNT += 1;
            printf("MIN_KEYWORD_COUNT: %d\n", MIN_KEYWORD_COUNT);
            break;
    }
    glutPostRedisplay();
}

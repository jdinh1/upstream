//Jonathan Dinh
//cs335 Spring 2016 Homework-1
//4-9-2016
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
//
//. general animation framework
//. animation loop
//. object definition and movement
//. collision detection
//. mouse/keyboard interaction
//. object constructor
//. coding style
//. defined constants
//. use of static variables
//. dynamic memory allocation
//. simple opengl components
//. git
//
//elements we will add to program...
//. Game constructor
//. multiple particles
//. gravity7
//. collision detection
//. more objects
//
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <cstring>
#include <stdio.h>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include <stdlib.h>
#include "ppm.h"
#include "log.h"
extern "C" {
#include "fonts.h"
}

//macros
#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 800
#define random(a) (rand()%a)
#define GRAVITY 0.1
#define rnd() (((Flt)rand())/(Flt)RAND_MAX)
#define random(a) (rand()%a)
#define MakeVector(x, y, z, v) (v)[0]=(x),(v)[1]=(y),(v)[2]=(z)
#define VecCopy(a,b) (b)[0]=(a)[0];(b)[1]=(a)[1];(b)[2]=(a)[2]
#define VecDot(a,b)	((a)[0]*(b)[0]+(a)[1]*(b)[1]+(a)[2]*(b)[2])
#define VecSub(a,b,c) (c)[0]=(a)[0]-(b)[0]; \
                             (c)[1]=(a)[1]-(b)[1]; \
(c)[2]=(a)[2]-(b)[2]


//X Windows variables
Display *dpy;
Window win;
GLXContext glc;
GC gc;
typedef double Flt;
typedef double Vec[3];
typedef Flt	Matrix[4][4];

//Structures
struct Shape {
    float width, height;
    float radius;
    Vec center;
};

struct Lilypad {
    Shape s;
    Vec pos;
    Vec vel;
    struct Lilypad *next;
    struct Lilypad *prev;
    Lilypad() {
        next = NULL;
        prev = NULL;
    }
};

struct Circle {
    float radius;
    float newPosX;
    float newPosY;
    float jumpSpeed;
    float jumpSpeedMax;
    bool isJumping;
    Vec velocity;
    Vec center;
    int detail;
};

struct Game {
    int nlily;
    int timer;
    int maxtimer;
    int lilyspawnpoint; //y coordinate of where lilies spawn
    Lilypad *ihead;
    Circle c;
    Lilypad lilies[20];
    int n;
};

typedef struct t_bigfoot {
    Vec pos;
    Vec vel;
} Bigfoot;

//Global variables
Bigfoot bigfoot;
Ppmimage *bigfootImage=NULL;
Ppmimage *forestImage=NULL;
Ppmimage *forestTransImage=NULL;
Ppmimage *umbrellaImage=NULL;
GLuint bigfootTexture;
GLuint silhouetteTexture;
GLuint forestTexture;
GLuint forestTransTexture;
int showBigfoot=1;
int forest=1;
int silhouette=1;
int trees=1;
struct timespec timeStart, timeCurrent;
struct timespec timePause;
const double physicsRate = 1.0 / 30.0;
double physicsCountdown=0.0;
const double oobillion = 1.0 / 1e9;
double timeSpan=0.0;
unsigned int upause=0;

//Function prototypes
unsigned char *buildAlphaData(Ppmimage *img);
void init(void);
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_mouse(XEvent *e, Game *game);
void timeCopy(struct timespec *dest, struct timespec *source);
void movement(Game *game);
void render(Game *game);
void showMenu(int x, int y, const char* msg);
void screenUpdate(Game *game);
void deleteLily(Lilypad *node, Game *game);
void physics(void);
void moveBigfoot(void);
int xres=WINDOW_WIDTH, yres=WINDOW_HEIGHT;
int check_keys(XEvent *e, Game *game);
double timeDiff(struct timespec *start, struct timespec *end);

int main(void)
{
    int done=0;
    srand(time(NULL));
    //declare game object
    Game game;
    game.ihead = NULL;
    initXWindows();
    init_opengl();
    game.n=0;
    srand(time(0));	
    init();
    //initiate game variables
    game.nlily = 0;
    game.timer = 0;
    game.maxtimer = 50;
    game.ihead = NULL;
    game.lilyspawnpoint = WINDOW_HEIGHT + 15;
    game.c.radius = 15.0;
    game.c.center[0] = WINDOW_WIDTH/2.0;
    game.c.center[1] = 15;
    game.c.detail = 150;
    game.c.jumpSpeed = 0;
    game.c.jumpSpeedMax = 25;


    while(!done) {
        while(XPending(dpy)) {
            XEvent e;
            XNextEvent(dpy, &e);
            check_mouse(&e, &game);
            done = check_keys(&e, &game);
        }

        physics();
        movement(&game);
        render(&game);
        glXSwapBuffers(dpy, win);
    }
    cleanupXWindows();
    return 0;
}

double timeDiff(struct timespec *start, struct timespec *end) 
{
    return (double)(end->tv_sec - start->tv_sec ) +
        (double)(end->tv_nsec - start->tv_nsec) * oobillion;
}

void timeCopy(struct timespec *dest, struct timespec *source) 
{
    memcpy(dest, source, sizeof(struct timespec));
}

void set_title(void)
{
    //Set the window title bar.
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "Testing framework for Upstream");
}

void cleanupXWindows(void) 
{
    //do not change
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

void initXWindows(void) 
{
    //do not change
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    int w=WINDOW_WIDTH, h=WINDOW_HEIGHT;
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        std::cout << "\n\tcannot connect to X server\n" << std::endl;
        exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if(vi == NULL) {
        std::cout << "\n\tno appropriate visual found\n" << std::endl;
        exit(EXIT_FAILURE);
    } 
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
            InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    set_title();
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
}

void init() {
    MakeVector(-150.0,180.0,0.0, bigfoot.pos);
    MakeVector(6.0,0.0,0.0, bigfoot.vel);
}

void init_opengl(void)
{

    //OpenGL initialization
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //Set 2D mode (no perspective)
    glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    //Set the screen background color
    glClearColor(0.1, 0.1, 0.1, 1.0);
    //Do this to allow fonts
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);

    //bigfoot
    bigfootImage     = ppm6GetImage("./images/bigfoot.ppm");
    forestImage      = ppm6GetImage("./images/forest.ppm");
    forestTransImage = ppm6GetImage("./images/forestTrans.ppm");
    glGenTextures(1, &bigfootTexture);
    glGenTextures(1, &silhouetteTexture);
    glGenTextures(1, &forestTexture);

    int w = bigfootImage->width;
    int h = bigfootImage->height;
    //
    glBindTexture(GL_TEXTURE_2D, bigfootTexture);
    //
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, bigfootImage->data);
    //-------------------------------------------------------------------------
    //
    //silhouette
    //this is similar to a sprite graphic
    //
    glBindTexture(GL_TEXTURE_2D, silhouetteTexture);
    //
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    //
    //must build a new set of data...
    unsigned char *silhouetteData = buildAlphaData(bigfootImage);	
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, silhouetteData);
    free(silhouetteData);

    //forest
    glBindTexture(GL_TEXTURE_2D, forestTexture);
    //
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3,
            forestImage->width, forestImage->height,
            0, GL_RGB, GL_UNSIGNED_BYTE, forestImage->data);
    //-------------------------------------------------------------------------
    //
    //forest transparent part
    //
    glBindTexture(GL_TEXTURE_2D, forestTransTexture);
    //
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    //
    //must build a new set of data...
    w = forestTransImage->width;
    h = forestTransImage->height;
    unsigned char *ftData = buildAlphaData(forestTransImage);	
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, ftData);
    free(ftData);

}


void check_mouse(XEvent *e, Game *game)
{
    static int savex = 0;
    static int savey = 0;
    static int n = 0;

    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {
            //Left button was pressed
            if (!game->c.isJumping) {
                game->c.isJumping = true;
                game->c.velocity[1] = 12.0;
            }
            return;
        }
        if (e->xbutton.button==3) {
            //Right button was pressed

            return;
        }
    }
    //Did the mouse move?
    if (savex != e->xbutton.x || savey != e->xbutton.y) {
        savex = e->xbutton.x;
        savey = WINDOW_HEIGHT - e->xbutton.y;

        if (++n < 10)
            return;
        game->c.newPosX = savex;
        game->c.newPosY = savey;
    }
}

int check_keys(XEvent *e, Game *game)
{
    //Was there input from the keyboard?
    if (e->type == KeyPress) {
        int key = XLookupKeysym(&e->xkey, 0);
        switch (key) {
	    case XK_f:
		forest ^= 1;
		break;
            case XK_b:
		showBigfoot ^= 1;
                break;
            case XK_Escape:
                return 1;
        }
        //You may check other keys here.

    }
    return 0;
}

void physics(void)
{
    if (showBigfoot)
        moveBigfoot();
}

void moveBigfoot()
{
    //move bigfoot...
    int addgrav = 1;
    //Update position
    bigfoot.pos[0] += bigfoot.vel[0];
    bigfoot.pos[1] += bigfoot.vel[1];
    //Check for collision with window edges
    if ((bigfoot.pos[0] < -140.0 && bigfoot.vel[0] < 0.0) ||
            (bigfoot.pos[0] >= (float)xres+140.0 && bigfoot.vel[0] > 0.0)) {
        bigfoot.vel[0] = -bigfoot.vel[0];
        addgrav = 0;
    }
    if ((bigfoot.pos[1] < 150.0 && bigfoot.vel[1] < 0.0) ||
            (bigfoot.pos[1] >= (float)yres && bigfoot.vel[1] > 0.0)) {
        bigfoot.vel[1] = -bigfoot.vel[1];
        addgrav = 0;
    }
    //Gravity
    if (addgrav)
        bigfoot.vel[1] -= 0.75;
}

void createLily(const int n, Game *game)
{
    //if (game->nlily >= 1) return;
    for (int i =0; i < n; i++) {
        Lilypad *node = new Lilypad;
        if (node == NULL) {
            //Log("error allocating node.\n");
            exit(EXIT_FAILURE);
        }
        node->prev = NULL;
        node->next = NULL;
        int random = rand() % (WINDOW_WIDTH-80) + 40;
        node->pos[0] = WINDOW_WIDTH - random;
        node->pos[1] = game->lilyspawnpoint;
        node->vel[1] = -2.0f;
        node->s.width = 10.0;
        node->s.height = 10.0;
        node->next = game->ihead;
        if (game->ihead != NULL) {
            game->ihead->prev = node;		
        }
        game->ihead = node;
        game->nlily++;
    }
}

void checkLilies(Game *game)
{
    //game timer for when to spawn new lily
    game->timer++;
    if (game->timer >= game->maxtimer) {
        createLily(1,game);
        game->nlily++;
        game->timer = 0;
    }
    //lilies falling down
    Lilypad *node = game->ihead;
    while (node) {
        node->pos[1] += node->vel[1];
        //delete lily if it falls below certain height
        if (node->pos[1] <= 60.0) {
            deleteLily(node, game);		
        }	
        node = node->next;
    }
}

void deleteLily(Lilypad *node, Game *game) 
{
    if (node) {
        if (node->next == NULL && node->prev == NULL) {		
            game->ihead = NULL;
            //delete(node);
            //node = NULL;
        } else if (node->next && node->prev) { 	
            node->prev->next = node->next;
            node->next->prev = node->prev;
            delete(node);
            node = NULL;
        } else if (node->prev == NULL && node->next) {		
            game->ihead = node->next;
            node->next->prev = NULL;
            delete(node);
            node = NULL;
        } else if (node->next == NULL && node->prev) {		
            node->prev->next = NULL;
            delete(node);
            node = NULL;
        }	
    }
    game->nlily--;

}

void drawLilies(Game *game)
{	
    Lilypad *node = game->ihead;
    while (node) {
        //glPushMatrix();
        //glTranslated(node->pos[0],node->pos[1],node->pos[2]);
        //std::cout << node->pos.x << " " << node->pos.y << " " << n++ << std::endl;
        float w = node->s.width;
        float h = node->s.height;
        //Vec *c = *node->pos;
        //std::cout << c->x << " " << c->y << " " << std::endl;
        glBegin(GL_QUADS);
        glVertex2i(node->pos[0]-w, node->pos[1]-h);
        glVertex2i(node->pos[0]-w, node->pos[1]+h);
        glVertex2i(node->pos[0]+w, node->pos[1]+h);
        glVertex2i(node->pos[0]+w, node->pos[1]-h);
        glEnd();
        glPopMatrix();
        node = node->next;
    }
}

void updateBallPosition(Game *game) {

    if (game->c.isJumping) {
        game->c.center[1] += game->c.velocity[1];
        game->c.velocity[1] -= 0.6;
        if (game->c.center[1] <= 0) {
            game->c.isJumping = false;
            game->c.center[1] = 15;
        }
    }
    if (game->c.center[0] == game->c.newPosX && 
            game->c.center[1] == game->c.newPosY) 
        return;
    if (game->c.center[0] < game->c.newPosX) {
        if (game->c.isJumping)
            game->c.center[0] += 15.0;
        else
            game->c.center[0] += 10.0;
        if (game->c.center[0] > game->c.newPosX)
            game->c.center[0] = game->c.newPosX;	
    } else if (game->c.center[0] > game->c.newPosX) {
        if (game->c.isJumping)
            game->c.center[0] -= 15.0;
        else
            game->c.center[0] -= 10.0;
        if (game->c.center[0] < game->c.newPosX)
            game->c.center[0] = game->c.newPosX;
    }
    //check for ball's collision with lilies	
    Lilypad *node = game->ihead;
    while (node) {
        float d1 = node->pos[1] - game->c.center[1];
        float d2 = node->pos[0] - game->c.center[0];
        float dist = sqrt(d1*d1 + d2*d2);
        if (dist <= game->c.radius+15.0) {
            //rejump the ball
            game->c.isJumping = true;
            game->c.velocity[1] = 12.0;
            deleteLily(node,game);
        }	
        node = node->next;	
    }

}

void screenUpdate(Game *game)
{
    //move screen along with ball if pass a certain height
    /*if (game->c.center.y >= WINDOW_HEIGHT - 150) {
      game->c.center.y = WINDOW_HEIGHT - 65;
    //game->c.center.y += 25;
    game->liliespawnpoint += 25;
    game->maxtimer =12;
    }
    else
    game->lilyspawnpoint = WINDOW_HEIGHT + 15;
    game->maxtimer =50;*/
}

unsigned char *buildAlphaData(Ppmimage *img)
{
    //add 4th component to RGB stream...
    int i;
    int a,b,c;
    unsigned char *newdata, *ptr;
    unsigned char *data = (unsigned char *)img->data;
    newdata = (unsigned char *)malloc(img->width * img->height * 4);
    ptr = newdata;
    for (i=0; i<img->width * img->height * 3; i+=3) {
        a = *(data+0);
        b = *(data+1);
        c = *(data+2);
        *(ptr+0) = a;
        *(ptr+1) = b;
        *(ptr+2) = c;
        //get largest color component...
        //*(ptr+3) = (unsigned char)((
        //		(int)*(ptr+0) +
        //		(int)*(ptr+1) +
        //		(int)*(ptr+2)) / 3);
        //d = a;
        //if (b >= a && b >= c) d = b;
        //if (c >= a && c >= b) d = c;
        //*(ptr+3) = d;
        *(ptr+3) = (a|b|c);
        ptr += 4;
        data += 3;
    }
    return newdata;
}

void movement(Game *game)
{
    if (game->n <= 0)
        return;
}

void drawCircle(float x, float y, float radius, int detail)
{
    float radian = 2.0 * 3.14;
    glPushMatrix();
    glColor3ub(90,140,90);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= detail; i++) {
        glVertex2f(
                x + (radius * cos(i * radian / detail)),
                y + (radius * sin(i * radian / detail))
                );

    }
    glEnd();
    glPopMatrix();
}

void render(Game *game)
{
    glClear(GL_COLOR_BUFFER_BIT);
    updateBallPosition(game);

    //Images
    float wid = 120.0f;
    glColor3f(1.0, 1.0, 1.0);
    if (forest) {
        glBindTexture(GL_TEXTURE_2D, forestTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
        glEnd();
    }
    if (showBigfoot) {
        glPushMatrix();
        glTranslatef(bigfoot.pos[0], bigfoot.pos[1], bigfoot.pos[2]);
        if (!silhouette) {
            glBindTexture(GL_TEXTURE_2D, bigfootTexture);
        } else {
            glBindTexture(GL_TEXTURE_2D, silhouetteTexture);
            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0.0f);
            glColor4ub(255,255,255,255);
        }
        glBegin(GL_QUADS);
        if (bigfoot.vel[0] > 0.0) {
            glTexCoord2f(0.0f, 1.0f); glVertex2i(-wid,-wid);
            glTexCoord2f(0.0f, 0.0f); glVertex2i(-wid, wid);
            glTexCoord2f(1.0f, 0.0f); glVertex2i( wid, wid);
            glTexCoord2f(1.0f, 1.0f); glVertex2i( wid,-wid);
        } else {
            glTexCoord2f(1.0f, 1.0f); glVertex2i(-wid,-wid);
            glTexCoord2f(1.0f, 0.0f); glVertex2i(-wid, wid);
            glTexCoord2f(0.0f, 0.0f); glVertex2i( wid, wid);
            glTexCoord2f(0.0f, 1.0f); glVertex2i( wid,-wid);
        }
        glEnd();
        glPopMatrix();
        //
        if (trees && silhouette) {
            glBindTexture(GL_TEXTURE_2D, forestTransTexture);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
            glTexCoord2f(0.0f, 0.0f); glVertex2i(0, yres);
            glTexCoord2f(1.0f, 0.0f); glVertex2i(xres, yres);
            glTexCoord2f(1.0f, 1.0f); glVertex2i(xres, 0);
            glEnd();
        }
        glDisable(GL_ALPHA_TEST);
    }

    //glDisable(GL_TEXTURE_2D);
    glColor3ub(90,140,90);
    checkLilies(game);
    drawLilies(game); 
    screenUpdate(game);
    drawCircle(game->c.center[0], game->c.center[1], 
               game->c.radius, game->c.detail);

}



////////////////////////////////////////////////////
// anim.cpp version 4.1
// Template code for drawing an articulated figure.
// CS 174A 
////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#include "GL/glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#ifdef _WIN32
#include "GL/freeglut.h"
#else
#include <GLUT/glut.h>
#endif

#include "Ball.h"
#include "FrameSaver.h"
#include "Timer.h"
#include "Shapes.h"
#include "tga.h"

#include "Angel/Angel.h"

#ifdef __APPLE__
#define glutInitContextVersion(a,b)
#define glutInitContextProfile(a)
#define glewExperimental int glewExperimentalAPPLE
#define glewInit()
#endif

/////////////////////////////////////////////////////
// These are global variables
//
//
// Add additional ones if you need,
// especially for animation
//////////////////////////////////////////////////////

FrameSaver FrSaver ;
Timer TM ;

BallData *Arcball = NULL ;
int Width = 800;
int Height = 800 ;
int Button = -1 ;
float Zoom = 1 ;
int PrevY = 0 ;

int Animate = 0 ;
int Recording = 0 ;

void resetArcball() ;
void save_image();
void instructions();
void set_colour(float r, float g, float b) ;

const int STRLEN = 100;
typedef char STR[STRLEN];

#define PI 3.1415926535897
#define X 0
#define Y 1
#define Z 2

//texture
GLuint texture_cube;
GLuint texture_earth;

// Structs that hold the Vertex Array Object index and number of vertices of each shape.
ShapeData cubeData;
ShapeData sphereData;
ShapeData coneData;
ShapeData cylData;

// Matrix stack that can be used to push and pop the modelview matrix.
class MatrixStack {
    int    _index;
    int    _size;
    mat4*  _matrices;

   public:
    MatrixStack( int numMatrices = 32 ):_index(0), _size(numMatrices)
        { _matrices = new mat4[numMatrices]; }

    ~MatrixStack()
	{ delete[]_matrices; }

    void push( const mat4& m ) {
        assert( _index + 1 < _size );
        _matrices[_index++] = m;
    }

    mat4& pop( void ) {
        assert( _index - 1 >= 0 );
        _index--;
        return _matrices[_index];
    }

	//peek function, returns the matrix at the top of the stack without popping it
	mat4& peek( void ) {
		assert( _index - 1 >= 0 );
		return _matrices[_index-1];
	}
};

MatrixStack  mvstack;
mat4         model_view;
GLint        uModelView, uProjection, uView;
GLint        uAmbient, uDiffuse, uSpecular, uLightPos, uShininess;
GLint        uTex, uEnableTex;

// The eye point and look-at point.
// Currently unused. Use to control a camera with LookAt().
Angel::vec4 eye(0, 0.0, 50.0,1.0);
Angel::vec4 ref(0.0, 0.0, 0.0,1.0);
Angel::vec4 up(0.0,1.0,0.0,0.0);

double TIME = 0.0 ;


/////////////////////////////////////////////////////
//    PROC: drawCylinder()
//    DOES: this function 
//          render a solid cylinder  oriented along the Z axis. Both bases are of radius 1. 
//          The bases of the cylinder are placed at Z = 0, and at Z = 1.
//
//          
// Don't change.
//////////////////////////////////////////////////////
void drawCylinder(void)
{
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( cylData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cylData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawCone()
//    DOES: this function 
//          render a solid cone oriented along the Z axis with base radius 1. 
//          The base of the cone is placed at Z = 0, and the top at Z = 1. 
//         
// Don't change.
//////////////////////////////////////////////////////
void drawCone(void)
{
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( coneData.vao );
    glDrawArrays( GL_TRIANGLES, 0, coneData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawCube()
//    DOES: this function draws a cube with dimensions 1,1,1
//          centered around the origin.
// 
// Don't change.
//////////////////////////////////////////////////////
void drawCube(void)
{
    glBindTexture( GL_TEXTURE_2D, texture_cube );
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( cubeData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cubeData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawSphere()
//    DOES: this function draws a sphere with radius 1
//          centered around the origin.
// 
// Don't change.
//////////////////////////////////////////////////////
void drawSphere(void)
{
    glBindTexture( GL_TEXTURE_2D, texture_earth);
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( sphereData.vao );
    glDrawArrays( GL_TRIANGLES, 0, sphereData.numVertices );
}


void resetArcball()
{
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}


/*********************************************************
 PROC: set_colour();
 DOES: sets all material properties to the given colour
 -- don't change
 **********************************************************/

void set_colour(float r, float g, float b)
{
    float ambient  = 0.2f;
    float diffuse  = 0.6f;
    float specular = 0.2f;
    glUniform4f(uAmbient,  ambient*r,  ambient*g,  ambient*b,  1.0f);
    glUniform4f(uDiffuse,  diffuse*r,  diffuse*g,  diffuse*b,  1.0f);
    glUniform4f(uSpecular, specular*r, specular*g, specular*b, 1.0f);
}


/*********************************************************
 PROC: instructions()
 DOES: display instruction in the console window.
 -- No need to change

 **********************************************************/
void instructions()
{
    printf("Press:\n");
    printf("  s to save the image\n");
    printf("  r to restore the original view.\n") ;
    printf("  0 to set it to the zero state.\n") ;
    printf("  a to toggle the animation.\n") ;
    printf("  m to toggle frame dumping.\n") ;
    printf("  q to quit.\n");
}


/*********************************************************
 PROC: myinit()
 DOES: performs most of the OpenGL intialization
 -- change these with care, if you must.
 
 **********************************************************/
void myinit(void)
{
    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram(program);
    
    // Generate vertex arrays for geometric shapes
    generateCube(program, &cubeData);
    generateSphere(program, &sphereData);
    generateCone(program, &coneData);
    generateCylinder(program, &cylData);
    
    uModelView  = glGetUniformLocation( program, "ModelView"  );
    uProjection = glGetUniformLocation( program, "Projection" );
    uView       = glGetUniformLocation( program, "View"       );
    
	/////////////////////////////////////////////////////////////////////
	//background color
	/////////////////////////////////////////////////////////////////////
    glClearColor( 0.5, 0.6, 0.7, 1.0 ); 
    
    uAmbient   = glGetUniformLocation( program, "AmbientProduct"  );
    uDiffuse   = glGetUniformLocation( program, "DiffuseProduct"  );
    uSpecular  = glGetUniformLocation( program, "SpecularProduct" );
    uLightPos  = glGetUniformLocation( program, "LightPosition"   );
    uShininess = glGetUniformLocation( program, "Shininess"       );
    uTex       = glGetUniformLocation( program, "Tex"             );
    uEnableTex = glGetUniformLocation( program, "EnableTex"       );
    
    glUniform4f(uAmbient,    0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uDiffuse,    0.6f,  0.6f,  0.6f, 1.0f);
    glUniform4f(uSpecular,   0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uLightPos,  15.0f, 15.0f, 30.0f, 0.0f);
    glUniform1f(uShininess, 100.0f);
    
    glEnable(GL_DEPTH_TEST);
    
    Arcball = new BallData;
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}


//////////////////////////////////////////////////////
//    PROC: myKey()
//    DOES: this function gets caled for any keypresses
// 
//////////////////////////////////////////////////////
void myKey(unsigned char key, int x, int y)
{
    float time ;
    switch (key) {
        case 'q':
        case 27:
            exit(0); 
        case 's':
            FrSaver.DumpPPM(Width,Height) ;
            break;
        case 'r':
            resetArcball() ;
            glutPostRedisplay() ;
            break ;
        case 'a': // togle animation
            Animate = 1 - Animate ;
            // reset the timer to point to the current time		
            time = TM.GetElapsedTime() ;
            TM.Reset() ;
            // printf("Elapsed time %f\n", time) ;
            break ;
        case '0':
            //reset your object
            break ;
        case 'm':
            if( Recording == 1 )
            {
                printf("Frame recording disabled.\n") ;
                Recording = 0 ;
            }
            else
            {
                printf("Frame recording enabled.\n") ;
                Recording = 1  ;
            }
            FrSaver.Toggle(Width);
            break ;
        case 'h':
        case '?':
            instructions();
            break;
    }
    glutPostRedisplay() ;

}


/**********************************************
 PROC: myMouseCB()
 DOES: handles the mouse button interaction
 
 -- don't change
 **********************************************************/
void myMouseCB(int button, int state, int x, int y)
{
    Button = button ;
    if( Button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
        HVect arcball_coords;
        arcball_coords.x = 2.0*(float)x/(float)Width-1.0;
        arcball_coords.y = -2.0*(float)y/(float)Height+1.0;
        Ball_Mouse(Arcball, arcball_coords) ;
        Ball_Update(Arcball);
        Ball_BeginDrag(Arcball);

    }
    if( Button == GLUT_LEFT_BUTTON && state == GLUT_UP )
    {
        Ball_EndDrag(Arcball);
        Button = -1 ;
    }
    if( Button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN )
    {
        PrevY = y ;
    }


    // Tell the system to redraw the window
    glutPostRedisplay() ;
}


/**********************************************
 PROC: myMotionCB()
 DOES: handles the mouse motion interaction
 
 -- don't change
 **********************************************************/
void myMotionCB(int x, int y)
{
    if( Button == GLUT_LEFT_BUTTON )
    {
        HVect arcball_coords;
        arcball_coords.x = 2.0*(float)x/(float)Width - 1.0 ;
        arcball_coords.y = -2.0*(float)y/(float)Height + 1.0 ;
        Ball_Mouse(Arcball,arcball_coords);
        Ball_Update(Arcball);
        glutPostRedisplay() ;
    }
    else if( Button == GLUT_RIGHT_BUTTON )
    {
        if( y - PrevY > 0 )
            Zoom  = Zoom * 1.03 ;
        else 
            Zoom  = Zoom * 0.97 ;
        PrevY = y ;
        glutPostRedisplay() ;
    }
}


/**********************************************
 PROC: myReshape()
 DOES: handles the window being resized
 
 -- don't change
 **********************************************************/
void myReshape(int w, int h)
{
    Width = w;
    Height = h;
    
    glViewport(0, 0, w, h);
    
    mat4 projection = Perspective(50.0f, (float)w/(float)h, 1.0f, 1000.0f);
    glUniformMatrix4fv( uProjection, 1, GL_TRUE, projection );
}


/*********************************************************
 **********************************************************
 **********************************************************
 
 PROC: display()
 DOES: this gets called by the event handler to draw the scene
       so this is where you need to build your BEE
 
 MAKE YOUR CHANGES AND ADDITIONS HERE
 
 ** Add other procedures, such as drawLegs
 *** Use a hierarchical approach
 
 **********************************************************
 **********************************************************
 **********************************************************/

//global matrices to hold position of various models
mat4 position;
mat4 position2;
mat4 leftlegpos;
mat4 rightlegpos;
mat4 rightarmpos;

void display(void)
{
    // Clear the screen with the background colour (set in myinit)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    mat4 model_trans(1.0f);
    mat4 view_trans(1.0f);
    
    view_trans *= Translate(0.0f, 0.0f, -15.0f); //the same effect as zoom out
    
    // below deals with zoom in/out by mouse
    HMatrix r;
    Ball_Value(Arcball,r);
    mat4 mat_arcball_rot(
                         r[0][0], r[0][1], r[0][2], r[0][3],
                         r[1][0], r[1][1], r[1][2], r[1][3],
                         r[2][0], r[2][1], r[2][2], r[2][3],
                         r[3][0], r[3][1], r[3][2], r[3][3]);
    view_trans *= mat_arcball_rot;
    view_trans *= Scale(Zoom);
    
    glUniformMatrix4fv( uView, 1, GL_TRUE, model_view );
    
    mvstack.push(model_trans);//push, now identity is on the stack
    
   
/**************************************************************
   Your drawing/modeling starts here
***************************************************************/
    
///////////////////////////////////////////////////////////////////////
//Implementing the ground
///////////////////////////////////////////////////////////////////////

	//model ground
	vec4 eye(-4.0,4.0,20.0,1.0);
	vec4 look(2.0,0.0,2.0,1.0);
	vec4 up(0.0,1.0,0.0,0.0);
	view_trans = LookAt(eye,look,up);

	

	////////////////////////////////////////////////////
	//VIEW TRANSFORMATIONS
	////////////////////////////////////////////////////
	
	//first 5 seconds
	if(TIME<5.0)
		view_trans *= RotateXYZ(0.0,10*TIME, 0.0);

	else if(5.0 <= TIME && TIME < 10.0){
		eye = vec4(0.0,0.0,1.0,1.0);
		look = vec4(-20.0,-30.0,5.0,1.0);
		view_trans = LookAt(eye,look,up);
		view_trans *= Translate(0.5, -1.0, -1.5);
	}
	else if(TIME >= 10.0 && TIME < 15.0){
		eye = vec4(-2.5,0.1,4.6,1.0);
		look = vec4(-10.0,10.0,-15.0,1.0);
		up = vec4(0.0,1.0,0.0,0.0);
		view_trans = LookAt(eye,look,up);
	}
	else if(TIME >= 15.0 && TIME < 21.5){
		GLfloat time = TIME - 15.0;
		GLfloat time2 = time/2;
		eye = vec4(0.0, 8.0, 20.0, 1.0);
		look = vec4(0.0, 0.0, 0.0, 1.0);
		up = vec4(0.0, 1.0, 0.0, 0.0);
		view_trans = LookAt(eye,look,up);
		if(TIME <21.0)
		view_trans *= RotateXYZ(0.0, 60*time ,0.0);
	}

	else if(TIME >= 21.5 && TIME < 28.3){
		eye = vec4(5.0, 2.0, -1.0, 1.0);
		look = vec4(-10.0, 0.0, 5.0, 1.0);
		up = vec4(0.0, 1.0, 0.0, 0.0);
		view_trans = LookAt(eye,look,up);
	}
	else if(TIME >= 28.3 && TIME < 29.5){
		GLfloat time = TIME - 28.3;
		eye = vec4(2*sin(50*time)*(time-1.2), 30.0, 2.0, 1.0);
		look = vec4(sin(50*time)*(time-1.2), 0.0, -1.0, 1.0);
		up = vec4(0.0, 0.0, -1.0, 0.0);
		view_trans = LookAt(eye,look,up);
	}
	else if(TIME >= 29.5 && TIME < 34.0){
		eye = vec4(-0.5, 1.0, 0.5, 1.0);
		look = vec4(-2.0, 2.0, 5.0, 1.0);
		up = vec4(0.0, 1.0, 0.0, 0.0);
		view_trans = LookAt(eye,look,up);
		
	}
	else if(TIME >= 34.0){
		if(TIME >= 40.7 && TIME < 41.2){
			GLfloat time = TIME - 40.7;
			eye = vec4(-3.0, 1.0 + 0.5*sin(40*time), 9.0, 1.0);
			look = vec4(2.0, 2.0, -6.0, 1.0);
			up = vec4(0.0, 1.0, 0.0, 0.0);
			view_trans = LookAt(eye,look,up);
		}
		else{
			eye = vec4(-3.0, 1.0, 9.0, 1.0);
			look = vec4(2.0, 2.0, -6.0, 1.0);
			up = vec4(0.0, 1.0, 0.0, 0.0);
			view_trans = LookAt(eye,look,up);
		}
	}
	

	/////////////////////////////////////////////////////////
	//MODELLING THE OBJECTS
	////////////////////////////////////////////////////////

	/////GROUND//////

	model_trans = mat4(1.0);
	model_trans *= Scale(50.0, 1.0, 50.0);
	view_trans *= Translate(0.0, -1.0, 0.0);
	model_view = view_trans * model_trans;
	set_colour(0.1, 0.4, 0.1);
	drawCube();
	
	/////TREE/////
	model_trans = mvstack.pop();
	model_trans *= Translate(0.0, 1.25, 0.0);
	mvstack.push(model_trans);
	model_trans *= Scale(1.0, 1.5, 1.0);
	model_view = view_trans * model_trans;
	set_colour(0.5, 0.4, 0.2);

	//tree segment 1
	drawCube();

	model_trans = mvstack.pop();
	model_trans *= Translate(0.0, 2.75, 0.0);

	//movements of the tree
	if(TIME >= 36.0 && TIME < 40.7){
		GLfloat time = TIME - 36.0;
		model_trans *= RotateXYZ(0.0,-20.0,0.0);
		model_trans *= Translate(0.0, -1.5, 0.0);
		model_trans *= RotateXYZ((time)*(time)*time,0.0,0.0);
		model_trans *= Translate(0.0, 1.5, 0.0);
		model_trans *= RotateXYZ(0.0,20.0,0.0);
	}
	if(TIME >= 40.7){
		model_trans *= RotateXYZ(0.0,-20.0,0.0);
		model_trans *= Translate(0.0, -1.5, 0.0);
		model_trans *= RotateXYZ(4.7*4.7*4.7,0.0,0.0);
		model_trans *= Translate(0.0, 1.5, 0.0);
		model_trans *= RotateXYZ(0.0,20.0,0.0);
	}

	mvstack.push(model_trans);
	model_trans *= Scale(1.0, 4.5, 1.0);
	model_view = view_trans * model_trans;
	//tree segment 2
	drawCube();

	
	model_trans = mvstack.pop();
	model_trans *= Translate(0.0, 3.0, 0.0);
	
	mvstack.push(model_trans);
	model_trans *= RotateXYZ(90.0, 0.0, 0.0);
	model_trans *= Scale(3.0);
	model_view = view_trans * model_trans;
	set_colour(0.2, 0.6, 0.2);
	//tree segment 3
	drawCone();

	model_trans = mvstack.pop();
	model_trans *= Translate(0.0, 2.5, 0.0);
	model_trans *= RotateXYZ(90.0, 0.0, 0.0);
	model_trans *= Scale(2.0);
	model_view = view_trans* model_trans;
	//tree segment 4
	drawCone();


	/////AXE ON THE GROUND/////

	//movements of the axe
	if(TIME < 10.0){
	model_trans = mat4(1.0);
	
	model_trans *= Translate(-2.0, 0.5, 3.0);
	model_trans *= RotateXYZ(0.0,30.0,0.0);
	mvstack.push(model_trans);
	model_trans *= Scale(0.8, 0.05, 0.05);
	model_trans *= RotateXYZ(0.0,90.0,0.0);
	
	model_view = view_trans * model_trans;
	set_colour(0.4,0.4,0.2);
	drawCylinder();

	model_trans = mvstack.pop();
	model_trans *= Translate(-0.5, 0.0, -0.2);
	model_trans *= Scale(0.4,0.05,0.5);
	model_view = view_trans * model_trans;
	set_colour(0.75,0.75,0.75);
	drawCube();
	}
	
	/////PICKING UP THE AXE/////

	if(5.0 <= TIME && 10.0 > TIME){
	model_trans = mat4(1.0);
	model_trans *= Translate(-2.0, 1.5, 3.0);
	if(TIME < 9.0){
		model_trans *= Translate(0.0,-3*TIME +26.0,0.0);
	}
	model_trans *= Scale(0.1,1.5,0.1);
	model_trans *= RotateXYZ(90.0,0.0,0.0);

	
	model_view = view_trans * model_trans;
	set_colour(0.8,0.7,0.4);
	drawCylinder();
	}


	/////DRAWING THE PERSON/////

	
	if(TIME >=10.0){
	model_trans = mat4(1.0);
	model_trans *= Translate(-3.0, 1.8, 3.0);
	
	if(TIME >=10.0 && TIME < 11.0){
		model_trans *= Translate(0.0,-0.4,0.0);
		model_trans *= RotateXYZ(85.0,0.0,0.0);
	
	}
	if(TIME >=11.0 && TIME < 12.0){
		GLfloat time = TIME-11.0;
		model_trans *= Translate(0.0, -(0.4 - time*0.4),0.0);
		model_trans *= RotateXYZ(85.0 - time*85,0.0,0.0);
	}
	if(TIME >=14.5 && TIME < 15.0){
		GLfloat time = TIME-14.5;
		model_trans *= Translate(time*0.5,0.0,0.0);
		model_trans *= RotateXYZ(0.0,time*80,0.0);
	}
	if(TIME >=15.0 && TIME < 25.0){
		model_trans *= Translate(-2.0, 0.0, -2.0);
		if(TIME >= 20.0){
			model_trans *= Translate(-(0-5.0*0.35)*(4.0-5.0*0.35),0.0,-(0.0-1.0*0.3)*(3.1-1.0*0.3));
			model_trans *= RotateXYZ(0.0,(5.0*17+90)-30,0.0);
			position = model_trans;
		}
		else{
		
		GLfloat time = TIME-15.0;
		GLfloat time2 = 6.0-time;
		model_trans *= Translate(-(0-time*0.35)*(4.0-time*0.35),0.0,-(0.0-time2*0.3)*(3.1-time2*0.3));
		model_trans *= RotateXYZ(0.0,(time*17+90)-30,0.0);
		
		}
		
	}
	if(TIME >= 25.0 && TIME < 27.0){
		GLfloat time = TIME - 25.0;
		model_trans = position;
		model_trans *= Translate(0.0*time, 0.0, 0.1*time);
		model_trans *= RotateXYZ(5*time, -30*time, 0.0);
		position2 = model_trans;
	}
	if(TIME >=27.0 && TIME < 28.0){
		model_trans = position2;
	}
	if(TIME >=28.0 && TIME < 28.3){
		GLfloat time = TIME - 28.0;
		model_trans = position2;
		model_trans *= RotateXYZ(0.0, 350*time, 0.0);
	}
	if(TIME >= 28.3 && TIME < 29.5){
		model_trans = position2;
	}
	if(TIME >= 29.5 && TIME < 39.0){
		GLfloat time = TIME - 30.5;
		model_trans *= Translate(2.0,0.0,-1.0);
		model_trans *= RotateXYZ(0.0,150.0,0.0);
		if(TIME >= 32.5)
			model_trans *= RotateXYZ(-10.0,0.0,0.0);
		else if(TIME >= 30.5)
			model_trans *= RotateXYZ(-5*time,0.0,0.0);
	}
	if(TIME >= 36.5 && TIME < 39.0){
		GLfloat time = TIME - 36.5;
		model_trans *= Translate(0.0,0.0,-0.2*time);
		position = model_trans;
	}
	if(TIME >= 39.0){
		GLfloat time = TIME - 39.0;
		model_trans = position;
		//model_trans *= Translate(2.0,0.0,-1.0);
		
		//model_trans *= RotateXYZ(0.0,150.0,0.0);
		model_trans *= RotateXYZ(-10.0,0.0,0.0);
		model_trans *= Translate(0.0,-1.5,0.0);
		model_trans *= RotateXYZ(-time*40,0.0,0.0);
		model_trans *= Translate(0.0,1.5,0.0);
	}

	mvstack.push(model_trans);
	model_trans *= Scale(0.6, 1.0, 0.2);
	model_view = view_trans * model_trans;
	set_colour(0.05,0.05,0.05);
	drawCube();

	//head
	model_trans = mvstack.peek();
	if(TIME >= 25.0 && TIME < 27.0){
		GLfloat time = TIME - 25.0;
		
		model_trans *= RotateXYZ(0.0, 30*time, 0.0);
	}
	if(TIME >=27.0 && TIME < 28.0){
		model_trans *= RotateXYZ(0.0, 60.0, 0.0);
	}
	if(TIME >=28.0 && TIME < 28.3){
		GLfloat time = TIME - 28.0;
		model_trans *= RotateXYZ(0.0, 60 - 200*time, 0.0);
	}
	if(TIME >= 29.5 && TIME < 30.5){
		model_trans *= RotateXYZ(30.0,0.0,0.0);
	}
	if(TIME >= 30.5 && TIME < 32.5){
		GLfloat time = TIME - 30.5;
		model_trans *= RotateXYZ(30.0 - time*20,0.0,0.0);
	}
	if(TIME >= 32.5 && TIME < 42.0){
		model_trans *= RotateXYZ(-10.0,0.0,0.0);
	}
	

	model_trans *= Translate(0.0,0.8,0.0);
	model_trans *= Scale(0.3,0.3,0.08);
	model_view = view_trans * model_trans;
	set_colour(0.7,0.6,0.4);
	drawCylinder();

	//left leg
	model_trans = mvstack.peek();
	model_trans *= Translate(0.2,-0.9,0.0);
	if(TIME >=10.0 && TIME < 11.0){
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(-85.0,0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
	}
	if(TIME >=11.0 && TIME < 12.0){
		GLfloat time = TIME-11.0;
		
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(-(85.0 - time*85),0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
	}
	if(TIME >=14.5 && TIME < 20.0){
		GLfloat time = TIME-14.5;
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(30*sin(4*time),0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
	}
	if(TIME >= 25.0 && TIME < 27.0){
		GLfloat time = TIME - 25.0;
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(10*time,0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
		leftlegpos = model_trans;
	}
	if(TIME >=27.0 && TIME < 28.0){
		model_trans = leftlegpos;
	}
	if(TIME >=28.0 && TIME < 28.3){
		GLfloat time = TIME - 28.0;
		model_trans = leftlegpos;
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(-100*time,0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
	}
	if(TIME >= 34.0){
		model_trans *= Translate(0.0,0.6,0.0);
		model_trans *= RotateXYZ(15.0,0.0,0.0);
		model_trans *= Translate(0.0,-0.6,0.0);
		if(TIME >= 36.5){
			GLfloat time = TIME - 36.5;
			model_trans *= Translate(0.0,0.6,0.0);
			model_trans *= RotateXYZ(20*sin(2.0*time + 180),0.0,0.0);
			model_trans *= Translate(0.0,-0.6,0.0);
		}
	}

	model_trans *= Scale(0.05, 0.5, 0.05);
	model_trans *= RotateXYZ(90.0,0.0,0.0);
	model_view = view_trans * model_trans;
	drawCylinder();

	//right leg
	model_trans = mvstack.peek();
	model_trans *= Translate(-0.2,-0.9,0.0);
	if(TIME >=10.0 && TIME < 11.0){
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(-85.0,0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
	}
	if(TIME >=11.0 && TIME < 12.0){
		GLfloat time = TIME-11.0;
		
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(-(85.0 - time*85),0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
	}
	if(TIME >=14.5 && TIME < 20.0){
		GLfloat time = TIME-14.5;
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(30*sin(4*time + 210),0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);

	}
	if(TIME >= 25.0 && TIME < 27.0){
		GLfloat time = TIME - 25.0;
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(-10*time,0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
		rightlegpos = model_trans;
	}
	if(TIME >=27.0 && TIME < 28.0){
		model_trans = rightlegpos;
	}
	if(TIME >=28.0 && TIME < 28.3){
		GLfloat time = TIME - 28.0;
		model_trans = rightlegpos;
		model_trans *= Translate(0.0,0.4,0.0);
		model_trans *= RotateXYZ(100*time,0.0,0.0);
		model_trans *= Translate(0.0,-0.4,0.0);
	}
	if(TIME >= 34.0){
		model_trans *= Translate(0.0,0.6,0.0);
		model_trans *= RotateXYZ(10.0,0.0,0.0);
		model_trans *= Translate(0.0,-0.6,0.0);
		if(TIME >= 36.5){
			GLfloat time = TIME - 36.5;
			model_trans *= Translate(0.0,0.6,0.0);
			model_trans *= RotateXYZ(20*sin(2.0*time),0.0,0.0);
			model_trans *= Translate(0.0,-0.6,0.0);
		}
	}

	model_trans *= Scale(0.05, 0.5, 0.05);
	model_trans *= RotateXYZ(90.0,0.0,0.0);
	model_view = view_trans * model_trans;
	drawCylinder();

	//left arm
	model_trans = mvstack.peek();
	model_trans *= Translate(0.45, 0.0, 0.0);
	model_trans *= RotateXYZ(0.0, 0.0, 20.0);
	model_trans *= Scale(0.05, 0.4, 0.05);
	model_trans *= RotateXYZ(90.0,0.0,0.0);
	model_view = view_trans * model_trans;
	drawCylinder();

	//right arm
	model_trans = mvstack.pop();
	model_trans *= Translate(-0.45, 0.0, 0.0);
	model_trans *= RotateXYZ(0.0, 0.0, -20.0);
	if(TIME < 22.0){
		model_trans *= Translate(0.0, 0.4, 0.0);
		model_trans *= RotateXYZ(-80.0, 0.0, -20.0);
		model_trans *= Translate(0.0, -0.4, 0.0);
	}
	
	if(TIME >= 22.0 && TIME <25.0){
		GLfloat time = TIME-22.0;
		model_trans *= Translate(0.0, 0.4, 0.0);
		model_trans *= RotateXYZ(-80.0, 0.0, -20.0 + time*30);
		model_trans *= Translate(-0.0, -0.4, 0.0);
	}
	if(TIME >= 25.0 && TIME < 27.0){
		GLfloat time = TIME-25.0;
		model_trans *= Translate(0.0, 0.4, 0.0);
		model_trans *= RotateXYZ(-80.0 - time*50, 0.0, -20.0 + 90);
		model_trans *= Translate(-0.0, -0.4, 0.0);
		rightarmpos = model_trans;
	}
	if(TIME >=27.0 && TIME < 28.0){
		model_trans = rightarmpos;
	}
	if(TIME >=28.0 && TIME < 28.3){
		GLfloat time = TIME - 28.0;
		model_trans = rightarmpos;
		model_trans *= Translate(0.0, 0.4, 0.0);
		model_trans *= RotateXYZ(time*800, 0.0, 0.0);
		model_trans *= Translate(-0.0, -0.4, 0.0);
	}
	
	
	mvstack.push(model_trans);
	model_trans *= Scale(0.05, 0.4, 0.05);
	model_trans *= RotateXYZ(90.0,0.0,0.0);
	model_view = view_trans * model_trans;
	drawCylinder();

	//axe
	model_trans = mvstack.pop();
	model_trans *= Translate(0.0,-0.35,0.3);
	model_trans *= RotateXYZ(90.0, 0.0, 0.0);
	mvstack.push(model_trans);
	model_trans *= Scale(0.03, 0.5, 0.03);
	model_trans *= RotateXYZ(90.0,0.0,0.0);

	model_view = view_trans * model_trans;
	set_colour(0.4,0.4,0.2);
	drawCylinder();

	model_trans = mvstack.pop();
	model_trans *= Translate(0.0, 0.38, 0.16);
	model_trans *= RotateXYZ(0.0,0.0,90.0);
	model_trans *= Scale(0.2,0.03,0.4);
	model_view = view_trans * model_trans;
	set_colour(0.75,0.75,0.75);
	drawCube();

	
	
	//mvstack.pop();
	}
	//model_trans = mvstack.pop();
	/*model_trans *= Translate(0.0, 1.5, 0.0);
	model_trans *= Scale(1.0, 6.0, 1.0);
	model_view = view_trans * model_trans;
	drawCube();
	*/
	
	//mvstack.pop();

	/*
    //model sun
    model_trans *= Scale(1.0);
    model_view = view_trans * model_trans;
    set_colour(0.8f, 0.0f, 0.0f);
    drawSphere();
    
    model_trans = mvstack.pop();//pop, get identity
    
    
    //model earth
    model_trans *= RotateY(20.0*TIME); //rotation about the sun
    model_trans *= Translate(5.0f, 0.0f, 0.0f);
    mvstack.push(model_trans); //how the eartch rotates about the sun
                               //will have effect on moon's movement,
                               //so we need to save this transformation on the stack
    
    model_trans *= RotateY(10.0*TIME);//self rotation of earth
    model_view = view_trans * model_trans;
    set_colour(0.0f, 0.0f, 0.8f);
    drawCube();
    
    model_trans = mvstack.pop();//pop, get the transformation of how the earth rotates about the sun
    
    
    //model moon
    model_trans *= RotateY(100.0*TIME);
    model_trans *= Translate(1.0f, 0.0f, 0.0f);
    model_trans *= Scale(0.2);
    model_view = view_trans * model_trans;
    set_colour(0.8f, 0.0f, 0.8f);
    drawCylinder();
    */
    
/**************************************************************
     Your drawing/modeling ends here
 ***************************************************************/
    
    glutSwapBuffers();
    if(Recording == 1)
        FrSaver.DumpPPM(Width, Height);
}


/*********************************************************
 **********************************************************
 **********************************************************
 
 PROC: idle()
 DOES: this gets called when nothing happens. 
       That's not true. 
       A lot of things happen can here.
       This is where you do your animation.
 
 MAKE YOUR CHANGES AND ADDITIONS HERE
 
 **********************************************************
 **********************************************************
 **********************************************************/
void idle(void)
{
    if( Animate == 1 )
    {
        // TM.Reset() ; // commenting out this will make the time run from 0
        // leaving 'Time' counts the time interval between successive calls to idleCB
        if( Recording == 0 )
            TIME = TM.GetElapsedTime() + 0.0;
        else{
            TIME += 0.033 ; // save at 30 frames per second.
        
        //Your code starts here

        //Your code ends here
		}
        printf("TIME %f\n", TIME) ;
        glutPostRedisplay() ;
    }
}

/*********************************************************
     PROC: main()
     DOES: calls initialization, then hands over control
           to the event handler, which calls 
           display() whenever the screen needs to be redrawn
**********************************************************/

int main(int argc, char** argv) 
{
    glutInit(&argc, argv);
    // If your code fails to run, uncommenting these lines may help.
    //glutInitContextVersion(3, 2);
    //glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition (0, 0);
    glutInitWindowSize(Width,Height);
    glutCreateWindow(argv[0]);
    printf("GL version %s\n", glGetString(GL_VERSION));
    glewExperimental = GL_TRUE;
    glewInit();
    
    instructions();
    myinit(); //performs most of the OpenGL intialization
    
    
    glutKeyboardFunc( myKey );   //keyboard event handler
    glutMouseFunc(myMouseCB) ;   //mouse button event handler
    glutMotionFunc(myMotionCB) ; //mouse motion event handler
    
    glutReshapeFunc (myReshape); //reshape event handler
    glutDisplayFunc(display);    //draw a new scene
    glutIdleFunc(idle) ;         //when nothing happens, do animaiton

    
    glutMainLoop();

    TM.Reset() ;
    return 0;         // never reached
}





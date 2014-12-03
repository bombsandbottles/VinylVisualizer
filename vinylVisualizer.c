/*
 * ===========================================================================================
 *   Author:  Harrison Zafrin (hzz200@nyu,edu)
 *   Organization:  Bombs and Bottles LLC.
 *
 *   Title: Vinyl Visualizer
 *   CProgramming Final Project 2014
 *
 *   Description:  A Vinyl Inspired Audio Visualizer With 
 *                Interactive Filtering and Sample Rate Modulation.
 *
 *   Credits : 
 *   Time Domain Based Filters Based Off - http://www.mega-nerd.com/Res/IADSPL/RBJ-filters.txt
 *   Sample Rate Modulation From - http://www.mega-nerd.com/SRC/api_full.html
 * ===========================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>         /* for memset */
#include <ncurses.h>        /* This library is for getting input without hitting return */
#include <portaudio.h>
#include <sndfile.h>         
#include <samplerate.h>     

// OpenGL
#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

// Platform-dependent sleep routines.
#if defined( __WINDOWS_ASIO__ ) || defined( __WINDOWS_DS__ )
#include <windows.h>
#define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
#else // Unix variants
#include <unistd.h>
#define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif


//-----------------------------------------------------------------------------
// global variables and #defines
//-----------------------------------------------------------------------------
#define SAMPLING_RATE           44100
#define FRAMES_PER_BUFFER       256  
#define BUFFER_SIZE             1024 
#define FORMAT                  paFloat32
#define SAMPLE                  float
#define MONO                    1
#define STEREO                  2
#define cmp_abs(x)              ( sqrt( (x).re * (x).re + (x).im * (x).im ) )
#define PI                      3.14159265358979323846264338327950288
#define ROTATION_INCR           .75f
#define INIT_WIDTH              800
#define INIT_HEIGHT             600


//Global Sound Data Struct
 typedef struct {
    
    //INFILE
    SNDFILE *infile;
    SF_INFO sf_info1;
    float buffer[FRAMES_PER_BUFFER * STEREO];

    //SAMPLE RATE

    //LOWPASS

    //HIGH PASS
    
} paData;

//Global Data Initialized
paData data;
PaStream *g_stream;



// width and height of the window
GLsizei g_width = INIT_WIDTH;
GLsizei g_height = INIT_HEIGHT;
GLsizei g_last_width = INIT_WIDTH;
GLsizei g_last_height = INIT_HEIGHT;

typedef double  MY_TYPE;
typedef char BYTE;   // 8-bit unsigned entity.

// global audio vars
GLint g_buffer_size = BUFFER_SIZE;
unsigned int g_channels = MONO;

// Threads Management
GLboolean g_ready = false;

// fill mode
GLenum g_fillmode = GL_FILL;

// light 0 position
GLfloat g_light0_pos[4] = { 2.0f, 1.2f, 4.0f, 1.0f };

// light 1 parameters
GLfloat g_light1_ambient[] = { .2f, .2f, .2f, 1.0f };
GLfloat g_light1_diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat g_light1_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
GLfloat g_light1_pos[4] = { -2.0f, 0.0f, -4.0f, 1.0f };

// fullscreen
GLboolean g_fullscreen = false;

// modelview stuff
GLfloat g_linewidth = 2.0f;

//DIANA Rotates
GLfloat g_inc = 0.0f;
GLfloat g_inc_y = 0.0;
GLfloat g_inc_x = 0.0;
bool g_key_rotate_y = false;
bool g_key_rotate_x =  false;

//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void idleFunc( );
void displayFunc( );
void reshapeFunc( int width, int height );
void keyboardFunc( unsigned char, int, int );
void specialKey( int key, int x, int y );
void specialUpKey( int key, int x, int y);
void initialize_graphics( );
void initialize_glut(int argc, char *argv[]);
void initialize_audio();
void stop_portAudio();

//Harrison Added
void drawTimeDomain(SAMPLE *buffer, float R, float G, float B, int y); 
void rotateView();


//-----------------------------------------------------------------------------
// Name: main
// Desc: ...
//-----------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
    // Initialize Glut
    initialize_glut(argc, argv);

    // Initialize PortAudio
    initialize_audio();

    // Print help
    help();

    // Wait until 'q' is pressed to stop the process
    glutMainLoop();

    // This will never get executed

    return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
// name: help()
// desc: Prints CLI Usage
//-----------------------------------------------------------------------------
void help()
{
    printf( "----------------------------------------------------\n" );
    printf( "Vinyl Visualizer\n" );
    printf( "----------------------------------------------------\n" );
    printf( "'h' - print this help message\n" );
    printf( "'f' - toggle fullscreen\n" );
    printf( "'j/k' - increase or decrease frequency by 5hz\n" );
    printf( "'m' to mute output audio\n" );
    printf( "'CURSOR ARROWS' - rotate signal view\n" );
    printf( "'q' - quit\n" );
    printf( "----------------------------------------------------\n" );
    printf( "\n" );
}

//-----------------------------------------------------------------------------
// Name: paCallback( )
// Desc: callback from portAudio
//-----------------------------------------------------------------------------
static int paCallback( const void *inputBuffer,
        void *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData ) 
{
    // TODO: Synthesize audio
    (void) inputBuffer; /* Prevent unused variable warning. */
    SAMPLE* out = (SAMPLE*)outputBuffer;
    paData *data = (paData*)userData;

    //Wave Samples
    SAMPLE sine, saw, tri, impulse;

    for (int i = 0; i < framesPerBuffer; i++)
    {
    	//Sine Wave
    	sine = synthesizeSineWave();
    	data->g_sinBuffer[i] = sine;

    	//Saw Wave
    	saw = synthesizeSawWave();
    	data->g_sawBuffer[i] = saw;

    	//Triangle Wave
    	tri = synthesizeTriWave();
    	data->g_triBuffer[i] = tri;

    	//Impulse Train
    	impulse = synthesizeImpulse();
    	data->g_impBuffer[i] = impulse;

    	//Combine Audio for Output
    	out[i] = ((sine + saw + tri + impulse) / 4) * data->amplitude;
    }

    g_ready = true;
    return 0;
}

//-----------------------------------------------------------------------------
// Name: initialize_audio( RtAudio *dac )
// Desc: Initializes PortAudio with the global vars and the stream
//-----------------------------------------------------------------------------
void initialize_audio() 
{
    PaStreamParameters outputParameters;
    PaError err;

    /* Initialize PortAudio */
    Pa_Initialize();

    /* Set output stream parameters */
    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = g_channels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = 
        Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    /* Open audio stream */
    err = Pa_OpenStream( &g_stream,
            NULL,
            &outputParameters,
            SAMPLING_RATE, 
            g_buffer_size, 
            paNoFlag, 
            paCallback, 
            &data );

    if (err != paNoError) {
        printf("PortAudio error: open stream: %s\n", Pa_GetErrorText(err));
    }

    /* Start audio stream */
    err = Pa_StartStream( g_stream );
    if (err != paNoError) {
        printf(  "PortAudio error: start stream: %s\n", Pa_GetErrorText(err));
    }

    //Initialize Audio Data
    data.frequency = INIT_FREQUENCY;
    data.amplitude = 1;
    data.waveAmplitude = 1;
}

void stop_portAudio() 
{
    PaError err;

    /* Stop audio stream */
    err = Pa_StopStream( g_stream );
    if (err != paNoError) {
        printf(  "PortAudio error: stop stream: %s\n", Pa_GetErrorText(err));
    }
    /* Close audio stream */
    err = Pa_CloseStream(g_stream);
    if (err != paNoError) {
        printf("PortAudio error: close stream: %s\n", Pa_GetErrorText(err));
    }
    /* Terminate audio stream */
    err = Pa_Terminate();
    if (err != paNoError) {
        printf("PortAudio error: terminate: %s\n", Pa_GetErrorText(err));
    }
}

//-----------------------------------------------------------------------------
// Name: keyboardFunc( )
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y )
{
    //printf("key: %c\n", key);
    switch( key )
    {
        // Print Help
        case 'h':
            help();
            break;

        // Fullscreen
        case 'f':
            if( !g_fullscreen )
            {
                g_last_width = g_width;
                g_last_height = g_height;
                glutFullScreen();
            }
            else
                glutReshapeWindow( g_last_width, g_last_height );

            g_fullscreen = !g_fullscreen;
            printf("[synthGL]: fullscreen: %s\n", g_fullscreen ? "ON" : "OFF" );
            break;

        //Change Frequency and Amplitude
        case '+':
            data.frequency += FREQUENCY_INCR;
            break;
        case '-':
            data.frequency -= FREQUENCY_INCR;
            break;
        case 'm':
            if (data.amplitude == 1)
            {
                //Mute
                data.amplitude = 0;
            }
            else if (data.amplitude == 0)
            {
                //UnMute
                data.amplitude = 1;
            }
            break;

        case 'q':
            // Close Stream before exiting
            stop_portAudio();

            exit( 0 );
            break;
    }
}

//-----------------------------------------------------------------------------
// Name: initialize_glut( )
// Desc: Initializes Glut with the global vars
//-----------------------------------------------------------------------------
void initialize_glut(int argc, char *argv[]) {
    // initialize GLUT
    glutInit( &argc, argv );
    // double buffer, use rgb color, enable depth buffer
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    // initialize the window size
    glutInitWindowSize( g_width, g_height );
    // set the window postion
    glutInitWindowPosition( 400, 100 );
    // create the window
    glutCreateWindow( "synthGL");
    // full screen
    if( g_fullscreen )
        glutFullScreen();

    // set the idle function - called when idle
    glutIdleFunc( idleFunc );
    // set the display function - called when redrawing
    glutDisplayFunc( displayFunc );
    // set the reshape function - called when client area changes
    glutReshapeFunc( reshapeFunc );
    // set the keyboard function - called on keyboard events
    glutKeyboardFunc( keyboardFunc );
    // set window's to specialKey callback
    glutSpecialFunc( specialKey );
    // set window's to specialUpKey callback (when the key is up is called)
    glutSpecialUpFunc( specialUpKey );

    // do our own initialization
    initialize_graphics( );  
}

//-----------------------------------------------------------------------------
// Name: idleFunc( )
// Desc: callback from GLUT
//-----------------------------------------------------------------------------
void idleFunc( )
{
    // render the scene
    glutPostRedisplay( );
}

//-----------------------------------------------------------------------------
// Name: specialUpKey( int key, int x, int y)
// Desc: Callback to know when a special key is pressed
//-----------------------------------------------------------------------------
void specialKey(int key, int x, int y) { 
    // Check which (arrow) key is pressed
    switch(key) {
		case GLUT_KEY_LEFT : // Arrow key left is pressed
	        g_key_rotate_y = true;
	      	g_inc_y = -ROTATION_INCR;
	      	break;
	    case GLUT_KEY_RIGHT :    // Arrow key right is pressed
	      	g_key_rotate_y = true;
	      	g_inc_y = ROTATION_INCR;
	      	break;
	    case GLUT_KEY_UP :        // Arrow key up is pressed
	      	g_key_rotate_x = true;
	      	g_inc_x = ROTATION_INCR;
	      	break;
	    case GLUT_KEY_DOWN :    // Arrow key down is pressed
	      	g_key_rotate_x = true;
	      	g_inc_x = -ROTATION_INCR;
	      	break;   
    }
}  

//-----------------------------------------------------------------------------
// Name: specialUpKey( int key, int x, int y)
// Desc: Callback to know when a special key is up
//-----------------------------------------------------------------------------
void specialUpKey( int key, int x, int y) {
    // Check which (arrow) key is unpressed
    switch(key) {
	    case GLUT_KEY_LEFT : // Arrow key left is unpressed
	      g_key_rotate_y = false;
	      break;
	    case GLUT_KEY_RIGHT :    // Arrow key right is unpressed
	      g_key_rotate_y = false;
	      break;
	    case GLUT_KEY_UP :        // Arrow key up is unpressed
	      g_key_rotate_x = false;
	      break;
	    case GLUT_KEY_DOWN :    // Arrow key down is unpressed
	      g_key_rotate_x = false;
	      break;  
    }
}

//-----------------------------------------------------------------------------
// Name: void rotateView ()
// Desc: Rotates the current view by the angle specified in the globals
//-----------------------------------------------------------------------------
void rotateView () 
{
  static GLfloat angle_x = 0;
  static GLfloat angle_y = 0;
  if (g_key_rotate_y) {
    glRotatef ( angle_y += g_inc_y, 0.0f, 1.0f, 0.0f );
  }
  else {
    glRotatef (angle_y, 0.0f, 1.0f, 0.0f );
  }
  
  if (g_key_rotate_x) {
    glRotatef ( angle_x += g_inc_x, 1.0f, 0.0f, 0.0f );
  }
  else {
    glRotatef (angle_x, 1.0f, 0.0f, 0.0f );
  }
}

//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void reshapeFunc( int w, int h )
{
    // save the new window size
    g_width = w; g_height = h;
    // map the view port to the client area
    glViewport( 0, 0, w, h );
    // set the matrix mode to project
    glMatrixMode( GL_PROJECTION );
    // load the identity matrix
    glLoadIdentity( );
    // create the viewing frustum
    //gluPerspective( 45.0, (GLfloat) w / (GLfloat) h, .05, 50.0 );
    gluPerspective( 45.0, (GLfloat) w / (GLfloat) h, 1.0, 1000.0 );
    // set the matrix mode to modelview
    glMatrixMode( GL_MODELVIEW );
    // load the identity matrix
    glLoadIdentity( );
    // position the view point
    gluLookAt( 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}


//-----------------------------------------------------------------------------
// Name: initialize_graphics( )
// Desc: sets initial OpenGL states and initializes any application data
//-----------------------------------------------------------------------------
void initialize_graphics()
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);                 // Black Background
    // set the shading model to 'smooth'
    glShadeModel( GL_SMOOTH );
    // enable depth
    glEnable( GL_DEPTH_TEST );
    // set the front faces of polygons
    glFrontFace( GL_CCW );
    // set fill mode
    glPolygonMode( GL_FRONT_AND_BACK, g_fillmode );
    // enable lighting
    glEnable( GL_LIGHTING );
    // enable lighting for front
    glLightModeli( GL_FRONT_AND_BACK, GL_TRUE );
    // material have diffuse and ambient lighting 
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );
    // enable color
    glEnable( GL_COLOR_MATERIAL );
    // normalize (for scaling)
    glEnable( GL_NORMALIZE );
    // line width
    glLineWidth( g_linewidth );

    // enable light 0
    glEnable( GL_LIGHT0 );

    // setup and enable light 1
    glLightfv( GL_LIGHT1, GL_AMBIENT, g_light1_ambient );
    glLightfv( GL_LIGHT1, GL_DIFFUSE, g_light1_diffuse );
    glLightfv( GL_LIGHT1, GL_SPECULAR, g_light1_specular );
    glEnable( GL_LIGHT1 );
}

//-----------------------------------------------------------------------------
// Name: displayFunc( )
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc( )
{
    // local variables
    SAMPLE sinBuffer[g_buffer_size];
    SAMPLE triBuffer[g_buffer_size];
    SAMPLE sawBuffer[g_buffer_size];
    SAMPLE impBuffer[g_buffer_size];
    SAMPLE outBuffer[g_buffer_size];

    // wait for data
    while( !g_ready ) usleep( 1000 );

    // copy currently playing audio into buffer
    memcpy( sinBuffer, data.g_sinBuffer, g_buffer_size * sizeof(SAMPLE) );
    memcpy( triBuffer, data.g_triBuffer, g_buffer_size * sizeof(SAMPLE) );
    memcpy( sawBuffer, data.g_sawBuffer, g_buffer_size * sizeof(SAMPLE) );
    memcpy( impBuffer, data.g_impBuffer, g_buffer_size * sizeof(SAMPLE) );
    //memcpy( outBuffer, data.g_outBuffer, g_buffer_size * sizeof(SAMPLE) );

    // Hand off to audio callback thread
    g_ready = false;

    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // TODO: Draw the signals on the screeen
    drawTimeDomain(sinBuffer, 0.0, 1.0, 0.0, 3); //Draws Sine
    drawTimeDomain(triBuffer, 0.0, 0.0, 1.0, 1); //Draws Triangle
    drawTimeDomain(sawBuffer, 1.0, 0.0, 0.0, -1); //Draws Saw
    drawTimeDomain(impBuffer, 0.9, 0.5, 0.3, -3); //Draws Impulse

    // flush gl commands
    glFlush( );

    // swap the buffers
    glutSwapBuffers( );
}

//-----------------------------------------------------------------------------
// Name: void drawTimeDomain(SAMPLE float R, float G, float B, Y Translate)
// Desc: Draws the Time Domain signal in the top of the screen
//-----------------------------------------------------------------------------
void drawTimeDomain(SAMPLE *buffer, float R, float G, float B, int y) 
{
    // Initialize initial xinc
    GLfloat x = -10;

    // Calculate increment x
    GLfloat xinc = fabs((2*x)/g_buffer_size);

    glColor3f(R, G, B);

    glPushMatrix(); 
    {
        // Rotate
        rotateView();

        // Translate
        glTranslatef(0, y, 0.0f);

        glBegin(GL_LINE_STRIP);

        // Draw Windowed Time Domain
        for (int i=0; i<g_buffer_size; i++)
        {
            glVertex3f(x, 0.5 * buffer[i], 0.0f);
            x += xinc;
        }

        glEnd(); 
    }
    glPopMatrix();

}
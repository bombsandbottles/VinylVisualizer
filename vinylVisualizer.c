/*
 * ===========================================================================================
 *   Author:  Harrison Zafrin (hzz200@nyu,edu)(harrison@bombsandbottles.com)
 *   Organization:  Bombs and Bottles LLC. - www.bombsandbottles.com
 *
 *   Title: Vinyl Visualizer
 *   CProgramming Final Project 2014
 *
 *   Description:  A Vinyl Inspired Audio Visualizer With 
 *                  Interactive Filtering and Sample Rate Modulation.
 *
 *   Credits : 
 *   Time Domain Based Filters Based Off - http://www.mega-nerd.com/Res/IADSPL/RBJ-filters.txt
 *   Sample Rate Modulation API - http://www.mega-nerd.com/SRC/api_full.html
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
#define MONO                    1
#define STEREO                  2
#define ITEMS_PER_BUFFER        (FRAMES_PER_BUFFER * 2)
#define FORMAT                  paFloat32
#define SAMPLE                  float
#define SRC_RATIO_INCREMENT     0.05
#define FILTER_CUTOFF_INCREMENT 100 //hz 
#define RESONANCE_INCREMENT     1   // Q Factor
#define PI                      3.14159265358979323846264338327950288
#define INITIAL_VOLUME          0.5
#define VOLUME_INCREMENT        0.1

#define cmp_abs(x)              ( sqrt( (x).re * (x).re + (x).im * (x).im ) )
#define ROTATION_INCR           .75f
#define INIT_WIDTH              1280
#define INIT_HEIGHT             720


/* Global Sound Data Struct */
 typedef struct {
    
    /* Audio File Members */
    SNDFILE* inFile;
    SF_INFO sfinfo1;
    float buffer[ITEMS_PER_BUFFER];
    int numberOfFrames;

    float amplitude;

    /* Sample Rate Converter Members */
    SRC_DATA  src_data;
    SRC_STATE* src_state;

    bool src_On;
    int src_error;
    int src_converter_type;
    double src_ratio;
    
    float src_inBuffer[ITEMS_PER_BUFFER];
    float src_outBuffer[ITEMS_PER_BUFFER];

    /* Low Pass Filter Members */
    bool  lpf_On;
    float lpf_freq;
    float lpf_res;

    /* High Pass Filter Members */
    bool  hpf_On;
    float hpf_freq;
    float hpf_res;

    /* OpenGL Members */
    
} paData;

//Global Data Initialized
paData data;
PaStream *g_stream;

// WxH Of OpenGL Window
GLsizei g_width = INIT_WIDTH;
GLsizei g_height = INIT_HEIGHT;
GLsizei g_last_width = INIT_WIDTH;
GLsizei g_last_height = INIT_HEIGHT;

typedef double  MY_TYPE;
typedef char BYTE;   // 8-bit unsigned entity.

//Global Audio Vars
GLint g_buffer_size = FRAMES_PER_BUFFER;

//Threads Management
GLboolean g_ready = false;

// Fill Mode
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
// Function Prototypes
//-----------------------------------------------------------------------------
void idleFunc( );
void displayFunc( );
void reshapeFunc( int width, int height );
void keyboardFunc( unsigned char, int, int );
void specialKey( int key, int x, int y );
void specialUpKey( int key, int x, int y);
void initialize_graphics( );
void initialize_glut(int argc, char *argv[]);

/* Audio Processing Functions */
void initialize_src_type();
void initialize_audio(const char* inFile);
void stop_portAudio();
void initialize_SRC_DATA();
void initialize_Filters();
void lowPassFilter(float *inBuffer);
void highPassFilter(float *inBuffer);

/* Command Line Prints */
void help();

//Harrison Added
void drawTimeDomain(SAMPLE *buffer, float R, float G, float B, int y); 
void rotateView();


//-----------------------------------------------------------------------------
// Name: Main
// Desc: ...
//-----------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
    /* Check Arguments */
    if ( argc != 2 ) {
        printf("Usage: %s: Input Audio\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Initialize SRC Algorithm */
    initialize_src_type();

    /* Initialize Glut */
    initialize_glut(argc, argv);

    /* Initialize PortAudio */
    initialize_audio(argv[1]);

    /* Print Help */
    help();

    /* Main Interactive Loop, Quits With 'q' */
    glutMainLoop();

    return EXIT_SUCCESS;
}

//-----------------------------------------------------------------------------
// name: initialize_src_type
// desc: Initializes SRC Algorithm, Stores Value in data.src_converter_type 
//-----------------------------------------------------------------------------
void initialize_src_type()
{
    /* Print User Menu */
    printf("\nChoose The Quality of Sample Rate Conversion\n"
            "Best Quality = 0\n"
            "Medium Quality = 1\n"
            "Fastest Quality = 2\n"
            "Zero Order Hold = 3\n"
            "Linear Processing = 4\n");
    printf("Enter a Number Between 0 and 4 : ");

    /* Get User Input */
    // bool found = false;
    // int c;
    // char* string;
    // while (!found)
    // {
    //     string = gets(string);
    //     c = atoi(string);
    //         if (c >= 0 || c <= 4)
    //         {
    //             found = true;
    //         }
    //     printf("Error: Please Enter a Number Between 0 and 4 : \n");
    // }
    int c = 0;
    /* Intialize SRC Algorithm */
    data.src_converter_type = c;
    printf("%d\n", data.src_converter_type);
}

//-----------------------------------------------------------------------------
// name: help()
// desc: Prints Command Line Usage
//-----------------------------------------------------------------------------
void help()
{
    /* Print Command Line Instructions */
    printf( "\n----------------------------------------------------\n" );
    printf( "Vinyl Visualizer\n" );
    printf( "----------------------------------------------------\n" );
    printf( "'h' - print this help message\n" );
    printf( "'f' - toggle fullscreen\n" );
    printf( "'j/k' - Increase/Decrease LPF Freq. Cutoff by 100hz\n" );
    printf( "'i/o' - Increase/Decrease LPF Resonance by 1.0 Q Factor\n" );
    printf( "'s/d' - Increase/Decrease HPF Freq. Cutoff by 100hz\n" );
    printf( "'w/e' - Increase/Decrease HPF Resonance by 1.0 Q Factor\n" );
    printf( "'m' To Mute Output Audio\n" );
    printf( "'r' Reset All Parameters\n" );
    printf( "'CURSOR ARROWS' - Change Speed Of Playback\n" );
    printf( "'q' - quit\n" );
    printf( "----------------------------------------------------\n" );
    printf( "\n" );
}

//-----------------------------------------------------------------------------
// Name: paCallback( )
// Desc: Callback from PortAudio
//-----------------------------------------------------------------------------
static int paCallback( const void *inputBuffer,
        void  *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData ) 
{
    /* Cast Appropriate Types and Define Some Useful Variables */
    (void) inputBuffer;
    float* out = (float*)outputBuffer;
    paData *data = (paData*)userData;
    int i, numberOfFrames;

    /* Read FramesPerBuffer Amount of Data from inFile into buffer[] */
    numberOfFrames = sf_readf_float(data->inFile, data->buffer, framesPerBuffer);

    /* Looping of inFile if EOF is Reached */
    if (numberOfFrames < framesPerBuffer) 
    {
        sf_seek(data->inFile, 0, SEEK_SET);
        numberOfFrames = sf_readf_float(data->inFile, 
                                        data->buffer+(numberOfFrames*data->sfinfo1.channels), 
                                        framesPerBuffer-numberOfFrames);  
    }

    /* Read Data from Buffer into SRC Data to Pass to src_process() */
    for (i = 0; i < framesPerBuffer * data->sfinfo1.channels; i++)
    {
        data->src_inBuffer[i] = data->buffer[i];
    }

    /* Inform SRC Data How Many Input Frames To Process */
    data->src_data.input_frames = numberOfFrames;
    data->src_data.end_of_input = 0;

    /* Perform SRC Modulation, Processed Samples are in src_outBuffer[] */
    if ((data->src_error = src_process (data->src_state, &data->src_data))) {   
        printf ("\nError : %s\n\n", src_strerror (data->src_error)) ;
        exit (1);
    }

    /* Perform Lowpass Filtering */
    if (data->lpf_On == true) {
        lowPassFilter(data->src_outBuffer);
    }

    /* Perform Highpass Filtering */
    if (data->hpf_On == true) {
        highPassFilter(data->src_outBuffer);
    }

    /* Write Processed SRC Data to Audio Out */
    for (i = 0; i < framesPerBuffer * data->sfinfo1.channels; i++)
    {
        out[i] = data->src_outBuffer[i] * data->amplitude;
    }

    g_ready = true;
    return 0;
}

//-----------------------------------------------------------------------------
// Name: initialize_audio()
// Desc: Initializes PortAudio With Globals
//-----------------------------------------------------------------------------
void initialize_audio(const char* inFile) 
{
    PaStreamParameters outputParameters;
    PaError err;

    /* Open the audio file */
    if (( data.inFile = sf_open( inFile, SFM_READ, &data.sfinfo1 )) == NULL ) 
    {
        printf("Error, Couldn't Open The File\n");
        exit (1);
    }

    /* Print info about audio file */
    printf("\nAudio File: %s\nFrames: %d\nSamples: %d\nChannels: %d\nSampleRate: %d\n",
            inFile, (int)data.sfinfo1.frames, (int)data.sfinfo1.frames * (int)data.sfinfo1.channels,
            (int)data.sfinfo1.channels, (int)data.sfinfo1.samplerate);

    /* Initialize PortAudio */
    Pa_Initialize();

    /* Set output stream parameters */
    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = data.sfinfo1.channels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = 
        Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    /* Initialize SRC */
    if ((data.src_state = src_new (data.src_converter_type, data.sfinfo1.channels, &data.src_error )) == NULL) 
    {   
        printf ("Error, SRC Initialization Failed\n");
        exit (1);
    }

    /* Sets Up The SRC_DATA Struct */
    initialize_SRC_DATA();

    /* Sets Up Filters */
    initialize_Filters();

    /* Set Initial Amplitude */
    data.amplitude = INITIAL_VOLUME;

    /* Open audio stream */
    err = Pa_OpenStream( &g_stream,
            NULL,
            &outputParameters,
            data.sfinfo1.samplerate, 
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

}

//-----------------------------------------------------------------------------
// Name: void stop_portAudio() 
// Desc: //Stops Port Audio
//-----------------------------------------------------------------------------
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
// Name: initialize_SRC_DATA()
// Desc: Sets Up The SRC_DATA Struct to Pass to src_process(SRC_STATE *state, SRC_DATA *data)
//-----------------------------------------------------------------------------
void initialize_SRC_DATA()
{
    data.src_ratio = 1;                             //Sets Default Playback Speed
    /*---------------*/
    data.src_data.data_in = data.src_inBuffer;      //Point to SRC inBuffer
    data.src_data.data_out = data.src_outBuffer;    //Point to SRC OutBuffer
    data.src_data.input_frames = 0;                 //Start with Zero to Force Load
    data.src_data.output_frames = ITEMS_PER_BUFFER
                            / data.sfinfo1.channels;//Number of Frames to Write Out
    data.src_data.src_ratio = data.src_ratio;       //Sets Default Playback Speed
}

//-----------------------------------------------------------------------------
// Name: initialize_Filters()
// Desc: Sets Default Filter Settings
//-----------------------------------------------------------------------------
void initialize_Filters()
{
    /* Lowpass Filter */
    data.lpf_On   = false;  //Default Off
    data.lpf_freq = 20000;  //Max Open
    data.lpf_res  = 1;      //No Resonance

    /* Highpas Filter */
    data.hpf_On   = false;  //Default Off
    data.hpf_freq = 20;     //Max Closed
    data.hpf_res  = 1;      //No Resonance
}

//-----------------------------------------------------------------------------
// Name: lowPassFilter(float *inBuffer)
// Desc: Applies 2 Pole lowPassFilter based on http://www.mega-nerd.com/Res/IADSPL/RBJ-filters.txt
//-----------------------------------------------------------------------------
void lowPassFilter(float *inBuffer)
{
    // Difference Equation
    /* y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2] */
    float   processed_sample;
    int     i;
    float   alpha, omega, cs;
    float   a0, a1, a2, b0, b1, b2;

    /* Account for Transient Response of Filter */
    float   x1, x2, y1, y2;  
            x1 = x2 = 0;
            y1 = y2 = 0;

    /* First Compute a Few Intermediate Variables */
    omega = 2.0 * PI * data.lpf_freq / data.sfinfo1.samplerate;
    alpha = sin(omega) / (2.0 * data.lpf_res);
    cs    = cos(omega);

    /* Calcuate Filter Coefficients */
    b0 =  (1.0 - cs) / 2.0;
    b1 =   1.0 - cs;
    b2 =  (1.0 - cs) / 2.0;
    a0 =   1.0 + alpha;
    a1 =  -2.0 * cs;
    a2 =   1.0 - alpha;

    /* Lowpass the Chunk of Data */
    for (i = 0; i < FRAMES_PER_BUFFER * data.sfinfo1.channels; i++)
    {
        /* Run a Sample Through the Difference Equation */
        processed_sample = (b0/a0) * inBuffer[i] + (b1/a0) * x1 + (b2/a0) * x2 - 
                           (a1/a0) * y1 - (a2/a0) * y2;

            /***********/
            /* Shift the Samples in the Equation So That n-1 == n */
            x2 = x1;
            x1 = inBuffer[i];

            /* Feedback Loop for Poles, Also Shift Samples So That n-1 == n */
            y2 = y1;
            y1 = processed_sample;
            /***********/

        /* Return Lowpassed Sample into lpf_outBuffer[] */
        inBuffer[i] = processed_sample;
    }
}

//-----------------------------------------------------------------------------
// Name: lowPassFilter(float *inBuffer)
// Desc: Applies 2 Pole lowPassFilter based on http://www.mega-nerd.com/Res/IADSPL/RBJ-filters.txt
//-----------------------------------------------------------------------------
void highPassFilter(float *inBuffer)
{
    // Difference Equation
    /* y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2] */
    float   processed_sample;
    int     i;
    float   alpha, omega, cs;
    float   a0, a1, a2, b0, b1, b2;

    /* Account for Transient Response of Filter */
    float   x1, x2, y1, y2;  
            x1 = x2 = 0;
            y1 = y2 = 0;

    /* First Compute a Few Intermediate Variables */
    omega = 2.0 * PI * data.hpf_freq / data.sfinfo1.samplerate;
    alpha = sin(omega) / (2.0 * data.hpf_res);
    cs    = cos(omega);

    /* Calcuate Filter Coefficients */
    b0 =  (1.0 + cs) / 2.0 ;
    b1 = -(1.0 + cs) ;
    b2 =  (1.0 + cs) / 2.0 ;
    a0 =   1.0 + alpha ;
    a1 =  -2.0 * cs ;
    a2 =   1.0 - alpha ;

    /* Lowpass the Chunk of Data */
    for (i = 0; i < FRAMES_PER_BUFFER * data.sfinfo1.channels; i++)
    {
        /* Run a Sample Through the Difference Equation */
        processed_sample = (b0/a0) * inBuffer[i] + (b1/a0) * x1 + (b2/a0) * x2 - 
                           (a1/a0) * y1 - (a2/a0) * y2;

            /***********/
            /* Shift the Samples in the Equation So That n-1 == n */
            x2 = x1;
            x1 = inBuffer[i];

            /* Feedback Loop for Poles, Also Shift Samples So That n-1 == n */
            y2 = y1;
            y1 = processed_sample;
            /***********/

        /* Return Highpassed Sample into lpf_outBuffer[] */
        inBuffer[i] = processed_sample;
    }
}

//-----------------------------------------------------------------------------
// Name: keyboardFunc( )
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y )
{
    initscr(); /* Start Curses Mode */
    cbreak();  /* Line Buffering Disabled*/
    noecho();  /* Comment This Out if You Want to Show Characters When They Are Typed */ 

    //printf("key: %c\n", key);
    switch( key )
    {
        /* Fullscreen */
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
            printf("[Visualizer]: fullscreen: %s\n", g_fullscreen ? "ON" : "OFF" );
            break;

        /* Resets Normal Default Playback */
        case 'r':
            data.src_data.src_ratio = 1;     //Sets Default Playback Speed
            initialize_Filters();
            data.amplitude = INITIAL_VOLUME;
            break;

        /* Temp Change SRC Ratio */
        case '-':
            data.src_data.src_ratio += SRC_RATIO_INCREMENT;
            break;
        case '+':
            data.src_data.src_ratio -= SRC_RATIO_INCREMENT;
            break;

        /* Low Pass Filter Controls */
        /****************************/
        /* Engage/Disengage Filter  */
        case 'l':
            if (data.lpf_On == false)
            {
                //Power On
                data.lpf_On = true;
            }
            else if (data.lpf_On == true)
            {
                //Power Off
                data.lpf_On = false;
            }
            break;

        /* Increase/Decrease Lpf Freq Cutoff */
        case 'j':
            data.lpf_freq -= FILTER_CUTOFF_INCREMENT;
                if (data.lpf_freq < 20)
                {
                    printf("Min Frequency Reached : 20hz\n");
                    data.lpf_freq = 20;
                }
                break;
        case 'k':
            data.lpf_freq += FILTER_CUTOFF_INCREMENT;
                if (data.lpf_freq > 20000)
                {
                    printf("Max Frequency Reached : 20khz\n");
                    data.lpf_freq = 20000;
                }
                break;

        /* Increase/Decrease Lpf Resonance  */
        case 'i':
            data.lpf_res  -= RESONANCE_INCREMENT;
                if (data.lpf_res <= 0)
                {
                    printf("Min Resonance Reached : 0\n");
                    data.lpf_res = 0;
                }
                break;
        case 'o':
            data.lpf_res  += RESONANCE_INCREMENT;
                if (data.lpf_res >= 10)
                {
                    printf("Max Resonance Reached : 10\n");
                    data.lpf_res = 10;
                }
                break;

        /* High Pass Filter Controls */
        /*****************************/
        /* Engage/Disengage Filter   */
        case 'a':
            if (data.hpf_On == false)
            {
                //Power On
                data.hpf_On = true;
            }
            else if (data.hpf_On == true)
            {
                //Power Off
                data.hpf_On = false;
            }
            break;                

        /* Increase/Decrease Hpf Freq Cutoff */
        case 's':
            data.hpf_freq -= FILTER_CUTOFF_INCREMENT;
                if (data.hpf_freq < 20)
                {
                    printf("Min Frequency Reached : 20hz\n");
                    data.hpf_freq = 20;
                }
                mvprintw(0,0,"Hpf: %.2f",data.hpf_freq);
                refresh();
                break;
        case 'd':
            data.hpf_freq += FILTER_CUTOFF_INCREMENT;
                if (data.hpf_freq > 20000)
                {
                    printf("Max Frequency Reached : 20khz\n");
                    data.hpf_freq = 20000;
                }
                mvprintw(0,0,"Hpf: %.2f",data.hpf_freq);
                refresh();
                break;

        /* Increase/Decrease Lpf Resonance  */
        case 'w':
            data.hpf_res  -= RESONANCE_INCREMENT;
                if (data.hpf_res <= 0)
                {
                    printf("Min Resonance Reached : 0\n");
                    data.hpf_res = 0;
                }
                break;
        case 'e':
            data.hpf_res  += RESONANCE_INCREMENT;
                if (data.hpf_res >= 10)
                {
                    printf("Max Resonance Reached : 10\n");
                    data.hpf_res = 10;
                }
                break;

        /* Amplitude Controls */
        /*****************************/
        /* Mute Output   */
        case 'm':
            if (data.amplitude > 0 )
            {
                //Mute
                data.amplitude = 0;
            }
            else if (data.amplitude == 0)
            {
                //UnMute
                data.amplitude = INITIAL_VOLUME;
            }
            break;

        /* Increase/Decrease Amplitude of Playbacl */
        case ',': 
            data.amplitude -= VOLUME_INCREMENT; 
            if (data.amplitude < 0) {
                data.amplitude = 0;
            }
            break;
        case '.': 
            data.amplitude += VOLUME_INCREMENT;
            if (data.amplitude > 1) {
                data.amplitude = 1;
            } 
            break;

        /* Exit */
        case 'q':
            /* Close Stream Before Exiting */
            stop_portAudio();

            /* Cleanup SRC */
            src_delete (data.src_state);

            /* End Curses Mode */
            endwin(); 
            exit(0);
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
    // SAMPLE sinBuffer[g_buffer_size];
    // SAMPLE triBuffer[g_buffer_size];
    // SAMPLE sawBuffer[g_buffer_size];
    // SAMPLE impBuffer[g_buffer_size];
    // SAMPLE outBuffer[g_buffer_size];

    // wait for data
    while( !g_ready ) usleep( 1000 );

    // copy currently playing audio into buffer
    // memcpy( sinBuffer, data.g_sinBuffer, g_buffer_size * sizeof(SAMPLE) );
    // memcpy( triBuffer, data.g_triBuffer, g_buffer_size * sizeof(SAMPLE) );
    // memcpy( sawBuffer, data.g_sawBuffer, g_buffer_size * sizeof(SAMPLE) );
    // memcpy( impBuffer, data.g_impBuffer, g_buffer_size * sizeof(SAMPLE) );
    //memcpy( outBuffer, data.g_outBuffer, g_buffer_size * sizeof(SAMPLE) );

    // Hand off to audio callback thread
    g_ready = false;

    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    // TODO: Draw the signals on the screeen
    // drawTimeDomain(sinBuffer, 0.0, 1.0, 0.0, 3); //Draws Sine
    // drawTimeDomain(triBuffer, 0.0, 0.0, 1.0, 1); //Draws Triangle
    // drawTimeDomain(sawBuffer, 1.0, 0.0, 0.0, -1); //Draws Saw
    // drawTimeDomain(impBuffer, 0.9, 0.5, 0.3, -3); //Draws Impulse

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
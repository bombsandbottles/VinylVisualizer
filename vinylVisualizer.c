/*
 * ===========================================================================================
 *   Author:  Harrison Zafrin (hzz200@nyu.edu)(harrison@bombsandbottles.com)
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
 *	 OpenGl Skeleton - Uri Nieto
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

/* OpenGL */
#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

/* Platform-Dependent Sleep Routines. */
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
#define FRAMES_PER_BUFFER       1024
#define MONO                    1
#define STEREO                  2
#define ITEMS_PER_BUFFER        (FRAMES_PER_BUFFER * 2)
#define FORMAT                  paFloat32
#define SAMPLE                  float
#define SRC_RATIO_INCREMENT     0.01
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

    float amplitude;

    /* Sample Rate Converter Members */
    SRC_DATA   src_data;
    SRC_STATE* src_state;

    int    src_error;
    int    src_converter_type;
    double src_ratio;
    
    float src_inBuffer[FRAMES_PER_BUFFER * 16];
    float src_outBuffer[FRAMES_PER_BUFFER * 2];

    /* Low Pass Filter Members */
    bool  lpf_On;
    int   lpf_freq;
    int   lpf_res;

    /* High Pass Filter Members */
    bool  hpf_On;
    int   hpf_freq;
    int   hpf_res;

    /* Filter On/Off */
    char* filterState[2];

    /* OpenGL Members */
    float gl_audioBuffer[ITEMS_PER_BUFFER];    
} paData;

/* Global Data Initialized */
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

/* OpenGL Functions */
void idleFunc( );
void displayFunc( );
void reshapeFunc( int width, int height );
void keyboardFunc( unsigned char, int, int );
void specialKey( int key, int x, int y );
void specialUpKey( int key, int x, int y);
void initialize_graphics( );
void initialize_glut(int argc, char *argv[]);
void drawTimeDomain(SAMPLE *buffer, float R, float G, float B); 
void rotateView();
void drawCircle(float r, int num_segments, float* buffer, bool scalar);

/* Audio Processing Functions */
void initialize_src_type();
void initialize_audio(const char* inFile);
void stop_portAudio();
void initialize_SRC_DATA();
void initialize_Filters();
static void lowPassFilter(float *inBuffer, int numChannels);
static void highPassFilter(float *inBuffer, int numChannels);
float computeRMS(float *buffer);
void brickwall(float *buffer, int numChannels);

/* Command Line Prints */
void help();
void printGUI();

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

    /* Start Curses Mode */
    initscr(); 
    cbreak();       // Line Buffering Disabled
    noecho();       // Comment This Out if You Want to Show Characters When They Are Typed
    curs_set(0);    // Make ncurses Cursor Invisible

    /* Print Help Menu and GUI */
    help();
    printGUI();

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
    bool valid = false;
    char buffer[4];
    int  srcType;

    /* Print User Menu */
    printf("\nChoose The Quality of Sample Rate Conversion\n"
            "Best Quality = 0\n"
            "Medium Quality = 1\n"
            "Fastest Quality = 2\n"
            "Zero Order Hold = 3\n"
            "Linear Processing = 4\n");
    printf("Enter a Number Between 0 and 4 : ");

    /* Read From stdin, Convert to int */
    while (!valid)
    {
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) 
        {
            srcType = atoi(buffer);
        }

        if (srcType > -1 && srcType < 5)
        {
            valid = true;
        }
        else
            valid = false;

        if (!valid)
        {
            printf("Error: Please Enter a Number Between 0 and 4 : ");
        }
    }

    /* Intialize SRC Algorithm */
    if (valid == true)
    {
        data.src_converter_type = srcType;
    }

}

//-----------------------------------------------------------------------------
// name: help()
// desc: Prints Command Line Usage
//-----------------------------------------------------------------------------
void help()
{
    /* Print Command Line Instructions */
    mvprintw(0,0,
     "----------------------------------------------------\n"
     "Vinyl Visualizer\n"
     "----------------------------------------------------\n" 
     "'f'   - Toggle Fullscreen\n" 
     "'j/k' - Increase/Decrease LPF Freq. Cutoff by 100hz\n" 
     "'i/o' - Increase/Decrease LPF Resonance by 1.0 Q Factor\n" 
     "'s/d' - Increase/Decrease HPF Freq. Cutoff by 100hz\n" 
     "'w/e' - Increase/Decrease HPF Resonance by 1.0 Q Factor\n" 
     "'-/=' - Increase/Decrease Speed/Pitch\n" 
     "'m'   - To Mute Output Audio\n" 
     "'r'   - Reset All Parameters\n" 
     "'CURSOR ARROWS' - Rotate Visuals\n" 
     "'q'   - Quit\n" 
     "----------------------------------------------------\n" 
     "\n" );
    refresh();
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
    int i, numberOfFrames, numInFrames;

    /* This if Statement Ensures Smooth VariSpeed Output */
    if (fmod((double)framesPerBuffer, data->src_data.src_ratio) == 0)
    {
    	numInFrames = framesPerBuffer;
    }
    else
    	numInFrames = (framesPerBuffer/data->src_data.src_ratio) + 2;

    /* Read FramesPerBuffer Amount of Data from inFile into buffer[] */
    numberOfFrames = sf_readf_float(data->inFile, data->src_inBuffer, numInFrames);

    /* Looping of inFile if EOF is Reached */
    if (numberOfFrames < framesPerBuffer/data->src_data.src_ratio) 
    {
        sf_seek(data->inFile, 0, SEEK_SET);
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
        lowPassFilter(data->src_outBuffer, data->sfinfo1.channels);
    }

    /* Perform Highpass Filtering */
    if (data->hpf_On == true) {
        highPassFilter(data->src_outBuffer, data->sfinfo1.channels);
    }

    /* Prevent Blowing Out the Speakers Hopefully */
    brickwall(data->src_outBuffer, data->sfinfo1.channels);

    // printf("%ld, %ld\n",data->src_data.output_frames_gen, data->src_data.input_frames_used);

    /* Write Processed SRC Data to Audio Out and Visual Out */
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

    /* Check for Compatibility */
    if (data.sfinfo1.channels > 2)
    {
    	printf("Error, File Must be Stereo or Mono\n");
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
            FRAMES_PER_BUFFER, 
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
    data.src_data.input_frames = 0;                 //Start with Zero to Force Load
    data.src_data.data_in = data.src_inBuffer;      //Point to SRC inBuffer
    data.src_data.data_out = data.src_outBuffer;    //Point to SRC OutBuffer
    data.src_data.output_frames = FRAMES_PER_BUFFER;//Number of Frames to Write Out
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

    /* Highpass Filter */
    data.hpf_On   = false;  //Default Off
    data.hpf_freq = 20;     //Max Closed
    data.hpf_res  = 1;      //No Resonance

    /* Define Off/On */
    data.filterState[0] = "Off";
    data.filterState[1] = "On";    
}

//-----------------------------------------------------------------------------
// Name: lowPassFilter(float *inBuffer)
// Desc: Applies 2 Pole lowPassFilter based on http://www.mega-nerd.com/Res/IADSPL/RBJ-filters.txt
//-----------------------------------------------------------------------------
static void lowPassFilter(float *inBuffer, int numChannels)
{
    // Difference Equation
    /* y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2] */

    float   processed_sample;
    int     i;
    float   alpha, omega, cs;
    float   a0, a1, a2, b0, b1, b2;

    /* Account for Transient Response of Filter */
    static  float   lx1 = 0, lx2 = 0, ly1 = 0, ly2 = 0;
    static  float   rx1 = 0, rx2 = 0, ry1 = 0, ry2 = 0; 

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

    /* Lowpass the Chunk of Data for Mono */
    if (numChannels == MONO)
    {
    	for (i = 0; i < FRAMES_PER_BUFFER; i++)
    	{
        	/* Run a Sample Through the Difference Equation */
        	processed_sample = (b0/a0) * inBuffer[i] + (b1/a0) * lx1 + (b2/a0) * lx2 - 
                           (a1/a0) * ly1 - (a2/a0) * ly2;

            /***********/
            /* Shift the Samples in the Equation So That n-1 == n */
            lx2 = lx1;
            lx1 = inBuffer[i];

            /* Feedback Loop for Poles, Also Shift Samples So That n-1 == n */
            ly2 = ly1;
            ly1 = processed_sample;
            /***********/

        	/* Return Lowpassed Sample into outBuffer[] */
        	inBuffer[i] = processed_sample;
    	}
    }

    /* Lowpass the Chunk of Data for Stereo */
    else if (numChannels == STEREO)
    {
    	/* Left Channel */
    	for (i = 0; i < FRAMES_PER_BUFFER; i++)
    	{
        	processed_sample = (b0/a0) * inBuffer[2*i] + (b1/a0) * lx1 + (b2/a0) * lx2 - 
                           (a1/a0) * ly1 - (a2/a0) * ly2;

            /***********/
            lx2 = lx1;
            lx1 = inBuffer[2*i];

            ly2 = ly1;
            ly1 = processed_sample;
            /***********/

        	/* Return Lowpassed Sample into outBuffer[] */
        	inBuffer[2*i] = processed_sample;
    	}

    	/* Right Channel */
    	for (i = 0; i < FRAMES_PER_BUFFER; i++)
    	{
        	processed_sample = (b0/a0) * inBuffer[2*i+1] + (b1/a0) * rx1 + (b2/a0) * rx2 - 
                           (a1/a0) * ry1 - (a2/a0) * ry2;

            /***********/
            rx2 = rx1;
            rx1 = inBuffer[2*i+1];

            ry2 = ry1;
            ry1 = processed_sample;
            /***********/

        	/* Return Lowpassed Sample into outBuffer[] */
        	inBuffer[2*i+1] = processed_sample;
    	}
    }

}

//-----------------------------------------------------------------------------
// Name: highPassFilter(float *inBuffer)
// Desc: Applies 2 Pole highPassFilter based on http://www.mega-nerd.com/Res/IADSPL/RBJ-filters.txt
//-----------------------------------------------------------------------------
static void highPassFilter(float *inBuffer, int numChannels)
{
    // Difference Equation
    /* y[n] = (b0/a0)*x[n] + (b1/a0)*x[n-1] + (b2/a0)*x[n-2] - (a1/a0)*y[n-1] - (a2/a0)*y[n-2] */

    float   processed_sample;
    int     i;
    float   alpha, omega, cs;
    float   a0, a1, a2, b0, b1, b2;

    /* Account for Transient Response of Filter */
    static  float   lx1 = 0, lx2 = 0, ly1 = 0, ly2 = 0;
    static  float   rx1 = 0, rx2 = 0, ry1 = 0, ry2 = 0; 

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

    /* Highpass the Chunk of Data for Mono */
    if (numChannels == MONO)
    {
    	for (i = 0; i < FRAMES_PER_BUFFER; i++)
    	{
        	/* Run a Sample Through the Difference Equation */
        	processed_sample = (b0/a0) * inBuffer[i] + (b1/a0) * lx1 + (b2/a0) * lx2 - 
                           (a1/a0) * ly1 - (a2/a0) * ly2;

            /***********/
            /* Shift the Samples in the Equation So That n-1 == n */
            lx2 = lx1;
            lx1 = inBuffer[i];

            /* Feedback Loop for Poles, Also Shift Samples So That n-1 == n */
            ly2 = ly1;
            ly1 = processed_sample;
            /***********/

        	/* Return Highpassed Sample into outBuffer[] */
        	inBuffer[i] = processed_sample;
    	}
    }

    /* Highpass the Chunk of Data for Stereo */
    else if (numChannels == STEREO)
    {
    	/* Left Channel */
    	for (i = 0; i < FRAMES_PER_BUFFER; i++)
    	{
        	processed_sample = (b0/a0) * inBuffer[2*i] + (b1/a0) * lx1 + (b2/a0) * lx2 - 
                           (a1/a0) * ly1 - (a2/a0) * ly2;

            /***********/
            lx2 = lx1;
            lx1 = inBuffer[2*i];

            ly2 = ly1;
            ly1 = processed_sample;
            /***********/

        	/* Return Highpassed Sample into outBuffer[] */
        	inBuffer[2*i] = processed_sample;
    	}

    	/* Right Channel */
    	for (i = 0; i < FRAMES_PER_BUFFER; i++)
    	{
        	processed_sample = (b0/a0) * inBuffer[2*i+1] + (b1/a0) * rx1 + (b2/a0) * rx2 - 
                           (a1/a0) * ry1 - (a2/a0) * ry2;

            /***********/
            rx2 = rx1;
            rx1 = inBuffer[2*i+1];

            ry2 = ry1;
            ry1 = processed_sample;
            /***********/

        	/* Return Highpassed Sample into outBuffer[] */
        	inBuffer[2*i+1] = processed_sample;
    	}
    }
}

//-----------------------------------------------------------------------------
// Name: keyboardFunc( )
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y )
{

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
            printGUI();
            break;

        /* Resets Normal Default Playback */
        case 'r':
            data.src_data.src_ratio = 1;     //Sets Default Playback Speed
            initialize_Filters();
            data.amplitude = INITIAL_VOLUME;
            printGUI();
            break;

        /* Change SRC Ratio */
        case '-':
        	if (data.src_data.src_ratio <= 2.0)
        	{
            data.src_data.src_ratio += SRC_RATIO_INCREMENT;
            }
            printGUI();
            break;
        case '=':
        	if (data.src_data.src_ratio >= 0.5 )
        	{
            data.src_data.src_ratio -= SRC_RATIO_INCREMENT;
            }
            printGUI();
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
            printGUI();
            break;

        /* Increase/Decrease Lpf Freq Cutoff */
        case 'j':
            data.lpf_freq -= FILTER_CUTOFF_INCREMENT;
                if (data.lpf_freq < 20)
                {
                    data.lpf_freq = 20;
                }
                printGUI();
                break;
        case 'k':
            data.lpf_freq += FILTER_CUTOFF_INCREMENT;
                if (data.lpf_freq > 20000)
                {
                    data.lpf_freq = 20000;
                }
                printGUI();
                break;

        /* Increase/Decrease Lpf Resonance  */
        case 'i':
            data.lpf_res  -= RESONANCE_INCREMENT;
                if (data.lpf_res <= 1)
                {
                    data.lpf_res = 1;
                }
                printGUI();
                break;
        case 'o':
            data.lpf_res  += RESONANCE_INCREMENT;
                if (data.lpf_res >= 10)
                {
                    data.lpf_res = 10;
                }
                printGUI();
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
            printGUI();
            break;                

        /* Increase/Decrease Hpf Freq Cutoff */
        case 's':
            data.hpf_freq -= FILTER_CUTOFF_INCREMENT;
                if (data.hpf_freq < 20)
                {
                    data.hpf_freq = 20;
                }
                printGUI();
                break;
        case 'd':
            data.hpf_freq += FILTER_CUTOFF_INCREMENT;
                if (data.hpf_freq > 20000)
                {
                    data.hpf_freq = 20000;
                }
                printGUI();
                break;

        /* Increase/Decrease Lpf Resonance  */
        case 'w':
            data.hpf_res  -= RESONANCE_INCREMENT;
                if (data.hpf_res <= 1)
                {
                    data.hpf_res = 1;
                }
                printGUI();
                break;
        case 'e':
            data.hpf_res  += RESONANCE_INCREMENT;
                if (data.hpf_res >= 10)
                {
                    data.hpf_res = 10;
                }
                printGUI();
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
    glutCreateWindow( "Vinyl Visualizer");
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
    /* Local Vairables */
    float visualBuffer[g_buffer_size];
    float visualBuffer2[g_buffer_size];

    /* Wait for Data */
    while( !g_ready ) usleep( 1000 );

    /* Copy Currently Playing Audio Into Buffer */
    memcpy( visualBuffer, data.src_outBuffer, g_buffer_size * sizeof(float) );
    memcpy( visualBuffer2, data.src_outBuffer, g_buffer_size * sizeof(float) );

    /* Hand Off to Audio Callback Thread */
    g_ready = false;

    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    /* Draw the Signal On The Screen */
    drawCircle(3, g_buffer_size, visualBuffer, false);  //Outer Circle
    drawCircle(3, g_buffer_size, visualBuffer, true);   //Inner Circle

    // flush gl commands
    glFlush( );

    // swap the buffers
    glutSwapBuffers( );
}

//-----------------------------------------------------------------------------
// Name: void drawTimeDomain(float r, float num_segments, float* buffer)
// Desc: Draws a Circle based on Radius r, length, and sample buffer
// Circle Skeleton From - http://slabode.exofire.net/circle_draw.shtml
//-----------------------------------------------------------------------------
void drawCircle(float r, int num_segments, float* buffer, bool scalar) 
{ 
    float theta = 2 * PI / (float)num_segments; 
    float tangetial_factor = tanf(theta);           //calculate the tangential factor 
    float radial_factor = cosf(theta);              //calculate the radial factor 
    
    float x = r;//we start at angle = 0 
    float y = 0; 
    int   i;

    // Get the actual volume
    GLfloat rms = computeRMS(buffer);

    // Get the value to scale the speaker based on the rms volume
    GLfloat scale = (rms * 2 + 0.3)/1.5; 

    glPushMatrix(); 
    {
        // //Translate
        glTranslatef(0, 0, 0.0f);
        
        //Rotate
        rotateView();

        /* Scale Size of Circle Based on Track Amplitude */
        if (scalar == true)
        {
            glScalef(scale, scale, scale);  
        }

        glBegin(GL_LINE_LOOP); 
            for (i = 0; i < num_segments; i++) 
            { 
                /* Change Color Based on Track Amplitude */
                glColor4f(1, -buffer[i], buffer[i], buffer[i]);
                
                /* Animate Circle Z-Depth Based on Track Amplitude */
                glVertex3f(x + 0, y + 0, buffer[i]); 
                                
                glRotatef(buffer[i], buffer[i], buffer[i], buffer[i]);
               
                /* Calculate the Tangential Vector 
                   Remember, the Radial Vector is (x, y), 
                   to Get the Tangential Vector we Flip Those Coordinates and Negate One of Them */
                float tx = -y; 
                float ty = x; 
                
                /* Add the Tangential Vector */ 
                x += tx * tangetial_factor; 
                y += ty * tangetial_factor; 
                
                /* Correct Using the Radial Factor */ 
                x *= radial_factor; 
                y *= radial_factor; 
            } 
        glEnd(); 
    }
    glPopMatrix();
}

//-----------------------------------------------------------------------------
// Name: float computeRMS(SAMPLE *buffer)
// Desc: Computes an RMS Value for use in Scaling Circle
//-----------------------------------------------------------------------------
float computeRMS(float *buffer) 
{
    float rms = 0;
    int i;
    for (i = 0; i < g_buffer_size; i++) {
        rms += buffer[i] * buffer[i];
    }
    return sqrtf(rms / g_buffer_size);
}

//-----------------------------------------------------------------------------
// Name: float computeRMS(SAMPLE *buffer)
// Desc: Computes an RMS Value for use in Scaling Circle
//-----------------------------------------------------------------------------
void brickwall(float *buffer, int numChannels) 
{
    int   i;
    float sample;

    /* Clip Sample if it Goes out of Bit Depth */
    for (i = 0; i < FRAMES_PER_BUFFER * numChannels; i++)
    {
        sample = buffer[i];
        if (sample > 32767) 
        {
            sample = 32767;
        }
        else if (sample < -32767) 
        {
            sample = -32767;
        }
        buffer[i] = sample;
    }
}

//-----------------------------------------------------------------------------
// Name: void printGUI() 
// Desc: Print Updateable GUI
//-----------------------------------------------------------------------------
void printGUI() 
{
    /* Speed Ratio */
    mvprintw(14,0,"Speed Ratio: %.2f\n", data.src_data.src_ratio);

    /* Low Pass Filter */
    mvprintw(15,0,"LPF: %s\n", data.filterState[(int)data.lpf_On]);
    mvprintw(16,0,"Frequency: %dhz\n", data.lpf_freq);
    mvprintw(17,0,"Resonance: %d\n", data.lpf_res);

    /* High Pass Filter */
    mvprintw(15,20,"HPF: %s\n", data.filterState[(int)data.hpf_On]);
    mvprintw(16,20,"Frequency: %dhz\n", data.hpf_freq);
    mvprintw(17,20,"Resonance: %d\n", data.hpf_res);

    mvprintw(18,0,"\n");
    refresh();
}
/* Testing libsamplerate 2*/
//
// Harrison Zafrin
//
/*************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>         /* for memset */
#include <ncurses.h>        /* This library is for getting input without hitting return */
#include <portaudio.h>
#include <sndfile.h>         
#include <samplerate.h> 

#define BUFFER_LEN              1024
#define INITIAL_VOLUME          0.5
#define INPUT_STEP_SIZE         8

//Global Sound Data Struct
 typedef struct {
    
    //Reading a File Members
    SNDFILE *infile;
    SF_INFO sf_info;

    float input  [BUFFER_LEN * 4];
    float output [BUFFER_LEN * 2];

    SRC_STATE   *src_state ;
    SRC_DATA    src_data ;
    int         error;

} paData;

//Function Prototypes
void init_data( paData *pa );
static int paCallback( const void *inputBuffer,
        void *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData );

/* 
 * Description: Main function
 */
int main( int argc, char **argv ) {

    PaStream *stream;
    PaStreamParameters outputParameters;
    PaError err;
    paData data;

    /* Check arguments */
    if ( argc != 2 ) {
        printf("Usage: %s: input audio\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Open the audio file */
    if (( data.infile = sf_open( argv[1], SFM_READ, &data.sf_info ) ) == NULL ) {
        printf("Error, couldn't open the file\n");
        return EXIT_FAILURE;
    }

    /* Print info about audio file */
    printf("Audio File:\nFrames: %d\nChannels: %d\nSampleRate: %d\n",
            (int)data.sf_info.frames, (int)data.sf_info.channels, 
            (int)data.sf_info.samplerate);

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/
        /* Initialize the sample rate converter. */
    if ((data.src_state = src_new (0, data.sf_info.channels, &data.error)) == NULL)
    {   printf ("\n\nError : src_new() failed : %s.\n\n", src_strerror (data.error)) ;
        exit (1);
        };

    data.src_data.src_ratio = 1;
    /* Start with zero to force load in while loop. */
    data.src_data.input_frames = 0;
    data.src_data.data_in = data.input ;
    data.src_data.data_out = data.output ; //Point to SRC OutBuffer
    data.src_data.output_frames = BUFFER_LEN; // /data.sf_info.channels; //Number of Frames to Write Out

    /********************************************************************************/
    /********************************************************************************/
    /********************************************************************************/

    /********************************************************************************/
    /********************************************************************************/

    /********************************************************************************/
    /********************************************************************************/

    /* Initialize PortAudio */
    Pa_Initialize();

    /* Set output stream parameters */
    outputParameters.device = Pa_GetDefaultOutputDevice();
    outputParameters.channelCount = data.sf_info.channels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = 
        Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;


    /* Open audio stream */
    err = Pa_OpenStream( &stream,
            NULL,
            &outputParameters,
            data.sf_info.samplerate, BUFFER_LEN, paNoFlag, 
            paCallback, &data );

    if (err != paNoError) {
        printf("PortAudio error: open stream: %s\n", Pa_GetErrorText(err));
    }

    /* Start audio stream */
    err = Pa_StartStream( stream );
    if (err != paNoError) {
        printf(  "PortAudio error: start stream: %s\n", Pa_GetErrorText(err));
    }

    /* Initialize interactive character input */
    initscr(); /* Start curses mode */
    cbreak();  /* Line buffering disabled*/
    noecho(); /* Comment this out if you want to show characters when they are typed */

    char ch;
    ch = '\0'; /* Init ch to null character */

    while (ch != 'q') {
        ch = getch(); /* If cbreak hadn't been called, you would have to press enter
                         before it gets to the program */
        switch (ch) {
            case 'd':
                data.src_data.src_ratio -= 0.5;
                break;
            case 'f':
                data.src_data.src_ratio += 2;
                break;
        }

    }
    /* End curses mode  */
    endwin();

    err = Pa_StopStream( stream );

    /* Stop audio stream */
    if (err != paNoError) {
        printf(  "PortAudio error: stop stream: %s\n", Pa_GetErrorText(err));
    }
    /* Close audio stream */
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        printf("PortAudio error: close stream: %s\n", Pa_GetErrorText(err));
    }
    /* Terminate audio stream */
    err = Pa_Terminate();
    if (err != paNoError) {
        printf("PortAudio error: terminate: %s\n", Pa_GetErrorText(err));
    }

    return 0;
}

/* 
 *  Description:  Callback for Port Audio
 */
static int paCallback( const void *inputBuffer,
        void *outputBuffer, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void *userData ) 
{

    int i;

    (void) inputBuffer; /* Prevent unused variable warning. */
    float *outBuf = (float*)outputBuffer;
    paData *data = (paData*)userData;

    /* Read FramesPerBuffer Amount of Data from inFile into buffer[] */
    data->src_data.input_frames = sf_readf_float(data->infile, data->input, BUFFER_LEN/data->src_data.src_ratio);

    data->src_data.end_of_input = 0;

    // for (i = 0; i < BUFFER_LEN * 2; i++) {
    //     data->input[i] = data->input[2*i];
    // }

    /* Process current block. */
    if ((data->error = src_process (data->src_state, &data->src_data)))
    {   printf ("\nError : %s\n", src_strerror (data->error)) ;
        exit (1) ;
    }

    
    for (i = 0; i < BUFFER_LEN * data->sf_info.channels; i++) {
        // Write into the "speakers" (outBuf) to listen to the delay
        outBuf[i] = data->output[i];
        // outBuf[i] = data->input[i];
    }

    return paContinue;
}


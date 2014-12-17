VinylVisualizer
===============

* ========================================================================
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
* ========================================================================

The main goal behind my project was to create an interactive music visualizer with realtime audio effects. The projects name is a play on the shape of the visualization and of its realtime audio effects "variable speed replay". In addition to variable speed replay, 2-Pole Lowpass and Highpass IIR Filters are also implemented.

Installation:
	As long as the dependencies listed below are installed there should be no problem using the included makefile.

Required Dependencies:

	Port Audio
	Libsndfile
	Libsamplerate
	OpenGL

Usage:  
====== 
	./VinylVisualizer < soundfile > 

	'f'   - Toggle Fullscreen 
	'j/k' - Increase/Decrease LPF Freq. Cutoff by 100hz 
	'i/o' - Increase/Decrease LPF Resonance by 1.0 Q Factor 
	's/d' - Increase/Decrease HPF Freq. Cutoff by 100hz 
	'w/e' - Increase/Decrease HPF Resonance by 1.0 Q Factor 
	'-/=' - Increase/Decrease Speed/Pitch 
	'm'   - To Mute Output Audio 
	'r'   - Reset All Parameters 
	'CURSOR ARROWS' - Rotate Visuals 
	'q'   - Quit 
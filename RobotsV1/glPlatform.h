/*--------------------------------------------------------------------------+
 |	Final Project CSC412 - Spring 2023										|
 |	A Simple global header to check the development platform and load the 	|
 |	proper OpenGL, glu, and glut headers.									|
 |	Supports macOS, Windows, Linux,											|
 |																			|
 |	Author:		Jean-Yves Hervé, University of Rhode Island					|
 |								 Dept. of Com[puter Science and Statistics	|
 |								 3D Group for Interactive Visualization		|
 |	Fall 2013, modified Fall 2022											|
 +-------------------------------------------------------------------------*/
#ifndef GL_PLATFORM_H
#define GL_PLATFORM_H

//-----------------------------------------------------------------------
//  Determines which OpenGL & glut header files to load, based on
//  the development platform and target (OS & compiler)
//-----------------------------------------------------------------------

//  Windows platform
#if (defined(_WIN32) || defined(_WIN64))
    //  Visual
    #if defined(_MSC_VER)
		#include <Windows.h>
        #include <GL\gl.h>
		#include <gl/glut.h>
    //  gcc-based compiler
    #elif defined(__CYGWIN__) || defined(__MINGW32__)
        #include <GL/gl.h>
        #include <GL/glut.h>
    #elif (defined( __MWERKS__) && __INTEL__))
		#error not supported anymore
    #endif
//  Linux and Unix
#elif  (defined(__FreeBSD__) || defined(__linux__) || defined(sgi) || defined(__NetBSD__) || defined(__OpenBSD) || defined(__QNX__))
    #include <GL/gl.h>
    #include <GL/glut.h>

//  Macintosh
#elif defined(__APPLE__)
	#if 1
		#include <GLUT/GLUT.h>
		//	Here ask Xcode/clang++ to suppress "deprecated" warnings
		#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	#else
		#include <GL/freeglut.h>
		#include <GL/gl.h>
	#endif
#else
	#error undknown OS
#endif

#endif	//	GL_PLATFORM_H

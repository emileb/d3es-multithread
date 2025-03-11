/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include <SDL.h>

#include "sys/platform.h"
#include "framework/Licensee.h"

#include "renderer/tr_local.h"

#if defined(_WIN32) && defined(ID_ALLOW_TOOLS)
#include "sys/win32/win_local.h"
#include <SDL_syswm.h>

// from SDL_windowsopengl.h (internal SDL2 header)
#ifndef WGL_ARB_pixel_format
#define WGL_NUMBER_PIXEL_FORMATS_ARB   0x2000
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_DRAW_TO_BITMAP_ARB         0x2002
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_NEED_PALETTE_ARB           0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB    0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB     0x2006
#define WGL_SWAP_METHOD_ARB            0x2007
#define WGL_NUMBER_OVERLAYS_ARB        0x2008
#define WGL_NUMBER_UNDERLAYS_ARB       0x2009
#define WGL_TRANSPARENT_ARB            0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB  0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB            0x200C
#define WGL_SHARE_STENCIL_ARB          0x200D
#define WGL_SHARE_ACCUM_ARB            0x200E
#define WGL_SUPPORT_GDI_ARB            0x200F
#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_STEREO_ARB                 0x2012
#define WGL_PIXEL_TYPE_ARB             0x2013
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_RED_BITS_ARB               0x2015
#define WGL_RED_SHIFT_ARB              0x2016
#define WGL_GREEN_BITS_ARB             0x2017
#define WGL_GREEN_SHIFT_ARB            0x2018
#define WGL_BLUE_BITS_ARB              0x2019
#define WGL_BLUE_SHIFT_ARB             0x201A
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_ALPHA_SHIFT_ARB            0x201C
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_ACCUM_RED_BITS_ARB         0x201E
#define WGL_ACCUM_GREEN_BITS_ARB       0x201F
#define WGL_ACCUM_BLUE_BITS_ARB        0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB       0x2021
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_AUX_BUFFERS_ARB            0x2024
#define WGL_NO_ACCELERATION_ARB        0x2025
#define WGL_GENERIC_ACCELERATION_ARB   0x2026
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_SWAP_EXCHANGE_ARB          0x2028
#define WGL_SWAP_COPY_ARB              0x2029
#define WGL_SWAP_UNDEFINED_ARB         0x202A
#define WGL_TYPE_RGBA_ARB              0x202B
#define WGL_TYPE_COLORINDEX_ARB        0x202C
#endif

#ifndef WGL_ARB_multisample
#define WGL_SAMPLE_BUFFERS_ARB         0x2041
#define WGL_SAMPLES_ARB                0x2042
#endif

#endif // _WIN32 and ID_ALLOW_TOOLS


#if SDL_VERSION_ATLEAST(2, 0, 0)
static SDL_Window *window = NULL;
static SDL_GLContext context = NULL;
#else
static SDL_Surface *window = NULL;
#define SDL_WINDOW_OPENGL SDL_OPENGL
#define SDL_WINDOW_FULLSCREEN SDL_FULLSCREEN
#endif

static void SetSDLIcon() {
    Uint32 rmask, gmask, bmask, amask;

    // ok, the following is pretty stupid.. SDL_CreateRGBSurfaceFrom() pretends to use a void* for the data,
    // but it's really treated as endian-specific Uint32* ...
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

#include "doom_icon.h" // contains the struct d3_icon

    SDL_Surface *icon = SDL_CreateRGBSurfaceFrom((void *) d3_icon.pixel_data, d3_icon.width,
                                                 d3_icon.height,
                                                 d3_icon.bytes_per_pixel * 8,
                                                 d3_icon.bytes_per_pixel * d3_icon.width,
                                                 rmask, gmask, bmask, amask);

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_SetWindowIcon(window, icon);
#else
    SDL_WM_SetIcon(icon, NULL);
#endif

    SDL_FreeSurface(icon);
}

/*
===================
GLimp_Init
===================
*/
bool GLimp_Init(glimpParms_t parms) {
    common->Printf("Initializing OpenGL subsystem\n");

    assert(SDL_WasInit(SDL_INIT_VIDEO));

    Uint32 flags = SDL_WINDOW_OPENGL;

    if (parms.fullScreen)
        flags |= SDL_WINDOW_FULLSCREEN;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16); // Defaults to 24 which is not needed and fails on old Tegras
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    //SDL_GL_SetAttribute(SDL_GL_STEREO, parms.stereo ? 1 : 0);
    common->Printf("multiSamples = %d", parms.multiSamples);

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, parms.multiSamples ? 1 : 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, parms.multiSamples);

    window = SDL_CreateWindow(ENGINE_VERSION,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              parms.width, parms.height, flags);

    if (!window) {
        common->Printf("FAILED TO CREATE WINDOWS");
    } else {
        common->Printf("WINDOW CREATED OK");
    }

    context = SDL_GL_CreateContext(window);

    if (!context) {
        common->Printf("FAILED TO CREATE CONTEXT");
    } else {
        common->Printf("CONTEXT CREATED OK");
    }

    SDL_GetWindowSize(window, &glConfig.vidWidth, &glConfig.vidHeight);

    glConfig.isFullscreen = (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) == SDL_WINDOW_FULLSCREEN;

	glConfig.vidWidthReal = glConfig.vidWidth;
	glConfig.vidHeightReal = glConfig.vidHeight;

	// If vidWidthReal and vidWidth are different then the framebuffer will automatically be used
	if (r_framebufferWidth.GetInteger() !=0 && r_framebufferHeight.GetInteger() !=0)
	{
		glConfig.vidWidth = r_framebufferWidth.GetInteger();
		glConfig.vidHeight = r_framebufferHeight.GetInteger();

		common->Printf("Rendering in to framebuffer [%d,%d]\n", glConfig.vidWidth, glConfig.vidHeight);
	}

    //common->Printf("Using %d color bits, %d depth, %d stencil display\n",
    //				channelcolorbits, tdepthbits, tstencilbits);

    glConfig.colorBits = 24;
    glConfig.depthBits = 16;
    glConfig.stencilBits = 8;

    glConfig.displayFrequency = 0;


    if (!window) {
        common->Warning("No usable GL mode found: %s", SDL_GetError());
        return false;
    }

    GLimp_WindowActive(true);

    return true;
}

/*
===================
GLimp_SetScreenParms
===================
*/
bool GLimp_SetScreenParms(glimpParms_t parms) {
    common->DPrintf("TODO: GLimp_ActivateContext\n");
    return true;
}

/*
===================
GLimp_Shutdown
===================
*/
void GLimp_Shutdown() {
    common->Printf("Shutting down OpenGL subsystem\n");

#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (context) {
        SDL_GL_DeleteContext(context);
        context = NULL;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }
#endif
}

/*
===================
GLimp_SwapBuffers
===================
*/
void GLimp_SwapBuffers() {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_GL_SwapWindow(window);
#else
    SDL_GL_SwapBuffers();
#endif
}

static bool gammaOrigError = false;
static bool gammaOrigSet = false;
static unsigned short gammaOrigRed[256];
static unsigned short gammaOrigGreen[256];
static unsigned short gammaOrigBlue[256];

/*
=================
GLimp_SetGamma
=================
*/
void GLimp_SetGamma(unsigned short red[256], unsigned short green[256], unsigned short blue[256]) {
    if (!window) {
        common->Warning("GLimp_SetGamma called without window");
        return;
    }

    if (!gammaOrigSet) {
        gammaOrigSet = true;
#if SDL_VERSION_ATLEAST(2, 0, 0)
        if ( SDL_GetWindowGammaRamp( window, gammaOrigRed, gammaOrigGreen, gammaOrigBlue ) == -1 ) {
#else
        if (SDL_GetGammaRamp(gammaOrigRed, gammaOrigGreen, gammaOrigBlue) == -1) {
#endif
            gammaOrigError = true;
            common->Warning("Failed to get Gamma Ramp: %s\n", SDL_GetError());
        }
    }


#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (SDL_SetWindowGammaRamp(window, red, green, blue))
#else
    if (SDL_SetGammaRamp(red, green, blue))
#endif
        common->Warning("Couldn't set gamma ramp: %s", SDL_GetError());
}

/*
=================
GLimp_ResetGamma

Restore original system gamma setting
=================
*/
void GLimp_ResetGamma() {
    if (gammaOrigError) {
        common->Warning("Can't reset hardware gamma because getting the Gamma Ramp at startup failed!\n");
        common->Warning("You might have to restart the game for gamma/brightness in shaders to work properly.\n");
        return;
    }

    if (gammaOrigSet) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
        SDL_SetWindowGammaRamp( window, gammaOrigRed, gammaOrigGreen, gammaOrigBlue );
#else
        SDL_SetGammaRamp(gammaOrigRed, gammaOrigGreen, gammaOrigBlue);
#endif
    }
}


/*
=================
GLimp_ActivateContext
=================
*/
void GLimp_ActivateContext() {
    SDL_GL_MakeCurrent(window, context);
}

/*
=================
GLimp_DeactivateContext
=================
*/
void GLimp_DeactivateContext() {
    SDL_GL_MakeCurrent(window, NULL);
}

/*
===================
GLimp_ExtensionPointer
===================
*/
#ifdef __ANDROID__
#include <dlfcn.h>
#endif

GLExtension_t GLimp_ExtensionPointer(const char *name) {
    assert(SDL_WasInit(SDL_INIT_VIDEO));

#ifdef __ANDROID__
    static void *glesLib = NULL;

    if( !glesLib )
    {
        int flags = RTLD_LOCAL | RTLD_NOW;
        glesLib = dlopen("libGLESv2_CM.so", flags);
        //glesLib = dlopen("libGLESv3.so", flags);
        if( !glesLib )
        {
            glesLib = dlopen("libGLESv2.so", flags);
        }
    }

    GLExtension_t ret =  (GLExtension_t)dlsym(glesLib, name);
    //common->Printf("GLimp_ExtensionPointer %s  %p\n",name,ret);
    return ret;
#endif

    return (GLExtension_t) SDL_GL_GetProcAddress(name);
}

void GLimp_WindowActive(bool active)
{
    LOGI( "GLimp_WindowActive %d", active );

    tr.windowActive = active;

    if(!active)
    {
        tr.BackendThreadShutdown();
    }
}

void GLimp_GrabInput(int flags) {
    if (!window) {
        common->Warning("GLimp_GrabInput called without window");
        return;
    }

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_ShowCursor( (flags & GRAB_HIDECURSOR) ? SDL_DISABLE : SDL_ENABLE );
    SDL_SetRelativeMouseMode( (flags & GRAB_RELATIVEMOUSE) ? SDL_TRUE : SDL_FALSE );
    SDL_SetWindowGrab( window, (flags & GRAB_GRABMOUSE) ? SDL_TRUE : SDL_FALSE );
#else
    SDL_ShowCursor((flags & GRAB_HIDECURSOR) ? SDL_DISABLE : SDL_ENABLE);
    // ignore GRAB_GRABMOUSE, SDL1.2 doesn't support grabbing without relative mode
    // so only grab if we want relative mode
    SDL_WM_GrabInput((flags & GRAB_RELATIVEMOUSE) ? SDL_GRAB_ON : SDL_GRAB_OFF);
#endif
}

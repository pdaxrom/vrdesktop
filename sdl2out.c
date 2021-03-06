#include <SDL.h>
#ifdef USE_GLES2
#include <SDL_opengles2.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>
#endif

#include "shader.h"
#include "vrout.h"

static SDL_Window *window;
static SDL_Surface *framebuffer;
static SDL_GLContext context;
static SDL_DisplayMode mode;

static float scale_x = 1;
static float scale_y = 1;

/*
    // normal
    static const GLfloat textureVertices[] = {
         0.0f,  1.0f,
         1.0f,  1.0f,
         0.0f,  0.0f,
         1.0f,  0.0f,
    };

    // rot 180
    static const GLfloat textureVertices[] = {
         1.0f,  0.0f,
         0.0f,  0.0f,
         1.0f,  1.0f,
         0.0f,  1.0f,
    };

    // rot 180 mirr
    static const GLfloat textureVertices[] = {
         0.0f,  0.0f,
         1.0f,  0.0f,
         0.0f,  1.0f,
         1.0f,  1.0f,
    };

    // rot 270
    static const GLfloat textureVertices[] = {
         0.0f,  0.0f,
         0.0f,  1.0f,
         1.0f,  0.0f,
         1.0f,  1.0f,
    };

    // rot 270 mirr
    static const GLfloat textureVertices[] = {
         1.0f,  0.0f,
         1.0f,  1.0f,
         0.0f,  0.0f,
         0.0f,  1.0f,
    };

    // rot 90
    static const GLfloat textureVertices[] = {
         1.0f,  1.0f,
         1.0f,  0.0f,
         0.0f,  1.0f,
         0.0f,  0.0f,
    };

    // rot 90 mirr
    static const GLfloat textureVertices[] = {
         0.0f,  1.0f,
         0.0f,  0.0f,
         1.0f,  1.0f,
         1.0f,  0.0f,
    };
*/

int InitVideo(int index, int w, int h)
{
    enum {
    	ATTRIB_VERTEX,
    	ATTRIB_TEXTUREPOSITON,
    	NUM_ATTRIBUTES
    };

    static const GLfloat squareVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

    static const GLfloat textureVertices[][8] = {
	{
         0.0f,  1.0f,
         1.0f,  1.0f,
         0.0f,  0.0f,
         1.0f,  0.0f,
	},
	{
         1.0f,  1.0f,
         1.0f,  0.0f,
         0.0f,  1.0f,
         0.0f,  0.0f,
	},
	{
         1.0f,  0.0f,
         0.0f,  0.0f,
         1.0f,  1.0f,
         0.0f,  1.0f,
	},
	{
         0.0f,  0.0f,
         0.0f,  1.0f,
         1.0f,  0.0f,
         1.0f,  1.0f,
	}
    };

    int texRotate = 0;

    SDL_Log("Video starting...");

//    SDL_GetDesktopDisplayMode(index, &mode);

    SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);

    mode.w = w;
    mode.h = h;

    if (mode.w < mode.h) {
	texRotate = 3;
    }

    SDL_Log("Window size %dx%d\n", mode.w, mode.h);

    window = SDL_CreateWindow("VRDESKTOP", SDL_WINDOWPOS_UNDEFINED_DISPLAY(index), SDL_WINDOWPOS_UNDEFINED_DISPLAY(index),
    							mode.w, mode.h, SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS);
//    window = SDL_CreateWindow("VRDESKTOP", SDL_WINDOWPOS_UNDEFINED_DISPLAY(index), SDL_WINDOWPOS_UNDEFINED_DISPLAY(index),
//    							mode.w, mode.h, SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED | SDL_WINDOW_BORDERLESS);

    if(!window) {
    	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation fail : %s\n", SDL_GetError());
    	return 1;
    }

    context = SDL_GL_CreateContext(window);
    if (!context) {
    	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create GL context : %s\n", SDL_GetError());
    	return 1;
    }

    SDL_GL_MakeCurrent(window, context);

    // Start of GL init

    GLuint vertexShader = -1;
    GLuint fragmentShader = -1;

    if (process_shader(&vertexShader, "shaders/shader.vert", GL_VERTEX_SHADER)) {
    	SDL_Log("Unable load vertex shader");
    	return 1;
    }

    if (process_shader(&fragmentShader, "shaders/shader.frag", GL_FRAGMENT_SHADER)) {
    	SDL_Log("Unable load fragment shader");
    	return 1;
    }

    GLuint shaderProgram  = glCreateProgram ();                 // create program object
    glAttachShader ( shaderProgram, vertexShader );             // and attach both...
    glAttachShader ( shaderProgram, fragmentShader );           // ... shaders to it

    glBindAttribLocation(shaderProgram, ATTRIB_VERTEX, "position");
    glBindAttribLocation(shaderProgram, ATTRIB_TEXTUREPOSITON, "inputTextureCoordinate");

    glLinkProgram ( shaderProgram );    // link the program
    glUseProgram  ( shaderProgram );    // and select it for usage

    glActiveTexture(GL_TEXTURE0);
    GLuint videoFrameTexture = 0;
    glGenTextures(1, &videoFrameTexture);
    glBindTexture(GL_TEXTURE_2D, videoFrameTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindTexture(GL_TEXTURE_2D, videoFrameTexture);

    GLint tex = glGetUniformLocation(shaderProgram, "tex");

    glUniform1i(tex, 0);

    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTUREPOSITON, 2, GL_FLOAT, 0, 0, textureVertices[texRotate]);
    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITON);

    glViewport ( 0 , 0 , mode.w , mode.h );

    // End of GL init
    return 1;
}

void FinishVideo()
{
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);

    SDL_Log("Video finished...");
}

void RenderVideo(unsigned char *pixels, int w, int h)
{
    if (mode.w > mode.h) {
	double scale = (double) (mode.w / 2) / (double) w;
	double dH = (double) h * scale;

	glViewport (0, (mode.h - dH) / 2, mode.w / 2, dH);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glViewport (mode.w / 2,  (mode.h - dH) / 2, mode.w / 2, dH);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
	double scale = (double) (mode.h / 2) / (double) w;
	double dH = (double) h * scale;

	glViewport ((mode.w - dH) / 2, 0, dH, mode.h / 2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glViewport ((mode.w - dH) / 2, mode.h / 2, dH, mode.h / 2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    SDL_GL_SwapWindow(window);
}

#include <SDL_image.h>

int Show3DSurface(SDL_Surface *image)
{
    SDL_Rect srcrect;

    SDL_Surface *surf = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ARGB8888, 0);

    SDL_Surface *left = SDL_CreateRGBSurface(0, surf->w / 2, surf->h, surf->format->BitsPerPixel, surf->format->Rmask, surf->format->Gmask, surf->format->Bmask, surf->format->Amask);
    if (left == NULL) {
	SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
	return 0;
    }

    SDL_Surface *right = SDL_CreateRGBSurface(0, surf->w / 2, surf->h, surf->format->BitsPerPixel, surf->format->Rmask, surf->format->Gmask, surf->format->Bmask, surf->format->Amask);
    if (right == NULL) {
	SDL_Log("SDL_CreateRGBSurface() failed: %s", SDL_GetError());
	SDL_FreeSurface(left);
	return 0;
    }

    srcrect.x = 0;
    srcrect.y = 0;
    srcrect.w = surf->w / 2;
    srcrect.h = surf->h;

    if (SDL_BlitSurface(surf, &srcrect, left, NULL) < 0) {
	SDL_Log("SDL_BlitSurface() failed: %s", SDL_GetError());
    }

    srcrect.x = surf->w / 2;
    srcrect.y = 0;
    srcrect.w = surf->w / 2;
    srcrect.h = surf->h;

    if (SDL_BlitSurface(surf, &srcrect, right, NULL) < 0) {
	SDL_Log("SDL_BlitSurface() failed: %s", SDL_GetError());
    }

    if (mode.w > mode.h) {
	double scale = (double) (mode.w / 2) / (double) left->w;
	double dH = (double) left->h * scale;

	glViewport (0, (mode.h - dH) / 2, mode.w / 2, dH);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, left->w, left->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, left->pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glViewport (mode.w / 2,  (mode.h - dH) / 2, mode.w / 2, dH);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, right->w, right->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, right->pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
	double scale = (double) (mode.h / 2) / (double) left->w;
	double dH = (double) left->h * scale;

	glViewport ((mode.w - dH) / 2, 0, dH, mode.h / 2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, left->w, left->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, left->pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glViewport ((mode.w - dH) / 2, mode.h / 2, dH, mode.h / 2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, right->w, right->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, right->pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    SDL_GL_SwapWindow(window);

    SDL_FreeSurface(surf);
    SDL_FreeSurface(left);
    SDL_FreeSurface(right);

    return 0;
}

void ShowLogo(char *logofile)
{
    IMG_Init(IMG_INIT_JPG);

    SDL_Surface *image = IMG_Load(logofile);

    if (image) {
/*
	SDL_Surface *logo = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ABGR8888, 0);

	double ratio = (double) logo->h / (double) logo->w;
	double dH = (double) mode.w * ratio;

	glViewport ( 0, (mode.h - dH) / 2, mode.w, dH);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, logo->w, logo->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, logo->pixels);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	SDL_GL_SwapWindow(window);

	sleep(5);

	SDL_FreeSurface(logo);
 */

	Show3DSurface(image);

	sleep(5);

	SDL_FreeSurface(image);
    }

    IMG_Quit();
}

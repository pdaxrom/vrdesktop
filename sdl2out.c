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

static GLuint shaderProgram2fb = 0;
static GLuint shaderProgram2lens = 0;
static GLuint framebufferTexture = 0;
static GLuint framebufferName = 0;
static GLuint renderedTexture = 0;

static int texRotate = 0;

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

enum {
    	ATTRIB_U_POSITION,
	ATTRIB_U_TEXTURE,
    	ATTRIB_U_RESOLUTION,
	ATTRIB_U_ANGLE,
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

int InitVideo(int index, int w, int h)
{
    texRotate = 0;

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

    if (process_shader(&vertexShader, "shaders/2lens.vert", GL_VERTEX_SHADER)) {
    	SDL_Log("Unable load vertex shader");
    	return 1;
    }

    if (process_shader(&fragmentShader, "shaders/2lens.frag", GL_FRAGMENT_SHADER)) {
    	SDL_Log("Unable load fragment shader");
    	return 1;
    }

    shaderProgram2lens = glCreateProgram();
    glAttachShader(shaderProgram2lens, vertexShader);
    glAttachShader(shaderProgram2lens, fragmentShader);
    glLinkProgram(shaderProgram2lens);

    if (process_shader(&vertexShader, "shaders/2fb.vert", GL_VERTEX_SHADER)) {
    	SDL_Log("Unable load vertex shader");
    	return 1;
    }

    if (process_shader(&fragmentShader, "shaders/2fb.frag", GL_FRAGMENT_SHADER)) {
    	SDL_Log("Unable load fragment shader");
    	return 1;
    }

    shaderProgram2fb = glCreateProgram();
    glAttachShader(shaderProgram2fb, vertexShader);
    glAttachShader(shaderProgram2fb, fragmentShader);
    glLinkProgram(shaderProgram2fb);

    glGenFramebuffers(1, &framebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferName);

    glGenTextures(1, &renderedTexture);
    glBindTexture(GL_TEXTURE_2D, renderedTexture);
    if (mode.w > mode.h) {
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mode.w / 2, mode.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    } else {
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mode.w, mode.h / 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

    // Build the texture that will serve as the depth attachment for the framebuffer.
    GLuint depth_renderbuffer;
    glGenRenderbuffers(1, &depth_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer);
    if (mode.w > mode.h) {
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mode.w / 2, mode.h);
    } else {
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mode.w, mode.h / 2);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_renderbuffer);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE){
	fprintf(stderr, "Gl framebuffer error!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenTextures(1, &framebufferTexture);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mode.w / 2, mode.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // End of GL init
    return 1;
}

void FinishVideo()
{
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);

    SDL_Log("Video finished...");
}

void RenderLensTexture(SDL_Surface *surf, double scale, int x, int y, int w, int h)
{
    double img_scale = (double) (w / 2) / (double) surf->w;
    double lens_angle = scale * M_PI;// * 0.9;

    // To texture
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferName);
    glEnable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram2fb);
    glBindAttribLocation(shaderProgram2fb, ATTRIB_U_POSITION, "u_position");
    glBindAttribLocation(shaderProgram2fb, ATTRIB_U_TEXTURE, "u_texture");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);

    glVertexAttribPointer(ATTRIB_U_POSITION, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_U_POSITION);
    glVertexAttribPointer(ATTRIB_U_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices[texRotate]);
    glEnableVertexAttribArray(ATTRIB_U_TEXTURE);

    if (w > h) {
	double dH = (double) surf->h * img_scale;
	glViewport (0, (h - dH) / 2, w / 2, dH);
    } else {
	double dW = (double) surf->w * img_scale;
	glViewport ((w - dW) / 2, 0, dW, h / 2);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surf->pixels);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(ATTRIB_U_POSITION);
    glDisableVertexAttribArray(ATTRIB_U_TEXTURE);

    // To framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

//    glUseProgram(shaderProgram2fb);
//    glBindAttribLocation(shaderProgram2fb, ATTRIB_U_POSITION, "u_position");
//    glBindAttribLocation(shaderProgram2fb, ATTRIB_U_TEXTURE, "u_texture");

    glUseProgram(shaderProgram2lens);
    glBindAttribLocation(shaderProgram2lens, ATTRIB_U_POSITION, "u_position");
    glBindAttribLocation(shaderProgram2lens, ATTRIB_U_TEXTURE, "u_texture");
    glUniform2f(glGetUniformLocation(shaderProgram2lens, "u_offset"), x, y);
    if (w > h) {
	glUniform2f(glGetUniformLocation(shaderProgram2lens, "u_resolution"), w / 2, h);
    } else {
	glUniform2f(glGetUniformLocation(shaderProgram2lens, "u_resolution"), w, h / 2);
    }
    glUniform1f(glGetUniformLocation(shaderProgram2lens, "u_angle"), lens_angle);

    glBindTexture(GL_TEXTURE_2D, renderedTexture);

    glVertexAttribPointer(ATTRIB_U_POSITION, 2, GL_FLOAT, 0, 0, squareVertices);
    glEnableVertexAttribArray(ATTRIB_U_POSITION);
    glVertexAttribPointer(ATTRIB_U_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices[texRotate]);
    glEnableVertexAttribArray(ATTRIB_U_TEXTURE);

    if (w > h) {
	glViewport (x, y, w / 2, h);
    } else {
	glViewport (x, y, w, h / 2);
    }
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(ATTRIB_U_POSITION);
    glDisableVertexAttribArray(ATTRIB_U_TEXTURE);
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
    int use_w, use_h;

    SDL_Rect srcrect;

    SDL_Surface *surf = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_ARGB8888, 0);

    if (mode.w > mode.h) {
	use_w = mode.w / 2;
	use_h = mode.h;
    } else {
	use_w = mode.w;
	use_h = mode.h / 2;
    }

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

    double lens_scale = 0.5f; //50 / 100 * 2.0 - 1.0;

    if (mode.w > mode.h) {
	RenderLensTexture(left, lens_scale, 0, 0, mode.w, mode.h);
	RenderLensTexture(right, lens_scale, mode.w / 2, 0, mode.w, mode.h);
    } else {
	RenderLensTexture(left, lens_scale, 0, 0, mode.w, mode.h);
	RenderLensTexture(right, lens_scale, 0, mode.h / 2, mode.w, mode.h);
#if 0

	double scale = (double) (mode.h / 2) / (double) left->h;
	double dW = (double) left->w * scale;

	{
	    double lens_scale = 0.0f; //50 / 100 * 2.0 - 1.0;
	    double lens_angle = lens_scale * M_PI;// * 0.9;

//	    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	    glUniform2f(glGetUniformLocation(shaderProgram2lens, "u_resolution"), mode.w, mode.h / 2);
	    glUniform1f(glGetUniformLocation(shaderProgram2lens, "u_angle"), lens_angle);

	    glViewport ((mode.w - dH) / 2, 0, dH, mode.h / 2);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, left->w, left->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, left->pixels);
	    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

//	glViewport ((mode.w - dH) / 2, mode.h / 2, dH, mode.h / 2);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, right->w, right->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, right->pixels);
//	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#endif
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

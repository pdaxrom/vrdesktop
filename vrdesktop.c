#include <stdio.h>
#include <SDL.h>
#include "vrout.h"
#include "xcbgrabber.h"

#define HMD_WIDTH	2560
#define HMD_HEIGHT	1440

int main(int argc, char *argv[])
{
    int i;
    int HMDindex = -1;
    int HMDwidth, HMDheight;
    SDL_DisplayMode mode;

    SDL_Init(SDL_INIT_VIDEO);

    for(i = 0; i < SDL_GetNumVideoDisplays(); ++i){
	int should_be_zero = SDL_GetCurrentDisplayMode(i, &mode);
	if(should_be_zero != 0) {
	    SDL_Log("Could not get display mode for video display #%d: %s", i, SDL_GetError());
	} else {
	    SDL_Log("Display #%d: current display mode is %dx%dpx @ %dhz.", i, mode.w, mode.h, mode.refresh_rate);
	    if (mode.w == HMD_WIDTH && mode.h == HMD_HEIGHT) {
		HMDindex = i;
		HMDwidth = mode.w;
		HMDheight = mode.h;
		SDL_Log("HMD found");
	    }
	}
    }

    struct XGrabber *cfg = GrabberInit(1920, 1080, 4);

    if (cfg) {
    if (InitVideo(HMDindex, HMDwidth, HMDheight)) {
//    if (InitVideo(HMDindex, 1280, 800)) {
	int quit = 0;
	SDL_Event e;

	while (!quit) {
	    while (SDL_PollEvent(&e) != 0) {
		if (e.type == SDL_QUIT) {
		    quit = 1;
		}
	    }

	    if (GrabberGetScreen(cfg, 0, 0, 1920, 1080)) {
	    } else {
		printf("Cannot capture!\n");
	    }

	    usleep(20000);
	}

	FinishVideo();
    } else {
	SDL_Log("Cannot initialize HMD!");
    }
	GrabberFinish(cfg);
    } else {
	SDL_Log("Cannot initialize xcb grabber.\n");
    }

    return 0;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <xcb/xcb.h>
#include <sys/shm.h>
#include <xcb/shm.h>
#include <inttypes.h>

#include <SDL.h>
#include <SDL_image.h>

#include "vrout.h"

#define XCB_ALL_PLANES (~0)

typedef struct {
    xcb_connection_t *connection;
    xcb_screen_t *screen;

    uint32_t shmid;
    uint8_t *shmaddr;
    xcb_shm_seg_t shmseg;
} XGrabber;

static int CheckXcbShm(XGrabber *cfg)
{
    xcb_shm_query_version_cookie_t cookie = xcb_shm_query_version(cfg->connection);
    xcb_shm_query_version_reply_t *reply;

    reply = xcb_shm_query_version_reply(cfg->connection, cookie, NULL);
    if (reply) {
        free(reply);
        return 1;
    }

    return 0;
}

static int AllocateShmBuffer(XGrabber *cfg, int max_size)
{
    cfg->shmid = shmget(IPC_PRIVATE, max_size, IPC_CREAT | 0777);
    if (cfg->shmid < 0) {
	fprintf(stderr, "shmget() error!\n");
	return 0;
    }

    cfg->shmseg = xcb_generate_id(cfg->connection);
    xcb_shm_attach(cfg->connection, cfg->shmseg, cfg->shmid, 0);
    cfg->shmaddr = shmat(cfg->shmid, NULL, 0);
    shmctl(cfg->shmid, IPC_RMID, 0);
    if ((int64_t)cfg->shmaddr == -1 || !cfg->shmaddr) {
	fprintf(stderr, "shmat() error!\n");
	return 0;
    }

/*

    cfg->shmaddr = shmat(cfg->shmid, 0, 0);
    if (cfg->shmaddr == (uint8_t *) -1) {
	fprintf(stderr, "shmat() error!");
	return 0;
    }

    cfg->shmseg = xcb_generate_id(cfg->connection);
    xcb_void_cookie_t attach_cookie = xcb_shm_attach_checked(cfg->connection, cfg->shmseg, cfg->shmid, 0);
    if (xcb_request_check(cfg->connection, attach_cookie) != NULL) {
	fprintf(stderr, "xcb_shm_attach_checked() error!\n");
	return 0;
    }
*/

    return 1;
}

void FreeShmBuffer(XGrabber *cfg)
{
	xcb_shm_detach_checked(cfg->connection, cfg->shmseg);
	
	shmdt(cfg->shmaddr);
}

int GrabberGetScreen(XGrabber *cfg, int x, int y, int w, int h)
{
    xcb_generic_error_t *e = NULL;
    xcb_shm_get_image_cookie_t cookie = xcb_shm_get_image(cfg->connection, cfg->screen->root, x, y, w, h, XCB_ALL_PLANES, XCB_IMAGE_FORMAT_Z_PIXMAP, cfg->shmseg, 0);
    xcb_shm_get_image_reply_t *reply = xcb_shm_get_image_reply(cfg->connection, cookie, &e);
    xcb_flush(cfg->connection);
   if (e) {
	fprintf(stderr,
	    "Cannot get the image data "
	    "event_error: response_type:%u error_code:%u "
	    "sequence:%u resource_id:%u minor_code:%u major_code:%u.\n",
	    e->response_type, e->error_code,
	    e->sequence, e->resource_id, e->minor_code, e->major_code);
	    free(e);
    }

    if (reply != NULL) {
//	SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(cfg->shmaddr, w, h, 8 * 4, w * 4, 0, 0, 0, 0);
//	IMG_SavePNG(surf, "haha.png");

//	FILE *outf = fopen("haha.dat", "wb");
//	if (outf) {
//	    fwrite(cfg->shmaddr, 1, w * h * 4, outf);
//	    fclose(outf);
//	}

	RenderVideo(cfg->shmaddr, w, h);

	free(reply);
	return 1;
    }
    return 0;
}

static xcb_screen_t *ScreenOfDisplay(XGrabber *cfg, int screenNum)
{
  xcb_screen_iterator_t iter;

  iter = xcb_setup_roots_iterator (xcb_get_setup (cfg->connection));
  for (; iter.rem; --screenNum, xcb_screen_next (&iter)) {
    if (screenNum == 0) {
      cfg->screen = iter.data;
      return cfg->screen;
    }
  }

  return NULL;
}

XGrabber *GrabberInit(int w, int h, int d)
{
    int i, screenNum;
    XGrabber *cfg = malloc(sizeof(XGrabber));

    cfg->connection = xcb_connect (NULL, &screenNum);

    if (!CheckXcbShm(cfg)) {
	SDL_Log("no xcb shm!");
	xcb_disconnect(cfg->connection);
	free(cfg);
	return NULL;
    }

    ScreenOfDisplay(cfg, screenNum);

    if (AllocateShmBuffer(cfg, w * h * d)) {

    } else {
	SDL_Log("Cannot allocate shm buffer!");
	xcb_disconnect(cfg->connection);
	free(cfg);
	return NULL;
    }

    SDL_Log("");
    SDL_Log("Informations of screen %"PRIu32":", cfg->screen->root);
    SDL_Log("  width.........: %"PRIu16, cfg->screen->width_in_pixels);
    SDL_Log("  height........: %"PRIu16, cfg->screen->height_in_pixels);
    SDL_Log("  white pixel...: %"PRIu32, cfg->screen->white_pixel);
    SDL_Log("  black pixel...: %"PRIu32, cfg->screen->black_pixel);
    SDL_Log("");

    return cfg;
}

void GrabberFinish(XGrabber *cfg)
{
    FreeShmBuffer(cfg);
    xcb_disconnect(cfg->connection);
    free(cfg);
}

/*

int main(int argc, char *argv[])
{
    XGrabber cfg;
    int i, screenNum;

    xcb_get_geometry_cookie_t gc;
    xcb_get_geometry_reply_t *geo;

    cfg.connection = xcb_connect (NULL, &screenNum);

    if (!CheckXcbShm(&cfg)) {
	fprintf(stderr, "no xcb shm!\n");
	return -1;
    }


//    const xcb_setup_t *setup = xcb_get_setup (connection);
//    xcb_screen_iterator_t iter = xcb_setup_roots_iterator (setup);

    printf("screenNum %d\n", screenNum);

    // we want the screen at index screenNum of the iterator
//    for (i = 0; i < screenNum; ++i) {
//        xcb_screen_next (&iter);
//    }

//    xcb_screen_t *screen = iter.data;
    ScreenOfDisplay(&cfg, screenNum);

    gc  = xcb_get_geometry(cfg.connection, cfg.screen->root);
    geo = xcb_get_geometry_reply(cfg.connection, gc, NULL);

    printf("width  %d\n", geo->width);
    printf("height %d\n", geo->height);

    if (AllocateShmBuffer(&cfg, 1920 * 1080 * 4)) {

	printf ("\n");
	printf ("Informations of screen %"PRIu32":\n", cfg.screen->root);
	printf ("  width.........: %"PRIu16"\n", cfg.screen->width_in_pixels);
	printf ("  height........: %"PRIu16"\n", cfg.screen->height_in_pixels);
	printf ("  white pixel...: %"PRIu32"\n", cfg.screen->white_pixel);
	printf ("  black pixel...: %"PRIu32"\n", cfg.screen->black_pixel);
	printf ("\n");

InitVideo(1280, 800);

int quit = 0;
SDL_Event e;
while (!quit) {
    while (SDL_PollEvent(&e) != 0) {
	if (e.type == SDL_QUIT) {
	    quit = 1;
	}
    }
	if (GetScreen(&cfg, 0, 0, 1920, 1080)) {
//	    printf("Captured!\n");

//	    int w = 1920;
//	    int h = 1080;

//	    SDL_Surface *surf = SDL_CreateRGBSurfaceFrom(cfg.shmaddr, w, h, 8 * 4, w * 4, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
//	    SDL_SaveBMP(surf, "haha.bmp");

	} else {
	    printf("Cannot capture!\n");
	}
	usleep(20000);
}

FinishVideo();

	FreeShmBuffer(&cfg);
    } else {
	fprintf(stderr, "Cannot allocate shm buffer!\n");
    }

    xcb_disconnect (cfg.connection);

    return 0;
}

*/
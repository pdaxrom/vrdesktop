#ifndef __XCBGRABBER_H__
#define __XCBGRABBER_H__

//typedef struct XGrabber;

struct XGrabber *GrabberInit(int w, int h, int d);
void GrabberFinish(struct XGrabber *cfg);
int GrabberGetScreen(struct XGrabber *cfg, int x, int y, int w, int h);

#endif

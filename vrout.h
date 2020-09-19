#ifndef __VROUT__
#define __VROUT__

int InitVideo(int w, int h);
void FinishVideo();
void RenderVideo(unsigned char *pixels, int w, int h);

#endif

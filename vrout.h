#ifndef __VROUT__
#define __VROUT__

int InitVideo(int index, int w, int h);
void FinishVideo();
void RenderVideo(unsigned char *pixels, int w, int h);
void ShowLogo(char *logofile);

#endif

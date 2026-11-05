/* AuthMee: zbar is used without its video/window modules. */
struct zbar_video_s;
int zbar_video_init(struct zbar_video_s *v, unsigned long fmt) { (void)v; (void)fmt; return -1; }

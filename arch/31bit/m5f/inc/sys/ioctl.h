/* sys/ioctl.h for M5f programs on CMS: no device control (cmsrt: -1) */
#ifndef M5F_SYS_IOCTL_H
#define M5F_SYS_IOCTL_H
int ioctl(int fd, unsigned long req, ...);
#endif

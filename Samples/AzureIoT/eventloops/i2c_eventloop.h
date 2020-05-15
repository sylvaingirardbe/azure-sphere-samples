#include "../applibs_versions.h"
#include <applibs/eventloop.h>

int initI2cTimer(EventLoop *eventLoop);
int closeI2cTimer();
int AccelTimerEventHandler(EventLoopTimer *eventData);
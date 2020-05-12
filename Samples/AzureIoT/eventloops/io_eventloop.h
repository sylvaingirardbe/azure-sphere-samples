#include <applibs/eventloop.h>

int initIo(EventLoop *eventLoop);
void closeIo(void);
static void ButtonPollTimerEventHandler(EventLoopTimer *timer);
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState);
static void SendMessageButtonHandler(void);
static void SendOrientationButtonHandler(void);

static GPIO_Value_Type sendMessageButtonState = GPIO_Value_High;
static GPIO_Value_Type sendOrientationButtonState = GPIO_Value_High;

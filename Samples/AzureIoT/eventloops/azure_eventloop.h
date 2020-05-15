#include <applibs/eventloop.h>

#include "../eventloop_timer_utilities.h"

#define SCOPEID_LENGTH 20
static EventLoopTimer *azureTimer = NULL;
static void AzureTimerEventHandler(EventLoopTimer *timer);
int initAzure(EventLoop *eventLoop);
void closeAzure(void);
static void SendSimulatedTemperature(void);
static int SendTemperature();

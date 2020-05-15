#include "../applibs_versions.h"
#include <applibs/eventloop.h>

#include "../eventloop_timer_utilities.h"
#include "../build_options.h"
#include "../exitcodes.h"
#include "../i2c.h"

#include "i2c_eventloop.h"

EventLoopTimer *accelTimer = NULL;

int initI2cTimer(EventLoop *eventLoop) {
    // Init the epoll interface to periodically run the AccelTimerEventHandler routine where we read the sensors

	// Define the period in the build_options.h file
	struct timespec accelReadPeriod = { .tv_sec = ACCEL_READ_PERIOD_SECONDS,.tv_nsec = ACCEL_READ_PERIOD_NANO_SECONDS };
	// event handler data structures. Only the event handler field needs to be populated.
	accelTimer = CreateEventLoopPeriodicTimer(eventLoop, &AccelTimerEventHandler, &accelReadPeriod);
	if (accelTimer == NULL) {
		return ExitCode_Init_AccelleroMeterTimer;
	}

    initI2c();
}

int closeI2cTimer() {
    DisposeEventLoopTimer(accelTimer);
    closeI2c();
    return 0;
}

int AccelTimerEventHandler(EventLoopTimer *eventData)
{
#if (defined(IOT_CENTRAL_APPLICATION) || defined(IOT_HUB_APPLICATION))
	static bool firstPass = true;
#endif
	// Consume the event.  If we don't do this we'll come right back 
	// to process the same event again
    if (ConsumeEventLoopTimerEvent(accelTimer) != 0) {
        return 0;
    }

    if(readSensorData() != 0) {
        return -1;
    }
}
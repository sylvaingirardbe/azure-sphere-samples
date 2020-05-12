#include <signal.h>
#include <string.h>
#include <errno.h>

#include "../applibs_versions.h"
#include <applibs/log.h>
#include <applibs/gpio.h>

#include <applibs/eventloop.h>

#include <hw/avnet_mt3620_sk.h>

#include "../eventloop_timer_utilities.h"
#include "../exitcodes.h"
#include "../fd.h"
#include "../azure_io.h"

#include "io_eventloop.h"

// File descriptors - initialized to invalid value
// Buttons
int sendMessageButtonGpioFd = -1;
int sendOrientationButtonGpioFd = -1;

// LED
int deviceTwinStatusLedGpioFd = -1;

static bool deviceIsUp = false; // Orientation

EventLoopTimer *buttonPollTimer = NULL;

extern volatile sig_atomic_t exitCode;

// Button state variables

int initIo(EventLoop *eventLoop) {
    // Open SAMPLE_BUTTON_1 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_1 as input\n");
    sendMessageButtonGpioFd = GPIO_OpenAsInput(MT3620_GPIO12);
    if (sendMessageButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_1: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_MessageButton;
    }

    // Open SAMPLE_BUTTON_2 GPIO as input
    Log_Debug("Opening SAMPLE_BUTTON_2 as input\n");
    sendOrientationButtonGpioFd = GPIO_OpenAsInput(MT3620_GPIO13);
    if (sendOrientationButtonGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_BUTTON_2: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_OrientationButton;
    }

    // SAMPLE_LED is used to show Device Twin settings state
    Log_Debug("Opening SAMPLE_LED as output\n");
    deviceTwinStatusLedGpioFd =
        GPIO_OpenAsOutput(MT3620_GPIO8, GPIO_OutputMode_PushPull, GPIO_Value_High);
    if (deviceTwinStatusLedGpioFd == -1) {
        Log_Debug("ERROR: Could not open SAMPLE_LED: %s (%d).\n", strerror(errno), errno);
        return ExitCode_Init_TwinStatusLed;
    }

    // Set up a timer to poll for button events.
    static const struct timespec buttonPressCheckPeriod = {.tv_sec = 0, .tv_nsec = 1000 * 1000};
    buttonPollTimer = CreateEventLoopPeriodicTimer(eventLoop, &ButtonPollTimerEventHandler,
                                                   &buttonPressCheckPeriod);
    if (buttonPollTimer == NULL) {
        return ExitCode_Init_ButtonPollTimer;
    }
}

void closeIo() {
    DisposeEventLoopTimer(buttonPollTimer);

    Log_Debug("Closing file descriptors\n");

    // Leave the LEDs off
    if (deviceTwinStatusLedGpioFd >= 0) {
        GPIO_SetValue(deviceTwinStatusLedGpioFd, GPIO_Value_High);
    }
    CloseFdAndPrintError(sendMessageButtonGpioFd, "SendMessageButton");
    CloseFdAndPrintError(sendOrientationButtonGpioFd, "SendOrientationButton");
    CloseFdAndPrintError(deviceTwinStatusLedGpioFd, "StatusLed");
}

/// <summary>
/// Pressing SAMPLE_BUTTON_1 will:
///     Send a 'Button Pressed' event to Azure IoT Central
/// </summary>
static void SendMessageButtonHandler(void)
{
    if (IsButtonPressed(sendMessageButtonGpioFd, &sendMessageButtonState)) {
        SendTelemetry("ButtonPress", "True");
    }
}

/// <summary>
/// Pressing SAMPLE_BUTTON_2 will:
///     Send an 'Orientation' event to Azure IoT Central
/// </summary>
static void SendOrientationButtonHandler(void)
{
    if (IsButtonPressed(sendOrientationButtonGpioFd, &sendOrientationButtonState)) {
        deviceIsUp = !deviceIsUp;
        SendTelemetry("Orientation", deviceIsUp ? "Up" : "Down");
    }
}

/// <summary>
/// Button timer event:  Check the status of the buttons
/// </summary>
static void ButtonPollTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0) {
        exitCode = ExitCode_ButtonTimer_Consume;
        return;
    }
    SendMessageButtonHandler();
    SendOrientationButtonHandler();
}

/// <summary>
///     Check whether a given button has just been pressed.
/// </summary>
/// <param name="fd">The button file descriptor</param>
/// <param name="oldState">Old state of the button (pressed or released)</param>
/// <returns>true if pressed, false otherwise</returns>
static bool IsButtonPressed(int fd, GPIO_Value_Type *oldState)
{
    bool isButtonPressed = false;
    GPIO_Value_Type newState;
    int result = GPIO_GetValue(fd, &newState);
    if (result != 0) {
        Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
        exitCode = ExitCode_IsButtonPressed_GetValue;
    } else {
        // Button is pressed if it is low and different than last known state.
        isButtonPressed = (newState != *oldState) && (newState == GPIO_Value_Low);
        *oldState = newState;
    }

    return isButtonPressed;
}

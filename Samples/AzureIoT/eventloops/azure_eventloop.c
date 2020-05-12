#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include <applibs/eventloop.h>
#include <applibs/networking.h>
#include <applibs/log.h>

#include "../eventloop_timer_utilities.h"

#include "../exitcodes.h"
#include "../azure_io.h"

#include "azure_eventloop.h"

// Azure IoT poll periods
const int AzureIoTDefaultPollPeriodSeconds = 5;

extern volatile sig_atomic_t exitCode;

/// <summary>
/// Azure timer event:  Check connection status and send telemetry
/// </summary>
static void AzureTimerEventHandler(EventLoopTimer *timer)
{
    if (ConsumeEventLoopTimerEvent(timer) != 0)
    {
        exitCode = ExitCode_AzureTimer_Consume;
        return;
    }

    bool isNetworkReady = false;
    if (Networking_IsNetworkingReady(&isNetworkReady) != -1)
    {
        if (isNetworkReady && !iothubAuthenticated)
        {
            SetupAzureClient(azureTimer);
        }
    }
    else
    {
        Log_Debug("Failed to get Network state\n");
    }

    if (iothubAuthenticated)
    {
        SendSimulatedTemperature();
        IoTHubDeviceClient_LL_DoWork(iothubClientHandle);
    }
}

/// <summary>
///     Generates a simulated Temperature and sends to IoT Hub.
/// </summary>
static void SendSimulatedTemperature(void)
{
    static float temperature = 30.0;
    float deltaTemp = (float)(rand() % 20) / 20.0f;
    if (rand() % 2 == 0)
    {
        temperature += deltaTemp;
    }
    else
    {
        temperature -= deltaTemp;
    }

    char tempBuffer[20];
    int len = snprintf(tempBuffer, 20, "%3.2f", temperature);
    if (len > 0) {
        SendTelemetry("Temperature", tempBuffer);
    }
}

int initAzure(EventLoop *eventLoop)
{
    // azureIoTPollPeriodSeconds = AzureIoTDefaultPollPeriodSeconds;
    struct timespec azureTelemetryPeriod = {.tv_sec = AzureIoTDefaultPollPeriodSeconds, .tv_nsec = 0};
    azureTimer =
        CreateEventLoopPeriodicTimer(eventLoop, &AzureTimerEventHandler, &azureTelemetryPeriod);
    if (azureTimer == NULL)
    {
        return ExitCode_Init_AzureTimer;
    }
}

void closeAzure(void)
{
    DisposeEventLoopTimer(azureTimer);
}

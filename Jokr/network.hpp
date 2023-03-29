#pragma once

#ifdef __PSP__

#include <pspsdk.h>
#include <pspnet_inet.h>
#include <psputility.h>
#include <pspnet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <arpa/inet.h>

#include <string.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#define RESOLVE_NAME "google.com"

extern "C"
{
    char *basename(const char *filename)
    {
        char *p = strrchr(filename, '/');
        return p ? p + 1 : (char *)filename;
    }
}

int connect_to_apctl(int config)
{
    int err;
    int stateLast = -1;
    int elapsed = 0;

    /* Connect using the first profile */
    err = sceNetApctlConnect(config);
    if (err != 0)
    {
        printf("sceNetApctlConnect returns %08X\n", err);
        return 0;
    }

    printf(": Connecting...\n");

    // print APCTL profile info
    union SceNetApctlInfo info;
    if (sceNetApctlGetInfo(1, &info) == 0)
        printf("  SSID: %s\n", info.ssid);
    if (sceNetApctlGetInfo(2, &info) == 0)
        printf("  BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
               info.bssid[0], info.bssid[1], info.bssid[2],
               info.bssid[3], info.bssid[4], info.bssid[5]);
    if (sceNetApctlGetInfo(3, &info) == 0)
        printf("  Channel: %d\n", info.channel);
    // name
    if (sceNetApctlGetInfo(4, &info) == 0)
        printf("  Name: %s\n", info.name);

    while (1)
    {
        int state;
        err = sceNetApctlGetState(&state);

        if (elapsed > 10000)
        {
            printf(": Timeout\n");
            config++;
            err = sceNetApctlConnect(config);
            if (err != 0)
            {
                printf("sceNetApctlConnect returns %08X\n", err);
                return 0;
            }
            printf(": Connecting...\n");
            elapsed = 0;
            stateLast = -1;
        }

        if (err != 0)
        {
            printf(": sceNetApctlGetState returns $%x\n", err);
            break;
        }
        if (state > stateLast)
        {
            printf("  connection state %d of 4\n", state);
            stateLast = state;
        }
        if (state == 4)
            break; // connected with static IP

        // wait a little before polling again
        sceKernelDelayThread(50 * 1000); // 50ms
        elapsed += 50;
    }
    printf(": Connected!\n");

    if (err != 0)
    {
        return 0;
    }

    return 1;
}

void do_resolver(void)
{
    int rid = -1;
    char buf[1024];
    struct in_addr addr;
    char name[1024];

    do
    {
        /* Create a resolver */
        if (sceNetResolverCreate(&rid, buf, sizeof(buf)) < 0)
        {
            printf("Error creating resolver\n");
            break;
        }

        printf("Created resolver %08x\n", rid);

        /* Resolve a name to an ip address */
        if (sceNetResolverStartNtoA(rid, RESOLVE_NAME, &addr, 2, 3) < 0)
        {
            printf("Error resolving %s\n", RESOLVE_NAME);
            break;
        }

        printf("Resolved %s to %s\n", RESOLVE_NAME, inet_ntoa(addr));

        /* Resolve the ip address to a name */
        if (sceNetResolverStartAtoN(rid, &addr, name, sizeof(name), 2, 3) < 0)
        {
            printf("Error resolving ip to name\n");
            break;
        }

        printf("Resolved ip to %s\n", name);
    } while (0);

    if (rid >= 0)
    {
        sceNetResolverDelete(rid);
    }
}

int net_thread(SceSize args, void *argp)
{
    int err;

    do
    {
        if ((err = pspSdkInetInit()))
        {
            printf("Error, could not initialise the network %08X\n", err);
            break;
        }

        if (connect_to_apctl(1))
        {
            // connected, get my IPADDR and run test
            union SceNetApctlInfo info;

            if (sceNetApctlGetInfo(8, &info) != 0)
                strcpy(info.ip, "unknown IP");

            do_resolver();
        }
    } while (0);

    return 0;
}

int netDialog(SDL_Renderer *renderer)
{
    int done = 0;

    pspUtilityNetconfData data;

    memset(&data, 0, sizeof(data));
    data.base.size = sizeof(data);
    data.base.language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
    data.base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;
    data.base.graphicsThread = 17;
    data.base.accessThread = 19;
    data.base.fontThread = 18;
    data.base.soundThread = 16;
    data.action = PSP_NETCONF_ACTION_CONNECTAP;

    struct pspUtilityNetconfAdhoc adhocparam;
    memset(&adhocparam, 0, sizeof(adhocparam));
    data.adhocparam = &adhocparam;

    sceUtilityNetconfInitStart(&data);

    while (!done)
    {
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        switch (sceUtilityNetconfGetStatus())
        {
        case PSP_UTILITY_DIALOG_NONE:
            break;

        case PSP_UTILITY_DIALOG_VISIBLE:
            sceUtilityNetconfUpdate(1);
            break;

        case PSP_UTILITY_DIALOG_QUIT:
            printf("PSP_UTILITY_DIALOG_QUIT");
            sceUtilityNetconfShutdownStart();
            break;

        case PSP_UTILITY_DIALOG_FINISHED:
            done = 1;
            break;

        default:
            break;
        }
    }

    return 1;
}

void netInit(void)
{
    sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024);
    sceNetInetInit();
    sceNetApctlInit(0x8000, 48);
}

void startNetworking(SDL_Renderer *renderer)
{
    int rc = sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    if (rc < 0)
        printf("net common didn't load.\n");
    rc = sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    if (rc < 0)
        printf("inet didn't load.\n");

    netInit();
    netDialog(renderer);
}

void netTerm(void)
{
    sceNetApctlTerm();
    sceNetInetTerm();
    sceNetTerm();
}

void stopNetworking()
{
    sceUtilityUnloadNetModule(PSP_NET_MODULE_INET);
    sceUtilityUnloadNetModule(PSP_NET_MODULE_COMMON);
    netTerm();
}

#else
#define stopNetworking()
#define startNetworking(x)
#endif

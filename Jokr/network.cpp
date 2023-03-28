#include "network.hpp"

#ifdef __PSP__
#include <pspdebug.h>
#include <psputility.h>
#include <pspdisplay.h>

#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <pspnet_resolver.h>
#include <psphttp.h>

#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int netDialogActive = -1;

#ifdef __PSP__

#define BUF_WIDTH (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)

#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>

/* Graphics stuff, based on cube sample */
static unsigned int __attribute__((aligned(16))) list[262144];

void setupGu()
{
  sceGuInit();

  sceGuStart(GU_DIRECT, list);
  sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_WIDTH);
  sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void *)0x88000, BUF_WIDTH);
  sceGuDepthBuffer((void *)0x110000, BUF_WIDTH);
  sceGuOffset(2048 - (SCR_WIDTH / 2), 2048 - (SCR_HEIGHT / 2));
  sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
  sceGuDepthRange(0xc350, 0x2710);
  sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuDepthFunc(GU_GEQUAL);
  sceGuEnable(GU_DEPTH_TEST);
  sceGuFrontFace(GU_CW);
  sceGuShadeModel(GU_SMOOTH);
  sceGuEnable(GU_CULL_FACE);
  sceGuEnable(GU_CLIP_PLANES);
  sceGuFinish();
  sceGuSync(0, 0);

  sceDisplayWaitVblankStart();
  sceGuDisplay(GU_TRUE);
}

// not needed in the new SDK
extern "C"
{
  char *basename(const char *filename)
  {
    char *p = strrchr(filename, '/');
    return p ? p + 1 : (char *)filename;
  }
}

void loadNetworkingLibs()
{
  int rc = sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
  if (rc < 0)
    printf("net common didn't load.\n");
  rc = sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
  if (rc < 0)
    printf("inet didn't load.\n");
}

void httpInit()
{
  pspDebugScreenPrintf("Loading module SCE_SYSMODULE_HTTP\n");
  sceUtilityLoadNetModule(PSP_NET_MODULE_HTTP);

  pspDebugScreenPrintf("Running sceHttpInit\n");
  sceHttpInit(4 * 1024 * 1024);
}

void httpTerm()
{
  pspDebugScreenPrintf("Running sceHttpTerm\n");
  sceHttpEnd();

  pspDebugScreenPrintf("Unloading module SCE_SYSMODULE_HTTP\n");
  sceUtilityUnloadNetModule(PSP_NET_MODULE_HTTP);
}

int goOnline()
{
  sceNetInit(128 * 1024, 42, 4 * 1024, 42, 4 * 1024);
  sceNetInetInit();
  sceNetApctlInit(0x8000, 48);
  sceNetResolverInit();
  if (!netDialog())
  {
    printf("Could not access networking dialog! %d", 30000);
    stopNetworking();
    return 1;
  }
  httpInit();
  return 0;
}

pspUtilityNetconfData data;

int netDialog()
{
  memset(&data, 0, sizeof(data));
  data.base.size = sizeof(data);
  data.base.language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
  data.base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;
  data.base.graphicsThread = 17;
  data.base.accessThread = 19;
  data.base.fontThread = 18;
  data.base.soundThread = 16;
  data.action = PSP_NETCONF_ACTION_CONNECTAP;

  netDialogActive = -1;
  int result = sceUtilityNetconfInitStart(&data);
  printf("sceUtilityNetconfInitStart: %08x\n", result);
  if (result < 0)
  {
    data.base.size = sizeof(pspUtilityNetconfData) - 12;
    result = sceUtilityNetconfInitStart(&data);
    printf("sceUtilityNetconfInitStart again: %08x\n", result);
    if (result < 0)
      return 0;
  }
  netDialogActive = 0;

  return 1;
}

// returns -1 on quit, 0 on active, and 1 on success
int drawNetDialog()
{
  int done = 0;

  switch (sceUtilityNetconfGetStatus())
  {
  case PSP_UTILITY_DIALOG_NONE:
    printf("None\n");
    break;
  case PSP_UTILITY_DIALOG_INIT:
    break;
  case PSP_UTILITY_DIALOG_VISIBLE:
    sceUtilityNetconfUpdate(1);
    break;
  case PSP_UTILITY_DIALOG_QUIT:
    printf("NetDialog was quit.\n");
    sceUtilityNetconfShutdownStart();
    done = -1;
    break; // cancelled??
  case PSP_UTILITY_DIALOG_FINISHED:
    printf("NetDialog completed successfully.\n");
    sceUtilityNetconfShutdownStart();
    done = 1;
    break;
  default:
    printf("NetconfGetStatus: %08x\n", sceUtilityNetconfGetStatus());
    break;
  }

  return done;
}

void startNetworking()
{
  printf("Loading Netowrk libs...\n");
  loadNetworkingLibs();
  printf("Initing networking\n");
  goOnline();

  printf("Starting dialog\n");
  while (netDialogActive != 1)
  {
    netDialogActive = drawNetDialog();

    if (netDialogActive == -1)
    {
      printf("Dialog was quit.\n");
      break;
    }

    sceDisplayWaitVblankStart();
  }

  printf("Dialog finished.\n");
  sceGuFinish();
  atexit(stopNetworking);
}

void stopNetworking()
{
  printf("Shutting down networking.\n");
  httpTerm();
  sceNetResolverTerm();
  sceNetApctlTerm();
  sceNetInetTerm();
  sceNetTerm();
}
#endif
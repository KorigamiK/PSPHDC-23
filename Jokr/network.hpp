#pragma once

#ifdef __PSP__
int drawNetDialog();
int netDialog();
void stopNetworking();
void startNetworking();
#else
#define stopNetworking()
#define startNetworking()
#endif

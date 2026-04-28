#ifndef SERVER_H
#define SERVER_H

#include "compositor.h"

int init_server();
int cleanup_server(struct gemstone_server *server);

#endif
#include <stdio.h>
#include <iostream>
#include <vector>
#include "relay_server.h"

int main() {
	CRelayServer *server = new CRelayServer();
	server->LoadConfiguration();
	if (server->Start())
		printf("sever init\n");
	int a = 1;
	while (a) {
		std::cin >> a;
	}
	return 0;
}

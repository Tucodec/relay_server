#include <iostream>
#include <string>
#include <simple_uv/log4z.h>
#include "master.h"
using namespace zsummer::log4z;

bool is_exit = false;

int main(int argc, char** argv){
	ILog4zManager::getRef().start();
	CTestGateWay server;
	server.LoadConfig();
	if(!server.Start()) {
		fprintf(stdout,"Start Server error:%s\n",server.GetLastErrMsg());
	}
	server.SetKeepAlive(1,60);//enable Keepalive, 60s
	while(!is_exit) {
		uv_thread_sleep(100000);
	}
	return 0;
}
#include "server.h"
#include "channel.h"

int32_t main(int32_t argc, char* argv[])
{
	int32_t ret = 0;
	std::string channel;
	
	for (int32_t arg=0; arg < argc; ++arg)
	{
		if (!strncmp(argv[arg], "--channel=", 10)){
			channel.assign(argv[arg] + 10);
		}
	}
	
	if (channel.empty()) 
	{
		ret = Server::run();		
	}
	else
	{
		ret = Channel::run(channel);		
	}

	return ret;
}
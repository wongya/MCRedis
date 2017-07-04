#include "../MCRedis.h"
#include <stdio.h>

#ifdef WIN32
#	include <winsock2.h>
#endif //WIN32

int main()
{
#ifdef WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 0), &wsa);
#endif // WIN32

	auto fn=[](MCRedis::CConnection* ptr)
	{
		ptr->sendCommand(MCRedis::CCommand("select",0));
		printf("New session allocated\n");
	};
	MCRedis::MiddleWare::CDefaultMiddleWare mw1("redis-test.sk8irc.0001.apn2.cache.amazonaws.com",6379,fn);
	MCRedis::MiddleWare::CSentinelSupport mw2("redis-test.sk8irc.0001.apn2.cache.amazonaws.com", 26379, "mymaster", MCRedis::MiddleWare::CSentinelSupport::ERole::MASTER, fn);
	MCRedis::CConnectionPool<std::mutex,decltype(mw2), decltype(mw1)> redisPool(std::move(mw2), std::move(mw1));
	if(redisPool.create(1)==false)
		printf("create redis connection pool failed\n");

	{
		MCRedis::CRunner<decltype(redisPool)> runner(redisPool);
		for(int x=0;x<10;++x)
		{
			auto rpy=runner.run(MCRedis::CCommand("select",0));
			printf("rpy %d-%s\n", x, rpy.getStr().c_str());
			//runner.append(MCRedis::CCommand("select",0));
			//auto rpy=runner.runCommands([](MCRedis::CReply& rpy){});
			//printf("rpy %d-%s\n",x,rpy.at(0).getStr().c_str());
		}
	}

	{
		MCRedis::CAsyncRunner<decltype(redisPool)> runner(redisPool);
		//for(int x=0;x<10;++x)
		{
			runner.append(MCRedis::CCommand("info"));
			runner.runCommands(
				[](MCRedis::CReply& rpy)
				{
					printf("rpy %s\n",rpy.getStr().c_str());
				}
			);
			//runner.run(MCRedis::CCommand("select",0));
			//MCRedis::CReply rpy=runner.getResult();
			//printf("async rpy %d-%s\n",x,rpy.getStr().c_str());
			//runner.run(MCRedis::CCommand("debug","sleep","1"));
		}
	}
	MCRedis::CAsyncRunner<decltype(redisPool)>::join();
	return 0;
}

#include "../MCRedis.h"
#include <stdio.h>

#ifdef _MSC_VER
#	include <winsock2.h>
#endif //_MSC_VER

int main()
{
#ifdef _MSC_VER
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 0), &wsa);
#endif // _MSC_VER

	auto fn=[](MCRedis::CConnection* ptr)
	{
		ptr->sendCommand(MCRedis::CCommand("select",0));
		printf("New session allocated\n");
	};
	MCRedis::MiddleWare::CDefaultMiddleWare mw("localhost",6379,fn);
	MCRedis::CConnectionPool<std::mutex,decltype(mw)> redisPool(std::move(mw));
	if(redisPool.create(1)==false)
		printf("create redis connection pool failed\n");

	{
		MCRedis::CRunner<decltype(redisPool)> runner(redisPool);
		for(int x=0;x<10;++x)
		{
			//auto rpy=runner.run(MCRedis::CCommand("select",0));
			runner.append(MCRedis::CCommand("select",0));
			auto rpy=runner.runCommands([](MCRedis::CReply& rpy){});
			printf("rpy %d-%s\n",x,rpy.getStr().c_str());
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

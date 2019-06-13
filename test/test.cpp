#include "../MCRedis.h"
#include <stdio.h>

#ifdef WIN32
#	include <winsock2.h>
#endif //WIN32


void redisClientTest()
{
	MCRedis::MiddleWare::CClusterSupport mw("192.168.244.128", 6379, 3);
	MCRedis::CConnectionPool<std::mutex, decltype(mw)> redisClient(std::move(mw));
	redisClient.create(10);

	//MCRedis::CRunner<decltype(redisClient)> runner(redisClient);
	MCRedis::CAsyncRunner<decltype(redisClient)> runner(redisClient);
	runner.append(MCRedis::CCommand("SET", "{12345}345", 1));
	runner.append(MCRedis::CCommand("GET", "{qqqqq}345"));
	runner.append(MCRedis::CCommand("SET", "{1}2345", 1));
	runner.append(MCRedis::CCommand("GET", "{1}2345"));
	runner.append(MCRedis::CCommand("SET", "{hehe}2345", 1));
	runner.append(MCRedis::CCommand("GET", "{hehe}2345"));
	/*auto rpy = */runner.runCommands(
		[](MCRedis::CReply& rpy)
		{
			printf("rpy %s\n", rpy.getStr().c_str());
		}
	);
	runner.getResult();

	//redisClient.sendCommand(MCRedis::CCommand("SET", "{1}2345", 1));
	//if (rpy.getType() != MCRedis::CReply::EType::INTEGER)
	//	return;
	//rpy = redisClient.sendCommand(MCRedis::CCommand("GET", "{1}2345"));
	//if (rpy.getType() != MCRedis::CReply::EType::INTEGER)
	//	return;


	//MCRedis::CRunner<decltype(redisClient)> runner(redisClient);
	//rpy = runner.run(MCRedis::CCommand("select", 0));
	//printf("rpy %s\n", rpy.getStr().c_str());

	redisClient.clear();

	exit(0);
}

int main()
{
#ifdef WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 0), &wsa);
#endif // WIN32

	redisClientTest();

	auto fn=[](MCRedis::CConnection* ptr)
	{
		ptr->setTimeout(3);
		ptr->sendCommand(MCRedis::CCommand("select",0));
		printf("New session allocated\n");
	};
	MCRedis::MiddleWare::CDefaultMiddleWare mw1("192.168.244.128",6000,fn);

	MCRedis::MiddleWare::CSentinelSupport mw2({ std::make_tuple("192.168.244.128", 26379) }, "mymaster", MCRedis::MiddleWare::CSentinelSupport::ERole::MASTER, fn);
	MCRedis::CConnectionPool<std::mutex,decltype(mw2), decltype(mw1)> redisPool(std::move(mw2), std::move(mw1));
	if(redisPool.create(2)==false)
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

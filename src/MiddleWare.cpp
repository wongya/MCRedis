#include "../MCRedis.h"
#include "../MCRedis/MiddleWare.h"

namespace MCRedis
{
	namespace MiddleWare
	{
		CSentinelSupport::CSentinelSupport(std::string hostName, uint16_t port, std::string masterName, ERole role, callback_t callback/* = nullptr*/, uint32_t timeoutSec /*= 3*/)
			: masterName_(masterName)
			, role_(role)
			, callback_(callback)
			, timeoutSec_(timeoutSec)
		{
			std::unique_ptr<CConnection> p(new CConnection);
			if (p->connect(hostName.c_str(), port) == false)
				return;

			CReply rpy=p->sendCommand(CCommand("SENTINEL", "sentinels", masterName_));
			for (auto& sub_r : rpy.getReplies())
			{
				lstSentinel_.push_back(std::make_tuple(sub_r.getReplies().at(3).getStr(), (uint16_t)std::stol(sub_r.getReplies().at(5).getStr())));
			}
			lstSentinel_.push_back(std::make_tuple(hostName, port));

			std::srand(std::random_device()());
		}

		CSentinelSupport::CSentinelSupport(std::vector<std::tuple<std::string, uint16_t>> lstSentinel, std::string masterName, ERole role, callback_t callback /*= nullptr*/, uint32_t timeoutSec /*= 3*/)
			: masterName_(masterName)
			, role_(role)
			, timeoutSec_(timeoutSec)
			, callback_(callback)
		{
			if (lstSentinel.empty() == true)
				return;

			for (auto& host : lstSentinel)
			{
				lstSentinel_.push_back(host);

				std::unique_ptr<CConnection> p(new CConnection);
				if (p->connect(std::get<0>(host).c_str(), std::get<1>(host), timeoutSec_) == false)
					continue;

				CReply rpy = p->sendCommand(CCommand("SENTINEL", "sentinels", masterName_));
				if (rpy.getType() != CReply::EType::ARRAY)
				{
					lstSentinel_.clear();
					return;
				}
			}

			std::srand(std::random_device()());
		}

		CConnection* CSentinelSupport::getConnection() const noexcept
		{
			if(lstSentinel_.empty()==true)
				return nullptr;

			std::unique_ptr<CConnection> sentinel;
			lstSentinel_t::value_type target;
			for (auto iter = lstSentinel_.begin(); iter != lstSentinel_.end(); ++iter)
			{
				target = *(iter);

				sentinel.reset(new CConnection());
				if (sentinel->connect(std::get<0>(target).c_str(), std::get<1>(target), timeoutSec_) == true)
					break;
				sentinel.reset();
			}
			if (sentinel == nullptr)
				return nullptr;

			CConnection* newConnection = nullptr;
			if (role_ == ERole::MASTER)
			{
				CReply rpy = sentinel->sendCommand(CCommand("SENTINEL", "get-master-addr-by-name", masterName_));
				if (rpy.getType() != CReply::EType::ARRAY || rpy.getReplies().size() < 2)
					return nullptr;

				newConnection = new CConnection;
				if (newConnection->connect(rpy.getReplies().at(0).getStr().c_str(), (uint16_t)std::stoul(rpy.getReplies().at(1).getStr())) == false)
				{
					delete newConnection;
					return nullptr;
				}
			}
			else if (role_ == ERole::SLAVE)
			{
				CReply rpy = sentinel->sendCommand(CCommand("SENTINEL", "slaves", masterName_));
				if (rpy.getType() != CReply::EType::ARRAY)
					return nullptr;

				std::vector<std::tuple<std::string, uint16_t>> slaves;
				slaves.reserve(rpy.getReplies().size());

				for (auto& sub_r : rpy.getReplies())
				{
					std::string ip= sub_r.getReplies().at(3).getStr();
					uint16_t port=(uint16_t)std::stoul(sub_r.getReplies().at(5).getStr());
	
					//flag
					if (sub_r.getReplies().at(9).getStr() != "slave")
						continue;

					//role-reported
					if (sub_r.getReplies().at(23).getStr() != "slave")
						continue;

					slaves.push_back(std::make_tuple(std::move(ip),port));
				}

				if (slaves.empty() == true)
					return nullptr;

				auto target = slaves.at(std::rand()%slaves.size());

				newConnection = new CConnection;
				if (newConnection->connect(std::get<0>(target).c_str(), std::get<1>(target)) == false)
				{
					delete newConnection;
					return nullptr;
				}
			}
			if (callback_ != nullptr && newConnection != nullptr)
				callback_(newConnection);
			return newConnection;
		}
	}
}

#include "../MCRedis.h"
#include "../MCRedis/MiddleWare.h"
#include <sstream>
#include <cstring>

namespace MCRedis
{
	namespace MiddleWare
	{
		CSentinelSupport::CSentinelSupport(std::string hostName, uint16_t port, std::string masterName, ERole role, callback_t callback/* = nullptr*/, uint32_t timeoutSec /*= 3*/)
			: masterName_(masterName)
			, role_(role)
			, timeoutSec_(timeoutSec)
			, callback_(callback)
		{
			std::unique_ptr<CConnection> p(new CConnection);
			if (p->connect(hostName.c_str(), port, timeoutSec_) == false)
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

			std::copy(lstSentinel.begin(), lstSentinel.end(), std::back_inserter(lstSentinel_));

			std::srand(std::random_device()());
		}

		CConnection* CSentinelSupport::getConnection(uint32_t ) const noexcept
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
				if (newConnection->connect(std::get<0>(target).c_str(), std::get<1>(target), timeoutSec_) == false)
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

	namespace MiddleWare
	{
		CClusterSupport::CClusterSupport(std::string hostName, uint16_t port, uint32_t timeoutSec)
		{
			timeoutSec_ = timeoutSec;
			lstHost_.push_back(std::make_tuple(hostName, port));
		}

		CClusterSupport::CClusterSupport(lstHost_t lstHost, uint32_t timeoutSec)
		{
			timeoutSec_ = timeoutSec;
			std::copy(lstHost.begin(), lstHost.end(), std::back_inserter(lstHost_));
		}

		CConnection* CClusterSupport::getConnection(uint32_t slot) const noexcept
		{
			using statusMap_t = std::map<std::string, std::string>;
			auto fnToStatusMap = [](const std::string& str) -> statusMap_t
			{
				statusMap_t statusMap;

				std::istringstream iss(str);
				std::string line;
				while (std::getline(iss, line))
				{
					if (line.empty() == true || line.at(0) == '#' || line.at(0) == '\r')
						continue;
					auto pos = line.find_first_of(':');
					if (pos == std::string::npos)
						continue;
					
					statusMap.insert(statusMap_t::value_type(line.substr(0, pos), line.substr(pos + 1, line.size() - (pos + 2))));
				}
				return statusMap;
			};

			std::unique_ptr<CConnection> conn;
			for (auto host : lstHost_)
			{
				CConnection* connection = new CConnection();
				if (connection->connect(std::get<0>(host).c_str(), std::get<1>(host), timeoutSec_) == true)
				{
					conn.reset(connection);
					break;
				}
			}
			if (conn == nullptr)
				return nullptr;

			std::vector<std::tuple<uint32_t, uint32_t, std::string, uint16_t>> lstNode;
			{
				auto rpy = conn->sendCommand(CCommand("CLUSTER", "info"));
				if (rpy.getType() != CReply::EType::STRING)
					return nullptr;

				auto infoStatus = fnToStatusMap(rpy.getStr());
				auto iter = infoStatus.find("cluster_state");
				if (iter == infoStatus.end() || iter->second != "ok")
					return nullptr;

				iter = infoStatus.find("cluster_known_nodes");
				if (iter != infoStatus.end())
					lstNode.reserve(std::stoull(iter->second));
			}

			{
				auto rpy = conn->sendCommand(CCommand("CLUSTER", "slots"));
				for (auto& nodes : rpy.getReplies())
				{
					uint32_t beginSlotReange = (uint32_t)nodes.getReplies().at(0).getInteger();
					uint32_t endSlotReange = (uint32_t)nodes.getReplies().at(1).getInteger();
					std::string masterIp = nodes.getReplies().at(2).getReplies().at(0).getStr();
					uint16_t masterPort = (uint16_t)nodes.getReplies().at(2).getReplies().at(1).getInteger();
					lstNode.push_back({ beginSlotReange ,endSlotReange , masterIp, masterPort });
				}
			}

			for (auto node : lstNode)
			{
				uint32_t beginSlotReange = std::get<0>(node);
				uint32_t endSlotReange = std::get<1>(node);
				std::string masterIp = std::get<2>(node);
				uint16_t masterPort = std::get<3>(node);
				if (slot >= beginSlotReange && slot <= endSlotReange)
				{
					CConnection* newConnection = new CConnection;
					if (newConnection->connect(masterIp.c_str(), masterPort, timeoutSec_) == false)
					{
						delete newConnection;
						return nullptr;
					}
					newConnection->setSlotRange(beginSlotReange, endSlotReange);
					return newConnection;
				}
			}

			return nullptr;
		}

		uint32_t CClusterSupport::getSlot(const char* key, size_t keyLen) const noexcept
		{
			if (key == nullptr)
				return std::numeric_limits<uint32_t>::max();

			auto fnHashSlot = [](const char* key, size_t keylen) -> uint32_t
			{
				auto fnCRC16 = [](const char *str, size_t len) -> uint16_t
				{
					static const uint16_t crctable[256] =
					{
						0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
						0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
						0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
						0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
						0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
						0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
						0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
						0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
						0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
						0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
						0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
						0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
						0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
						0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
						0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
						0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
						0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
						0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
						0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
						0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
						0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
						0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
						0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
						0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
						0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
						0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
						0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
						0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
						0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
						0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
						0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
						0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
					};

					uint16_t crc = 0;
					for (size_t i = 0; i < len; ++i)
						crc = (crc << 8) ^ crctable[((crc >> 8) ^ *str++) & 0x00FF];
					return crc;
				};

				int s, e; /* start-end indexes of { and } */

				/* Search the first occurrence of '{'. */
				for (s = 0; s < keylen; s++)
					if (key[s] == '{') break;

				/* No '{' ? Hash the whole key. This is the base case. */
				if (s == keylen) return fnCRC16(key, keylen) & 16383;

				/* '{' found? Check if we have the corresponding '}'. */
				for (e = s + 1; e < keylen; e++)
					if (key[e] == '}') break;

				/* No '}' or nothing between {} ? Hash the whole key. */
				if (e == keylen || e == s + 1) return fnCRC16(key, keylen) & 16383;

				/* If we are here there is both a { and a } on its right. Hash
				* what is in the middle between { and }. */
				return fnCRC16(key + s + 1, e - s - 1) & 16383;
			};
			return fnHashSlot(key, keyLen);
		}
	}
}

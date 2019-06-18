#pragma once

namespace MCRedis
{
	namespace MiddleWare
	{
		class CDefaultMiddleWare
		{
		protected:
			using callback_t = std::function<void(CConnection*)>;
			std::string		hostName_;
			uint16_t		port_;
			uint32_t		timeoutSec_;

			callback_t		callback_;

		public:
			CDefaultMiddleWare(std::string hostName, uint16_t port, callback_t callback, uint32_t timeoutSec = 3) : hostName_(hostName), port_(port), timeoutSec_(timeoutSec), callback_(callback) {}
			~CDefaultMiddleWare() = default;

		public:
			std::vector<CConnection*>	create(size_t num) const noexcept
			{
				std::vector<CConnection*> lstConn;
				lstConn.reserve(num);
				for (size_t x = 0; x < num; ++x)
				{
					auto p = getConnection();
					if (p == nullptr)
						continue;
					lstConn.push_back(p);
				}
				return lstConn;
			}

			CConnection*	getConnection(uint32_t /*slot*/ = 0) const noexcept
			{	
				CConnection* p = new CConnection;
				if (p->connect(hostName_.c_str(), port_, timeoutSec_) == false)
				{
					delete p;
					return nullptr;
				}
				if(callback_!=nullptr)
					callback_(p);
				return p;
			}
			void			freeConnection(CConnection* p) noexcept { delete p; }
			uint32_t		getSlot(const char*, size_t ) const noexcept { return std::numeric_limits<uint32_t>::max(); }
		};

		class CSentinelSupport
		{
		public:
			enum class ERole
			{
				MASTER,
				SLAVE,
			};

		protected:
			using lstSentinel_t = std::vector<std::tuple<std::string, uint16_t>>;
			using callback_t = std::function<void(CConnection*)>;

			lstSentinel_t	lstSentinel_;

			std::string		masterName_;
			ERole			role_;

			uint32_t		timeoutSec_;

			callback_t		callback_;

		public:
			CSentinelSupport(std::string hostName, uint16_t port, std::string masterName, ERole role, callback_t callback = nullptr, uint32_t timeoutSec = 3);
			CSentinelSupport(std::vector<std::tuple<std::string , uint16_t>> lstSentinel, std::string masterName, ERole role, callback_t callback = nullptr, uint32_t timeoutSec = 3);
			~CSentinelSupport() = default;

		public:
			std::vector<CConnection*>	create(size_t num) const noexcept
			{
				std::vector<CConnection*> lstConn;
				lstConn.reserve(num);
				for (size_t x = 0; x < num; ++x)
				{
					auto p = getConnection();
					if (p == nullptr)
						continue;
					lstConn.push_back(p);
				}
				return lstConn;
			}
			CConnection*	getConnection(uint32_t slot = 0) const noexcept;
			void			freeConnection(CConnection* p) noexcept { delete p; }
			uint32_t		getSlot(const char*, size_t ) const noexcept { return std::numeric_limits<uint32_t>::max(); }
		};

		class CClusterSupport
		{
		protected :
			using lstHost_t = std::vector<std::tuple<std::string, uint16_t>>;
			lstHost_t		lstHost_;
			uint32_t		timeoutSec_;

		public : 
			CClusterSupport(std::string hostName, uint16_t port, uint32_t timeoutSec_ = 3);
			CClusterSupport(lstHost_t lstHost, uint32_t timeoutSec_ = 3);
			~CClusterSupport() = default;

		public:
			std::vector<CConnection*>	create(size_t num) const noexcept;
			CConnection*	getConnection(uint32_t slot) const noexcept;
			void			freeConnection(CConnection* p) noexcept { delete p; }
			uint32_t		getSlot(const char* key, size_t keyLen) const noexcept;

		protected:
			std::vector<std::tuple<uint32_t, uint32_t, std::string, uint16_t>> _getNodes() const noexcept;
		};
	}
}

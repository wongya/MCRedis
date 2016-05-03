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
			callback_t		callback_;

		public:
			CDefaultMiddleWare(std::string hostName, uint16_t port, callback_t callback) : hostName_(hostName), port_(port), callback_(callback) {}
			~CDefaultMiddleWare() = default;

		public:
			CConnection*	getConnection() const noexcept
			{
				CConnection* p = new CConnection;
				if (p->connect(hostName_.c_str(), port_) == false)
				{
					delete p;
					return nullptr;
				}
				if(callback_!=nullptr)
					callback_(p);
				return p;
			}
			void			freeConnection(CConnection* p) noexcept { delete p; }
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

			callback_t		callback_;

		public:
			CSentinelSupport(std::string hostName, uint16_t port, std::string masterName, ERole role, callback_t callback = nullptr);
			~CSentinelSupport() = default;

		public:
			CConnection*	getConnection() const noexcept;
			void			freeConnection(CConnection* p) noexcept { delete p; }
		};
	}
}

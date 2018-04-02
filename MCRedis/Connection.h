#pragma once

namespace MCRedis
{
	class CCommand;
}

namespace MCRedis
{
	class CConnection
	{
	protected :
		class CImpl;
		using impl_ptr = std::unique_ptr<CImpl>;
		impl_ptr	impl_;

	public :
		CConnection();
		~CConnection();
		
		CConnection(const CConnection&)=delete;
		CConnection& operator=(const CConnection&)=delete;

	public :
		bool		connect(const char* host, unsigned short port, uint32_t timeoutSec = 0) noexcept;
		bool		setTimeout(uint32_t timeoutSec) noexcept;

		CReply		sendCommand(CCommand&& command) noexcept;
		CReply		sendCommand(CCommand& command) noexcept;
		bool		appendCommand(CCommand&& command) noexcept;
		bool		appendCommand(CCommand& command) noexcept;
		CReply		getReply() noexcept;

	public : 
		int			errorNo() const noexcept;
		const char*	errorStr() const noexcept;
		bool		isValid() const noexcept;
	};
}

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

	public :
		bool		connect(const char* host, unsigned short port) noexcept;
		CReply		sendCommand(CCommand&& command) noexcept;
		CReply		sendCommand(CCommand& command) noexcept;
		bool		appendCommand(CCommand&& command) noexcept;
		bool		appendCommand(CCommand& command) noexcept;
		CReply		getReply() noexcept;

		int			errorNo() const noexcept;
		const char*	errorStr() const noexcept;
	};
}

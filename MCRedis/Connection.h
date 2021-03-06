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
		using slotRange_t = std::tuple<uint32_t, uint32_t>;
		impl_ptr		impl_;
		slotRange_t		slotRange_;

	public :
		CConnection();
		~CConnection();
		
		CConnection(const CConnection&)=delete;
		CConnection& operator=(const CConnection&)=delete;

	public :
		bool			connect(const char* host, unsigned short port, uint32_t timeoutSec = 0) noexcept;
		bool			setTimeout(uint32_t timeoutSec) noexcept;
		void			setSlotRange(uint32_t rBegin, uint32_t rEnd) noexcept { slotRange_ = std::make_tuple(rBegin, rEnd); }

		CReply			sendCommand(CCommand&& command) noexcept;
		CReply			sendCommand(CCommand& command) noexcept;
		bool			appendCommand(CCommand&& command) noexcept;
		bool			appendCommand(CCommand& command) noexcept;
		CReply			getReply() noexcept;

	public : 
		int				errorNo() const noexcept;
		const char*		errorStr() const noexcept;
		bool			isValid() const noexcept;
		slotRange_t		getSlotRange() const noexcept { return slotRange_; }
	};
}

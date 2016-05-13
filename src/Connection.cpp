#include "../MCRedis.h"
#include "../MCRedis/Connection.h"

#ifdef _MSC_VER
#	pragma warning(push,0)
#endif // _MSC_VER
#include "hiredis.h"
#ifdef _MSC_VER
#	pragma warning(pop)
#endif // _MSC_VER

namespace MCRedis
{
	class CConnection::CImpl
	{
	protected :
		struct context_deletor
		{
			void operator()(redisContext* c)
			{
				redisFree(c);
			}
		};
		struct reply_deletor
		{
			void operator()(redisReply* r)
			{
				freeReplyObject(r);
			}
		};
		using context_ptr_t = std::unique_ptr<redisContext, context_deletor>;
		using result_ptr_t = std::unique_ptr<redisReply, reply_deletor>;
		context_ptr_t	contextPtr_;

		std::string		host_;
		uint16_t		port_;

	public :
		CImpl() {}
		~CImpl() = default;

	public :
		bool			connect(std::string host, uint16_t port) noexcept;
		bool			reconnect() noexcept;

		CReply			sendCommand(CCommand& command) noexcept
		{
			redisReply* rawReply=reinterpret_cast<redisReply*>(redisCommandArgv(contextPtr_.get(), (int)command.lstCommand_.size(), command.lstCommand_.data(), command.lstCommandSize_.data()));
			if (rawReply == nullptr)
				return CReply(contextPtr_->err, contextPtr_->errstr);
			return CReply(result_ptr_t(rawReply).get());
		}

		bool			appendCommand(CCommand& command) noexcept
		{
			return redisAppendCommandArgv(contextPtr_.get(), (int)command.lstCommand_.size(), command.lstCommand_.data(), command.lstCommandSize_.data()) != REDIS_ERR;
		}

		CReply			getReply() noexcept
		{
			redisReply* rawReply=nullptr;
			if (redisGetReply(contextPtr_.get(), reinterpret_cast<void**>(&rawReply)) == REDIS_ERR)
				return CReply(contextPtr_->err, contextPtr_->errstr);
			return CReply(result_ptr_t(rawReply).get());
		}

		int				errorNo() const noexcept { return contextPtr_->err; }
		const char*		errorStr() const noexcept { return contextPtr_->errstr; }

		bool			isValid() const noexcept { return contextPtr_ != nullptr; }
	};

	bool CConnection::CImpl::connect(std::string host, uint16_t port) noexcept
	{
		host_ = host;
		port_ = port;
		return reconnect();
	}

	bool CConnection::CImpl::reconnect() noexcept
	{
		contextPtr_.reset(redisConnect(host_.c_str(), port_));
		if (contextPtr_->err != REDIS_OK)
		{
			contextPtr_.reset();
			return false;
		}
		return true;
	}

	CConnection::CConnection()
		: impl_(new CConnection::CImpl)
	{
	}

	CConnection::~CConnection() = default;

	bool CConnection::connect(const char* host, unsigned short port) noexcept
	{
		return impl_->connect(host, port);
	}

	CReply CConnection::sendCommand(CCommand&& command) noexcept
	{
		return impl_->sendCommand(command);
	}

	CReply CConnection::sendCommand(CCommand& command) noexcept
	{
		return impl_->sendCommand(command);
	}

	bool CConnection::appendCommand(CCommand&& command) noexcept
	{
		return impl_->appendCommand(command);
	}

	bool CConnection::appendCommand(CCommand& command) noexcept
	{
		return impl_->appendCommand(command);
	}

	CReply CConnection::getReply() noexcept
	{
		return impl_->getReply();
	}

	int CConnection::errorNo() const noexcept
	{
		return impl_->errorNo();
	}

	const char*	CConnection::errorStr() const noexcept
	{
		return impl_->errorStr();
	}

	bool CConnection::isValid() const noexcept
	{
		return impl_->isValid();
	}
}

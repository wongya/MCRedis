#pragma once

class redisReply;

namespace MCRedis
{
	class CReply
	{
	public:
		enum class EType
		{
			STRING = 1,
			ARRAY = 2,
			INTEGER = 3,
			NIL = 4,
			STATUS = 5,
			ERROR = 6,
			ERROR_CLIENT = 7,
		};

	protected:
		using lstReply_t = std::vector<CReply>;

		EType			type_ = EType::ERROR;

		std::string		str_;
		int64_t			integer_ = 0;
		lstReply_t		lstReply_;

	public:
		CReply() = default;
		CReply(redisReply* rawReply) noexcept;
		CReply(CReply&& rhs)
			: type_(rhs.type_)
			, str_(std::forward<std::string>(rhs.str_))
			, integer_(rhs.integer_)
			, lstReply_(std::forward<lstReply_t>(rhs.lstReply_))
		{}
		CReply(int errorNo, const char* strError) noexcept;
		~CReply() = default;

		CReply& operator=(CReply&& rhs)
		{
			type_=rhs.type_;
			str_=std::forward<std::string>(rhs.str_);
			integer_=rhs.integer_;
			lstReply_=std::forward<lstReply_t>(rhs.lstReply_);
			return *this;
		}

	public:
		EType				getType() const { return type_; }
		const std::string&	getStr() const { return str_; }
		int64_t				getInteger() const { return integer_; }
		const lstReply_t&	getReplies() const { return lstReply_; }
	};
}

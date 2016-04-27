#include "../MCRedis.h"
#include "../MCRedis/Reply.h"

#ifdef _MSC_VER
#	pragma warning(push,0)
#endif // _MSC_VER
#include "hiredis.h"
#ifdef _MSC_VER
#	pragma warning(pop)
#endif // _MSC_VER

namespace MCRedis
{
	CReply::CReply(redisReply* rawReply) noexcept
	{
		static_assert(REDIS_REPLY_STRING == (int32_t)EType::STRING, "Result type mismatched.");
		static_assert(REDIS_REPLY_STATUS == (int32_t)EType::STATUS, "Result type mismatched.");
		static_assert(REDIS_REPLY_ERROR == (int32_t)EType::ERROR, "Result type mismatched.");
		static_assert(REDIS_REPLY_INTEGER == (int32_t)EType::INTEGER, "Result type mismatched.");
		static_assert(REDIS_REPLY_ARRAY == (int32_t)EType::ARRAY, "Result type mismatched.");

		if (rawReply == nullptr)
			return;

		type_ = (EType)rawReply->type;
		switch (rawReply->type)
		{
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_STATUS:
		case REDIS_REPLY_ERROR:
			str_ = std::string(rawReply->str, rawReply->len);
			break;
		case REDIS_REPLY_INTEGER:
			integer_ = rawReply->integer;
			break;
		case REDIS_REPLY_ARRAY:
			lstReply_.reserve(rawReply->elements);
			for (size_t x = 0; x < rawReply->elements; ++x)
				lstReply_.emplace_back(rawReply->element[x]);
			break;
		}
	}

	CReply::CReply(int errorNo, const char* strError) noexcept
	{
		type_ = EType::ERROR_CLIENT;
		integer_ = errorNo;
		str_ = strError;
	}
}

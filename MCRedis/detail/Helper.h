#pragma once

namespace MCRedis
{
	namespace detail
	{
		template <typename Tuple, typename TCallback, size_t TIndex, size_t TRemainIndex>
		struct _tuple_ierator
		{
			void operator()(Tuple& tuple, TCallback& fn)
			{
				if (fn(std::get<TIndex>(tuple)) == true)
					return;
				_tuple_ierator<Tuple, TCallback, TIndex + 1, TRemainIndex - 1>{}(tuple, fn);
			}
		};

		template <typename Tuple, typename TCallback, size_t TIndex>
		struct _tuple_ierator<Tuple, TCallback, TIndex, 0>
		{
			void operator()(Tuple& tuple, TCallback& fn) {}
		};
	}
}

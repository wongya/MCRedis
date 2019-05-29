#pragma once

namespace MCRedis
{
	namespace detail
	{
		class null_mutex
		{
		public:
			null_mutex() = default;
			~null_mutex() = default;

		public:
			void			lock() {}
			void			unlock() {}
			void			try_lock() {}
		};
	}
}

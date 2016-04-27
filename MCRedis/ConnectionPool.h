#pragma once

namespace MCRedis
{
	namespace detail
	{
		class null_mutex
		{
		public :
			null_mutex() = default;
			~null_mutex() = default;

		public :
			void			lock() {}
			void			unlock() {}
			void			try_lock() {}
		};
	}

	template <typename TMutex, typename ...TMiddleWare>
	class CConnectionPool
	{
	public :
		using mutex_t = TMutex;
		using conn_ptr_t = std::unique_ptr<CConnection,std::function<void(CConnection*)>>;
		using pool_t = std::forward_list<conn_ptr_t>;
		using lstMW_t = std::tuple<TMiddleWare...>;

	public:
		class CConnectionRAII
		{
		protected:
			CConnectionPool*	parent_;
			conn_ptr_t			connPtr_;

		public:
			CConnectionRAII(CConnectionPool* parent, conn_ptr_t&& conn_ptr) : parent_(parent), connPtr_(std::forward<conn_ptr_t>(conn_ptr)) {}
			CConnectionRAII(CConnectionRAII&& rhs) : parent_(rhs.parent_), connPtr_(std::forward<conn_ptr_t>(rhs.connPtr_)) {}
			~CConnectionRAII()
			{
				if (connPtr_ != nullptr && connPtr_->errorNo()==0 )
				{
					std::lock_guard<mutex_t> grab(parent_->mutex_);
					parent_->pool_.push_front(std::move(connPtr_));
				}
			}

		public:
			CConnection*		operator->() const { return connPtr_.get(); }
			CConnection*		operator->() { return connPtr_.get(); }
			bool				operator==(CConnection* p) const { return connPtr_.get() == p; }
		};

	protected:
		lstMW_t		mw_;
		mutex_t		mutex_;
		pool_t		pool_;

	public:
		CConnectionPool(TMiddleWare&&... mw) : mw_(std::forward<TMiddleWare>(mw)...) {}
		~CConnectionPool() = default;

	public:
		bool					create(size_t defaultNum);
		void					clear();
		CConnectionRAII			get();

	protected : 
		template <size_t TIndex>
		conn_ptr_t				_getConnection()
		{
			auto& mw = std::get<TIndex>(mw_);
			conn_ptr_t connPtr(mw.getConnection(), [&mw](CConnection* p) { mw.freeConnection(p); });
			if (connPtr == nullptr)
				return _getConnection<TIndex+1>();
			return std::move(connPtr);
		}
		template <>
		conn_ptr_t				_getConnection<std::tuple_size<lstMW_t>::value>() { return nullptr; }
	};

	template <typename TMutex, typename ...TMiddleWare>
	bool CConnectionPool<TMutex,TMiddleWare...>::create(size_t defaultNum)
	{
		for (size_t x = 0; x < defaultNum; ++x)
		{
			conn_ptr_t connPtr(_getConnection<0>());
			if (connPtr==nullptr)
				return false;

			std::lock_guard<mutex_t> grab(mutex_);
			pool_.push_front(std::move(connPtr));
		}

		return true;
	}
	
	template <typename TMutex, typename ...TMiddleWare>
	void CConnectionPool<TMutex, TMiddleWare...>::clear()
	{
		std::lock_guard<mutex_t> grab(mutex_);
		pool_.clear();
	}

	template <typename TMutex, typename ...TMiddleWare>
	typename CConnectionPool<TMutex, TMiddleWare...>::CConnectionRAII CConnectionPool<TMutex, TMiddleWare...>::get()
	{
		std::lock_guard<mutex_t> grab(mutex_);
		if (pool_.empty() == true)
		{
			conn_ptr_t connPtr(_getConnection<0>());
			if (connPtr == nullptr)
				return CConnectionRAII(this, nullptr);
			return CConnectionRAII(this, std::move(connPtr));
		}

		CConnectionRAII conn(this, std::move(pool_.front()));
		pool_.pop_front();
		return std::move(conn);
	}
}

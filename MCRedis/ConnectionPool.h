#pragma once

namespace MCRedis
{
	template <typename TMutex, typename ...TMiddleWare>
	class CConnectionPool
	{
	public:
		using mutex_t = TMutex;
		using conn_ptr_t = std::unique_ptr<CConnection, std::function<void(CConnection*)>>;
		using lstMW_t = std::tuple<TMiddleWare...>;
		using pool_t = std::map<uint32_t, std::forward_list<conn_ptr_t>>;

	public:
		class CConnectionRAII
		{
		protected:
			CConnectionPool*	parent_;
			conn_ptr_t			connPtr_;

		public:
			CConnectionRAII() = delete;
			CConnectionRAII(const CConnectionRAII&) = delete;
			CConnectionRAII(CConnectionPool* parent, conn_ptr_t&& conn_ptr) : parent_(parent), connPtr_(std::forward<conn_ptr_t>(conn_ptr)) {}
			CConnectionRAII(CConnectionRAII&& rhs) : parent_(rhs.parent_), connPtr_(std::forward<conn_ptr_t>(rhs.connPtr_)) {}
			~CConnectionRAII()
			{
				if (connPtr_ != nullptr && connPtr_->errorNo() == 0)
				{
					std::lock_guard<mutex_t> grab(parent_->mutex_);
					parent_->pool_[std::get<1>(connPtr_->getSlotRange())].push_front(std::move(connPtr_));
				}
			}

		public:
			CConnection*		operator->() const { return connPtr_.get(); }
			CConnection*		operator->() { return connPtr_.get(); }
			bool				operator==(CConnection* p) const { return connPtr_.get() == p; }
			CConnectionRAII&	operator=(const CConnectionRAII&) = delete;
		};

	protected:
		lstMW_t		mw_;
		mutex_t		mutex_;
		pool_t		pool_;

	public:
		CConnectionPool(TMiddleWare&&... mw) : mw_(std::forward<TMiddleWare>(mw)...) {}
		~CConnectionPool() = default;

		CConnectionPool(const CConnectionPool&) = delete;
		CConnectionPool& operator=(const CConnectionPool&) = delete;

	public:
		bool					create(size_t defaultNum);
		void					clear();
		CConnectionRAII			get()
		{
			uint32_t slot = _getSlot(nullptr, 0);
			return std::move(_get(slot));
		}
		CConnectionRAII			get(const char* key, size_t keyLen)
		{
			uint32_t slot = _getSlot(key, keyLen);
			return std::move(_get(slot));
		}

	protected:
		struct _pre_create
		{
			size_t defaultNum_;
			std::vector<CConnection*> lstConnection_;

			_pre_create(size_t defaultNum) : defaultNum_(defaultNum) {}

			template<typename T>
			bool operator()(T& mw)
			{
				lstConnection_ = mw.create(defaultNum_);
				return lstConnection_.empty() == false;
			}
		};

		struct _get_connection
		{
			uint32_t slotNo_;
			CConnection* conn_ = nullptr;

			_get_connection(uint32_t slotNo) : slotNo_(slotNo) {}

			template<typename T>
			bool operator()(T& mw)
			{
				conn_ = mw.getConnection(slotNo_);
				return conn_ != nullptr;
			}
		};

		struct _get_slot
		{
			const char* key_ = nullptr;
			size_t keyLen_ = 0;
			uint32_t slotNo_;

			_get_slot(const char* key, size_t keyLen) : key_(key), keyLen_(keyLen) {}

			template<typename T>
			bool operator()(T& mw)
			{
				slotNo_ = mw.getSlot(key_, keyLen_);
				return slotNo_ != std::numeric_limits<uint32_t>::max();
			}
		};

		uint32_t				_getSlot(const char* key, size_t keyLen) 
		{
			_get_slot fnGetSlot(key, keyLen);
			detail::_tuple_ierator<lstMW_t, _get_slot, 0, sizeof...(TMiddleWare)>{}(mw_, fnGetSlot);
			return  fnGetSlot.slotNo_;
		}

		CConnectionRAII			_get(uint32_t slot)
		{
			std::lock_guard<mutex_t> grab(mutex_);
			auto iter = pool_.lower_bound(slot);
			if (iter == pool_.end() || iter->second.empty() == true)
			{
				_get_connection fnGetConn(slot);
				detail::_tuple_ierator<lstMW_t, _get_connection, 0, sizeof...(TMiddleWare)>{}(mw_, fnGetConn);
				conn_ptr_t connPtr(fnGetConn.conn_);
				if (connPtr == nullptr)
					return CConnectionRAII(this, nullptr);
				return CConnectionRAII(this, std::move(connPtr));
			}

			CConnectionRAII conn(this, std::move(iter->second.front()));
			iter->second.pop_front();
			return std::move(conn);
		}
	};

	template <typename TMutex, typename ...TMiddleWare>
	bool CConnectionPool<TMutex, TMiddleWare...>::create(size_t defaultNum)
	{
		_pre_create fnCreator(defaultNum);
		detail::_tuple_ierator<lstMW_t, _pre_create, 0, sizeof...(TMiddleWare)>{}(mw_, fnCreator);
		if (fnCreator.lstConnection_.empty() == true)
			return false;
		for (auto conn : fnCreator.lstConnection_)
		{
			conn_ptr_t connPtr(conn, [](CConnection* p) { delete p; });

			std::lock_guard<mutex_t> grab(mutex_);
			pool_[std::get<1>(connPtr->getSlotRange())].push_front(std::move(connPtr));
		}
		return true;
	}

	template <typename TMutex, typename ...TMiddleWare>
	void CConnectionPool<TMutex, TMiddleWare...>::clear()
	{
		std::lock_guard<mutex_t> grab(mutex_);
		for (auto iter = pool_.begin(); iter != pool_.end(); ++iter)
			iter->second.clear();
	}

	namespace deprecated
	{
		template <typename TMutex, typename ...TMiddleWare>
		class CConnectionPool
		{
		public:
			using mutex_t = TMutex;
			using conn_ptr_t = std::unique_ptr<CConnection, std::function<void(CConnection*)>>;
			using pool_t = std::forward_list<conn_ptr_t>;
			using lstMW_t = std::tuple<TMiddleWare...>;

		public:
			class CConnectionRAII
			{
			protected:
				CConnectionPool*	parent_;
				conn_ptr_t			connPtr_;

			public:
				CConnectionRAII() = delete;
				CConnectionRAII(const CConnectionRAII&) = delete;
				CConnectionRAII(CConnectionPool* parent, conn_ptr_t&& conn_ptr) : parent_(parent), connPtr_(std::forward<conn_ptr_t>(conn_ptr)) {}
				CConnectionRAII(CConnectionRAII&& rhs) : parent_(rhs.parent_), connPtr_(std::forward<conn_ptr_t>(rhs.connPtr_)) {}
				~CConnectionRAII()
				{
					if (connPtr_ != nullptr && connPtr_->errorNo() == 0)
					{
						std::lock_guard<mutex_t> grab(parent_->mutex_);
						parent_->pool_.push_front(std::move(connPtr_));
					}
				}

			public:
				CConnection*		operator->() const { return connPtr_.get(); }
				CConnection*		operator->() { return connPtr_.get(); }
				bool				operator==(CConnection* p) const { return connPtr_.get() == p; }
				CConnectionRAII&	operator=(const CConnectionRAII&) = delete;
			};

		protected:
			lstMW_t		mw_;
			mutex_t		mutex_;
			pool_t		pool_;

		public:
			CConnectionPool(TMiddleWare&&... mw) : mw_(std::forward<TMiddleWare>(mw)...) {}
			~CConnectionPool() = default;

			CConnectionPool(const CConnectionPool&) = delete;
			CConnectionPool& operator=(const CConnectionPool&) = delete;

		public:
			bool					create(size_t defaultNum);
			void					clear();
			CConnectionRAII			get();

		protected:
#ifdef _MSC_VER
			template <size_t TIndex>
			conn_ptr_t 				_getConnectionImpl()
			{
				auto& mw = std::get<TIndex>(mw_);
				conn_ptr_t connPtr(mw.getConnection(), [&mw](CConnection* p) { mw.freeConnection(p); });
				if (connPtr == nullptr)
					return _getConnectionImpl<TIndex + 1>();
				return std::move(connPtr);
			}
			template <>
			conn_ptr_t 				_getConnectionImpl<sizeof...(TMiddleWare)>() { return nullptr; }
			conn_ptr_t				_getConnection() { return _getConnectionImpl<0>(); }
#else  //_MSC_VER
			template <size_t TIndex, typename TDummy = void>
			struct _getConnectionImpl
			{
				conn_ptr_t operator()(lstMW_t& mw_)
				{
					auto& mw = std::get<TIndex>(mw_);
					conn_ptr_t connPtr(mw.getConnection(), [&mw](CConnection* p) { mw.freeConnection(p); });
					if (connPtr == nullptr)
						return _getConnectionImpl<TIndex + 1>{}(mw_);
					return std::move(connPtr);
				}
			};
			template <typename TDummy>
			struct _getConnectionImpl<std::tuple_size<lstMW_t>::value, TDummy> {
				conn_ptr_t operator()(lstMW_t&) { return nullptr; }
			};
			conn_ptr_t				_getConnection() { return _getConnectionImpl<0>{}(mw_); }
#endif //_MSC_VER
		};

		template <typename TMutex, typename ...TMiddleWare>
		bool CConnectionPool<TMutex, TMiddleWare...>::create(size_t defaultNum)
		{
			for (size_t x = 0; x < defaultNum; ++x)
			{
				conn_ptr_t connPtr(_getConnection());
				if (connPtr == nullptr)
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
				conn_ptr_t connPtr(_getConnection());
				if (connPtr == nullptr)
					return CConnectionRAII(this, nullptr);
				return CConnectionRAII(this, std::move(connPtr));
			}

			CConnectionRAII conn(this, std::move(pool_.front()));
			pool_.pop_front();
			return std::move(conn);
		}
	}
}

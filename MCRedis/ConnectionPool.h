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
			CConnectionPool*		parent_;
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

	public:
		CReply					sendCommand(CCommand&& cmd) noexcept
		{
			auto cmdKey = cmd.getKey();
			uint32_t slot = _getSlot(cmdKey.first, cmdKey.second);
			auto conn = _get(slot);
			if (conn == nullptr)
				return CReply(-1, "No connections");
			return std::move(conn->sendCommand(std::move(cmd)));
		}

	protected:
#ifdef _MSC_VER
		template <size_t TIndex>
		conn_ptr_t 				_getConnectionImpl(uint32_t slotNo)
		{
			auto& mw = std::get<TIndex>(mw_);
			conn_ptr_t connPtr(mw.getConnection(slotNo), [&mw](CConnection* p) { mw.freeConnection(p); });
			if (connPtr == nullptr)
				return _getConnectionImpl<TIndex + 1>(slotNo);
			return std::move(connPtr);
		}
		template <>
		conn_ptr_t 				_getConnectionImpl<sizeof...(TMiddleWare)>(uint32_t) { return nullptr; }
		conn_ptr_t				_getConnection(uint32_t slotNo) { return _getConnectionImpl<0>(slotNo); }

		template <size_t TIndex>
		uint32_t	 			_getSlotImpl(const char* key, size_t keyLen)
		{
			auto& mw = std::get<TIndex>(mw_);
			uint32_t slot = mw.getSlot(key, keyLen);
			if (slot == std::numeric_limits<uint32_t>::max())
				return _getSlotImpl<TIndex + 1>(key, keyLen);
			return slot;
		}
		template <>
		uint32_t 				_getSlotImpl<sizeof...(TMiddleWare)>(const char*, size_t ) { return std::numeric_limits<uint32_t>::max(); }
		uint32_t				_getSlot(const char* key, size_t keyLen) { return _getSlotImpl<0>(key, keyLen); }
#else  //_MSC_VER
		template <size_t TIndex, typename TDummy = void>
		struct _getConnectionImpl
		{
			conn_ptr_t operator()(lstMW_t& mw_, uint32_t slotNo)
			{
				auto& mw = std::get<TIndex>(mw_);
				conn_ptr_t connPtr(mw.getConnection(slotNo), [&mw](CConnection* p) { mw.freeConnection(p); });
				if (connPtr == nullptr)
					return _getConnectionImpl<TIndex + 1>{}(mw_, slotNo);
				return std::move(connPtr);
			}
		};
		template <typename TDummy>
		struct _getConnectionImpl<std::tuple_size<lstMW_t>::value, TDummy> {
			conn_ptr_t operator()(lstMW_t&, uint32_t) { return nullptr; }
		};
		conn_ptr_t				_getConnection(uint32_t slotNo) { return _getConnectionImpl<0>{}(mw_, slotNo); }

		template <size_t TIndex, typename TDummy = void>
		struct _getSlotImpl
		{
			uint32_t operator()(lstMW_t& mw_, const char* key, size_t keyLen)
			{
				auto& mw = std::get<TIndex>(mw_);
				uint32_t slot = mw.getSlot(key, keyLen);
				if (slot == std::numeric_limits<uint32_t>::max())
					return _getSlotImpl<TIndex + 1>{}(mw_, key, keyLen);
				return slot;
			}
		};
		template <typename TDummy>
		struct _getSlotImpl<std::tuple_size<lstMW_t>::value, TDummy> {
			uint32_t operator()(lstMW_t&, const char*, size_t) { return std::numeric_limits<uint32_t>::max(); }
		};
		uint32_t				_getSlot(const char* key, size_t keyLen) { return _getSlotImpl<0>{}(mw_, key, keyLen); }
#endif //_MSC_VER

		CConnectionRAII			_get(uint32_t slot)
		{
			std::lock_guard<mutex_t> grab(mutex_);
			auto iter = pool_.lower_bound(slot);
			if (iter == pool_.end() || iter->second.empty() == true)
			{
				conn_ptr_t connPtr(_getConnection(slot));
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
		srand((uint32_t)time(nullptr));
		for (size_t x = 0; x < defaultNum; ++x)
		{
			uint32_t randNum = (uint32_t)rand() % 16384;
			conn_ptr_t connPtr(_getConnection(randNum));
			if (connPtr == nullptr)
				return false;

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

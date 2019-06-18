#pragma once

#include "Command.h"
#include "Reply.h"
#include "Connection.h"
#include "MiddleWare.h"
#include "ConnectionPool.h"

namespace MCRedis
{
	namespace _detail
	{
		template <typename pool_t, uint32_t TRetryCount>
		CReply runSingleCommand(pool_t& pool, CCommand& cmd)
		{
			auto cmdKey = cmd.getKey();
			CReply rpy;

			uint32_t tryCount = TRetryCount;
			while (tryCount-- > 0)
			{
				auto conn = pool.get(cmdKey.first, cmdKey.second);
				if (conn == nullptr || conn->isValid() == false)
					continue;
				rpy = std::move(conn->sendCommand(cmd));
			}
			return rpy;
		}

		template <typename pool_t, typename conn_t, typename lstCommand_t, uint32_t TRetryCount>
		std::vector<std::pair<conn_t, lstCommand_t>> mapCommandPerNode(pool_t& pool, lstCommand_t& lstCommand)
		{
			std::tuple<uint32_t, uint32_t> prevSlotRange = std::make_tuple(0, 0);
			std::vector<std::pair<conn_t, lstCommand_t>> lstNodeCommand;
			for (auto& cmd : lstCommand)
			{
				auto cmdKey = cmd.getKey();

				uint32_t tryCount = TRetryCount;
				while (tryCount-- > 0)
				{
					conn_t conn = pool.get(cmdKey.first, cmdKey.second);
					if (conn == nullptr || conn->isValid() == false)
					{
						if (tryCount == 0)
							return{};

						std::this_thread::yield();
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
						continue;
					}

					if (conn->getSlotRange() != prevSlotRange)
					{
						prevSlotRange = conn->getSlotRange();
						lstCommand_t lstTmpCommand;
						lstTmpCommand.emplace_back(std::move(cmd));
						lstNodeCommand.emplace_back(std::make_pair(std::move(conn), std::move(lstTmpCommand)));
					}
					else
						lstNodeCommand.rbegin()->second.push_back(std::move(cmd));
					break;
				}
			}
			return lstNodeCommand;
		};
	}

	template <typename TConnectionPool,size_t TRetryCount=5>
	class CRunner
	{
	public:
		using pool_t = TConnectionPool;
		using conn_t = typename pool_t::CConnectionRAII;
		using lstCommand_t = std::vector<CCommand>;
		using callback_t = std::function<void(CReply&)>;
		using lstRpy_t = std::vector<CReply>;

	protected :
		pool_t&			pool_;
		lstCommand_t	lstCommand_;

	public:
		CRunner(pool_t& pool) : pool_(pool) {}
		CRunner(CRunner&& rhs) : pool_(rhs.pool_) {}
		~CRunner() = default;
		CRunner(CRunner&) = delete;
		CRunner& operator=(const CRunner&) = delete;

	public:
		CReply		run(CCommand&& cmd, callback_t callback=nullptr) noexcept
		{
			lstCommand_.push_back(std::forward<CCommand>(cmd));
			lstRpy_t rpy(_run(callback));
			if (rpy.empty())
				return CReply();
			return std::move(rpy.at(0));
		}

		void		append(CCommand&& cmd) noexcept { lstCommand_.push_back(std::forward<CCommand>(cmd)); }
		lstRpy_t	runCommands(callback_t callback=nullptr) noexcept { return _run(callback); }
		
	protected:
		lstRpy_t	_run(std::function<void(CReply&)> callback) noexcept
		{
			lstRpy_t lstRpy;
			lstRpy.reserve(lstCommand_.size());

			if (lstCommand_.size() == 1)
			{
				auto rpy = _detail::runSingleCommand<pool_t, TRetryCount>(pool_, lstCommand_[0]);
				if (callback != nullptr)
					callback(rpy);
				lstRpy.push_back(std::move(rpy));
				return lstRpy;
			}

			auto lstNodeCommand = _detail::mapCommandPerNode<pool_t, conn_t, lstCommand_t, TRetryCount>(pool_, lstCommand_);
			if (lstNodeCommand.empty() == true)
			{
				lstRpy.resize(lstCommand_.size());
				return lstRpy;
			}

			for (auto& elem : lstNodeCommand)
			{
				auto conn = std::move(elem.first);
				auto& lstCmd = elem.second;

				for (auto& cmd : lstCmd)
				{
					if (conn->appendCommand(cmd) == false)
					{
						lstRpy.resize(lstCommand_.size());
						return lstRpy;
					}
				}

				for (size_t x = 0; x < lstCmd.size(); ++x)
				{
					CReply rpy = std::move(conn->getReply());
					if (callback != nullptr)
						callback(rpy);
					lstRpy.push_back(std::move(rpy));
				}
			}

			lstCommand_.clear();
			return lstRpy;
		}
	};

	template <typename TConnectionPool,size_t TRetryCount=5>
	class CAsyncRunner
	{
	protected : 
		using result_t = std::future<MCRedis::CReply>;

		class CTaskHolder
		{
		protected : 
			using mapTask_t = std::map<uint64_t,result_t>;
			using lstId_t = std::vector<uint64_t>;
			using mutex_t = std::mutex;

		protected : 
			mapTask_t	mapTask_;
			uint64_t	lastId_=0;

			bool		terminate_=false;
			lstId_t		lstReturnedId_;
			std::future<void> returnIdHandler_;

			mutex_t		mutex_;

		public : 
			CTaskHolder()
			{
				returnIdHandler_=std::async(std::launch::async,
					[this]()
					{
						while(terminate_==false)
						{
							if(lstReturnedId_.empty())
							{
								std::this_thread::yield();
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
								continue;
							}

							std::unique_lock<mutex_t> grab(mutex_);
							for(auto id : lstReturnedId_)
								mapTask_.erase(id);
							lstReturnedId_.clear();
						}
					}
				);
			}
			~CTaskHolder() = default;
			CTaskHolder(const CTaskHolder&) = delete;
			CTaskHolder& operator=(const CTaskHolder&) = delete;

		public:
			uint64_t	addTask(result_t&& result) noexcept
			{
				std::unique_lock<mutex_t> grab(mutex_);
				uint64_t id = ++lastId_;
				return mapTask_.insert(mapTask_t::value_type(id, std::forward<result_t>(result))).second ? id : 0;
			}

			result_t	getResult(uint64_t id) noexcept
			{
				std::unique_lock<mutex_t> grab(mutex_);
				result_t result;
				auto iter = mapTask_.find(id);
				if (iter == mapTask_.end())
					return result;
				result = std::move(iter->second);
				mapTask_.erase(id);
				return result;
			}
			void		remove(uint64_t id) noexcept
			{
				std::unique_lock<mutex_t> grab(mutex_);
				mapTask_.erase(id);
			}

			void		returnId(uint64_t id) noexcept
			{
				std::unique_lock<mutex_t> grab(mutex_);
				lstReturnedId_.push_back(id);
			}

			void		stop() noexcept
			{
				terminate_=true;
				returnIdHandler_.wait();

				std::unique_lock<mutex_t> grab(mutex_);
				for (auto& iter : mapTask_)
					iter.second.wait();
				mapTask_.clear();
			}
		};

	public:
		using pool_t = TConnectionPool;
		using conn_t = typename pool_t::CConnectionRAII;
		using lstCommand_t = std::vector<CCommand>;
		using callback_t = std::function<void(CReply&)>;

	protected:
		pool_t&			pool_;
		lstCommand_t	lstCommand_;
		uint64_t		resultId_ = 0;

	public:
		CAsyncRunner() = delete;
		CAsyncRunner(pool_t& pool) : pool_(pool) {}
		CAsyncRunner(CAsyncRunner&& rhs) : pool_(rhs.pool_) {}
		~CAsyncRunner()
		{
			if (resultId_ != 0)
				_getTaskHolder().returnId(resultId_);
		}

		CAsyncRunner& operator=(const CAsyncRunner&) = delete;

	public:
		void	run(CCommand&& cmd, callback_t callback = nullptr) noexcept
		{
			lstCommand_.push_back(std::forward<CCommand>(cmd));
			_run(callback);
		}

		void	append(CCommand&& cmd) noexcept { lstCommand_.push_back(std::forward<CCommand>(cmd)); }
		void	runCommands(callback_t callback=nullptr) noexcept { _run(callback); }
		CReply	getResult() noexcept
		{
			if(resultId_==0)
				return CReply();
			auto result=_getTaskHolder().getResult(resultId_);
			resultId_=0;
			if(result.valid()==false)
				return CReply();
			return result.get();
		}

		static void join()
		{
			_getTaskHolder().stop();
		}

	protected:
		void	_run(callback_t callback) noexcept
		{
			if(resultId_!=0)
				_getTaskHolder().returnId(resultId_);

			class ArgWrapper
			{
			public : 
				lstCommand_t	 lstCommand_;
			public:
				ArgWrapper(lstCommand_t&& lstCommand) : lstCommand_(std::move(lstCommand)) {}
			};

			resultId_ = _getTaskHolder().addTask(std::async(std::launch::async,
#ifdef __linux
				[](ArgWrapper&& arg, callback_t callback, pool_t& pool) -> CReply
#else  //__linux
				[](ArgWrapper& arg, callback_t callback, pool_t& pool) -> CReply
#endif //__linux
				{
					if (arg.lstCommand_.size() == 1)
					{
						auto rpy = _detail::runSingleCommand<pool_t, TRetryCount>(pool, arg.lstCommand_[0]);
						if (callback != nullptr)
							callback(rpy);
						return rpy;
					}

					CReply rpy;
					auto lstNodeCommand = _detail::mapCommandPerNode<pool_t, conn_t, lstCommand_t, TRetryCount>(pool, arg.lstCommand_);
					if (lstNodeCommand.empty() == true)
						return rpy;

					for (auto& elem : lstNodeCommand)
					{
						auto conn = std::move(elem.first);
						auto& lstCmd = elem.second;

						for (auto& cmd : lstCmd)
						{
							if (conn->appendCommand(cmd) == false)
								return rpy;
						}

						for (size_t x = 0; x < lstCmd.size(); ++x)
						{
							rpy = std::move(conn->getReply());
							if (callback != nullptr)
								callback(rpy);
						}
					}
					return rpy;
				}
#ifdef __linux
				, std::move(ArgWrapper(std::move(lstCommand_)))
#else  //__linux
				, ArgWrapper(std::move(lstCommand_))
#endif //__linux
				, callback
				, std::ref(pool_)
			));
		}

		static CTaskHolder& _getTaskHolder()
		{
#ifdef __linux
			static CTaskHolder taskHolder;
#else  // __linux
#			pragma warning(push)
#			pragma warning(disable:4640)
			static CTaskHolder taskHolder;
#			pragma warning(pop)
#endif // __linux
			return taskHolder;
		}
	};
}


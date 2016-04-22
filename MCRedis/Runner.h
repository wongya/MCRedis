#pragma once

#include "../MCRedis.h"

namespace MCRedis
{
	template <typename TConnectionPool,size_t TRetryCount=5>
	class CRunner
	{
	public:
		using pool_t = TConnectionPool;
		using conn_t = typename pool_t::CConnectionRAII;
		using lstCommand_t = std::vector<CCommand>;

	protected :
		pool_t&			pool_;
		lstCommand_t	lstCommand_;

	public:
		CRunner(pool_t& pool) : pool_(pool) {}
		CRunner(CRunner&& rhs) : pool_(rhs.pool_) {}
		~CRunner() = default;

	public:
		CReply	run(CCommand&& cmd, std::function<void(CReply&)> callback=nullptr)
		{
			lstCommand_.push_back(std::forward<CCommand>(cmd));
			return _run(callback);
		}

		void	append(CCommand&& cmd) { lstCommand_.push_back(std::forward<CCommand>(cmd)); }
		CReply	run (std::function<void(CReply&)> callback=nullptr) { return _run(callback); }
		
	protected:
		CReply	_run(std::function<void(CReply&)> callback)
		{
			CReply rpy;
			
			uint32_t tryCount=TRetryCount;
			while(tryCount-->0)
			{
				conn_t conn=std::move(pool_.get());
				for(auto& iter : lstCommand_)
					conn->appendCommand(iter);
				rpy=std::move(conn->getReply());
				if(rpy.getType()!=CReply::EType::ERROR_CLIENT)
					break;
			}
			
			lstCommand_.clear();
			if(callback!=nullptr)
				callback(rpy);		
			return rpy;			
		}
	};

	template <typename TConnectionPool,size_t TRetryCount=5>
	class CAsyncRunner
	{
	protected : 
		using result_t = std::future<CReply>;

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
								std::this_thread::sleep_for(std::chrono::milliseconds(100));
								std::this_thread::yield();
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

		public : 
			uint64_t	addTask(result_t&& result)
			{
				std::unique_lock<mutex_t> grab(mutex_);
				uint64_t id=++lastId_;
				return mapTask_.insert(mapTask_t::value_type(id,std::forward<result_t>(result))).second ? id:0;
			}

			result_t	getResult(uint64_t id)
			{
				std::unique_lock<mutex_t> grab(mutex_);
				result_t result;
				auto iter=mapTask_.find(id);
				if(iter==mapTask_.end())
					return result;
				result=std::move(iter->second);
				mapTask_.erase(id);
				return result;
			}
			void		remove(uint64_t id)
			{
				std::unique_lock<mutex_t> grab(mutex_);
				mapTask_.erase(id);
			}

			void		returnId(uint64_t id)
			{
				std::unique_lock<mutex_t> grab(mutex_);
				lstReturnedId_.push_back(id);
			}

			void		stop()
			{
				terminate_=true;
				returnIdHandler_.wait();

				std::unique_lock<mutex_t> grab(mutex_);
				mapTask_.clear();
			}
		};

	public:
		using pool_t = TConnectionPool;
		using conn_t = typename pool_t::CConnectionRAII;
		using lstCommand_t = std::vector<CCommand>;
		using callback_t = std::function<void(CReply&)>;

	protected :
		pool_t&			pool_;
		lstCommand_t	lstCommand_;
		uint64_t		resultId_=0;

	public:
		CAsyncRunner(pool_t& pool) : pool_(pool) {}
		CAsyncRunner(CAsyncRunner&& rhs) : pool_(rhs.pool_) {}
		~CAsyncRunner()
		{
			if(resultId_!=0)
				_getTaskHolder().returnId(resultId_);
		}

	public:
		void	run(CCommand&& cmd, callback_t callback=nullptr)
		{
			lstCommand_.push_back(std::forward<CCommand>(cmd));
			_run(callback);
		}

		void	append(CCommand&& cmd) { lstCommand_.push_back(std::forward<CCommand>(cmd)); }
		void	run(callback_t callback=nullptr) { _run(callback); }
		CReply	getResult()
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
		void	_run(callback_t callback)
		{
			if(resultId_!=0)
				_getTaskHolder().returnId(resultId_);

			resultId_=_getTaskHolder().addTask(std::async(std::launch::async,
				[](lstCommand_t&& lstCommand,callback_t callback,pool_t& pool) -> CReply
				{
					CReply rpy;

					uint32_t tryCount=TRetryCount;
					while(tryCount-->0)
					{
						conn_t conn=std::move(pool.get());
						for(auto& iter : lstCommand)
							conn->appendCommand(iter);
						rpy=std::move(conn->getReply());
						if(rpy.getType()!=CReply::EType::ERROR_CLIENT)
							break;
					}
					if(callback!=nullptr)
						callback(rpy);
					return rpy;			
				}
				, std::move(lstCommand_)
				, callback
				, std::ref(pool_)
			));
		}

		static CTaskHolder& _getTaskHolder()
		{
			static CTaskHolder taskHolder;
			return taskHolder;
		}
	};
}

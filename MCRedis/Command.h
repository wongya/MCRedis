#pragma once

namespace MCRedis
{
	class CCommand
	{
		friend class CConnection;
	protected:
		std::vector<const char*>	lstCommand_;
		std::vector<size_t>			lstCommandSize_;

		std::vector<std::string>	lstCommandBuffer_;

	public:
		template <typename ...TArg>
		CCommand(TArg&&... args)
		{
			lstCommand_.resize(sizeof...(args));
			lstCommandSize_.resize(sizeof...(args));
			lstCommandBuffer_.resize(sizeof...(args));

			_expandArgument<0>(std::forward<TArg>(args)...);
		}
		CCommand(CCommand&& rhs)
			: lstCommand_(std::forward<decltype(rhs.lstCommand_)>(rhs.lstCommand_))
			, lstCommandSize_(std::forward<decltype(rhs.lstCommandSize_)>(rhs.lstCommandSize_))
			, lstCommandBuffer_(std::forward<decltype(rhs.lstCommandBuffer_)>(rhs.lstCommandBuffer_))
		{
		}
#ifdef __linux
#else  //__linux
		CCommand(const CCommand& rhs)
			: lstCommand_(rhs.lstCommand_)
			, lstCommandSize_(rhs.lstCommandSize_)
			, lstCommandBuffer_(rhs.lstCommandBuffer_)
		{
		}
#endif	//__linux
		~CCommand() = default;

	protected:
		template <size_t TIndex, typename T, typename ...TArg>
		typename std::enable_if<std::is_same<std::string, typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value, void>::type _expandArgument(T&& arg, TArg&&... args)
		{
			_expandArgumentStr<TIndex>(std::forward<T>(arg));
			_expandArgument<TIndex + 1>(std::forward<TArg>(args)...);
		}

		template <size_t TIndex, typename T, typename ...TArg>
		typename std::enable_if<std::is_same<std::string, typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value, void>::type _expandArgument(T&& arg)
		{
			_expandArgumentStr<TIndex>(std::forward<T>(arg));
		}

		template <size_t TIndex, typename T, typename ...TArg>
		typename std::enable_if<std::is_arithmetic<typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value, void>::type _expandArgument(T&& arg, TArg&&... args)
		{
			_expandArgumentStr<TIndex>(std::to_string(arg));
			_expandArgument<TIndex + 1>(std::forward<TArg>(args)...);
		}

		template <size_t TIndex, typename T, typename ...TArg>
		typename std::enable_if<std::is_arithmetic<typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value, void>::type _expandArgument(T&& arg)
		{
			_expandArgumentStr<TIndex>(std::to_string(arg));
		}

		template <size_t TIndex, typename T, typename ...TArg>
		typename std::enable_if<std::is_enum<typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value, void>::type _expandArgument(T&& arg, TArg&&... args)
		{
			_expandArgumentStr<TIndex>(std::to_string((uint32_t)arg));
			_expandArgument<TIndex + 1>(std::forward<TArg>(args)...);
		}

		template <size_t TIndex, typename T, typename ...TArg>
		typename std::enable_if<std::is_enum<typename std::remove_cv<typename std::remove_reference<T>::type>::type>::value, void>::type _expandArgument(T&& arg)
		{
			_expandArgumentStr<TIndex>(std::to_string((uint32_t)arg));
		}

		template <size_t TIndex, typename T, typename ...TArg>
		typename std::enable_if<std::is_same<const char*, typename std::decay<T>::type>::value, void>::type _expandArgument(T&& arg, TArg&&... args)
		{
			_expandArgumentStr<TIndex>(std::string(arg));
			_expandArgument<TIndex + 1>(std::forward<TArg>(args)...);
		}

		template <size_t TIndex, typename T, typename ...TArg>
		typename std::enable_if<std::is_same<const char*, typename std::decay<T>::type>::value, void>::type _expandArgument(T&& arg)
		{
			_expandArgumentStr<TIndex>(std::string(arg));
		}

		template <size_t TIndex>
		void _expandArgumentStr(std::string&& str)
		{
			lstCommandBuffer_[TIndex] = std::forward<std::string>(str);

			lstCommand_[TIndex] = lstCommandBuffer_[TIndex].c_str();
			lstCommandSize_[TIndex] = lstCommandBuffer_[TIndex].size();
		}

		template <size_t TIndex>
		void _expandArgumentStr(const std::string& str)
		{
			_expandArgumentStr<TIndex>(std::string(str));
		}
	};
}

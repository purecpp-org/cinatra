#pragma once
#include <chrono>

class Timer
{
private:
	template<typename T>
	using time_point = std::chrono::time_point<T>;
	
	template<typename T>
	using duration = std::chrono::duration<T>;

	using high_resolution_clock = std::chrono::high_resolution_clock;
public:

	Timer() : m_begin(high_resolution_clock::now()) {}
	void reset() { m_begin = high_resolution_clock::now(); }

	//默认输出毫秒.
	int64_t elapsed() const
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(high_resolution_clock::now() - m_begin).count();
	}

	//默认输出秒.
	double elapsed_second() const
	{
		return std::chrono::duration_cast<duration<double>>(high_resolution_clock::now() - m_begin).count();
	}

	//微秒.
	int64_t elapsed_micro() const
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(high_resolution_clock::now() - m_begin).count();
	}

	//纳秒.
	int64_t elapsed_nano() const
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(high_resolution_clock::now() - m_begin).count();
	}

	////秒.
	//int64_t elapsed_seconds() const
	//{
	//	return std::chrono::duration_cast<std::chrono::seconds>(high_resolution_clock::now() - m_begin).count();
	//}

	//分.
	int64_t elapsed_minutes() const
	{
		return std::chrono::duration_cast<std::chrono::minutes>(high_resolution_clock::now() - m_begin).count();
	}

	//时.
	int64_t elapsed_hours() const
	{
		return std::chrono::duration_cast<std::chrono::hours>(high_resolution_clock::now() - m_begin).count();
	}

private:
	time_point<high_resolution_clock> m_begin;
};


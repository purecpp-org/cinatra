
#include <cinatra_http/http_server.h>

#include <boost/bind.hpp>

#include <stdexcept>
#include <thread>


namespace cinatra
{
	io_service_pool::io_service_pool(std::size_t pool_size)
		: next_io_service_(0)
	{
		if (pool_size == 0)
			throw std::runtime_error("io_service_pool size is 0");

		for (std::size_t i = 0; i < pool_size; ++i)
		{
			io_service_ptr io_service(new boost::asio::io_service);
			work_ptr work(new boost::asio::io_service::work(*io_service));
			io_services_.push_back(io_service);
			work_.push_back(work);
		}
	}

	void io_service_pool::run()
	{
		std::vector<std::shared_ptr<std::thread> > threads;
		for (std::size_t i = 0; i < io_services_.size(); ++i)
		{
			threads.emplace_back(std::make_shared<std::thread>(boost::bind(&boost::asio::io_service::run, io_services_[i])));
		}

		for (std::size_t i = 0; i < threads.size(); ++i)
			threads[i]->join();
	}

	void io_service_pool::stop()
	{
		for (std::size_t i = 0; i < io_services_.size(); ++i)
			io_services_[i]->stop();
	}

	boost::asio::io_service& io_service_pool::get_io_service()
	{
		boost::asio::io_service& io_service = *io_services_[next_io_service_];
		++next_io_service_;
		if (next_io_service_ == io_services_.size())
			next_io_service_ = 0;
		return io_service;
	}
}

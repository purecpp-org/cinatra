
#pragma once

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>

#include <vector>
#include <memory>

namespace cinatra
{
	class io_service_pool
		: private boost::noncopyable
	{
	public:
		explicit io_service_pool(std::size_t pool_size);

		void run();

		void stop();

		boost::asio::io_service& get_io_service();

	private:
		using io_service_ptr = std::shared_ptr<boost::asio::io_service>;
		using work_ptr = std::shared_ptr<boost::asio::io_service::work>;

		std::vector<io_service_ptr> io_services_;
		std::vector<work_ptr> work_;
		std::size_t next_io_service_;
	};

}

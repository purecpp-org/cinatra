//
// io_service_pool.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#pragma once

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
//#include <boost/thread.hpp>
#include <memory>
#include <vector>
#include <thread>

namespace cinatra
{
	/// A pool of io_service objects.
	class IOServicePool
		: private boost::noncopyable
	{
	public:
		/// Construct the io_service pool.
		explicit IOServicePool(std::size_t pool_size)
			:next_io_service_(0)
		{
			if (pool_size == 0)
				throw std::runtime_error("io_service_pool size is 0");

			// Give all the io_services work to do so that their run() functions will not
			// exit until they are explicitly stopped.
			for (std::size_t i = 0; i < pool_size; ++i)
			{
				io_service_ptr io_service(new boost::asio::io_service);
				work_ptr work(new boost::asio::io_service::work(*io_service));
				io_services_.push_back(io_service);
				work_.push_back(work);
			}
		}

		/// Run all io_service objects in the pool.
		void run()
		{
			// Create a pool of threads to run all of the io_services.
			std::vector<std::shared_ptr<std::thread> > threads;
			for (auto service : io_services_)
			{
				std::shared_ptr<std::thread> thread(
					std::make_shared<std::thread>(
					boost::bind(&boost::asio::io_service::run, service)));
				threads.push_back(thread);
			}

			// Wait for all threads in the pool to exit.
			for (auto thread : threads)
				thread->join();
		}

		/// Stop all io_service objects in the pool.
		void stop()
		{
			// Explicitly stop all io_services.
			for (std::size_t i = 0; i < io_services_.size(); ++i)
				io_services_[i]->stop();
		}

		/// Get an io_service to use.
		boost::asio::io_service& get_io_service()
		{
			// Use a round-robin scheme to choose the next io_service to use.
			boost::asio::io_service& io_service = *io_services_[next_io_service_];
			++next_io_service_;
			if (next_io_service_ == io_services_.size())
				next_io_service_ = 0;
			return io_service;
		}

	private:
		typedef std::shared_ptr<boost::asio::io_service> io_service_ptr;
		typedef std::shared_ptr<boost::asio::io_service::work> work_ptr;

		/// The pool of io_services.
		std::vector<io_service_ptr> io_services_;

		/// The work that keeps the io_services running.
		std::vector<work_ptr> work_;

		/// The next io_service to use for a connection.
		std::size_t next_io_service_;
	};
} // namespace http

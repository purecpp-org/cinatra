

#include <cinatra/http_server.hpp>
#include <thread>

int main()
{
	cinatra::HTTPServer s(std::thread::hardware_concurrency());

	s.set_request_handler(
		[](cinatra::Request& req, cinatra::Response& res)
	{
		if (req.path() == "/")
		{
			res.write("Hello,world");
			return true;
		}
		
		if (req.path() == "/123")
		{
			int n = boost::lexical_cast<int>(req.query().get_val("n"));
			for (int i = 0; i < n; ++i)
			{
				res.direct_write("Hello " + boost::lexical_cast<std::string>(i)+"\n");
			}
			return true;
		}

		return false;
	})
		.set_error_handler([](int code, const std::string&, cinatra::Request&, cinatra::Response& res)
	{
		res.set_status_code(code);
		res.write("Error: " + boost::lexical_cast<std::string>(code));
		return true;
	})
	.listen("0.0.0.0", "http").run();

	return 0;
}

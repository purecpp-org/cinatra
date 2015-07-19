
#include <cinatra/cinatra.hpp>
#include <fstream>

struct CheckLoginAspect
{
	void before(const Request& req, Response& res)
	{
		std::cout << req.url() << endl;
	}

	void after(const Request& req, Response& res)
	{

	}
};

int main()
{
	cinatra::Cinatra<CheckLoginAspect> app;

	app.route("/hello/:name/:age/:test", [](const cinatra::Request& req, cinatra::Response& res, const std::string& a, int b, double c)
	{
		res.end("Name: " + a + " Age: " + lexical_cast<std::string>(b)+"Test: " + lexical_cast<std::string>(c));
	});

	app.error_handler(
		[](int code, const std::string&, const cinatra::Request&, cinatra::Response& res)
	{
		if (code != 404)
		{
			return false;
		}

		res.set_status_code(404);
		res.write(
			R"(<html>
			<head>
				<meta charset="UTF-8">
				<title>404</title>
			</head>
			<body>
			<img src="/img/404.jpg" width="100%" height="100%" />
			</body>
			</html>)"
		);

		return true;
	});

	app.public_dir("./public").threads(std::thread::hardware_concurrency()).listen("0.0.0.0", "http").run();

	return 0;
}

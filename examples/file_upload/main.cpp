
#include <cinatra/cinatra.hpp>
#include <iostream>
#include <fstream>
#include <vector>


int main()
{
	cinatra::SimpleApp app;
	app.route("/", [](cinatra::Request&, cinatra::Response& res)
	{
		res.redirect("/upload.html");
	});
	app.route("/do_upload", [](cinatra::Request& req, cinatra::Response& res)
	{
		if (req.method() != cinatra::Request::method_t::POST)
		{
			return;
		}

		std::vector<cinatra::item_t> items;
		if (!cinatra::data_body_parser(req, items))
		{
			res.end("upload failed...");
			return;
		}

		for (auto& item : items)
		{
			if (!item.is_file)
			{
				std::cout << "field name: " << item.content_disposition.get_val("name") << ", value: " << item.data << std::endl;
				continue;
			}
			std::ofstream out("./"+item.content_disposition.get_val("filename"), std::ios::binary | std::ios::out);
			if (!out || !out.write(item.data.data(), item.data.size()))
			{
				res.end("upload failed...");
				return;
			}
		}

		res.end("upload success!");
	});

	app.static_dir("./static").listen("HTTP").run();
	return 0;
}
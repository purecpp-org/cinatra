
#pragma once

#include <cinatra/utils.hpp>
#include <cinatra/request.hpp>
#include <cinatra/logging.hpp>

namespace cinatra
{
	inline CaseMap urlencoded_body_parser(const std::string& data)
	{
		return kv_parser<std::string::const_iterator, CaseMap, '=', '&'>(data.begin(), data.end(), false);
	}

	struct item_t
	{
		bool is_file;

		NcaseMultiMap content_disposition;
		// 只有在is_file为true时有效.
		std::string content_type;
		// is_file为false时为字段的值,
		// 为true时为文件内容
		std::string data;
	};



	void parse_disposition(const std::string& disposition, NcaseMultiMap& kv)
	{
		std::string key, val;
		bool is_k = true;
		auto trim = [](const std::string& str)
		{
			std::string result;
			for (auto c : str)
			{
				if (c == ' ' || c == '"')
				{
					continue;
				}

				result.push_back(c);
			}

			return result;
		};
		for (auto c : disposition)
		{
			if (c == ';')
			{
				is_k = true;
				if (!val.empty())
				{
					kv.add(trim(key), trim(val));
				}
				key.clear();
			}
			else if (c == '=')
			{
				is_k = false;
				val.clear();
			}
			else
			{
				if (is_k)
				{
					key.push_back(c);
				}
				else
				{
					val.push_back(c);
				}
			}
		}

		if (!is_k)
		{
			kv.add(trim(key), trim(val));
		}
	}

	bool parse_item(const std::string& part,item_t& item)
	{
		item.is_file = false;
		auto pos = part.find("\r\n\r\n");
		if (pos == std::string::npos)
		{
			return false;
		}
		std::string headers = part.substr(0, pos + 2);
		const auto body_start = pos + 4/*"\r\n\r\n".size*/;
		const auto body_size = part.size() - body_start - 2/*body末尾有两个字节的\r\n*/;
		item.data = part.substr(body_start, body_size);

// 		std::cout << "+++++++++++++++++++++++++++++" << std::endl;
// 		std::cout << "Headers: " << headers << "  Body: " << body << std::endl;
// 		std::cout << "-----------------------------" << std::endl;

		pos = 0;
		bool has_type = false;//Content-Type: 
		bool has_filename = false;//Content-Disposition: filename=""
		for (;;)
		{
			auto tmp = headers.find("\r\n", pos);
			if (tmp == std::string::npos)
			{
				break;
			}

			std::string header = headers.substr(pos, tmp - pos);
			auto pos1 = header.find(':');
			if (pos1 == std::string::npos)
			{
				return false;
			}
			std::string header_name = header.substr(0, pos1);
			std::string header_val = header.substr(pos1 + 1);
			if (boost::iequals(header_name, "Content-Disposition"))
			{
				parse_disposition(header_val, item.content_disposition);
				if (item.content_disposition.get_count("filename") != 0)
				{
					auto tempValue = item.content_disposition.get_val("filename");
					if (!tempValue.empty())
					{
						has_filename = true;
					}
				}
			}
			else if (boost::iequals(header_name, "Content-Type"))
			{
				item.content_type = header_val;
				has_type = true;
			}
			else
			{
				LOG_WARN << "Unknown header: " << header_name << ":" << header_val;
			}
			pos = tmp + 2;
		}
		if (has_type && has_filename)
			item.is_file = true;

		return true;
	}

	bool data_body_parser(Request& req, std::vector<item_t>& items)
	{
		const std::string& content_type = req.header().get_val("Content-Type");
		auto pos = content_type.find("boundary=");
		if (pos == std::string::npos)
		{
			LOG_ERR << "There is no boundary in Content-Type.";
			return false;
		}

		const std::string boundary = "--" + content_type.substr(pos + 9/* "boundary=".size */);

		pos = 0;
		std::string::size_type size = 0;
		for (;;)
		{
			auto tmp = req.body().find(boundary, pos);
			if (tmp == std::string::npos)
			{
				std::cout << req.body().substr(pos) << std::endl;
				break;
			}
			size = tmp - pos;
			
			if (tmp != 0)
			{
				item_t item;
				if (!parse_item(req.body().substr(pos, size),item))
				{
					return false;
				}
				items.push_back(item);
			}

			pos = tmp + boundary.size() + 2;
		}

		return true;
	}
}
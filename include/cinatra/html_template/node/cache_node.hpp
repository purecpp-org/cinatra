/*
 * CacheNode.h
 *
 *  Created on: 2015
 *      Author: jc
 */
#pragma once

#include <cinatra/html_template/node/node.hpp>
#include <cinatra/html_template/common.hpp>
#include <cinatra/html_template/context/context.hpp>
#include <cinatra/html_template/exception.hpp>
#include <cinatra/html_template/io/string_writer.hpp>
#include <cinatra/html_template/parser/expression_parser.hpp>
#include <cinatra/html_template/parser/fragment.hpp>

#include <iostream>
#include <ctime>
#include <functional>
#include <regex>
#include <chrono>
#include <unordered_map>

namespace cinatra
{

	class CacheNode : public Node
	{
	public:
		CacheNode() : Node(true){}

		virtual void render(Writer * stream, Context * context) const
		{
			static std::unordered_map< size_t, CacheRow > s_cached;

			ExpressionParser parser(context);

			size_t hashValue = 1;
			static auto hashFunc = std::hash< std::string >();

			for (auto varName : m_vars)
			{
				Json var = parser.parse(varName);
				hashValue ^= hashFunc(var.dump());
			}

			auto now = std::chrono::system_clock::now();
			auto found = s_cached.find(hashValue);

			auto renderChldrn = [=]() -> std::string
			{
				std::string rendered;
				StringWriter writer(rendered);
				renderChildren(&writer, context);
				return rendered;
			};
			std::string rendered;
			if (found == s_cached.end())
			{
				// insert new element into the cache.
				rendered = renderChldrn();
				s_cached[hashValue] = std::make_tuple(now, rendered);
			}
			else
			{
				// element found. Update if necessary.
				auto row = found->second;
				auto timePoint = std::get< 0 >(row);
				auto elapsedMSec = std::chrono::duration_cast<std::chrono::milliseconds>(now - timePoint).count();
				if (uint64_t(elapsedMSec) >= m_cacheTime)
				{
					// re-render
					rendered = renderChldrn();
					s_cached[hashValue] = std::make_tuple(now, rendered);
				}
				else
				{
					rendered = std::get< 1 >(row);
				}
			}
			stream->write(rendered);
		}

		virtual void processFragment(Fragment const * fragment)
		{
			static std::regex const splitter(R"(^cache\s+(\d+)\s+(.+)$)");
			std::smatch match;
			std::string cleaned = fragment->clean();
			if (!std::regex_match(cleaned, match, splitter))
			{
				throw TemplateSyntaxError(cleaned);
			}
			std::string cacheTimeStr = match[1];
			m_cacheTime = std::stol(cacheTimeStr);
			std::string varsStr = match[2];

			static std::regex const varNameExtractor(R"(\s+)");
			std::sregex_token_iterator varNamesIter(varsStr.begin(), varsStr.end(), varNameExtractor, -1);
			m_vars.clear();
			std::vector< std::string > possibleVars;
			static std::sregex_token_iterator const end;
			std::copy(varNamesIter, end, std::back_inserter(possibleVars));
			if (std::find(possibleVars.begin(), possibleVars.end(), "") != possibleVars.end())
			{
				throw TemplateSyntaxError(varsStr);
			}
			m_vars = possibleVars;
		}

		virtual void exitScope(std::string const & endTag)
		{
			if (endTag != "endcache")
				throw TemplateSyntaxError(endTag);
		}
		virtual std::string name() const{ return "Cache"; }

		virtual ~CacheNode(){}

		typedef std::tuple< std::chrono::time_point<
			std::chrono::system_clock>, std::string > CacheRow;

	protected:
		uint64_t m_cacheTime;
		std::vector< std::string > m_vars;
	};
}


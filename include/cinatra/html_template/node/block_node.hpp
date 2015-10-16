/*
 * BlockNode.h
 *
 *      Author: jc
 */

#pragma once

#include <cinatra/html_template/exception.hpp>
#include <cinatra/html_template/parser/fragment.hpp>
#include <cinatra/html_template/node/node.hpp>

#include <regex>

namespace cinatra
{

	class BlockNode : public Node
	{
	public:
		BlockNode()
			: Node(true)
		{}

		virtual void render(Writer * stream, Context * context) const
		{
			renderChildren(stream, context);
		}
		virtual void processFragment(Fragment const * fragment)
		{
			static std::regex const blockSplitter(R"(^block\s+(\w+)$)");
			std::smatch match;
			std::string cleaned = fragment->clean();
			if (!std::regex_match(cleaned, match, blockSplitter))
			{
				throw TemplateSyntaxError(fragment->clean());
			}
			m_blockName = match[1];
		}
		inline virtual void exitScope(std::string const & endTag)
		{
			if (endTag != "endblock")
				throw TemplateSyntaxError(endTag);
		}

		virtual std::string name() const
		{
			return "Block";
		}
		std::string blockName() const
		{
			return m_blockName;
		}

		virtual ~BlockNode(){}

	protected:
		std::string m_blockName;
	};

}


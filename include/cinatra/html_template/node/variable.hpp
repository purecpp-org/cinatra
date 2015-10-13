/*
 * Variable.h
 *
 *  Created on: 2014
 *      Author: jc
 */
#pragma once

#include <cinatra/html_template/node/node.hpp>


namespace cinatra
{
	class Fragment;

	class Variable : public Node
	{
	public:
		virtual void render(Writer * stream, Context * context) const
		{
			ExpressionParser parser(context);
			Json result = parser.parse(m_expression);
			switch (result.type())
			{
			case Json::NUL:
				stream->write("null");
				break;
			case Json::NUMBER:
				stream->write(dbl2str(result.number_value()));
				break;
			case Json::STRING:
				stream->write(result.string_value());
				break;
			case Json::BOOL:
				stream->write(result.bool_value() ? "true" : "false");
				break;
			case Json::ARRAY:
			case Json::OBJECT:
				stream->write(result.dump());
				break;
			}
		}


		virtual void processFragment(Fragment const * fragment)
		{
			m_expression = fragment->clean();
		}

		virtual std::string name() const
		{
			return "Variable";
		}

		virtual ~Variable()
		{}

	protected:
		std::string m_expression;
	};

}


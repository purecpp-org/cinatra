/*
 * ElseNode.h
 *
 *  Created on: 2014
 *      Author: jc
 */

#pragma once

#include <cinatra/html_template/node/node.hpp>
#include <cinatra/html_template/context/context.hpp>
#include <cinatra/html_template/parser/fragment.hpp>

namespace cinatra
{
	class Writer;

	class ElseNode : public Node
	{
	public:
		virtual void render(Writer * /*stream*/, Context * /*context*/) const{}

		virtual std::string name() const { return "Else"; }

		virtual ~ElseNode(){}
	};

}


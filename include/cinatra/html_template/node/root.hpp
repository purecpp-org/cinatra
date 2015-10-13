/*
 * Root.h
 *
 *  Created on: 2014
 *      Author: jc
 */

#pragma once

#include <cinatra/html_template/node/node.hpp>


#include <string>


namespace cinatra
{

	class Writer;

	class Root : public Node
	{
	public:
		Root(std::string id)
			: Node(), m_id(id)
		{}

		virtual void render(Writer * stream, Context * context) const
		{
			renderChildren(stream, context);
		}

		virtual std::string name() const
		{
			return "Root";
		}

		inline std::string id() const { return m_id; }

		virtual ~Root()
		{}

	protected:
		std::string m_id;
	};

}


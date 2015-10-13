/*
 * Template.h
 *
 *  Created on: 21 ���� 2014 �.
 *      Author: jc
 */
#pragma once

#include <cinatra/html_template/node/root.hpp>
#include <cinatra/html_template/io/string_writer.hpp>
#include <cinatra/html_template/parser/parser.hpp>

#include <memory>
#include <string>
#include <vector>


namespace cinatra
{

	class Context;
	class Reader;
	class Root;
	class Writer;

	class Template
	{
	public:
		virtual ~Template()
		{}

		void renderToStream(Writer * stream, Context * context) const
		{
			m_root->render(stream, context);
		}
		std::string render(Context * context) const
		{
			std::string result;
			StringWriter stringWriter(result);
			renderToStream(&stringWriter, context);
			return result;
		}

	protected:
		Template()
		{}
		void loadFromStream(Reader * stream)
		{
			Parser parser;
			m_root.reset(parser.loadFromStream(stream));
		}

	protected:
		std::shared_ptr< Root const > m_root;
	};

}


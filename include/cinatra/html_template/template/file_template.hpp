/*
 * FileTemplate.h
 *
 *  Created on: 2014
 *      Author: jc
 */
#pragma once

#include <cinatra/html_template/template/template.hpp>

#include <string>

namespace cinatra
{

	class FileTemplate : public Template
	{
	public:
		FileTemplate(std::string const & filePath)
			: m_filePath(filePath)
		{
			FileReader in(filePath);
			loadFromStream(&in);
		}
		virtual ~FileTemplate()
		{}

	private:
		std::string m_filePath;
	};

}


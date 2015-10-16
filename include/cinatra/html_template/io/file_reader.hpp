/*
 * FileReader.h
 *
 *      Author: jc
 */

#pragma once


#include <cinatra/html_template/exception.hpp>
#include <cinatra/html_template/io/reader.hpp>


#include <fstream>
#include <iterator>
#include <string>


namespace cinatra
{
	class FileReader : public Reader
	{
	public:
		FileReader(std::string const & fileName)
			: m_file(fileName), m_path(fileName)
		{
			if (!m_file.good())
			{
				throw IOError("Can not open " + fileName + " file.");
			}
		}

		virtual std::string read(size_t nBytes)
		{
			std::string result(nBytes, '\0');
			m_file.read(&result[0], nBytes);
			return result;
		}
		virtual std::string readAll()
		{
			return std::string(std::istreambuf_iterator< char >(m_file), std::istreambuf_iterator< char >());
		}
		virtual std::string id() const
		{
			return m_path;
		}

		virtual ~FileReader(){}

	protected:
		std::ifstream m_file;
		std::string m_path;

	private:
		FileReader(FileReader const &) { }
	};

}


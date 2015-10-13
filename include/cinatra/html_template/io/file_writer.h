/*
 * FileWriter.h
 *
 *      Author: jc
 */

#pragma once

#include <cinatra/html_template/io/writer.hpp>
#include <cinatra/html_template/exception.hpp>

#include <fstream>
#include <algorithm>
#include <iterator>


namespace cinatra
{

class FileWriter : public Writer
{
public:
	FileWriter(std::string const & fileName)
		: m_file(fileName)
	{
		if (!m_file.good())
		{
			throw IOError("Can not create " + fileName + " file.");
		}
	}

   virtual void write( std::string const & data )
   {
	   std::copy(data.begin(), data.end(), std::ostream_iterator< char >(m_file));
   }
   virtual void flush()
   {
	   m_file.flush();
   }

   virtual ~FileWriter(){};

protected:
   std::ofstream m_file;

private:
   FileWriter( FileWriter const & ) { }
};

} /* namespace GreenZone */

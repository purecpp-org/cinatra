/*
 * Parser.h
 *
 *  Created on: 2014
 *      Author: jc
 */

#pragma once

#include <functional>
#include <map>
#include <vector>


#define BLOCK_START_TOKEN   "{%"
#define BLOCK_END_TOKEN     "%}"
#define VAR_START_TOKEN     "{{"
#define VAR_END_TOKEN       "}}"
#define COMMENT_START_TOKEN "{#"
#define COMMENT_END_TOKEN   "#}"

namespace cinatra
{

	class Fragment;
	class Node;
	class Reader;
	class Root;

	enum class ElementType
	{
		VarElement,
		OpenBlockFragment,
		CloseBlockFragment,
		TextFragment
	};

	class Parser
	{
	public:
		inline Root * loadFromStream(Reader * stream) const;

		inline static void addPath(std::string path);
		inline static std::vector< std::string >& paths();

		virtual ~Parser(){}

	protected:
		inline Node * createNode(Fragment const * fragment) const;
	};

}

#include <cinatra/html_template/parser/parser.ipp>
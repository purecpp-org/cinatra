/*
 * Parser.cpp
 *
 *  Created on: 2014
 *      Author: jc
 */


#include <cinatra/html_template/common.hpp>
#include <cinatra/html_template/exception.hpp>
#include <cinatra/html_template/io/reader.hpp>
#include <cinatra/html_template/node/block_node.hpp>
#include <cinatra/html_template/node/cache_node.hpp>
#include <cinatra/html_template/node/each_node.hpp>
#include <cinatra/html_template/node/else_node.hpp>
#include <cinatra/html_template/node/extends_node.hpp>
#include <cinatra/html_template/node/if_node.hpp>
#include <cinatra/html_template/node/include_node.hpp>
#include <cinatra/html_template/node/root.hpp>
#include <cinatra/html_template/node/text_node.hpp>
#include <cinatra/html_template/node/variable.hpp>
#include <cinatra/html_template/parser/fragment.hpp>


#include <iostream>
#include <regex>
#include <stack>
#include <utility>


namespace cinatra
{
	Root * Parser::loadFromStream(Reader * stream) const
	{

		static std::string commentExpr = ([]() -> std::string
		{
			std::string result = "(" COMMENT_START_TOKEN ".*?" COMMENT_END_TOKEN ")";
			result = replaceString(result, "{", "\\{");
			result = replaceString(result, "}", "\\}");
			return result;
		})();

		static std::regex const commentMatcher(commentExpr);

		static std::string splitExpr = ([]() -> std::string 
		{
			std::string result = "(" VAR_START_TOKEN ".*?" VAR_END_TOKEN "|"
				BLOCK_START_TOKEN ".*?" BLOCK_END_TOKEN ")";
			result = replaceString(result, "{", "\\{");
			result = replaceString(result, "}", "\\}");
			return result;
		})();

		static std::regex const tokenSplitter(splitExpr);

		std::vector< std::shared_ptr< Fragment > > fragments;
		std::string templateSrc = stream->readAll();

		templateSrc = std::regex_replace(templateSrc, commentMatcher, "");

		std::sregex_token_iterator iter(
			templateSrc.begin(), templateSrc.end(), tokenSplitter, std::vector< int >{ -1, 0 });
		static std::sregex_token_iterator const end;
		for (; iter != end; ++iter)
		{
			if (!(*iter).length())
			{
				continue;
			}
			fragments.push_back(std::make_shared< Fragment >(*iter));
		}

		Root * root(new Root(stream->id()));

		std::stack< Node * > scopeStack;
		scopeStack.push(root);

		for (auto fragment : fragments)
		{
			if (!scopeStack.size())
			{
				throw Exception("nesting issues");
			}

			auto parentScope = scopeStack.top();
			if (fragment->type() == ElementType::CloseBlockFragment)
			{
				parentScope->exitScope(fragment->clean());
				scopeStack.pop();
				continue;
			}

			auto newNode = createNode(fragment.get());
			parentScope->addChild(newNode);
			if (newNode->createsScope())
			{
				scopeStack.push(newNode);
				newNode->enterScope();
			}
		}
		if (scopeStack.size() > 1)
		{
			throw Exception("There is non-closed tag " + scopeStack.top()->name());
		}
		return root;
	}

	Node * Parser::createNode(Fragment const * fragment) const
	{
		Node * node = nullptr;
		switch (fragment->type())
		{
		case ElementType::TextFragment:
			node = new TextNode();
			break;
		case ElementType::VarElement:
			node = new Variable();
			break;
		case ElementType::OpenBlockFragment:
		{
			auto finder = [fragment](std::pair< std::string, std::function< Node * () > > value)
			{
				return std::regex_search(fragment->clean(), std::regex(value.first));
			};

			const std::map<
				std::string, std::function< Node * () >
			> s_nodeCreators{
				{ R"(^if\s+.*$)", []() { return new IfNode();      } },
				{ R"(^else$)", []() { return new ElseNode();    } },
				{ R"(^for\s+\w[a-zA-Z0-9 _,]* \s*in\s+.+$)", []() { return new EachNode();    } },
				{ R"(^include\s+.+$)", []() { return new IncludeNode(); } },
				{ R"(^block\s+\w+$)", []() { return new BlockNode();   } },
				{ R"(^extends\s+.+$)", []() { return new ExtendsNode(); } },
				{ R"(^cache\s+\d+\s+.+)", []() { return new CacheNode();   } },
			};
			auto found = std::find_if(s_nodeCreators.begin(), s_nodeCreators.end(), finder);
			if (found == s_nodeCreators.end()) {
				throw TemplateSyntaxError(fragment->clean());
			}
			node = found->second();
		}
											 break;
		default:
			throw TemplateSyntaxError(fragment->clean());
		}
		node->processFragment(fragment);
		return node;
	}

	void Parser::addPath(std::string path)
	{
		path = replaceString(path, "\\", "/");
		if (path.back() != '/')
			path.push_back('/');
		paths().push_back(path);
	}

	std::vector< std::string >& Parser::paths()
	{
		static std::vector< std::string > s_paths{ "./", };
		return s_paths;
	}

}

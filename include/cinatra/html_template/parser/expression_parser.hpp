/*
 * ExpressionParser.h
 *
 *      Author: jc
 */

#pragma once

#include <cinatra/html_template/common.hpp>
#include <cinatra/html_template/context/context.hpp>
#include <cinatra/html_template/exception.hpp>
#include <cinatra/json/json.hpp>


#include <functional>
#include <set>
#include <string>
#include <algorithm>
#include <cmath>
#include <regex>
#include <set>
#include <vector>


#define MIN_PRIORITY 0
#define MAX_PRIORITY 3
#define ARGS_SIZE_CHECK(SIZE) if( args.size() != SIZE ) { \
   throw Exception( "Got " + std::to_string( args.size() ) + \
                    " arguments, expected " + std::to_string( SIZE ) ); \
	        }

namespace cinatra
{
	class Context;

	class ExpressionParser
	{
	public:
		typedef std::set< char > charset;

		ExpressionParser(Context const * context)
			: m_context(context)
		{
			auto binaryOperators = m_context->binaryOperators();
			for (auto i = binaryOperators.begin(); i != binaryOperators.end(); ++i)
			{
				std::string opString = std::get< 0 >(*i);
				std::copy(opString.begin(), opString.end(), std::inserter(m_binaryOperatorChars, m_binaryOperatorChars.begin()));
			}
		}

		Json parse(std::string expression) const
		{
			{
				// validating
				// FIXME: do not count brackets between quotes
				static std::vector< std::tuple< std::string, char, char > > const validationData
				{
					std::make_tuple("Parentheses mismatch", '(', ')'),
					std::make_tuple("Square brackets mismatch", '[', ']'),
					std::make_tuple("Braces mismatch", '{', '}'),
					std::make_tuple("Quotes mismatch", '"', '"'),
				};
				for (auto data : validationData)
				{
					if (std::count(expression.begin(), expression.end(), std::get< 1 >(data)) !=
						std::count(expression.begin(), expression.end(), std::get< 2 >(data)))
					{
						throw ExpressionException(expression, std::get< 0 >(data));
					}
				}
			}

			Json result;
			try
			{
				result = parseRecursive(expression);
			}
			catch (Exception const & ex)
			{
				throw ExpressionException(expression, std::string(" occurred an exception ") + ex.what());
			}
			return result;
		}

		virtual ~ExpressionParser(){}

	protected:
		Json parseRecursive(std::string expression) const
		{

			std::string err;

			// trying to convert the expression to json
			Json result = Json::parse(expression, err);
			if (!err.length())
			{
				return result;
			}

			trimString(expression);

			// perhaps that's a variable
			static std::regex const variableRegex(R"(^[a-zA-Z_][a-zA-Z_0-9\.]*$)");
			std::smatch match;
			if (std::regex_match(expression, match, variableRegex))
			{
				try
				{
					result = m_context->resolve(expression);
					return result;
				}
				catch (TemplateContextError const &)
				{
					// FFFFUUUUU~!
				}
			}

			// ok... then that's a complex expression, have to parse it... damn! I hate this
			if (expression.front() == '(' && expression.back() == ')')
			{
				// Ok... we might omit parentheses around this expression
				// but something like "(1 + 2) + (3 + 4)" breaks everything!
				// Checking for this sh#t
				bool skip = false;
				int pcount = 0;
				for (auto i = expression.begin(); i != expression.end(); ++i)
				{
					if (*i == '(')
					{
						pcount++;
					}
					else if (*i == ')')
					{
						pcount--;
					}
					if (!pcount && i + 1 != expression.end())
					{
						skip = true;
						break;
					}
				}

				if (!skip)
				{
					return parseRecursive(expression.substr(1, expression.length() - 2));
				}
			}

			// if the expression is a function call

			Context::Functions const & functions = m_context->functions();
			static std::regex const funcRegex(R"(^(\w+)\s*\((.+)\)$)");
			std::smatch funcMatch;
			if (std::regex_match(expression, funcMatch, funcRegex))
			{
				std::string funcName = funcMatch[1];
				Context::Functions::const_iterator foundFunc;
				if ((foundFunc = functions.find(funcName)) == functions.end())
				{
					throw ExpressionException(expression, "No such function: " + funcName);
				}
				std::string argsString = funcMatch[2];
				std::vector< Json > args;
				int pcount = 0, qbcount = 0, bcount = 0;
				bool inQuotes = false;
				decltype(argsString)::const_iterator start = argsString.begin(), current = argsString.begin();
				for (; current != argsString.end(); ++current)
				{
					if (*current == '"')
					{
						inQuotes = !inQuotes;
					}
					if (inQuotes)
					{
						continue;
					}
					if (*current == '(')
					{
						pcount++;
					}
					else if (*current == ')')
					{
						pcount--;
					}
					if (*current == '[')
					{
						qbcount++;
					}
					else if (*current == ']')
					{
						qbcount--;
					}
					if (*current == '{')
					{
						bcount++;
					}
					else if (*current == '}')
					{
						bcount--;
					}
					// monkeycoding... hate this!
					bool isInBrackets = pcount != 0 || qbcount != 0 || bcount != 0;
					bool nextIsEnd = current + 1 == argsString.end();
					if (!isInBrackets && (*current == ',' || nextIsEnd))
					{
						std::string argExpr;
						std::copy(start, current + (nextIsEnd ? 1 : 0), std::back_inserter(argExpr));
						args.push_back(parseRecursive(argExpr));
						start = current + 1; // FIXME: oops, it's dangerous
					}
				}
				try 
				{
					result = foundFunc->second(args);
				}
				catch (Exception const & ex)
				{
					throw ExpressionException(expression, foundFunc->first + " raised exception: " + ex.what());
				}
				return result;
			}

			// it the expression is an expression exactly...
			Context::BinaryOperators const & binaryOperators = m_context->binaryOperators();
			for (int priority = MIN_PRIORITY; priority <= MAX_PRIORITY; ++priority)
			{
				int len = expression.length(), i = 0, pcount = 0;
				bool inQuotes = false;
				std::string lhs, rhs;
				i = len - 1;
				while (i >= 0)
				{
					if (expression[i] == '"')
					{
						inQuotes = !inQuotes;
					}
					if (inQuotes)
					{
						i--;
						continue;
					}
					if (expression[i] == ')')
					{
						pcount++;
					}
					else if (expression[i] == '(')
					{
						pcount--;
					}
					auto finder = [&](Context::BinaryOperators::value_type const & opData)
					{
						if (priority != std::get< 1 >(opData))
							return false;
						std::string op = std::get< 0 >(opData);
						std::string possibleOp;
						int sensor = i;
						for (;
							sensor < int(expression.length()) && m_binaryOperatorChars.find(expression[sensor]) != m_binaryOperatorChars.end();
							++sensor)
						{
							possibleOp.push_back(expression[sensor]);
						}
						return op == possibleOp;
					};
					Context::BinaryOperators::const_iterator opIter;
					if (!pcount && m_binaryOperatorChars.find(expression[i]) != m_binaryOperatorChars.end() &&
						(opIter = std::find_if(binaryOperators.begin(), binaryOperators.end(), finder)) != binaryOperators.end())
					{
						rhs = expression.substr(i + std::get< 0 >(*opIter).length());
						lhs = expression.substr(0, i);
						try {
							result = std::get< 2 >(*opIter)(parseRecursive(lhs), parseRecursive(rhs));
						}
						catch (Exception const & ex)
						{
							throw ExpressionException(expression, "operator " + std::get< 0 >(*opIter) + " raised exception: " + ex.what());
						}
						return result;
					}
					i--;
				}
			}

			// else
			throw ExpressionException(expression, "Wrong syntax or undefined variable");
		}


	protected:
		Context const * m_context;
		charset m_binaryOperatorChars;
	};

}


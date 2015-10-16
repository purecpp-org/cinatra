/*
 * Context.h
 *
 *  Created on: 21 ���� 2014 �.
 *      Author: jc
 */
#pragma once

#include <cinatra/json/json.hpp>
#include <cinatra/html_template/common.hpp>
#include <cinatra/html_template/exception.hpp>

#include <functional>
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>
#include <random>
#include <regex>


#define ARGS_SIZE_CHECK(SIZE) if( args.size() != SIZE ) { \
   throw Exception( "Got " + std::to_string( args.size() ) + \
                    " arguments, expected " + std::to_string( SIZE ) ); \
										        }
namespace cinatra
{
	class Context
	{
	public:
		typedef std::function< Json(Json const &, Json const &) > BinaryOperator;
		typedef std::vector< std::tuple< std::string, int, BinaryOperator > > BinaryOperators;

		typedef std::function< Json(std::vector< Json > const &) > Function;
		typedef std::map< std::string, Function > Functions;

		Context(std::string const & json)
			: Context()
		{
			std::string err;
			m_json = Json::parse(json, err);
			if (err.size())
			{
				throw JsonError(err);
			}
			if (!m_json.is_object())
			{
				throw JsonError("Context data must be presented in dictionary type.");
			}
		}

		Context(Json const & json)
			: Context()
		{
			m_json = json;
			if (!m_json.is_object())
			{
				throw JsonError("Context data must be presented in dictionary type.");
			}
		}

		Json json() const
		{
			return m_json;
		}
		void setJson(Json const & json)
		{
			if (!m_json.is_object())
			{
				throw JsonError("Context data must be presented in dictionary type.");
			}
			m_json = json;
		}

		Json resolve(std::string const & name) const
		{
			Json result = m_json;
			static const std::regex splitter(R"(\.)");
			std::sregex_token_iterator iter(name.begin(), name.end(), splitter, -1);
			static const std::sregex_token_iterator end;
			for (; iter != end; ++iter)
			{
				Json tmp = result[*iter];
				if (tmp.is_null())
				{
					throw TemplateContextError(name);
				}
				result = tmp;
			}
			return result;
		}

		BinaryOperators const & binaryOperators() const
		{
			return m_binaryOperations;
		}
		Functions const & functions() const
		{
			return m_functions;
		}

		virtual ~Context()
		{}

	protected:
		Context()
			: m_binaryOperations(
		{
			std::make_tuple("+", 2, [](Json const & lhs, Json const & rhs) -> Json
			{
				static std::map< std::tuple< Json::Type, Json::Type >,
					std::function< Json(Json const &, Json const &) > > const possibleOperations
				{
					{
						std::make_tuple(Json::NUMBER, Json::NUMBER),

						[](Json const & lhs, Json const & rhs) -> Json
						{
							// Yes-yes, I know about implicit constructors feature
							// but I'd rather to call explicit instead
							return Json(lhs.number_value() + rhs.number_value());
						}
					},

					{
						std::make_tuple(Json::STRING, Json::STRING),
						[](Json const & lhs, Json const & rhs) -> Json
						{
							return Json(lhs.string_value() + rhs.string_value());
						}
					},
					{
						std::make_tuple(Json::NUMBER, Json::STRING),
						[](Json const & lhs, Json const & rhs) -> Json
						{
							return Json(dbl2str(lhs.number_value()) + rhs.string_value());
						}
					},

					{
						std::make_tuple(Json::STRING, Json::NUMBER),
						[](Json const & lhs, Json const & rhs) -> Json
						{
							return Json(lhs.string_value() + dbl2str(rhs.number_value()));
						}
					}
				};
				decltype(possibleOperations)::const_iterator foundOperation;
				if ((foundOperation = possibleOperations.find(
					std::make_tuple(lhs.type(), rhs.type()))) == possibleOperations.end())
				{
					throw Exception("Type mismatch: can not add " + lhs.dump() + " to " + rhs.dump());
				}
				return foundOperation->second(lhs, rhs);
			}),

				std::make_tuple("-", 2, [](Json const & lhs, Json const & rhs) -> Json
			{
				static std::map< std::tuple< Json::Type, Json::Type >,
					std::function< Json(Json const &, Json const &) > > const possibleOperations
				{
					{
						std::make_tuple(Json::NUMBER, Json::NUMBER),
						[](Json const & lhs, Json const & rhs) -> Json
						{
							return Json(lhs.number_value() - rhs.number_value());
						}
					}
				};
				decltype(possibleOperations)::const_iterator foundOperation;
				if ((foundOperation = possibleOperations.find(
					std::make_tuple(lhs.type(), rhs.type()))) == possibleOperations.end())
				{
					throw Exception("Type mismatch: can not subtract " + lhs.dump() + " from " + rhs.dump());
				}
				return foundOperation->second(lhs, rhs);
			}),

				std::make_tuple("*", 3, [](Json const & lhs, Json const & rhs) -> Json
			{
				static std::map< std::tuple< Json::Type, Json::Type >,
					std::function< Json(Json const &, Json const &) > > const possibleOperations
				{
					{
						std::make_tuple(Json::NUMBER, Json::NUMBER),
						[](Json const & lhs, Json const & rhs) -> Json
						{
							return Json(lhs.number_value() * rhs.number_value());
						}
					},

					{
						std::make_tuple(Json::STRING, Json::NUMBER),
						[](Json const & lhs, Json const & rhs) -> Json
						{
							std::string repeated;
							if (rhs.number_value() < 0.f)
							{
								throw Exception("String multiplier is negative");
							}
							for (auto i = 0; i < rhs.number_value(); ++i)
							{
								repeated += lhs.string_value();
							}
							return Json(repeated);
						}
					},
				};

				decltype(possibleOperations)::const_iterator foundOperation;
				if ((foundOperation = possibleOperations.find(
					std::make_tuple(lhs.type(), rhs.type()))) == possibleOperations.end())
				{
					throw Exception("Type mismatch: can not multiply " + lhs.dump() + " and " + rhs.dump());
				}
				return foundOperation->second(lhs, rhs);
			}),
				std::make_tuple("/", 3, [](Json const & lhs, Json const & rhs) -> Json
			{
				static std::map< std::tuple< Json::Type, Json::Type >,
					std::function< Json(Json const &, Json const &) > > const possibleOperations
				{
					{
						std::make_tuple(Json::NUMBER, Json::NUMBER),
						[](Json const & lhs, Json const & rhs) -> Json
						{
							return Json(lhs.number_value() / rhs.number_value());
						}
					}
				};
				decltype(possibleOperations)::const_iterator foundOperation;
				if ((foundOperation = possibleOperations.find(
					std::make_tuple(lhs.type(), rhs.type()))) == possibleOperations.end())
				{
					throw Exception("Type mismatch: can not divide " + lhs.dump() + " by " + rhs.dump());
				}
				return foundOperation->second(lhs, rhs);
			}),
				std::make_tuple(">", 1, [](Json const & lhs, Json const & rhs) -> Json
			{
				return lhs > rhs;
			}),
				std::make_tuple("<", 1, [](Json const & lhs, Json const & rhs) -> Json
			{
				return lhs < rhs;
			}),
				std::make_tuple("==", 1, [](Json const & lhs, Json const & rhs) -> Json
			{
				return lhs == rhs;
			}),
				std::make_tuple("!=", 1, [](Json const & lhs, Json const & rhs) -> Json
			{
				return lhs != rhs;
			}),
				std::make_tuple("<=", 1, [](Json const & lhs, Json const & rhs) -> Json
			{
				return lhs <= rhs;
			}),
				std::make_tuple(">=", 1, [](Json const & lhs, Json const & rhs) -> Json
			{
				return lhs >= rhs;
			}),
				std::make_tuple("&&", 0, [](Json const & lhs, Json const & rhs) -> Json
			{
				return lhs.bool_value() && rhs.bool_value();
			}),
				std::make_tuple("||", 0, [](Json const & lhs, Json const & rhs) -> Json
			{
				return lhs.bool_value() || rhs.bool_value();
			})
				// we need more gol... operators!
		})
		, m_functions(
			{
				{
					"sin", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(1);
						if (!args[0].is_number())
						{
							throw Exception("Function accepts only numeric arguments");
						}
						return Json(::sin(args[0].number_value()));
					}
				},
				{
					"cos", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(1);
						if (!args[0].is_number()) {
							throw Exception("Function accepts only numeric arguments");
						}
						return Json(::cos(args[0].number_value()));
					}
				},
				{
					"length", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(1);
						Json arg = args[0];
						if (arg.is_array()) {
							return Json(int(arg.array_items().size()));
						}
						else if (arg.is_object())
						{
							return Json(int(arg.object_items().size()));
						}
						else if (arg.is_string())
						{
							return Json(int(arg.string_value().size()));
						}
						else
						{
							throw Exception("Can not calculate length of non-iterable object: " + arg.dump());
						}
					}
				},

				{
					"not", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(1);
						return Json(!args[0].bool_value());
					}
				},
				{
					"get", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(2);
						Json container = args[0];
						Json key = args[1];
						Json result;
						if (container.is_array())
						{
							if (!key.is_number())
							{
								throw Exception("Key must be number, got " + key.dump());
							}
							return container[static_cast<size_t>(key.number_value())];
						}
						else if (container.is_object())
						{
							if (!key.is_string())
							{
								throw Exception("Key must be string, got " + key.dump());
							}
							return container[key.string_value()];
						}
						else if (container.is_string())
						{
							if (!key.is_number())
							{
								throw Exception("Key must be number, got " + key.dump());
							}
							return Json(
								std::string(1, container.string_value()[static_cast<size_t>(key.number_value())]));
						}
						throw Exception("Can not get anything from " + container.dump());
					}
				},
				{
					"lower", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(1);
						if (!args[0].is_string())
						{
							throw Exception("Can not make lower non-string objects");
						}
						std::string arg = args[0].string_value();
						std::string result;
						std::transform(arg.begin(), arg.end(), std::back_inserter(result), ::tolower);
						return Json(result);
					}
				},
				{
					"upper", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(1);
						if (!args[0].is_string())
						{
							throw Exception("Can not make upper non-string objects");
						}
						std::string arg = args[0].string_value();
						std::string result;
						std::transform(arg.begin(), arg.end(), std::back_inserter(result), ::toupper);
						return Json(result);
					}
				},
				{
					"contains", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(2);
						bool result = false;
						if (args[0].is_string() && args[1].is_object())
						{
							if (args[1][args[0].string_value()].type() != Json::NUL)
							{
								result = true;
							}
						}
						else if (args[0].is_string() && args[1].is_string())
						{
							result = args[1].string_value().find(args[0].string_value()) != std::string::npos;
						}
						else if (args[1].is_array())
						{
							auto arrayItems = args[1].array_items();
							result = std::find(arrayItems.begin(), arrayItems.end(), args[0]) != arrayItems.end();
						}
						else
						{
							throw Exception("Can not find appropriate signature");
						}
						return Json(result);
					}
				},
				{
					"to_json", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(1);
						return Json(args[0].dump());
					}
				},
				{
					"random", [](std::vector< Json > const & args) -> Json
					{
						ARGS_SIZE_CHECK(2);
						if (!args[0].is_number() || !args[1].is_number())
						{
							throw Exception("Function accepts only numeric arguments");
						}
						int a = int(args[0].number_value()),
							b = int(args[1].number_value());
						if (a == b)
						{
							return Json(a);
						}
						if (a > b)
						{
							std::swap(a, b);
						}
						std::random_device seed;
						std::default_random_engine generator(seed());
						std::uniform_int_distribution< int > distribution(a, b);
						return Json(distribution(generator));
					}
				},
			})
			{}

	protected:
		Json m_json;
		BinaryOperators m_binaryOperations;
		Functions m_functions;
	};

}



#pragma once

#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash.hpp>
#include <boost/unordered_map.hpp>

#include <map>
#include <string>
#include <vector>

namespace cinatra
{
	class CaseMap
	{
	public:
		void add(const std::string& key, const std::string& val)
		{
			map_.emplace(key, val);
		}
		const std::string& get_val(const std::string& key) const
		{
			auto iter = map_.find(key);
			if (iter == map_.end())
			{
				static const std::string empty_str;
				return empty_str;
			}

			return iter->second;
		}

		bool has_key(const std::string& key) const
		{
			return map_.find(key) != map_.end();
		}

		void clear()
		{
			map_.clear();
		}

		std::vector<std::pair<std::string, std::string>>
			get_all()
		{
			std::vector<std::pair<std::string, std::string>> result;
			for (auto pair : map_)
			{
				result.push_back(pair);
			}

			return result;
		}
		
		size_t size()
		{
			return map_.size();
		}
	private:
		std::map<std::string, std::string> map_;
	};

	class NcaseMultiMap
	{
	public:
		void add(const std::string& key, const std::string& val)
		{
			map_.emplace(key, val);
		}

		const std::string& get_val(const std::string& key) const
		{
			if (map_.count(key))
			{
				return map_.find(key)->second;
			}
			static const std::string empty_str;
			return empty_str;
		}

		std::size_t get_count(const std::string& key)
		{
			return map_.count(key);
		}

		std::vector<std::string>
			get_vals(const std::string& key) const
		{
			auto range = map_.equal_range(key);
			std::vector<std::string> values;
			for (auto iter = range.first;
				iter != range.second; ++iter)
			{
				values.push_back(iter->second);
			}

			return values;
		}

		bool val_equal(const std::string& key, const std::string& str)
		{
			return get_val(key) == str;
		}
		bool val_ncase_equal(const std::string& key, const std::string& str)
		{
			return boost::iequals(get_val(key), str);
		}

		void clear()
		{
			map_.clear();
		}

		std::vector < std::pair<std::string, std::string> >
			get_all()
		{
			std::vector<std::pair<std::string, std::string>> result;
			for (auto pair : map_)
			{
				result.push_back(pair);
			}

			return result;
		}

		size_t size()
		{
			return map_.size();
		}
	private:
		struct NcaseHash
		{
			size_t operator()(const std::string& key) const
			{
				std::size_t seed = 0;
				std::locale locale;

				for (auto c : key)
				{
					boost::hash_combine(seed, std::toupper(c, locale));
				}

				return seed;
			}
		};

		struct IsKeyEqu
		{
			bool operator()(const std::string& l, const std::string& r) const
			{
				return boost::iequals(l, r);
			}
		};
		boost::unordered_multimap<std::string, std::string, NcaseHash, IsKeyEqu> map_;
	};
}

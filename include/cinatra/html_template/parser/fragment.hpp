/*
 * Fragment.h
 *
 *  Created on: 2014
 *      Author: jc
 */
#pragma once

#include <string>


namespace cinatra
{
	class Fragment
	{
	public:
		Fragment(std::string const & rawText)
			: m_rawText(rawText)
		{
			m_cleanText = cleanFragment();
		}

		std::string cleanFragment() const
		{
			static std::vector< std::string > const starts{ BLOCK_START_TOKEN, VAR_START_TOKEN };
			if (std::find(starts.begin(),
				starts.end(), m_rawText.substr(0, 2)) != starts.end())
			{
				std::string result;
				std::copy(m_rawText.begin() + 2, m_rawText.end() - 2, std::back_inserter(result));
				trimString(result);
				return result;
			}
			return m_rawText;
		}

		ElementType type() const
		{
			std::string rawStart = m_rawText.substr(0, 2);
			if (rawStart == VAR_START_TOKEN)
			{
				return ElementType::VarElement;
			}
			else if (rawStart == BLOCK_START_TOKEN)
			{
				return (m_cleanText.find("end") == 0 ?
					ElementType::CloseBlockFragment : ElementType::OpenBlockFragment);
			}
			return ElementType::TextFragment;
		}

		std::string raw() const
		{
			return m_rawText;
		}
		std::string clean() const
		{
			return m_cleanText;
		}

		virtual ~Fragment(){}

	protected:
		std::string m_rawText;
		std::string m_cleanText;
	};

}


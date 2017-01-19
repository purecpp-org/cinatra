#include <cinatra_http/utils.h>

#include <boost/filesystem.hpp>
#include <algorithm>
#include <cctype>
#include <ctime>

#if defined(_MSC_VER) && !defined(gmtime_r)
#define gmtime_r(tp, tm) ((gmtime_s((tm), (tp)) == 0) ? (tm) : NULL)
#endif

namespace cinatra
{
	bool iequal(const char* src, size_t src_len, const char* dest, size_t dest_len)
	{
		if (src_len != dest_len)
			return false;

		for (size_t i = 0; i < src_len; i++)
		{
			if (std::tolower(src[i]) != std::tolower(dest[i]))
				return false;
		}

		return true;
	}

	//from nghttp2
	const char *MONTH[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	const char *DAY_OF_WEEK[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

	template <typename Iterator>
	Iterator cpydig(Iterator d, uint32_t n, size_t len)
	{
		auto p = d + len - 1;

		do
		{
			*p-- = (n % 10) + '0';
			n /= 10;
		} while (p >= d);

		return d + len;
	}

	std::string http_date(time_t t)
	{
		/* Sat, 27 Sep 2014 06:31:15 GMT */
		std::string res(29, 0);
		http_date(&res[0], t);
		return res;
	}


	char *http_date(char *res, time_t t)
	{
		struct tm tms;

		if (gmtime_r(&t, &tms) == nullptr)
		{
			return res;
		}

		auto p = res;

		auto s = DAY_OF_WEEK[tms.tm_wday];
		p = std::copy_n(s, 3, p);
		*p++ = ',';
		*p++ = ' ';
		p = cpydig(p, tms.tm_mday, 2);
		*p++ = ' ';
		s = MONTH[tms.tm_mon];
		p = std::copy_n(s, 3, p);
		*p++ = ' ';
		p = cpydig(p, tms.tm_year + 1900, 4);
		*p++ = ' ';
		p = cpydig(p, tms.tm_hour, 2);
		*p++ = ':';
		p = cpydig(p, tms.tm_min, 2);
		*p++ = ':';
		p = cpydig(p, tms.tm_sec, 2);
		s = " GMT";
		p = std::copy_n(s, 4, p);

		return p;
	}


	response reply_static_file(std::string const &static_path, request const &req)
	{
        if (req.url().find("..") != std::string::npos)
        {
            return response::stock_reply(response::bad_request);
        }
		response rep;
		if (rep.response_file((boost::filesystem::path(static_path) / req.url().to_string()).generic_string()))
		{
			return rep;
		}
		return response::stock_reply(response::not_found);
	}

	// from h2o
	size_t base64_encode(char *_dst, const void *_src, size_t len, int url_encoded)
	{
		static const char *MAP = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";
		static const char *MAP_URL_ENCODED = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789-_";

		char *dst = _dst;
		const uint8_t *src = reinterpret_cast<const uint8_t *>(_src);
		const char *map = url_encoded ? MAP_URL_ENCODED : MAP;
		uint32_t quad;

		for (; len >= 3; src += 3, len -= 3)
		{
			quad = ((uint32_t)src[0] << 16) | ((uint32_t)src[1] << 8) | src[2];
			*dst++ = map[quad >> 18];
			*dst++ = map[(quad >> 12) & 63];
			*dst++ = map[(quad >> 6) & 63];
			*dst++ = map[quad & 63];
		}
		if (len != 0)
		{
			quad = (uint32_t)src[0] << 16;
			*dst++ = map[quad >> 18];
			if (len == 2)
			{
				quad |= (uint32_t)src[1] << 8;
				*dst++ = map[(quad >> 12) & 63];
				*dst++ = map[(quad >> 6) & 63];
				if (!url_encoded)
					*dst++ = '=';
			}
			else
			{
				*dst++ = map[(quad >> 12) & 63];
				if (!url_encoded)
				{
					*dst++ = '=';
					*dst++ = '=';
				}
			}
		}

		*dst = '\0';
		return dst - _dst;
	}

	bool is_valid_utf8(unsigned char *s, size_t length)
	{
		for (unsigned char *e = s + length; s != e; )
		{
			if (s + 4 <= e && ((*(uint32_t *)s) & 0x80808080) == 0)
			{
				s += 4;
			}
			else
			{
				while (!(*s & 0x80))
				{
					if (++s == e)
					{
						return true;
					}
				}

				if ((s[0] & 0x60) == 0x40)
				{
					if (s + 1 >= e || (s[1] & 0xc0) != 0x80 || (s[0] & 0xfe) == 0xc0)
					{
						return false;
					}
					s += 2;
				}
				else if ((s[0] & 0xf0) == 0xe0)
				{
					if (s + 2 >= e || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 ||
						(s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) || (s[0] == 0xed && (s[1] & 0xe0) == 0xa0))
					{
						return false;
					}
					s += 3;
				}
				else if ((s[0] & 0xf8) == 0xf0)
				{
					if (s + 3 >= e || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 || (s[3] & 0xc0) != 0x80 ||
						(s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) || (s[0] == 0xf4 && s[1] > 0x8f) || s[0] > 0xf4)
					{
						return false;
					}
					s += 4;
				}
				else
				{
					return false;
				}
			}
		}
		return true;
	}

}



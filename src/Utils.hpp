/*
 * Utils.h
 *
 *  Created on: Mar 18, 2016
 *      Author: root
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <sstream>
#include <iostream>

#include <stdlib.h>


static const char uri_safe[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0 };

class Utils {

	public:

		template<class m, class k, class v>
		static const v & get_map_value(m maps, const k & key, const v& def) {
			auto ai = maps.find(key);
			if (ai == maps.end()) return def;
			return ai->second;
		}

		static void parse_querys(std::unordered_map<std::string, std::string>& querys,
				const std::string & query_string) {
			std::vector<std::string> parts;
			boost::split(parts, query_string, boost::is_any_of("&"));

			querys.clear();
			for (auto t : parts) {
				size_t p1 = t.size();
				if (p1 > 0) {
					size_t p0 = t.find("=");
					if (p0 == std::string::npos) continue;
					std::string key = t.substr(0, p0);
					std::string val = t.substr(p0 + 1, p1 - p0 - 1);
					querys[key] = val;
					//							std::cout << key << "\t" << val << std::endl;
				}
			}
		}

//		static void Base64Encode(const std::string& input, std::stringstream& result);

		static std::string gen_password() {
			std::string chars("abcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"1234567890"
					"_");

			boost::random::random_device rng;
			boost::random::uniform_int_distribution<int> index_dist(0, chars.size() - 1);

			char s[9];
			for (int i = 0; i < 8; ++i) {
				s[i] = chars[index_dist(rng)];
			}
			s[8] = '\0';
			return std::string(&s[0]);
		}

		static std::string uri_encode(const std::string &uri) {
			if (uri.length() == 0) return uri;
			std::string ret;
			const unsigned char *ptr = (const unsigned char *) uri.c_str();
			ret.reserve(uri.length());

			for (; *ptr; ++ptr) {
				if (!uri_safe[*ptr]) {
					char buf[5];
					memset(buf, 0, 5);
#ifdef WIN32
					_snprintf_s(buf, 5, "%%%X", (*ptr));
#else
					snprintf(buf, 5, "%%%X", (*ptr));
#endif
					ret.append(buf);
				}
				else if (*ptr == ' ') {
					ret += '+';
				}
				else {
					ret += *ptr;
				}
			}
			return ret;
		}

		static std::string uri_decode(const std::string &uri) {
			//Note from RFC1630:  "Sequences which start with a percent sign
			//but are not followed by two hexadecimal characters (0-9,A-F) are reserved
			//for future extension"
			if (uri.length() == 0) return uri;

			const unsigned char *ptr = (const unsigned char *) uri.c_str();
			std::string ret;
			ret.reserve(uri.length());
			for (; *ptr; ++ptr) {
				if (*ptr == '%') {
					if (*(ptr + 1)) {
						char a = *(ptr + 1);
						char b = *(ptr + 2);
						if (!((a >= 0x30 && a < 0x40) || (a >= 0x41 && a < 0x47))) continue;
						if (!((b >= 0x30 && b < 0x40) || (b >= 0x41 && b < 0x47))) continue;
						char buf[3];
						buf[0] = a;
						buf[1] = b;
						buf[2] = 0;
						ret += (char) strtoul(buf, NULL, 16);
						ptr += 2;
						continue;
					}
				}
				if (*ptr == '+') {
					ret += ' ';
					continue;
				}
				ret += *ptr;
			}
			return ret;
		}

};

#define GET_MAP_STR_DEF( a, b ) Utils::get_map_value( a, std::string( b ), std::string() );

#define GET_MAP_STR( a, b, c ) Utils::get_map_value( a, std::string( b ), c );

#endif /* UTILS_H_ */

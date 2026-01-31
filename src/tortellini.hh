#ifndef TORTELLINI_HH__
#define TORTELLINI_HH__
#pragma once

#ifndef NOMINMAX
#	define _TORTELLINI_UNDEFINE_NOMINMAX_PLEASE
#	define NOMINMAX
#endif

/*
	                                ⣀⣀⣀⣀⣀⣀⣀⡀
	                         ⢀⠄         ⠉⠉⠉⠛⠛⠿⠿⢶⣶⣤⣄⡀
	                      ⢀⣠⠞⠁                   ⠈⠉⠛⠻⠶⣄⡀
	                    ⢀⣴⠟⠁       ⣀⣠⣤⠴⠖⠒⠈⠉    ⠑⠲⢤⣄⡀   ⠉⠑⠤⡀
	                   ⣰⡿⠃     ⣠⣴⡾⠟⠋⠁             ⠉⠻⢷⣦⡀
	                  ⣼⡟    ⢀⣴⡿⠛⠁                    ⠉⠻⣦     ⡀
	                 ⣼⡟    ⣠⡿⠋                 ⣀⣤⡀      ⠑⠄   ⢰⡀
	                ⢸⡟    ⣰⠟    ⢀⣤⣦⡄          ⠰⣿⣿⣿            ⣇
	                ⣿⠁   ⢠⡏     ⠸⣿⣿⡟   ⣤⣤⣀⣠⣾⣦  ⠉⠉⠁       ⢰    ⣿
	               ⢀⡏    ⢸            ⡀⠙⠿⣿⣿⠟⠁           ⢠⡏    ⣿⡄
	               ⢸⠃    ⠈           ⢸⣷                ⣴⡟     ⣿⠃
	               ⠘                 ⠘⠡⢤⣤⣀⣀          ⣠⣾⠟     ⢸⣿
	                                    ⠉⠙⠛⠛⠓     ⢀⣴⠾⠋⠁      ⣼⠇
	                       ⠑⠲⢶⣤⣤⣤⣤⣄⣀⣀⣀⣀  ⡼      ⠐⠊⠉         ⢠⡟
	                ⠸⣄         ⠈⠉⠉⠉⠉⠁   ⢸⡇                 ⢀⡞
	                 ⢻⣦                 ⢸⡇              ⢀⠄⢀⠎
	                  ⠹⢷⡀               ⠘⣿⡀         ⢀⣠⣴⠞⠁
	                    ⠑                ⠘⢧      ⢀⣠⡶⠟⠋⠁ ⠠⣷
	                       ⠒⠤⣤⣀⣀           ⠑    ⠊⠉     ⣀ ⠈
	                          ⠉⠛⠻⠿⠶⠶⠤⠤⠄  ⠠⠤⠤⣤⣤⣤⣤⣶⣶⡶⠶⠞⠛⠉

	                                TORTELLINI
	                  The really, really dumb INI file format.

	                           Made for Tide Online.
	                         Cooked up on 6 Mar, 2020.

	----------------------------------------------------------------------------

	Released under a dual-license. Take your pick, no whammies.

	                                 UNLICENSE

	This is free and unencumbered software released into the public domain.

	Anyone is free to copy, modify, publish, use, compile, sell, or
	distribute this software, either in source code form or as a compiled
	binary, for any purpose, commercial or non-commercial, and by any
	means.

	In jurisdictions that recognize copyright laws, the author or authors
	of this software dedicate any and all copyright interest in the
	software to the public domain. We make this dedication for the benefit
	of the public at large and to the detriment of our heirs and
	successors. We intend this dedication to be an overt act of
	relinquishment in perpetuity of all present and future rights to this
	software under copyright law.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.

	For more information, please refer to <http://unlicense.org/>

	                                MIT LICENSE

	Copyright (c) 2020 Josh Junon

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.

	----------------------------------------------------------------------------
*/

#include <map>
#include <string>
#include <algorithm>
#include <locale>
#include <type_traits>
#include <limits>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace tortellini {

class ini final {
	friend class value;

	struct case_insensitive {
		struct case_insensitive_compare {
			bool operator()(const unsigned char &l, const unsigned char &r) const {
				// < for map
				return std::tolower(l) < std::tolower(r);
			}

			inline static bool compare(const unsigned char &l, const unsigned char &r) {
				// == for OOB calls
				return std::tolower(l) == std::tolower(r);
			}
		};

		static inline bool compare(const std::string &l, const std::string &r) noexcept {
			// std::equal (like ==) for OOB calls
			if (l.size() != r.size()) {
				return false;
			}

			return std::equal(
				l.begin(), l.end(),
				r.begin(),
				case_insensitive_compare::compare
			);
		}

		inline bool operator()(const std::string &l, const std::string &r) const noexcept {
			// lex compare (like <) for map
			return std::lexicographical_compare(
				l.begin(), l.end(),
				r.begin(), r.end(),
				case_insensitive_compare()
			);
		}
	};

public:
	class section;

	class value final {
		friend section;

		std::string &_value;

		inline value(std::string &_value)
		: _value(_value)
		{}

		value(const value &) = delete;
		void operator=(const value &) = delete;

		inline value(value &&) = default;

		static inline unsigned long parse_ul(const std::string &s, unsigned long fallback) noexcept {
			if (s.empty()) return fallback;
			const char* c = s.c_str();
			char* e = nullptr;
			unsigned long v = std::strtoul(c, &e, 0);
			if (!c[0] || (e && *e)) return fallback;
			return v;
		}
		static inline unsigned long long parse_ull(const std::string &s, unsigned long long fallback) noexcept {
			if (s.empty()) return fallback;
			const char* c = s.c_str();
			char* e = nullptr;
			unsigned long long v = std::strtoull(c, &e, 0);
			if (!c[0] || (e && *e)) return fallback;
			return v;
		}
		static inline long parse_l(const std::string &s, long fallback) noexcept {
			if (s.empty()) return fallback;
			const char* c = s.c_str();
			char* e = nullptr;
			long v = std::strtol(c, &e, 0);
			if (!c[0] || (e && *e)) return fallback;
			return v;
		}
		static inline long long parse_ll(const std::string &s, long long fallback) noexcept {
			if (s.empty()) return fallback;
			const char* c = s.c_str();
			char* e = nullptr;
			long long v = std::strtoll(c, &e, 0);
			if (!c[0] || (e && *e)) return fallback;
			return v;
		}
		static inline float parse_f(const std::string &s, float fallback) noexcept {
			if (s.empty()) return fallback;
			const char* c = s.c_str();
			char* e = nullptr;
			float v = std::strtof(c, &e);
			if (!c[0] || (e && *e)) return fallback;
			return v;
		}
		static inline double parse_d(const std::string &s, double fallback) noexcept {
			if (s.empty()) return fallback;
			const char* c = s.c_str();
			char* e = nullptr;
			double v = std::strtod(c, &e);
			if (!c[0] || (e && *e)) return fallback;
			return v;
		}
		static inline long double parse_ld(const std::string &s, long double fallback) noexcept {
			if (s.empty()) return fallback;
			const char* c = s.c_str();
			char* e = nullptr;
			long double v = std::strtold(c, &e);
			if (!c[0] || (e && *e)) return fallback;
			return v;
		}

		template <typename T>
		typename std::enable_if<
			!std::is_convertible<T, std::string>::value,
			std::string
		>::type to_string(T r) const {
			if (std::is_same<T, bool>::value) {
				return r ? "yes" : "no";
			} else if (std::is_floating_point<T>::value) {
				char buf[32];
				int precision = (int)std::numeric_limits<double>::max_digits10 - 1;
				std::snprintf(buf, sizeof(buf), "%.*g", precision, (double)r);
				return std::string(buf);
			} else {
				return std::to_string(r);
			}
		}

		template <typename T>
		typename std::enable_if<
			std::is_convertible<T, std::string>::value,
			std::string
		>::type to_string(T r) const {
			return r;
		}

	public:
		template <typename T>
		inline value & operator =(const T &v) {
			_value = to_string(v);
			return *this;
		}

		inline bool operator |(bool fallback) const {
			return _value.empty()
				? fallback
				: (
					   case_insensitive::compare(_value, "1")
					|| case_insensitive::compare(_value, "true")
					|| case_insensitive::compare(_value, "yes")
				);
		}

		inline std::string operator |(std::string fallback) const {
			_value.erase(
				std::remove( _value.begin(), _value.end(), '\"' ),
				_value.end()
				);
			return _value.empty() ? fallback : _value;
		}

		inline std::string operator |(const char *fallback) const {
			_value.erase(
				std::remove( _value.begin(), _value.end(), '\"' ),
				_value.end()
				);
			return _value.empty() ? fallback : _value;
		}

		inline unsigned long operator |(unsigned long fallback) const {
			return parse_ul(_value, fallback);
		}

		inline unsigned long long operator |(unsigned long long fallback) const {
			return parse_ull(_value, fallback);
		}

		inline long operator |(long fallback) const {
			return parse_l(_value, fallback);
		}

		inline long long operator |(long long fallback) const {
			return parse_ll(_value, fallback);
		}

		inline float operator |(float fallback) const {
			return parse_f(_value, fallback);
		}

		inline double operator |(double fallback) const {
			return parse_d(_value, fallback);
		}

		inline long double operator |(long double fallback) const {
			return parse_ld(_value, fallback);
		}

		inline int operator |(int fallback) const {
			return (int)parse_l(_value, (long)fallback);
		}

		inline unsigned int operator |(unsigned int fallback) const {
			/*
				This is necessary because there is no std::stou.
			*/
			unsigned long ul = parse_ul(_value, (unsigned long)fallback);
			if (sizeof(unsigned int) != sizeof(unsigned long)
				&& ul > std::numeric_limits<unsigned int>::max()) {
				return fallback;
			}
			return (unsigned int)ul;
		}
	};

	class section final {
		friend class ini;


		inline section(std::map<std::string, std::string, case_insensitive> &_mapref)
		: _mapref(_mapref)
		{}

		section(const section &) = delete;
		void operator=(const section &) = delete;

		inline section(section &&) = default;

	public:
		inline value operator[](std::string key) const {
			return value(_mapref[key]);
		}
		
		std::map<std::string, std::string, case_insensitive> &_mapref;
	};

private:
	std::map<std::string, std::map<std::string, std::string, case_insensitive>, case_insensitive> _sections;

	static inline void ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
			return !std::isspace(ch);
		}));
	}

	static inline void rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	static inline void trim(std::string &s) {
		ltrim(s);
		rtrim(s);
	}

	static inline void replace_all(std::string &s, const std::string &search, const std::string &replace) {
		size_t pos = 0;
		while ((pos = s.find(search, pos)) != std::string::npos) {
			s.replace(pos, search.length(), replace);
			pos += replace.length();
		}
	}

public:
	class iterator {
		friend class ini;
		using section_iterator = decltype(_sections)::iterator;
		section_iterator _itr;
		inline iterator(section_iterator itr)
		: _itr(itr)
		{}

	public:
		struct section_pair {
			std::string name;
			::tortellini::ini::section section;
		};

		inline ~iterator() = default;

		inline section_pair operator*() noexcept {
			return { _itr->first, section(_itr->second) };
		}

		inline iterator & operator++() {
			_itr++;
			return *this;
		}

		inline bool operator==(const iterator &other) const {
			return _itr == other._itr;
		}
		inline bool operator!=(const iterator &other) const {
			return _itr != other._itr;
		}
	};

	inline ini() = default;
	inline ini(const ini &) = default;
	inline ini(ini &&) = default;

	inline section operator[](std::string name) noexcept {
		return section(_sections[name]);
	}

	inline iterator begin() {
		return {_sections.begin()};
	}

	inline iterator end() {
		return {_sections.end()};
	}

	template <typename TStream>
	friend inline TStream & operator>>(TStream &stream, tortellini::ini &ini) {
		std::string line;
		std::string section_name = "";
		bool first_line = true;

		while (std::getline(stream, line)) {
			if (first_line) {
				first_line = false;

				// trim BOM if it is present.
				if (line.length() >= 3 && line.substr(0, 3) == "\xEF\xBB\xBF") {
					line = line.substr(3);
				}
			}

			trim(line);

			if (line.empty()) continue;

			if (line[0] == '[') {
				size_t idx = line.find_first_of("]");
				if (idx == std::string::npos) continue; // invalid, drop line
				section_name = line.substr(1, idx - 1);
				trim(section_name);
				continue;
			}

			std::string key;
			std::string value;

			size_t idx = line.find_first_of("=");
			if (idx == std::string::npos) continue; // invalid, drop line

			key = line.substr(0, idx);
			trim(key);

			if (key.empty()) continue;

			value = line.substr(idx + 1);
			trim(value);

			if (value.length() > 0 && value.front() == '"' && (value.length() == 1 || value.back() != '"')) {
				std::string subline;
				while (std::getline(stream, subline)) {
					trim(subline);
					value += "\n" + subline;
					if (!subline.empty() && subline.back() == '"') {
						break;
					}
				}
			}

			replace_all(value, "\\n", "\n");

			if (value.empty()) continue; // not really "invalid" but we choose not to keep it

			ini[section_name][key] = value;
		}

		return stream;
	}

	template <typename TStream>
	friend inline TStream & operator<<(TStream &stream, const tortellini::ini &ini) {
		bool has_sections = false;

		// force emit empty section if it exists
		{
			const auto &itr = ini._sections.find("");
			if (itr != ini._sections.cend()) {
				for (const auto &kv : itr->second) {
					if (kv.first.empty() || kv.second.empty()) continue;
					stream << kv.first << " = " << kv.second << '\n';
					has_sections = true;
				}
			}
		}

		for (const auto &section : ini._sections) {
			if (section.first.empty()) continue; // already emitted

			bool has_emitted = false;

			for (const auto &kv : section.second) {
				if (kv.first.empty() || kv.second.empty()) continue;

				if (!has_emitted) {
					if (has_sections) stream << '\n';
					stream << '[' << section.first << ']' << '\n';
					has_emitted = true;
					has_sections = true;
				}

				stream << kv.first << " = " << kv.second << '\n';
			}
		}

		return stream;
	}
};

}

#ifdef _TORTELLINI_UNDEFINE_NOMINMAX_PLEASE
#	undef NOMINMAX
#endif
#endif

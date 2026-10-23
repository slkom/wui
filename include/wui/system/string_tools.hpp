//
// Copyright (c) 2021-2026 Intent Garden Org
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//

#pragma once

#include <string>
#include <algorithm>
#include <charconv>
#include <locale>
#include <cctype>
#include <iostream>
#include <cmath>

namespace wui
{

/// trim from start (in place)
inline void ltrim(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

/// trim from end (in place)
inline void rtrim(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

/// trim from both ends (in place)
inline void trim(std::string &s)
{
    rtrim(s);
    ltrim(s);
}

/// trim from start (copying)
inline std::string ltrim_copy(std::string s)
{
    ltrim(s);
    return s;
}

/// trim from end (copying)
inline std::string rtrim_copy(std::string s)
{
    rtrim(s);
    return s;
}

/// trim from both ends (copying)
inline std::string trim_copy(std::string s)
{
    trim(s);
    return s;
}

/// return true if string is digit
inline bool is_digit(const std::string& s)
{
    return !s.empty() && std::find_if(s.begin(),
        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

/// <summary>
/// Is numeric [R].
/// Validates the entire (future concatenated) string. Used by the wui::input class.
/// </summary>
/// <param name="s">new add string</param>
/// <param name="line">old full string</param>
/// <returns>true if string is numeric [R]</returns>
inline bool is_number(std::string_view s, std::string_view line)
{
    if (s.empty())
    {
        return false;
    }
    char ch_punc = '.';
    std::locale lcl;
    if (std::has_facet<std::numpunct<char>>(lcl))
    {
        const auto& facet = std::use_facet<std::numpunct<char>>(lcl);
        ch_punc = facet.decimal_point();
    }

#ifndef NDEBUG
    char ch_punct_c = 0;
    auto* le = localeconv();
    if (le && le->decimal_point)
    {
        ch_punct_c = le->decimal_point[0];
    }
    if (ch_punc && ch_punct_c && ch_punc != ch_punct_c)
    {
        std::cerr << "numeric punctuation : 'C' different 'STD lib', use std::locale::global()" << std::endl;
    }
#endif

    const char chset[]{ ch_punc, '-', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'e', 'E', '\0' };
    const char ch = s.data()[0];
    return s.find_first_not_of(chset) == std::string::npos
        && !std::any_of(line.cbegin(), line.cend(), [ch, ch_punc](const char c) {
        return (c == ch_punc || '-' == c || 'e' == c || 'E' == c)
            && std::toupper(ch) == std::toupper(c); });
}

}


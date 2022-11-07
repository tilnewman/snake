#ifndef SNAKE_CHECK_MACROS_HPP_INCLUDED
#define SNAKE_CHECK_MACROS_HPP_INCLUDED
//
// check-macros.hpp
//
#include <cassert>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

//
// These are all three options for how to handle an M_CHECK_... failure:
//    #define M_FAIL_HANDLER(exp, desc_str) assert((exp));
//    #define M_FAIL_HANDLER(exp, desc_str) throw std::runtime_error(desc_str);
//    #define M_FAIL_HANDLER(exp, desc_str) ;

#ifdef NDEBUG
#define M_FAIL_HANDLER(exp, desc_str) throw std::runtime_error(desc_str);
#else
#define M_FAIL_HANDLER(exp, desc_str) assert((exp));
#endif

//

#if !defined(M_DISABLE_ALL_CHECK_MACROS)

#define M_MAKE_CODE_POSITION_STR_SS(macro_name, streamable_extra_info)        \
    std::string _m_desc_str;                                                  \
    {                                                                         \
        std::ostringstream _m_desc_ss;                                        \
                                                                              \
        _m_desc_ss << streamable_extra_info;                                  \
        const std::string _m_extra_str{ _m_desc_ss.str() };                   \
                                                                              \
        _m_desc_ss.str("");                                                   \
                                                                              \
        _m_desc_ss << macro_name << "  at:  " << __FILE__ << "::" << __func__ \
                   << "()::" << __LINE__;                                     \
                                                                              \
        if (!_m_extra_str.empty())                                            \
        {                                                                     \
            _m_desc_ss << ":  \"" << _m_extra_str << "\"";                    \
        }                                                                     \
                                                                              \
        _m_desc_str = _m_desc_ss.str();                                       \
    }

#define M_MAKE_CODE_POSITION_STR(macro_name) M_MAKE_CODE_POSITION_STR_SS(macro_name, "");

//

#define M_LOG_SS(streamable_message)                                 \
    {                                                                \
        M_MAKE_CODE_POSITION_STR_SS("M_LOG_SS", streamable_message); \
        std::cout << _m_desc_str << std::endl;                       \
    }

#define M_LOG(streamable_message) M_LOG_SS(streamable_message);

//

#define M_MAKE_DESCRIPTION_STR_SS(exp, macro_name, streamable_extra_info)                     \
    std::string _m_desc_str;                                                                  \
    {                                                                                         \
        std::ostringstream _m_desc_ss;                                                        \
                                                                                              \
        _m_desc_ss << streamable_extra_info;                                                  \
        const std::string _m_extra_str{ _m_desc_ss.str() };                                   \
                                                                                              \
        _m_desc_ss.str("");                                                                   \
                                                                                              \
        _m_desc_ss << "ERROR:  " << macro_name << '(' << #exp << ") failed at:  " << __FILE__ \
                   << "::" << __func__ << "()::" << __LINE__;                                 \
                                                                                              \
        if (!_m_extra_str.empty())                                                            \
        {                                                                                     \
            _m_desc_ss << ":  \"" << _m_extra_str << "\"";                                    \
        }                                                                                     \
                                                                                              \
        _m_desc_str = _m_desc_ss.str();                                                       \
    }

#define M_MAKE_DESCRIPTION_STR(exp, macro_name) M_MAKE_DESCRIPTION_STR_SS((exp), macro_name, "");

//

#define M_CHECK_LOG_SS(exp, streamable_extra_info)                                  \
    {                                                                               \
        if (!(exp))                                                                 \
        {                                                                           \
            M_MAKE_DESCRIPTION_STR_SS((exp), "M_CHECK_LOG", streamable_extra_info); \
            std::cout << _m_desc_str << std::endl;                                  \
        }                                                                           \
    }

#define M_CHECK_LOG(exp) M_CHECK_LOG_SS((exp), "");

//

#define M_CHECK_THROW_SS(exp, streamable_extra_info)                                  \
    {                                                                                 \
        if (!(exp))                                                                   \
        {                                                                             \
            M_MAKE_DESCRIPTION_STR_SS((exp), "M_CHECK_THROW", streamable_extra_info); \
            std::cout << _m_desc_str << std::endl;                                    \
            throw std::runtime_error(_m_desc_str);                                    \
        }                                                                             \
    }

#define M_CHECK_THROW(exp) M_CHECK_THROW_SS((exp), "");

//

#define M_CHECK_ASSERT_SS(exp, streamable_extra_info)                                  \
    {                                                                                  \
        if (!(exp))                                                                    \
        {                                                                              \
            M_MAKE_DESCRIPTION_STR_SS((exp), "M_CHECK_ASSERT", streamable_extra_info); \
            std::cout << _m_desc_str << std::endl;                                     \
            assert((exp));                                                             \
        }                                                                              \
    }

#define M_CHECK_ASSERT(exp) M_CHECK_ASSERT_SS((exp), "");

//

#define M_CHECK_SS(exp, streamable_extra_info)                                  \
    {                                                                           \
        if (!(exp))                                                             \
        {                                                                       \
            M_MAKE_DESCRIPTION_STR_SS((exp), "M_CHECK", streamable_extra_info); \
            std::cout << _m_desc_str << std::endl;                              \
            M_FAIL_HANDLER((exp), _m_desc_str);                                 \
        }                                                                       \
    }

#define M_CHECK(exp) M_CHECK_SS(exp, "");

#else // defined(M_DISABLE_ALL_CHECK_MACROS)

#define M_LOG_SS(streamable_message) ;
#define M_MAKE_DESCRIPTION_STR_SS(exp, macro_name, streamable_extra_info) ;
#define M_MAKE_DESCRIPTION_STR(exp, macro_name) ;
#define M_CHECK_LOG_SS(exp, streamable_extra_info) ;
#define M_CHECK_LOG(exp) ;
#define M_CHECK_THROW_SS(exp, streamable_extra_info) ;
#define M_CHECK_THROW(exp) ;
#define M_CHECK_ASSERT_SS(exp, streamable_extra_info) ;
#define M_CHECK_ASSERT(exp) ;
#define M_CHECK_SS(exp, streamable_extra_info) ;
#define M_CHECK(exp) ;

#endif // defined(M_DISABLE_ALL_CHECK_MACROS)

#endif // SNAKE_CHECK_MACROS_HPP_INCLUDED

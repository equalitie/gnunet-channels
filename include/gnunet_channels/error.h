#pragma once

#include <string>
#include <boost/system/error_code.hpp>

// Inspired by this post
// http://breese.github.io/2016/06/18/unifying-error-codes.html

namespace gnunet_channels { namespace error {

    enum error_t {
        error_parsing_argv = 1,
        cant_create_pipes,
        invalid_target_id,
        failed_to_open_port,
    };
    
    struct category : public boost::system::error_category
    {
        const char* name() const noexcept override
        {
            return "gnunet_channels_errors";
        }
    
        std::string message(int e) const
        {
            switch (e) {
                case error::error_parsing_argv:
                    return "error parsing argv";
                case error::cant_create_pipes:
                    return "cant create pipes";
                case error::invalid_target_id:
                    return "invalid target id";
                case error::failed_to_open_port:
                    return "failed to open port";
                default:
                    return "unknown gnunet_channels error";
            }
        }
    };
    
    inline
    boost::system::error_code
    make_error_code(::gnunet_channels::error::error_t e)
    {
        static category c;
        return boost::system::error_code(static_cast<int>(e), c);
    }

}} // gnunet_channels::error namespace

namespace boost { namespace system {

    template<>
    struct is_error_code_enum<::gnunet_channels::error::error_t>
        : public std::true_type {};


}} // boost::system namespace

#ifndef CL_ARGS_H
#define CL_ARGS_H

/*

This file is part of VROOM.

Copyright (c) 2015-2018, Julien Coupey.
All rights reserved (see LICENSE).

*/

#include <string>
#include <unordered_map>

#include "structures/typedefs.h"

namespace vroom
{
    namespace io
    {
        // Profile name used as key.
        using Servers = std::unordered_map<std::string, Server>;

        struct CLArgs {
            // Listing command-line options.
            Servers     servers;           // -a and -p
            bool        geometry;          // -g
            std::string input_file;        // -i
            std::string output_file;       // -o
            ROUTER      router;            // -r
            std::string input;             // cl arg
            size_t      nb_threads;        // -t
            size_t      exploration_level; // -x

            static const size_t max_exploration_level;

            CLArgs();
        };

        void update_host(Servers& servers, const std::string& value);

        void update_port(Servers& servers, const std::string& value);

    } // namespace io
} // namespace vroom

#endif

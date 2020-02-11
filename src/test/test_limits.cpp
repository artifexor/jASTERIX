/*
 * This file is part of ATSDB.
 *
 * ATSDB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ATSDB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with ATSDB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "jasterix.h"
#include "logger.h"
#include "test_jasterix.h"
#include "files.h"
#include "test_jasterix.h"
#include "system.h"

#if USE_LOG4CPP
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/Layout.hh"
#include "log4cpp/SimpleLayout.hh"
#endif

#if USE_BOOST
#include "boost/date_time/posix_time/posix_time.hpp"
#endif

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include <thread>
#include <chrono>

using namespace std;

std::string definition_path;
std::string filename;

unsigned int sum_num_frames {0};

void test_frame_limit_callback (std::unique_ptr<nlohmann::json> json_data, size_t num_frames, size_t num_records,
                        size_t num_errors)
{
    sum_num_frames += num_frames;

    json_data = nullptr;
}

TEST_CASE( "jASTERIX Frame Limit", "[jASTERIX Limits]" )
{
    loginf << "frame limit test: start" << logendl;

    jASTERIX::jASTERIX jasterix (definition_path, false, false, false);
    jASTERIX::frame_limit = 17333;

    REQUIRE(jASTERIX::Files::fileExists(filename));

    jasterix.decodeFile(filename, "ioss", test_frame_limit_callback);

    REQUIRE (sum_num_frames == 17333);

    loginf << "frame limit test: end" << logendl;
}


int main (int argc, char **argv)
{
    static_assert (sizeof(size_t) >= 8, "code requires size_t with at least 8 bytes");

    // setup logging
#if USE_LOG4CPP
    log4cpp::Appender *console_appender_ = new log4cpp::OstreamAppender("console", &std::cout);
    console_appender_->setLayout(new log4cpp::SimpleLayout());

    log4cpp::Category& root = log4cpp::Category::getRoot();
    root.setPriority(log4cpp::Priority::INFO);
    root.addAppender(console_appender_);
#endif

    Catch::Session session;

    // Build a new parser on top of Catch's
    using namespace Catch::clara;
    auto cli = session.cli() // Get Catch's composite command line parser
            | Opt( definition_path, "definition_path" ) // bind variable to a new option, with a hint string
            ["--definition_path"]    // the option names it will respond to
            ("path for definition files")
            | Opt( filename, "filename" ) // bind variable to a new option, with a hint string
            ["--filename"]    // the option names it will respond to
            ("path for file to decode");        // description string for the help output

    // Now pass the new composite back to Catch so it uses that
    session.cli(cli);

    // Let Catch (using Clara) parse the command line
    int returnCode = session.applyCommandLine (argc, argv);
    if( returnCode != 0 ) // Indicates a command line error
        return returnCode;

    if(definition_path.size())
        loginf << "definition_path: '" << definition_path << "'" << logendl;
    else
    {
        loginf << "definition_path variable missing" << logendl;
        return -1;
    }

    if(filename.size())
        loginf << "filename: '" << filename << "'" << logendl;
    else
    {
        loginf << "filename variable missing" << logendl;
        return -1;
    }

    return session.run();
}

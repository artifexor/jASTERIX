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
#include "jasterix/global.h"
#include "logger.h"
#include "string_conv.h"
#include "tracker_tod_diff/toddifferencecalculator.h"

#include <boost/program_options.hpp>

#include "boost/date_time/posix_time/posix_time.hpp"

namespace po = boost::program_options;

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <tbb/tbb.h>

#if USE_LOG4CPP
#include "log4cpp/Layout.hh"
#include "log4cpp/OstreamAppender.hh"
#include "log4cpp/SimpleLayout.hh"
#endif

using namespace std;

TodDifferenceCalculator global_tod_calc;
bool global_ref {false}; // true if ref, false if tst
bool global_framing {false}; // true if framing, false if nope

void process_callback(std::unique_ptr<nlohmann::json> data_chunk, size_t num_frames,
                    size_t num_records, size_t num_errors)
{
    assert(data_chunk);
    assert(!num_errors);
    global_tod_calc.process(move(data_chunk), global_ref, global_framing);
}

int main(int argc, char** argv)
{
    static_assert(sizeof(size_t) >= 8, "code requires size_t with at least 8 bytes");

    // setup logging

#if USE_LOG4CPP
    log4cpp::Appender* console_appender_ = new log4cpp::OstreamAppender("console", &std::cout);
    console_appender_->setLayout(new log4cpp::SimpleLayout());

    log4cpp::Category& root = log4cpp::Category::getRoot();
    root.setPriority(log4cpp::Priority::INFO);
    root.addAppender(console_appender_);
#endif

    tbb::task_scheduler_init guard(std::thread::hardware_concurrency());

    std::string filename_ref;
    std::string filename_tst;
    std::string framing{""};
    std::string definition_path;
    //std::string only_cats {"62"};
    bool debug{false};
    bool debug_include_framing{false};
    //bool log_performance{true};

    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
                "filename_ref", po::value<std::string>(&filename_ref), "input reference file name")(
                "filename_tst", po::value<std::string>(&filename_tst), "input test file name")(
                "definition_path", po::value<std::string>(&definition_path),
                "path to jASTERIX definition files")(
                "framing", po::value<std::string>(&framing),
                "input framine format, as specified in the framing definitions."
                " raw/netto is default")(
                "frame_limit", po::value<int>(&jASTERIX::frame_limit),
                "number of frames to process with framing, default -1, use -1 to disable.")(
                "frame_chunk_size", po::value<int>(&jASTERIX::frame_chunk_size),
                "number of frames to process in one chunk, default 10000, use -1 to disable.")(
                "data_block_limit", po::value<int>(&jASTERIX::data_block_limit),
                "number of data blocks to process without framing, default -1, use -1 to disable.")(
                "data_block_chunk_size", po::value<int>(&jASTERIX::data_block_chunk_size),
                "number of data blocks to process in one chunk, default 10000, use -1 to disable.")(
                "single_thread", po::bool_switch(&jASTERIX::single_thread),
                "process data in single thread");

    try
    {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            loginf << desc;
            return 1;
        }
    }
    catch (exception& e)
    {
        logerr << "tracker tod diff: unable to parse command line parameters: \n"
               << e.what() << logendl;
        return -1;
    }

    boost::posix_time::ptime total_start_time = boost::posix_time::microsec_clock::local_time();

    std::vector<unsigned int> cat_list {62};

    // check if basic configuration works
    try
    {
        loginf << "tracker tod diff: startup with filename_ref '" << filename_ref << "' filename_tst '" << filename_tst
               << "' framing '" << framing << "' definition_path '" << definition_path << "' debug " << debug
               << logendl;

        { // ref
            loginf << "tracker tod diff: decoding ref data";

            jASTERIX::jASTERIX asterix(definition_path, false, debug, !debug_include_framing);

            if (cat_list.size())
            {
                asterix.decodeNoCategories();

                for (auto cat_it : cat_list)
                    asterix.setDecodeCategory(cat_it, true);
            }

            boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();

            global_ref = true;
            if (framing == "netto" || framing == "")
            {
                global_framing = false;
                asterix.decodeFile(filename_ref, process_callback);
            }
            else
            {
                global_framing = true;
                asterix.decodeFile(filename_ref, framing, process_callback);
            }

            size_t num_frames = asterix.numFrames();
            size_t num_records = asterix.numRecords();

            boost::posix_time::time_duration diff =
                    boost::posix_time::microsec_clock::local_time() - start_time;

            string time_str = to_string(diff.hours()) + "h " + to_string(diff.minutes()) + "m " +
                    to_string(diff.seconds()) + "s " +
                    to_string(diff.total_milliseconds() % 1000) + "ms";

            double seconds = diff.total_milliseconds() / 1000.0;

            loginf << "tracker tod diff: ref decoded " << num_frames << " frames, " << num_records
                   << " records in " << time_str << ": " << num_frames / seconds << " fr/s, "
                   << num_records / seconds << " rec/s" << logendl;
        }

        { // tst
            loginf << "tracker tod diff: decoding tst data";

            jASTERIX::jASTERIX asterix(definition_path, false, debug, !debug_include_framing);

            if (cat_list.size())
            {
                asterix.decodeNoCategories();

                for (auto cat_it : cat_list)
                    asterix.setDecodeCategory(cat_it, true);
            }

            boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::local_time();

            global_ref = false;
            if (framing == "netto" || framing == "")
            {
                asterix.decodeFile(filename_tst, process_callback);
            }
            else
            {
                asterix.decodeFile(filename_tst, framing, process_callback);
            }

            size_t num_frames = asterix.numFrames();
            size_t num_records = asterix.numRecords();

            boost::posix_time::time_duration diff =
                    boost::posix_time::microsec_clock::local_time() - start_time;

            string time_str = to_string(diff.hours()) + "h " + to_string(diff.minutes()) + "m " +
                    to_string(diff.seconds()) + "s " +
                    to_string(diff.total_milliseconds() % 1000) + "ms";

            double seconds = diff.total_milliseconds() / 1000.0;

            loginf << "tracker tod diff: tst decoded " << num_frames << " frames, " << num_records
                   << " records in " << time_str << ": " << num_frames / seconds << " fr/s, "
                   << num_records / seconds << " rec/s" << logendl;
        }

    }
    catch (exception& ex)
    {
        logerr << "tracker tod diff: caught exception: " << ex.what() << logendl;

        // assert (false);

        return -1;
    }
    catch (...)
    {
        logerr << "tracker tod diff: caught exception" << logendl;

        // assert (false);

        return -1;
    }

    loginf << "tracker tod diff: calculating first estimate";

    float time_diff = global_tod_calc.calculate(-1, -1, 0.01); // first estimate, no time diff, pos window

    loginf << "tracker tod diff: got estimate: time diff " << fixed << setprecision(3) << time_diff
           << " (" << timeStringFromDouble(time_diff) << ")";;

//    if (time_diff != -1)
//    {
//        loginf << "tracker tod diff: calculating second estimate";

//        time_diff = global_tod_calc.calculate(2.5, time_diff, -1); // second estimate with time diff & no pos window

//        loginf << "tracker tod diff: second estimate: time diff " << fixed << setprecision(3) << time_diff
//               << " (" << timeStringFromDouble(time_diff) << ")";
//    }
//    else
//        loginf << "tracker tod diff: not able to calculate second estimate";

    boost::posix_time::time_duration diff =
            boost::posix_time::microsec_clock::local_time() - total_start_time;

    string time_str = to_string(diff.hours()) + "h " + to_string(diff.minutes()) + "m " +
            to_string(diff.seconds()) + "s " +
            to_string(diff.total_milliseconds() % 1000) + "ms";

    loginf << "tracker tod diff: done after " << time_str << logendl;

    loginf << "tracker tod diff: shutdown" << logendl;

    return 0;
}

#include "toddifferencecalculator.h"
#include "logger.h"

#include <limits>
#include <algorithm>
#include <iomanip>

using namespace nlohmann;
using namespace std;

TodDifferenceCalculator::TodDifferenceCalculator()
{

}

void TodDifferenceCalculator::process (std::unique_ptr<nlohmann::json> data_chunk, bool ref, bool framing)
{
    assert (data_chunk);

    unsigned int category{0};

    if (!framing)
    {
        assert(data_chunk->contains("data_blocks"));
        assert(data_chunk->at("data_blocks").is_array());

        // std::vector<std::string> keys{"content", "records"};

        for (json& data_block : data_chunk->at("data_blocks"))
        {
            if (!data_block.contains("category"))
            {
                logerr << "TodDifferenceCalculator: process: data block without asterix category";
                continue;
            }

            category = data_block.at("category");

            if (category != 62) // undecoded block
                continue;

            assert(data_block.contains("content"));
            assert(data_block.at("content").contains("records"));
            processRecords(data_block.at("content").at("records"), ref);

            //            if (category == 1)
            //                checkCAT001SacSics(data_block);

            //            loginf << "TodDifferenceCalculator: process: applying JSON function without framing";
            //            JSON::applyFunctionToValues(data_block, keys, keys.begin(), process_lambda, false);
            //            JSON::applyFunctionToValues(data_block, keys, keys.begin(), count_lambda, false);
        }
    }
    else
    {
        assert(data_chunk->contains("frames"));
        assert(data_chunk->at("frames").is_array());

        //std::vector<std::string> keys{"content", "records"};

        for (json& frame : data_chunk->at("frames"))
        {
            if (!frame.contains("content"))  // frame with errors
                continue;

            assert(frame.at("content").is_object());

            if (!frame.at("content").contains("data_blocks"))  // frame with errors
                continue;

            assert(frame.at("content").at("data_blocks").is_array());

            for (json& data_block : frame.at("content").at("data_blocks"))
            {
                if (!data_block.contains("category"))  // data block with errors
                {
                    logerr << "TodDifferenceCalculator: process: data block without asterix "
                              "category";
                    continue;
                }

                category = data_block.at("category");

                if (category != 62) // undecoded block
                    continue;

                assert(data_block.contains("content"));

                assert(data_block.at("content").contains("records"));

                processRecords(data_block.at("content").at("records"), ref);

                //                if (category == 1)
                //                    checkCAT001SacSics(data_block);

                //                JSON::applyFunctionToValues(data_block, keys, keys.begin(), process_lambda, false);
                //                JSON::applyFunctionToValues(data_block, keys, keys.begin(), count_lambda, false);
            }
        }
    }
}

void TodDifferenceCalculator::processRecords (nlohmann::json& records, bool ref)
{
    assert(records.is_array());

    unsigned int ta;

    for (auto& rec_it : records.get<json::array_t>())
    {
        if (rec_it.contains("070") && rec_it.at("070").contains("Time Of Track Information")
                && rec_it.contains("295")  // data ages
                && rec_it.contains("380")  // add
                && rec_it.at("380").contains("ADR") && rec_it.at("380").at("ADR").contains("Target Address") // ta
                && rec_it.contains("105") && rec_it.at("105").contains("Latitude") // pos
                && rec_it.at("105").contains("Longitude"))
        {
            ta = rec_it.at("380").at("ADR").at("Target Address");

            if (ref)
                ref_updates_[ta].emplace_back(rec_it);
            else
                tst_updates_[ta].emplace_back(rec_it);
        }
    }
}

void TodDifferenceCalculator::calculate ()
{
    loginf << "TodDifferenceCalculator: calculate: num ref targets " << ref_updates_.size()
           << " num tst targets " << tst_updates_.size();

    //unsigned int unique_cnt = 0;
    //bool first_all_min_diff = true;
    //float all_min_diff{0}, all_max_diff{0}, all_avg_diff{0};
    vector<float> time_differences;

    for (auto& target_it : ref_updates_)
    {
        if (tst_updates_.count(target_it.first))
        {
//            bool first_target_min_diff = true;
//            float target_min_diff{0}, target_max_diff{0};

            for (auto& ref_tu_it : target_it.second)
            {
                if (!ref_tu_it.hasAllData() || !ref_tu_it.hasAllSameAges())
                    continue;

                TrackUpdate* unique_other = nullptr;

                for (auto& tst_tu_it : tst_updates_.at(target_it.first))
                {
                    if (!tst_tu_it.hasAllData() || !tst_tu_it.hasAllSameAges())
                        continue;

                    if (ref_tu_it.sameData(tst_tu_it)) // && ref_tu_it.getCommonAge() < 4.0
                        //&& tst_tu_it.getCommonAge() < 4.0
                    {
                        if (unique_other)
                        {
                            unique_other = nullptr;
                            break;
                        }
                        else
                            unique_other = &tst_tu_it;

//                        float tr_tod_ref = ref_tu_it.tod() - ref_tu_it.getCommonAge();
//                        float tr_tod_tst = tst_tu_it.tod() - tst_tu_it.getCommonAge();
//                        float diff = tr_tod_ref - tr_tod_tst;

//    //                    loginf << "suitable ref " << ref_tu_it.tod() << " tst " << unique_other->tod() << " diff "
//    //                           << diff << " data '" << unique_other->dataStr() << "'";

//                        time_differences.push_back(diff);
                    }
                }

                if (unique_other)
                {
                    float tr_tod_ref = ref_tu_it.tod() - ref_tu_it.getCommonAge();
                    float tr_tod_tst = unique_other->tod() - unique_other->getCommonAge();
                    float diff = tr_tod_ref - tr_tod_tst;

//                    loginf << "suitable ref " << ref_tu_it.tod() << " tst " << unique_other->tod() << " diff "
//                           << diff << " data '" << unique_other->dataStr() << "'";

                    time_differences.push_back(diff);
                }
            }

//            if (!first_target_min_diff)
//            {
//                if (first_all_min_diff)
//                {
//                    all_min_diff = target_min_diff;
//                    all_max_diff = target_max_diff;
//                    first_all_min_diff = false;
//                }
//                else
//                {
//                    all_min_diff = min(all_min_diff, target_min_diff);
//                    all_max_diff = max(all_max_diff, target_max_diff);

//                }
//            }
        }
    }

    if (time_differences.size())
    {
        float min_diff = *std::min_element(time_differences.begin(), time_differences.end());
        float max_diff = *std::max_element(time_differences.begin(), time_differences.end());

        float diff_avg = 0;

        map<float, unsigned int> counts;

        for (auto diff_it : time_differences)
        {
            diff_avg += diff_it;
            counts[diff_it] += 1;
        }

        diff_avg  /= (float)time_differences.size();

        unsigned int threshold = (float) time_differences.size()*0.005;

        for (auto& cnt_it : counts)
        {
            if (cnt_it.second >= threshold)
            loginf << "diff value " << fixed << setprecision(3) << cnt_it.first << " cnt " << cnt_it.second;
        }

        // median
        float diff_med;
        sort(time_differences.begin(), time_differences.end());
            if (time_differences.size() % 2 == 0)
            {
              diff_med = (time_differences[time_differences.size()/2 - 1]
                      + time_differences[time_differences.size()/2]) / 2;
            }
            else
            {
              diff_med = time_differences[time_differences.size()/2];
            }


        loginf << "TodDifferenceCalculator: calculate: unique_cnt " << time_differences.size()
               << " diff min " << fixed << setprecision(3) << min_diff
               << " avg " << fixed << setprecision(3)<< diff_avg
               << " med " << fixed << setprecision(3) << diff_med
               << " max " << fixed << setprecision(3) << max_diff;
    }
    else
        loginf << "TodDifferenceCalculator: calculate: no suitable data found";
}

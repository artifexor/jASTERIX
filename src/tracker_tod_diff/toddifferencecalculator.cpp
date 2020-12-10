#include "toddifferencecalculator.h"
#include "logger.h"
#include "string_conv.h"

#include <limits>
#include <algorithm>
#include <iomanip>
#include <unordered_set>
#include <iterator>

using namespace nlohmann;
using namespace std;

const float SECS_IN_DAY {24.0*3600.0};

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
        }
    }
    else
    {
        assert(data_chunk->contains("frames"));
        assert(data_chunk->at("frames").is_array());

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

float TodDifferenceCalculator::calculate (float max_t_diff, float estimated_t_diff, float max_pos_diff)
{
    loginf << "TodDifferenceCalculator: calculate: num ref targets " << ref_updates_.size()
           << " num tst targets " << tst_updates_.size() << " max_t_diff " << max_t_diff
           << " estimated_t_diff " << estimated_t_diff << " max_pos_diff " << max_pos_diff;

    TrackUpdate::max_t_diff_ = max_t_diff;
    TrackUpdate::estimated_t_diff_ = estimated_t_diff;
    //TrackUpdate::max_pos_diff_ = max_pos_diff;

    vector<float> time_differences;

    for (auto& target_it : ref_updates_) // ref targets
    {
        if (tst_updates_.count(target_it.first))
        {
            //            bool first_target_min_diff = true;
            //            float target_min_diff{0}, target_max_diff{0};

            //unordered_set<TrackUpdate, TrackUpdate::HashFunction> previous_refs;

            for (auto& ref_tu_it : target_it.second)
            {
                if (!ref_tu_it.hasAllData() || !ref_tu_it.hasAllSameAges())
                    continue;

                // check if ref update unique
                if (count(target_it.second.begin(), target_it.second.end(), ref_tu_it) > 1)
                    continue;

                //                if (previous_refs.count(ref_tu_it)) // check ref for first occurance of data
                //                    continue;
                //                else
                //                    previous_refs.insert(ref_tu_it);

                TrackUpdate* unique_other = nullptr;

                // search for unique tst update
                for (auto& tst_tu_it : tst_updates_.at(target_it.first))
                {
                    if (!tst_tu_it.hasAllData() || !tst_tu_it.hasAllSameAges())
                        continue;

                    if (ref_tu_it.sameData(tst_tu_it))
                        //&& ref_tu_it.getCommonAge() < 5.0 && tst_tu_it.getCommonAge() < 5.0)
                    {
                        if (unique_other)
                        {
                            unique_other = nullptr;
                            break;
                        }
                        else
                            unique_other = &tst_tu_it;
                    }
                }

                if (unique_other)
                {
                    if (max_pos_diff == -1 || ref_tu_it.simliarPosition(*unique_other, max_pos_diff))
                    {

                        float tr_tod_ref = ref_tu_it.tod() - ref_tu_it.getCommonAge();
                        float tr_tod_tst = unique_other->tod() - unique_other->getCommonAge();
                        float diff = tr_tod_ref - tr_tod_tst;

                        if (diff < 0)
                            diff += SECS_IN_DAY;

                        //                    loginf << "suitable ref " << ref_tu_it.tod() << " tst " << unique_other->tod() << " diff "
                        //                           << diff << " data '" << unique_other->dataStr() << "'";

                        time_differences.push_back(diff);
                    }
                }
            }
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
                loginf << "diff value " << fixed << setprecision(3) << cnt_it.first
                       << " (" << timeStringFromDouble(cnt_it.first) << ")"
                       << " cnt " << cnt_it.second;
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
               << " (" << timeStringFromDouble(min_diff) << ")"
               << " max " << fixed << setprecision(3) << max_diff
               << " (" << timeStringFromDouble(max_diff) << ")"
               << " avg " << fixed << setprecision(3)<< diff_avg
               << " (" << timeStringFromDouble(diff_avg) << ")"
               << " med " << fixed << setprecision(3) << diff_med
               << " (" << timeStringFromDouble(diff_med) << ")";

        loginf << "TodDifferenceCalculator: calculate: finding peaks";

        auto peak = counts.end();
        unsigned int peak_cnt = 0;

        for (unsigned int cnt=0; cnt < counts.size(); ++cnt)
        {
            auto current = counts.begin();
            std::advance(current, cnt);

            if (current->second > peak_cnt)
            {
                peak = current;
                peak_cnt = current->second;
            }
        }

        float peaks_sum = 0;
        unsigned int peaks_cnt_sum = 0;

        if (peak != counts.end() && peak->second > time_differences.size()*0.05)
        {
            loginf << "TodDifferenceCalculator: calculate: found first peak "
                   << fixed << setprecision(3) << peak->first
                   << " cnt " << peak->second;

            unsigned int peak1_cnt = peak->second;
            peaks_sum += peak->first * peak->second; // tod * cnt
            peaks_cnt_sum += peak->second;

            auto peak_prev = peak;
            std::advance(peak_prev, -1);

            if (peak_prev != counts.end())
            {
                peaks_sum += peak_prev->first * peak_prev->second; // tod * cnt
                peaks_cnt_sum += peak_prev->second;
            }

            auto peak_next = peak;
            std::advance(peak_next, 1);

            if (peak_next != counts.end())
            {
                peaks_sum += peak_next->first * peak_next->second; // tod * cnt
                peaks_cnt_sum += peak_next->second;
            }

            if (peak_next != counts.end())
                counts.erase(peak_next);

            counts.erase(peak);

            if (peak_prev != counts.end())
                counts.erase(peak_prev);


            for (auto& cnt_it : counts)
            {
                if (cnt_it.second >= threshold)
                    loginf << "after first peak diff value " << fixed << setprecision(3) << cnt_it.first
                           << " (" << timeStringFromDouble(cnt_it.first) << ")"
                           << " cnt " << cnt_it.second;
            }

            // second peak
            peak = counts.end();
            peak_cnt = 0;

            for (unsigned int cnt=0; cnt < counts.size(); ++cnt)
            {
                auto current = counts.begin();
                std::advance(current, cnt);

                if (current->second > peak1_cnt*0.1f && current->second > peak_cnt)
                {
                    peak = current;
                    peak_cnt = current->second;
                }
            }

            if (peak != counts.end())
            {
                loginf << "TodDifferenceCalculator: calculate: found second peak "
                       << fixed << setprecision(3) << peak->first
                       << " cnt " << peak->second;

                peaks_sum += peak->first * peak->second; // tod * cnt
                peaks_cnt_sum += peak->second;

                auto peak_prev = peak;
                std::advance(peak_prev, -1);

                if (peak_prev != counts.end())
                {
                    peaks_sum += peak_prev->first * peak_prev->second; // tod * cnt
                    peaks_cnt_sum += peak_prev->second;
                    counts.erase(peak_prev);
                }

                auto peak_next = peak;
                std::advance(peak_next, 1);

                if (peak_next != counts.end())
                {
                    peaks_sum += peak_next->first * peak_next->second; // tod * cnt
                    peaks_cnt_sum += peak_next->second;
                    counts.erase(peak_next);
                }

                counts.erase(peak);
            }

            if (peaks_sum != 0)
            {
                float new_peak = peaks_sum / (float) peaks_cnt_sum;
                loginf << "TodDifferenceCalculator: calculate: calculated peak-based value "
                       << fixed << setprecision(3) << new_peak;
                return new_peak;
            }
        }

        loginf << "TodDifferenceCalculator: calculate: no peaks found, returning median";
        return diff_med;
    }
    else
        loginf << "TodDifferenceCalculator: calculate: no suitable data found";

    return -1;
}

#ifndef TODDIFFERENCECALCULATOR_H
#define TODDIFFERENCECALCULATOR_H

#include "trackupdate.h"

#include "json.hpp"

class TodDifferenceCalculator
{
public:
    TodDifferenceCalculator();

    void process (std::unique_ptr<nlohmann::json> data_chunk, bool ref, bool framing);
    void processRecords (nlohmann::json& records, bool ref);

    float calculate (float max_t_diff=-1, float estimated_t_diff=-1, float max_pos_diff=-1); // -1 for disabled, other for final calc, returns median

protected:
    std::map<unsigned int, std::vector<TrackUpdate>> ref_updates_;
    std::map<unsigned int, std::vector<TrackUpdate>> tst_updates_;
};

#endif // TODDIFFERENCECALCULATOR_H

#include "trackupdate.h"

#include <sstream>

using namespace std;

float TrackUpdate::max_t_diff_{-1};
float TrackUpdate::estimated_t_diff_{-1};
float TrackUpdate::max_pos_diff_{-1};

TrackUpdate::TrackUpdate(nlohmann::json& record)
{
    tod_ = record.at("070").at("Time Of Track Information");
    latitude_ = record.at("105").at("Latitude");
    longitude_ = record.at("105").at("Longitude");

    nlohmann::json& ages = record.at("295");
    nlohmann::json& adds = record.at("380");

//    {
//         "010": {
//             "SAC": 50,
//             "SIC": 240
//         },
//         "015": {
//             "Service Identification": 225
//         },
//         "040": {
//             "Track Number": 7302
//         },
//         "060": {
//             "CH": 0,
//             "G": 0,
//             "Mode-3/A reply": 1000,
//             "V": 0
//         },
//         "070": {
//             "Time Of Track Information": 36000.0
//         },
//         "080": {
//             "AAC": 0,
//             "ADS": 1,
//             "AFF": 0,
//             "AMA": 0,
//             "CNF": 0,
//             "CST": 0,
//             "FPC": 0,
//             "FX": 1,
//             "FX2": 1,
//             "FX3": 1,
//             "FX4": 0,
//             "KOS": 1,
//             "MD4": 0,
//             "MD5": 0,
//             "MDS": 0,
//             "ME": 0,
//             "MI": 0,
//             "MON": 1,
//             "MRH": 0,
//             "PSR": 1,
//             "SIM": 0,
//             "SPI": 0,
//             "SRC": 6,
//             "SSR": 0,
//             "STP": 0,
//             "SUC": 0,
//             "TSB": 0,
//             "TSE": 0
//         },
//         "100": {
//             "X": -100930.0,
//             "Y": 350137.0
//         },
//         "105": {
//             "Latitude": 50.638995068869995,
//             "Longitude": 12.574236048509999
//         },
//         "135": {
//             "Calculated Track Barometric Altitude": 34000.0,
//             "QNH": 0
//         },
//         "136": {
//             "Measured Flight Level": 340.0
//         },
//         "185": {
//             "Vx": -205.0,
//             "Vy": 101.5
//         },
//         "200": {
//             "ADF": 0,
//             "LONG": 0,
//             "TRANS": 0,
//             "VERT": 0
//         },
//         "210": {
//             "Ax": 0.0,
//             "Ay": 0.0
//         },
//         "220": {
//             "Rate of Climb/Descent": 0.0
//         },
//         "290": {
//             "ES": {
//                 "Age": 63.75
//             },
//             "MDS": {
//                 "Age": 5.5
//             },
//             "MLT": {
//                 "Age": 63.75
//             },
//             "PSR": {
//                 "Age": 63.75
//             },
//             "SSR": {
//                 "Age": 5.5
//             },
//             "available": [
//             ]
//         },
//         "295": {
//             "BVR": {
//                 "Age": 5.5
//             },
//             "FSS": {
//                 "Age": 5.5
//             },
//             "IAR": {
//                 "Age": 5.5
//             },
//             "MAC": {
//                 "Age": 5.5
//             },
//             "MDA": {
//                 "Age": 5.5
//             },
//             "MFL": {
//                 "Age": 5.5
//             },
//             "MHG": {
//                 "Age": 5.5
//             },
//             "available": [
//             ]
//         },
//         "340": {
//             "MDA": {
//                 "G": 0,
//                 "L": 0,
//                 "Mode-3/A reply": 1000,
//                 "V": 0
//             },
//             "MDC": {
//                 "G": 0,
//                 "Last Measured Mode C Code": 340.0,
//                 "V": 0
//             },
//             "POS": {
//                 "RHO": 149.015625,
//                 "THETA": 294.47204576442
//             },
//             "SID": {
//                 "SAC": 49,
//                 "SIC": 144
//             },
//             "TYP": {
//                 "RAB": 0,
//                 "SIM": 0,
//                 "TST": 0,
//                 "TYP": 5
//             },
//             "available": [
//             ]
//         },
//         "380": {
//             "ADR": {
//                 "Target Address": 4341942
//             },
//             "BPS": {
//                 "Barometric Pressure Setting": 213.3
//             },
//             "BVR": {
//                 "Barometric Vertical Rate": -162.5
//             },
//             "FSS": {
//                 "AH": 0,
//                 "AM": 0,
//                 "Altitude": 34000.0,
//                 "MV": 0
//             },
//             "IAR": {
//                 "Indicated Air Speed": 295.0
//             },
//             "ID": {
//                 "Target Identification": "ABW407  "
//             },
//             "MAC": {
//                 "Mach Number": 0.84
//             },
//             "MHG": {
//                 "Magnetic Heading": 286.5234373696
//             },
//             "available": [
//             ]
//         },
//         "500": {
//             "AA": {
//                 "AA (X-Component)": 0.5,
//                 "AA (Y-Component)": 0.75
//             },
//             "ABA": {
//                 "ABA": 1.0
//             },
//             "AGA": {
//                 "AGA": 1593.75
//             },
//             "APC": {
//                 "APC (X-Component)": 50.0,
//                 "APC (Y-Component)": 112.5
//             },
//             "APW": {
//                 "APW (Latitude Component)": 0.00101387349,
//                 "APW (Longitude Component)": 0.0007081021199999999
//             },
//             "ARC": {
//                 "ARC": 150.0
//             },
//             "ATV": {
//                 "ATV (X-Component)": 4.0,
//                 "ATV (Y-Component)": 7.75
//             },
//             "available": [
//             ]
//         },
//         "FSPEC": [
//         ],
//         "index": 709,
//         "length": 119
//     }
// ]
//},
//"length": 510
//}'

    stringstream ss;

    // bvr
    has_bvr_ = ages.contains("BVR") && ages.at("BVR").contains("Age")
            && adds.contains("BVR") && adds.at("BVR").contains("Barometric Vertical Rate");
    if (has_bvr_)
    {
        bvr_ = adds.at("BVR").at("Barometric Vertical Rate");
        bvr_age_ = ages.at("BVR").at("Age");

        ss << "bvr " << bvr_ << " age " << bvr_age_;
    }

    // fss
    has_fss_ = ages.contains("FSS") && ages.at("FSS").contains("Age")
            && adds.contains("FSS") && adds.at("FSS").contains("Altitude");
    if (has_fss_)
    {
        fss_ = adds.at("FSS").at("Altitude");
        fss_age_ = ages.at("FSS").at("Age");

        if (ss.str().size())
            ss << " ";
        ss << "fss " << fss_ << " age " << fss_age_;
    }

    // iar
    has_iar_ = ages.contains("IAR") && ages.at("IAR").contains("Age")
            && adds.contains("IAR") && adds.at("IAR").contains("Indicated Air Speed");
    if (has_iar_)
    {
        iar_ = adds.at("IAR").at("Indicated Air Speed");
        iar_age_ = ages.at("IAR").at("Age");

        if (ss.str().size())
            ss << " ";
        ss << "iar " << iar_ << " age " << iar_age_;
    }

    // mac
    has_mac_ = ages.contains("MAC") && ages.at("MAC").contains("Age")
            && adds.contains("MAC") && adds.at("MAC").contains("Mach Number");
    if (has_mac_)
    {
        mac_ = adds.at("MAC").at("Mach Number");
        mac_age_ = ages.at("MAC").at("Age");

        if (ss.str().size())
            ss << " ";
        ss << "mac " << mac_ << " age " << mac_age_;
    }

    // mda
    has_mda_ = ages.contains("MDA") && ages.at("MDA").contains("Age")
            && record.contains("060") && record.at("060").contains("Mode-3/A reply");
    if (has_mda_)
    {
        mda_ = record.at("060").at("Mode-3/A reply");
        mda_age_ = ages.at("MDA").at("Age");

        if (ss.str().size())
            ss << " ";
        ss << "mda " << mda_ << " age " << mda_age_;
    }

    // mfl
    has_mfl_ = ages.contains("MFL") && ages.at("MFL").contains("Age")
            && record.contains("136") && record.at("136").contains("Measured Flight Level");
    if (has_mfl_)
    {
        mfl_ = record.at("136").at("Measured Flight Level");
        mfl_age_ = ages.at("MFL").at("Age");

        if (ss.str().size())
            ss << " ";
        ss << "mfl " << mfl_ << " age " << mfl_age_;
    }

    // mhg
    has_mhg_ = ages.contains("MHG") && ages.at("MHG").contains("Age")
            && adds.contains("MHG") && adds.at("MHG").contains("Magnetic Heading");
    if (has_mhg_)
    {
        mhg_ = adds.at("MHG").at("Magnetic Heading");
        mhg_age_ = ages.at("MHG").at("Age");

        if (ss.str().size())
            ss << " ";
        ss << "mhg " << mhg_ << " age " << mhg_age_;
    }

    data_str_ = ss.str();
}


bool TrackUpdate::sameData (const TrackUpdate& other) const
{
    if (max_t_diff_ != -1 && fabs(tod_-other.tod_-estimated_t_diff_) > max_t_diff_)
        return false;

    if (max_pos_diff_ != -1 && !simliarPosition(other, max_pos_diff_))
        return false;

//    return hasAllData() && other.hasAllData() && bvr_ == other.bvr_
//            && fss_ == other.fss_ && iar_ == other.iar_ && mac_ == other.mac_ && mda_ == other.mda_
//            && mfl_ == other.mfl_ && mhg_ == other.mhg_;

    return hasAllData() && other.hasAllData() && mhg_ == other.mhg_;
}

bool TrackUpdate::simliarPosition(const TrackUpdate& other, float max_pos_diff) const
{
    return sqrt(pow(latitude_-other.latitude_, 2)+pow(longitude_-other.longitude_, 2)) < max_pos_diff;
}

float TrackUpdate::tod() const
{
    return tod_;
}

std::string TrackUpdate::dataStr() const
{
    return data_str_;
}

bool TrackUpdate::hasAllData () const
{
    //return has_bvr_ && has_fss_ && has_iar_ && has_mac_ && has_mda_ && has_mfl_ && has_mhg_;
    return has_mhg_;
}

bool TrackUpdate::hasAllSameAges () const
{
    if (!hasAllData())
        return false;

//    return bvr_age_ == fss_age_ && bvr_age_ == iar_age_ && bvr_age_ == mac_age_ && bvr_age_ == mda_age_
//            && bvr_age_ == mfl_age_ && bvr_age_ == mhg_age_;

    return bvr_age_ == mhg_age_;
}

float TrackUpdate::getCommonAge() const
{
    assert (hasAllSameAges());
    return bvr_age_;
}

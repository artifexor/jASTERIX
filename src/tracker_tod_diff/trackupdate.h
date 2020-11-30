#ifndef TRACKUPDATE_H
#define TRACKUPDATE_H

#include "json.hpp"

class TrackUpdate
{
public:
    TrackUpdate(nlohmann::json& record);

    bool sameData (const TrackUpdate& other) const;
    bool simliarPosition(const TrackUpdate& other) const;

    float tod() const;

    std::string dataStr() const;


    bool hasAllData () const;
    bool hasAllSameAges () const;
    float getCommonAge() const;

protected:
    float tod_;

    double latitude_ {0};
    double longitude_ {0};

    bool has_bvr_ {false}; // #16: Barometric Vertical Rate
    double bvr_ {0};
    float bvr_age_ {0};

    bool has_fss_ {false}; // #11: Final State Selected Altitude
    double fss_ {0};
    float fss_age_ {0};

    bool has_iar_ {false}; // #29: Indicated Airspeed Data
    double iar_ {0};
    float iar_age_ {0};

    bool has_mac_ {false}; // #30: Mach Number Data
    double mac_ {0};
    float mac_age_ {0};

    bool has_mda_ {false}; // #4: Mode 3/A age
    double mda_ {0};
    float mda_age_ {0};

    bool has_mfl_ {false}; // #1: Measured Flight Level
    double mfl_ {0};
    float mfl_age_ {0};

    bool has_mhg_ {false}; // #7: Magnetic Heading
    double mhg_ {0};
    float mhg_age_ {0};

    std::string data_str_;
};

#endif // TRACKUPDATE_H

/*
 Copyright (C) 2023 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of ORE, a free-software/open-source library
 for transparent pricing and risk analysis - http://opensourcerisk.org

 ORE is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program.
 The license is also available online at <http://opensourcerisk.org>

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/

/*! \file orea/simm/simmconcentrationisdav2_5A.hpp
    \brief SIMM concentration thresholds for SIMM version 2.5A
*/

#pragma once

#include <orea/simm/simmconcentration.hpp>
#include <orea/simm/simmbucketmapper.hpp>

#include <map>
#include <set>
#include <string>

namespace ore {
namespace analytics {

/*! Class giving the SIMM concentration thresholds as outlined in the document
    <em>ISDA SIMM Methodology, version 2.5A .
        Effective Date: December 3, 2022.</em>
*/
class SimmConcentration_ISDA_V2_5A : public SimmConcentrationBase {
public:
    //! Default constructor that adds fixed known mappings
    SimmConcentration_ISDA_V2_5A(const boost::shared_ptr<SimmBucketMapper>& simmBucketMapper);

    /*! Return the SIMM <em>concentration threshold</em> for a given SIMM
        <em>RiskType</em> and SIMM <em>Qualifier</em>.

        \warning If the risk type is not covered <code>QL_MAX_REAL</code> is
                 returned i.e. no concentration threshold
     */
    QuantLib::Real threshold(const CrifRecord::RiskType& riskType, const std::string& qualifier) const override;

private:
    //! Help getting SIMM buckets from SIMM qualifiers
    boost::shared_ptr<SimmBucketMapper> simmBucketMapper_;
};

} // namespace analytics
} // namespace ore

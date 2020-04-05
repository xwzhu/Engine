/*
 Copyright (C) 2016 Quaternion Risk Management Ltd
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

/*! \file ored/configuration/cdsvolcurveconfig.hpp
    \brief CDS and index CDS volatility configuration
    \ingroup configuration
*/

#pragma once

#include <ored/configuration/curveconfig.hpp>
#include <ored/configuration/volatilityconfig.hpp>
#include <boost/shared_ptr.hpp>

namespace ore {
namespace data {

/*! CDS and index CDS volatility configuration
    \ingroup configuration
*/
class CDSVolatilityCurveConfig : public CurveConfig {
public:
    //! Default constructor
    CDSVolatilityCurveConfig();
    
    //! Detailed constructor
    CDSVolatilityCurveConfig(
        const std::string& curveId,
        const std::string& curveDescription,
        const boost::shared_ptr<VolatilityConfig>& volatilityConfig,
        const std::string& dayCounter = "A365",
        const std::string& calendar = "NullCalendar",
        const std::string& strikeType = "",
        const std::string& quoteName = "");

    //! \name Inspectors
    //@{
    const boost::shared_ptr<VolatilityConfig>& volatilityConfig() const;
    const std::string& dayCounter() const;
    const std::string& calendar() const;
    const std::string& strikeType() const;
    const std::string& quoteName() const;
    //@}

    //! \name Serialisation
    //@{
    void fromXML(XMLNode* node) override;
    ore::data::XMLNode* toXML(ore::data::XMLDocument& doc) override;
    //@}

private:
    boost::shared_ptr<VolatilityConfig> volatilityConfig_;
    std::string dayCounter_;
    std::string calendar_;
    std::string strikeType_;
    std::string quoteName_;

    //! Populate CurveConfig::quotes_ with the required quotes.
    void populateQuotes();
};

}
}

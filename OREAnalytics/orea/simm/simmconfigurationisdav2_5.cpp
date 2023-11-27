/*
 Copyright (C) 2022 Quaternion Risk Management Ltd.
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

#include <orea/simm/simmconcentrationisdav2_5.hpp>
#include <orea/simm/simmconfigurationisdav2_5.hpp>
#include <ql/math/matrix.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/make_shared.hpp>

using QuantLib::InterestRateIndex;
using QuantLib::Matrix;
using QuantLib::Real;
using std::string;
using std::vector;

namespace ore {
namespace analytics {

QuantLib::Size SimmConfiguration_ISDA_V2_5::group(const string& qualifier, const std::map<QuantLib::Size,
                                                  std::set<string>>& categories) const {
    QuantLib::Size result = 0;
    for (const auto& kv : categories) {
        if (kv.second.empty()) {
            result = kv.first;
        } else {
            if (kv.second.count(qualifier) > 0) {
                // Found qualifier in category so return it
                return kv.first;
            }
        }
    }

    // If we get here, result should hold category with empty set
    return result;
}

QuantLib::Real SimmConfiguration_ISDA_V2_5::weight(const CrifRecord::RiskType& rt, boost::optional<string> qualifier,
                                                     boost::optional<std::string> label_1,
                                                     const std::string& calculationCurrency) const {

    if (rt == CrifRecord::RiskType::FX) {
        QL_REQUIRE(calculationCurrency != "", "no calculation currency provided weight");
        QL_REQUIRE(qualifier, "need a qualifier to return a risk weight for the risk type FX");

        QuantLib::Size g1 = group(calculationCurrency, ccyGroups_);
        QuantLib::Size g2 = group(*qualifier, ccyGroups_);
        return rwFX_[g1][g2];
    }

    return SimmConfigurationBase::weight(rt, qualifier, label_1);
}

QuantLib::Real SimmConfiguration_ISDA_V2_5::correlation(const CrifRecord::RiskType& firstRt, const string& firstQualifier,
                                                        const string& firstLabel_1, const string& firstLabel_2,
                                                        const CrifRecord::RiskType& secondRt, const string& secondQualifier,
                                                        const string& secondLabel_1, const string& secondLabel_2,
                                                        const std::string& calculationCurrency) const {

    if (firstRt == CrifRecord::RiskType::FX && secondRt == CrifRecord::RiskType::FX) {
        QL_REQUIRE(calculationCurrency != "", "no calculation currency provided corr");
        QuantLib::Size g = group(calculationCurrency, ccyGroups_);
        QuantLib::Size g1 = group(firstQualifier, ccyGroups_);
        QuantLib::Size g2 = group(secondQualifier, ccyGroups_);
        if (g == 0) {
            return fxRegVolCorrelation_[g1][g2];
        } else if (g == 1) {
            return fxHighVolCorrelation_[g1][g2];
        } else {
            QL_FAIL("FX Volatility group " << g << " not recognized");
        }
    }

    return SimmConfigurationBase::correlation(firstRt, firstQualifier, firstLabel_1, firstLabel_2, secondRt,
                                              secondQualifier, secondLabel_1, secondLabel_2);
}

SimmConfiguration_ISDA_V2_5::SimmConfiguration_ISDA_V2_5(const boost::shared_ptr<SimmBucketMapper>& simmBucketMapper,
                                                         const QuantLib::Size& mporDays, const std::string& name,
                                                         const std::string version)
     : SimmConfigurationBase(simmBucketMapper, name, version, mporDays) {

    // The differences in methodology for 1 Day horizon is described in
    // Standard Initial Margin Model: Technical Paper, ISDA SIMM Governance Forum, Version 10:
    // Section I - Calibration with one-day horizon
    QL_REQUIRE(mporDays_ == 10 || mporDays_ == 1, "SIMM only supports MPOR 10-day or 1-day");

    // Set up the correct concentration threshold getter
    if (mporDays == 10) {
        simmConcentration_ = boost::make_shared<SimmConcentration_ISDA_V2_5>(simmBucketMapper_);
    } else {
        // SIMM:Technical Paper, Section I.4: "The Concentration Risk feature is disabled"
        simmConcentration_ = boost::make_shared<SimmConcentrationBase>();
    }

    // clang-format off

    // Set up the members for this configuration
    // Explanations of all these members are given in the hpp file

    mapBuckets_ = {
        { CrifRecord::RiskType::IRCurve, { "1", "2", "3" } },
        { CrifRecord::RiskType::CreditQ, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "Residual" } },
        { CrifRecord::RiskType::CreditVol, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "Residual" } },
        { CrifRecord::RiskType::CreditNonQ, { "1", "2", "Residual" } },
        { CrifRecord::RiskType::CreditVolNonQ, { "1", "2", "Residual" } },
        { CrifRecord::RiskType::Equity, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "Residual" } },
        { CrifRecord::RiskType::EquityVol, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "Residual" } },
        { CrifRecord::RiskType::Commodity, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17" } },
        { CrifRecord::RiskType::CommodityVol, { "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17" } }
    };

    mapLabels_1_ = {
        { CrifRecord::RiskType::IRCurve, { "2w", "1m", "3m", "6m", "1y", "2y", "3y", "5y", "10y", "15y", "20y", "30y" } },
        { CrifRecord::RiskType::CreditQ, { "1y", "2y", "3y", "5y", "10y" } },
        { CrifRecord::RiskType::CreditNonQ, { "1y", "2y", "3y", "5y", "10y" } },
        { CrifRecord::RiskType::IRVol, { "2w", "1m", "3m", "6m", "1y", "2y", "3y", "5y", "10y", "15y", "20y", "30y" } },
        { CrifRecord::RiskType::InflationVol, { "2w", "1m", "3m", "6m", "1y", "2y", "3y", "5y", "10y", "15y", "20y", "30y" } },
        { CrifRecord::RiskType::CreditVol, { "1y", "2y", "3y", "5y", "10y" } },
        { CrifRecord::RiskType::CreditVolNonQ, { "1y", "2y", "3y", "5y", "10y" } },
        { CrifRecord::RiskType::EquityVol, { "2w", "1m", "3m", "6m", "1y", "2y", "3y", "5y", "10y", "15y", "20y", "30y" } },
        { CrifRecord::RiskType::CommodityVol, { "2w", "1m", "3m", "6m", "1y", "2y", "3y", "5y", "10y", "15y", "20y", "30y" } },
        { CrifRecord::RiskType::FXVol, { "2w", "1m", "3m", "6m", "1y", "2y", "3y", "5y", "10y", "15y", "20y", "30y" } }
    };

    mapLabels_2_ = {
        { CrifRecord::RiskType::IRCurve, { "OIS", "Libor1m", "Libor3m", "Libor6m", "Libor12m", "Prime", "Municipal" } },
        { CrifRecord::RiskType::CreditQ, { "", "Sec" } }
    };

    // Populate CCY groups that are used for FX correlations and risk weights
    // The groups consists of High Vol Currencies & regular vol currencies
    ccyGroups_ = {
        { 1, { "BRL", "RUB", "TRY", "ZAR" } },
        { 0, {  } }
    };

    vector<Real> temp;

    if (mporDays_ == 10) {
        // Risk weights
        temp = {
           7.4,  13.6,
           13.6, 14.6
        };
        rwFX_ = Matrix(2, 2, temp.begin(), temp.end());

        rwRiskType_ = {
            { CrifRecord::RiskType::Inflation, 63 },
            { CrifRecord::RiskType::XCcyBasis, 21 },
            { CrifRecord::RiskType::IRVol, 0.18 },
            { CrifRecord::RiskType::InflationVol, 0.18 },
            { CrifRecord::RiskType::CreditVol, 0.74 },
            { CrifRecord::RiskType::CreditVolNonQ, 0.74 },
            { CrifRecord::RiskType::CommodityVol, 0.6 },
            { CrifRecord::RiskType::FXVol, 0.47 },
            { CrifRecord::RiskType::BaseCorr, 10 }
        };

        rwBucket_ = {
            { CrifRecord::RiskType::CreditQ, { 75, 91, 78, 55, 67, 47, 187, 665, 262, 251, 172, 247, 665 } },
            { CrifRecord::RiskType::CreditNonQ, { 280, 1300, 1300 } },
            { CrifRecord::RiskType::Equity, { 26, 28, 34, 28, 23, 25, 29, 27, 32, 32, 18, 18, 34 } },
            { CrifRecord::RiskType::Commodity, { 27, 29, 33, 25, 35, 24, 40, 53, 44, 58, 20, 21, 13, 16, 13, 58, 17 } },
            { CrifRecord::RiskType::EquityVol, { 0.45, 0.45, 0.45, 0.45, 0.45, 0.45, 0.45, 0.45, 0.45, 0.45, 0.45, 0.96, 0.45 } }, 
        };

        rwLabel_1_ = {
            { { CrifRecord::RiskType::IRCurve, "1" }, { 115, 112, 96, 74, 66, 61, 56, 52, 53, 57, 60, 66 } },
            { { CrifRecord::RiskType::IRCurve, "2" }, { 15, 18, 9.0, 11, 13, 15, 18, 20, 19, 19, 20, 23 } },
            { { CrifRecord::RiskType::IRCurve, "3" }, { 119, 93, 80, 82, 90, 92, 95, 95, 94, 108, 105, 101 } },
        };

        // Historical volatility ratios
        historicalVolatilityRatios_[CrifRecord::RiskType::EquityVol] = 0.58;
        historicalVolatilityRatios_[CrifRecord::RiskType::CommodityVol] = 0.69;
        historicalVolatilityRatios_[CrifRecord::RiskType::FXVol] = 0.52;
        hvr_ir_ = 0.44;

        // Curvature weights 
        curvatureWeights_ = {
            { CrifRecord::RiskType::IRVol, { 0.5,
                                 0.5 * 14.0 / (365.0 / 12.0),
                                 0.5 * 14.0 / (3.0 * 365.0 / 12.0),
                                 0.5 * 14.0 / (6.0 * 365.0 / 12.0),
                                 0.5 * 14.0 / 365.0,
                                 0.5 * 14.0 / (2.0 * 365.0),
                                 0.5 * 14.0 / (3.0 * 365.0),
                                 0.5 * 14.0 / (5.0 * 365.0),
                                 0.5 * 14.0 / (10.0 * 365.0),
                                 0.5 * 14.0 / (15.0 * 365.0),
                                 0.5 * 14.0 / (20.0 * 365.0),
                                 0.5 * 14.0 / (30.0 * 365.0) }
            },
            { CrifRecord::RiskType::CreditVol, { 0.5 * 14.0 / 365.0,
                                     0.5 * 14.0 / (2.0 * 365.0),
                                     0.5 * 14.0 / (3.0 * 365.0),
                                     0.5 * 14.0 / (5.0 * 365.0),
                                     0.5 * 14.0 / (10.0 * 365.0) }
            }
        };
        curvatureWeights_[CrifRecord::RiskType::InflationVol] = curvatureWeights_[CrifRecord::RiskType::IRVol];
        curvatureWeights_[CrifRecord::RiskType::EquityVol] = curvatureWeights_[CrifRecord::RiskType::IRVol];
        curvatureWeights_[CrifRecord::RiskType::CommodityVol] = curvatureWeights_[CrifRecord::RiskType::IRVol];
        curvatureWeights_[CrifRecord::RiskType::FXVol] = curvatureWeights_[CrifRecord::RiskType::IRVol];
        curvatureWeights_[CrifRecord::RiskType::CreditVolNonQ] = curvatureWeights_[CrifRecord::RiskType::CreditVol];

    } else {
       // SIMM:Technical Paper, Section I.1: "All delta and vega risk weights should be replaced with the values for
       // one-day calibration given in the Calibration Results document."

        // Risk weights
        temp = {
            1.8,  3.2, 
           3.2, 3.4
        };
        rwFX_ = Matrix(2, 2, temp.begin(), temp.end());

        rwRiskType_ = {
            { CrifRecord::RiskType::Inflation, 15 },
            { CrifRecord::RiskType::XCcyBasis, 5.9 },
            { CrifRecord::RiskType::IRVol, 0.047 },
            { CrifRecord::RiskType::InflationVol, 0.047 },
            { CrifRecord::RiskType::CreditVol, 0.085 },
            { CrifRecord::RiskType::CreditVolNonQ, 0.085 },
            { CrifRecord::RiskType::CommodityVol, 0.16 },
            { CrifRecord::RiskType::FXVol, 0.096 },
            { CrifRecord::RiskType::BaseCorr, 2.5 }
        };

        rwBucket_ = {
            { CrifRecord::RiskType::CreditQ, { 21, 27, 16, 12, 14, 12, 48, 144, 51, 53, 38, 57, 144 } },
            { CrifRecord::RiskType::CreditNonQ, { 66, 250, 250 } },
            { CrifRecord::RiskType::Equity, { 9.3, 9.7, 10.0, 9.2, 7.7, 8.5, 9.5, 9.6, 10.0, 10, 5.9, 5.9, 10.0 } },
            { CrifRecord::RiskType::Commodity, { 9.0, 9.1, 8.1, 7.2, 10, 8.2, 9.7, 10, 10, 16, 6.2, 6.5, 4.6, 4.6, 4.0, 16, 5.1 } },
            { CrifRecord::RiskType::EquityVol, { 0.093, 0.093, 0.093, 0.093, 0.093, 0.093, 0.093, 0.093, 0.093, 0.093, 0.093, 0.25, 0.093 } }, 
        };

        rwLabel_1_ = {
            { { CrifRecord::RiskType::IRCurve, "1" }, { 19, 16, 12, 12, 13, 16, 16, 16, 16, 17, 16, 17 } },
            { { CrifRecord::RiskType::IRCurve, "2" }, { 1.7, 3.4, 1.8, 2.0, 3.3, 4.8, 5.8, 6.8, 6.5, 7.0, 7.5, 8.3 } },
            { { CrifRecord::RiskType::IRCurve, "3" }, { 49, 24, 16, 20, 23, 23, 33, 31, 34, 33, 33, 27 } },
        };

        // Historical volatility ratios
        historicalVolatilityRatios_[CrifRecord::RiskType::EquityVol] = 0.54;
        historicalVolatilityRatios_[CrifRecord::RiskType::CommodityVol] = 0.69;
        historicalVolatilityRatios_[CrifRecord::RiskType::FXVol] = 0.7;
        hvr_ir_ = 0.51;

        // Curvature weights
        //SIMM:Technical Paper, Section I.3, this 10-day formula for curvature weights is modified
        curvatureWeights_ = {
            { CrifRecord::RiskType::IRVol, { 0.5 / 10.0,
                                 0.5 * 1.40 / (365.0 / 12.0),
                                 0.5 * 1.40 / (3.0 * 365.0 / 12.0),
                                 0.5 * 1.40 / (6.0 * 365.0 / 12.0),
                                 0.5 * 1.40 / 365.0,
                                 0.5 * 1.40 / (2.0 * 365.0),
                                 0.5 * 1.40 / (3.0 * 365.0),
                                 0.5 * 1.40 / (5.0 * 365.0),
                                 0.5 * 1.40 / (10.0 * 365.0),
                                 0.5 * 1.40 / (15.0 * 365.0),
                                 0.5 * 1.40 / (20.0 * 365.0),
                                 0.5 * 1.40 / (30.0 * 365.0) }
            },
            { CrifRecord::RiskType::CreditVol, { 0.5 * 1.40 / 365.0,
                                     0.5 * 1.40 / (2.0 * 365.0),
                                     0.5 * 1.40 / (3.0 * 365.0),
                                     0.5 * 1.40 / (5.0 * 365.0),
                                     0.5 * 1.40 / (10.0 * 365.0) }
            }
        };
        curvatureWeights_[CrifRecord::RiskType::InflationVol] = curvatureWeights_[CrifRecord::RiskType::IRVol];
        curvatureWeights_[CrifRecord::RiskType::EquityVol] = curvatureWeights_[CrifRecord::RiskType::IRVol];
        curvatureWeights_[CrifRecord::RiskType::CommodityVol] = curvatureWeights_[CrifRecord::RiskType::IRVol];
        curvatureWeights_[CrifRecord::RiskType::FXVol] = curvatureWeights_[CrifRecord::RiskType::IRVol];
        curvatureWeights_[CrifRecord::RiskType::CreditVolNonQ] = curvatureWeights_[CrifRecord::RiskType::CreditVol];
    }


    // Valid risk types
    validRiskTypes_ = {
        CrifRecord::RiskType::Commodity,
        CrifRecord::RiskType::CommodityVol,
        CrifRecord::RiskType::CreditNonQ,
        CrifRecord::RiskType::CreditQ,
        CrifRecord::RiskType::CreditVol,
        CrifRecord::RiskType::CreditVolNonQ,
        CrifRecord::RiskType::Equity,
        CrifRecord::RiskType::EquityVol,
        CrifRecord::RiskType::FX,
        CrifRecord::RiskType::FXVol,
        CrifRecord::RiskType::Inflation,
        CrifRecord::RiskType::IRCurve,
        CrifRecord::RiskType::IRVol,
        CrifRecord::RiskType::InflationVol,
        CrifRecord::RiskType::BaseCorr,
        CrifRecord::RiskType::XCcyBasis,
        CrifRecord::RiskType::ProductClassMultiplier,
        CrifRecord::RiskType::AddOnNotionalFactor,
        CrifRecord::RiskType::PV,
        CrifRecord::RiskType::Notional,
        CrifRecord::RiskType::AddOnFixedAmount
    };

    // Risk class correlation matrix
    temp = {
        1.00, 0.29, 0.13, 0.28, 0.46, 0.32,
        0.29, 1.00, 0.54, 0.71, 0.52, 0.38,
        0.13, 0.54, 1.00, 0.46, 0.41, 0.12,
        0.28, 0.71, 0.46, 1.00, 0.49, 0.35,
        0.46, 0.52, 0.41, 0.49, 1.00, 0.41,
        0.32, 0.38, 0.12, 0.35, 0.41, 1.00
    };
    riskClassCorrelation_ = Matrix(6, 6, temp.begin(), temp.end());

    // FX correlations
    temp = {
        0.5, 0.27,
        0.27, 0.42
    };
    fxRegVolCorrelation_ = Matrix(2, 2, temp.begin(), temp.end());

    temp = {
        0.85, 0.54,
        0.54, 0.5
    };
    fxHighVolCorrelation_ = Matrix(2, 2, temp.begin(), temp.end());

    // Interest rate tenor correlations (i.e. Label1 level correlations)
    temp = {
        1.0, 0.74, 0.63, 0.55, 0.45, 0.36, 0.32, 0.28, 0.23, 0.2, 0.18, 0.16,
        0.74, 1.0, 0.8, 0.69, 0.52, 0.41, 0.35, 0.29, 0.24, 0.18, 0.17, 0.16,
        0.63, 0.8, 1.0, 0.85, 0.67, 0.53, 0.45, 0.39, 0.32, 0.24, 0.22, 0.22,
        0.55, 0.69, 0.85, 1.0, 0.83, 0.71, 0.62, 0.54, 0.45, 0.36, 0.35, 0.33,
        0.45, 0.52, 0.67, 0.83, 1.0, 0.94, 0.86, 0.78, 0.65, 0.58, 0.55, 0.53,
        0.36, 0.41, 0.53, 0.71, 0.94, 1.0, 0.95, 0.89, 0.78, 0.72, 0.68, 0.67,
        0.32, 0.35, 0.45, 0.62, 0.86, 0.95, 1.0, 0.96, 0.87, 0.8, 0.77, 0.74,
        0.28, 0.29, 0.39, 0.54, 0.78, 0.89, 0.96, 1.0, 0.94, 0.89, 0.86, 0.84,
        0.23, 0.24, 0.32, 0.45, 0.65, 0.78, 0.87, 0.94, 1.0, 0.97, 0.95, 0.94,
        0.2, 0.18, 0.24, 0.36, 0.58, 0.72, 0.8, 0.89, 0.97, 1.0, 0.98, 0.98,
        0.18, 0.17, 0.22, 0.35, 0.55, 0.68, 0.77, 0.86, 0.95, 0.98, 1.0, 0.99,
        0.16, 0.16, 0.22, 0.33, 0.53, 0.67, 0.74, 0.84, 0.94, 0.98, 0.99, 1.0
    };
    irTenorCorrelation_ = Matrix(12, 12, temp.begin(), temp.end());

    // CreditQ inter-bucket correlations 
    temp = {
        1.0, 0.36, 0.38, 0.35, 0.37, 0.33, 0.36, 0.31, 0.32, 0.33, 0.32, 0.3,
        0.36, 1.0, 0.46, 0.44, 0.45, 0.43, 0.33, 0.36, 0.38, 0.39, 0.4, 0.36,
        0.38, 0.46, 1.0, 0.49, 0.49, 0.47, 0.34, 0.36, 0.41, 0.42, 0.43, 0.39,
        0.35, 0.44, 0.49, 1.0, 0.48, 0.48, 0.31, 0.34, 0.38, 0.42, 0.41, 0.37,
        0.37, 0.45, 0.49, 0.48, 1.0, 0.48, 0.33, 0.35, 0.39, 0.42, 0.43, 0.38,
        0.33, 0.43, 0.47, 0.48, 0.48, 1.0, 0.29, 0.32, 0.36, 0.39, 0.4, 0.35,
        0.36, 0.33, 0.34, 0.31, 0.33, 0.29, 1.0, 0.28, 0.32, 0.31, 0.3, 0.28,
        0.31, 0.36, 0.36, 0.34, 0.35, 0.32, 0.28, 1.0, 0.33, 0.34, 0.33, 0.3,
        0.32, 0.38, 0.41, 0.38, 0.39, 0.36, 0.32, 0.33, 1.0, 0.38, 0.36, 0.34,
        0.33, 0.39, 0.42, 0.42, 0.42, 0.39, 0.31, 0.34, 0.38, 1.0, 0.38, 0.36,
        0.32, 0.4, 0.43, 0.41, 0.43, 0.4, 0.3, 0.33, 0.36, 0.38, 1.0, 0.35,
        0.3, 0.36, 0.39, 0.37, 0.38, 0.35, 0.28, 0.3, 0.34, 0.36, 0.35, 1.0
    };
    interBucketCorrelation_[CrifRecord::RiskType::CreditQ] = Matrix(12, 12, temp.begin(), temp.end());

    // Equity inter-bucket correlations
    temp = {
        1.0, 0.2, 0.2, 0.2, 0.13, 0.16, 0.16, 0.16, 0.17, 0.12, 0.18, 0.18,
        0.2, 1.0, 0.25, 0.23, 0.14, 0.17, 0.18, 0.17, 0.19, 0.13, 0.19, 0.19,
        0.2, 0.25, 1.0, 0.24, 0.13, 0.17, 0.18, 0.16, 0.2, 0.13, 0.18, 0.18,
        0.2, 0.23, 0.24, 1.0, 0.17, 0.22, 0.22, 0.22, 0.21, 0.16, 0.24, 0.24,
        0.13, 0.14, 0.13, 0.17, 1.0, 0.27, 0.26, 0.27, 0.15, 0.2, 0.3, 0.3,
        0.16, 0.17, 0.17, 0.22, 0.27, 1.0, 0.34, 0.33, 0.18, 0.24, 0.38, 0.38,
        0.16, 0.18, 0.18, 0.22, 0.26, 0.34, 1.0, 0.32, 0.18, 0.24, 0.37, 0.37,
        0.16, 0.17, 0.16, 0.22, 0.27, 0.33, 0.32, 1.0, 0.18, 0.23, 0.37, 0.37,
        0.17, 0.19, 0.2, 0.21, 0.15, 0.18, 0.18, 0.18, 1.0, 0.14, 0.2, 0.2,
        0.12, 0.13, 0.13, 0.16, 0.2, 0.24, 0.24, 0.23, 0.14, 1.0, 0.25, 0.25,
        0.18, 0.19, 0.18, 0.24, 0.3, 0.38, 0.37, 0.37, 0.2, 0.25, 1.0, 0.45,
        0.18, 0.19, 0.18, 0.24, 0.3, 0.38, 0.37, 0.37, 0.2, 0.25, 0.45, 1.0
    };
    interBucketCorrelation_[CrifRecord::RiskType::Equity] = Matrix(12, 12, temp.begin(), temp.end());

    // Commodity inter-bucket correlations
    temp = {
        1.0, 0.33, 0.21, 0.27, 0.29, 0.21, 0.48, 0.16, 0.41, 0.23, 0.18, 0.02, 0.21, 0.19, 0.15, 0.0, 0.24,
        0.33, 1.0, 0.94, 0.94, 0.89, 0.21, 0.19, 0.13, 0.21, 0.21, 0.41, 0.27, 0.31, 0.29, 0.21, 0.0, 0.6,
        0.21, 0.94, 1.0, 0.91, 0.85, 0.12, 0.2, 0.09, 0.19, 0.2, 0.36, 0.18, 0.22, 0.23, 0.23, 0.0, 0.54,
        0.27, 0.94, 0.91, 1.0, 0.84, 0.14, 0.24, 0.13, 0.21, 0.19, 0.39, 0.25, 0.23, 0.27, 0.18, 0.0, 0.59,
        0.29, 0.89, 0.85, 0.84, 1.0, 0.15, 0.17, 0.09, 0.16, 0.21, 0.38, 0.28, 0.28, 0.27, 0.18, 0.0, 0.55,
        0.21, 0.21, 0.12, 0.14, 0.15, 1.0, 0.33, 0.53, 0.26, 0.09, 0.21, 0.04, 0.11, 0.1, 0.09, 0.0, 0.24,
        0.48, 0.19, 0.2, 0.24, 0.17, 0.33, 1.0, 0.31, 0.72, 0.24, 0.14, -0.12, 0.19, 0.14, 0.08, 0.0, 0.24,
        0.16, 0.13, 0.09, 0.13, 0.09, 0.53, 0.31, 1.0, 0.24, 0.04, 0.13, -0.07, 0.04, 0.06, 0.01, 0.0, 0.16,
        0.41, 0.21, 0.19, 0.21, 0.16, 0.26, 0.72, 0.24, 1.0, 0.21, 0.18, -0.07, 0.12, 0.12, 0.1, 0.0, 0.21,
        0.23, 0.21, 0.2, 0.19, 0.21, 0.09, 0.24, 0.04, 0.21, 1.0, 0.14, 0.11, 0.11, 0.1, 0.07, 0.0, 0.14,
        0.18, 0.41, 0.36, 0.39, 0.38, 0.21, 0.14, 0.13, 0.18, 0.14, 1.0, 0.28, 0.3, 0.25, 0.18, 0.0, 0.38,
        0.02, 0.27, 0.18, 0.25, 0.28, 0.04, -0.12, -0.07, -0.07, 0.11, 0.28, 1.0, 0.18, 0.18, 0.08, 0.0, 0.21,
        0.21, 0.31, 0.22, 0.23, 0.28, 0.11, 0.19, 0.04, 0.12, 0.11, 0.3, 0.18, 1.0, 0.34, 0.16, 0.0, 0.34,
        0.19, 0.29, 0.23, 0.27, 0.27, 0.1, 0.14, 0.06, 0.12, 0.1, 0.25, 0.18, 0.34, 1.0, 0.13, 0.0, 0.26,
        0.15, 0.21, 0.23, 0.18, 0.18, 0.09, 0.08, 0.01, 0.1, 0.07, 0.18, 0.08, 0.16, 0.13, 1.0, 0.0, 0.21,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
        0.24, 0.6, 0.54, 0.59, 0.55, 0.24, 0.24, 0.16, 0.21, 0.14, 0.38, 0.21, 0.34, 0.26, 0.21, 0.0, 1.0
    };
    interBucketCorrelation_[CrifRecord::RiskType::Commodity] = Matrix(17, 17, temp.begin(), temp.end());

    // Equity intra-bucket correlations (exclude Residual and deal with it in the method - it is 0%) - changed
   intraBucketCorrelation_[CrifRecord::RiskType::Equity] = { 0.18, 0.23, 0.3, 0.26, 0.23, 0.35, 0.36, 0.33, 0.19, 0.2, 0.45, 0.45 };

    // Commodity intra-bucket correlations
   intraBucketCorrelation_[CrifRecord::RiskType::Commodity] = { 0.84, 0.98, 0.96, 0.97, 0.98, 0.88, 0.98, 0.49, 0.8, 0.46, 0.55, 0.46, 0.66, 0.18, 0.21, 0, 0.36 };

    // Initialise the single, ad-hoc type, correlations
    xccyCorr_ = 0.01;
    infCorr_ = 0.37;
    infVolCorr_ = 0.37;
    irSubCurveCorr_ = 0.99;
    irInterCurrencyCorr_ = 0.24;
    crqResidualIntraCorr_ = 0.5;
    crqSameIntraCorr_ = 0.93;
    crqDiffIntraCorr_ = 0.42;
    crnqResidualIntraCorr_ = 0.5;
    crnqSameIntraCorr_ = 0.82;
    crnqDiffIntraCorr_ = 0.27;
    crnqInterCorr_ = 0.4;
    fxCorr_ = 0.5;
    basecorrCorr_ = 0.24;

    // clang-format on
}

/* The CurvatureMargin must be multiplied by a scale factor of HVR(IR)^{-2}, where HVR(IR)
is the historical volatility ratio for the interest-rate risk class (see page 8 section 11(d)
of the ISDA-SIMM-v2.5 documentation).
*/
QuantLib::Real SimmConfiguration_ISDA_V2_5::curvatureMarginScaling() const { return pow(hvr_ir_, -2.0); }

void SimmConfiguration_ISDA_V2_5::addLabels2(const CrifRecord::RiskType& rt, const string& label_2) {
    // Call the shared implementation
    SimmConfigurationBase::addLabels2Impl(rt, label_2);
}

string SimmConfiguration_ISDA_V2_5::labels2(const boost::shared_ptr<InterestRateIndex>& irIndex) const {
    // Special for BMA
    if (boost::algorithm::starts_with(irIndex->name(), "BMA")) {
        return "Municipal";
    }

    // Otherwise pass off to base class
    return SimmConfigurationBase::labels2(irIndex);
}

} // namespace analytics
} // namespace ore

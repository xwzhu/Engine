/*
 Copyright (C) 2019 Quaternion Risk Management Ltd
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

/*! \file models/lgmvectorised.hpp
    \brief vectorised lgm model calculations
    \ingroup models
*/

#pragma once


#include <qle/math/randomvariable.hpp>
#include <qle/models/irlgm1fparametrization.hpp>

#include <ql/indexes/interestrateindex.hpp>

namespace QuantExt {

using namespace QuantLib;
using namespace QuantExt;

class LgmVectorised {
public:
    LgmVectorised(const boost::shared_ptr<IrLgm1fParametrization>& p) : p_(p) {}

    RandomVariable numeraire(const Time t, const RandomVariable& x,
                             const Handle<YieldTermStructure> discountCurve = Handle<YieldTermStructure>()) const;

    RandomVariable discountBond(const Time t, const Time T, const RandomVariable& x,
                                Handle<YieldTermStructure> discountCurve = Handle<YieldTermStructure>()) const;

    RandomVariable
    reducedDiscountBond(const Time t, const Time T, const RandomVariable& x,
                        const Handle<YieldTermStructure> discountCurve = Handle<YieldTermStructure>()) const;

    RandomVariable fixing(const boost::shared_ptr<InterestRateIndex>& index, const Date& fixingDate, const Time t,
                          const RandomVariable& x) const;

private:
    const boost::shared_ptr<IrLgm1fParametrization> p_;
};

} // namespace QuantExt

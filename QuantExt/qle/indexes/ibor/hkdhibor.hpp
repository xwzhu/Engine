/*
 Copyright (C) 2016 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of OpenRiskEngine, a free-software/open-source library
 for transparent pricing and risk analysis - http://openriskengine.org

 OpenRiskEngine is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program; if not, please email
 <users@openriskengine.org>. The license is also available online at
 <http://openriskengine.org/license.shtml>.

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/


/*! \file hkdhibor.hpp
    \brief HKD-HIBOR index
    \ingroup 
*/

#ifndef quantext_hkd_hibor_hpp
#define quantext_hkd_hibor_hpp

#include <ql/currencies/asia.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/calendars/hongkong.hpp>
#include <ql/indexes/iborindex.hpp>

using namespace QuantLib;

namespace QuantExt {

    //! HKD-HIBOR index
    /*! HKD-HIBOR rate overseen by The Hong Kong Association of Banks.

        See <http://www.hkab.org.hk>.

        \warning Check roll convention and EOM.
    */
    class HKDHibor : public IborIndex {
      public:
        HKDHibor(const Period& tenor, const Handle<YieldTermStructure>& h =
            Handle<YieldTermStructure>())
        : IborIndex("HKD-HIBOR", tenor, 0, HKDCurrency(), HongKong(),
              ModifiedFollowing, false, Actual365Fixed(), h) {}
    };

}

#endif


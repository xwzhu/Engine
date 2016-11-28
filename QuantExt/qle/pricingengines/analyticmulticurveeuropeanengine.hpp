/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2002, 2003, 2004 Ferdinando Ametrano
 Copyright (C) 2002, 2003 RiskMap srl
 Copyright (C) 2003, 2004, 2007 StatPro Italia srl

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

/*! \file analyticeuropeanengine.hpp
    \brief Analytic European engine
*/

#ifndef quantext_analytic_multicurve_european_engine_hpp
#define quantext_analytic_multicurve_european_engine_hpp

#include <ql/instruments/vanillaoption.hpp>
#include <ql/processes/blackscholesprocess.hpp>

namespace QuantExt {

    //! Enhanced version of the QL AnalyticEuropeanEngine (multi-curve)
    /*! \ingroup vanillaengines

        \test
        - the correctness of the returned value is tested by
          reproducing results available in literature.
        - the correctness of the returned greeks is tested by
          reproducing results available in literature.
        - the correctness of the returned greeks is tested by
          reproducing numerical derivatives.
        - the correctness of the returned implied volatility is tested
          by using it for reproducing the target value.
        - the implied-volatility calculation is tested by checking
          that it does not modify the option.
        - the correctness of the returned value in case of
          cash-or-nothing digital payoff is tested by reproducing
          results available in literature.
        - the correctness of the returned value in case of
          asset-or-nothing digital payoff is tested by reproducing
          results available in literature.
        - the correctness of the returned value in case of gap digital
          payoff is tested by reproducing results available in
          literature.
        - the correctness of the returned greeks in case of
          cash-or-nothing digital payoff is tested by reproducing
          numerical derivatives.
    */
    class AnalyticMultiCurveEuropeanEngine : public QuantLib::VanillaOption::engine {
      public:
          AnalyticMultiCurveEuropeanEngine(
                    const boost::shared_ptr<QuantLib::GeneralizedBlackScholesProcess>&,
                    const QuantLib::Handle<QuantLib::YieldTermStructure>&);
        void calculate() const;
      private:
        boost::shared_ptr<QuantLib::GeneralizedBlackScholesProcess> process_;
        QuantLib::Handle<QuantLib::YieldTermStructure> discountCurve_;
    };

}


#endif

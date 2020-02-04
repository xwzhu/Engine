/*
 Copyright (C) 2020 Quaternion Risk Management Ltd
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

#include <qle/cashflows/indexedcoupon.hpp>

#include <ql/time/daycounter.hpp>

namespace QuantExt {

IndexedCoupon::IndexedCoupon(const boost::shared_ptr<Coupon>& c, const Real qty, const boost::shared_ptr<Index>& index,
                             const Date& fixingDate)
    : Coupon(c->date(), c->nominal(), c->accrualStartDate(), c->accrualEndDate(), c->referencePeriodStart(),
             c->referencePeriodEnd(), c->exCouponDate()),
      c_(c), qty_(qty), index_(index), fixingDate_(fixingDate), initialFixing_(Null<Real>()) {
    registerWith(c);
}

IndexedCoupon::IndexedCoupon(const boost::shared_ptr<Coupon>& c, const Real qty, const Real initialFixing)
    : Coupon(c->date(), c->nominal(), c->accrualStartDate(), c->accrualEndDate(), c->referencePeriodStart(),
             c->referencePeriodEnd(), c->exCouponDate()),
      c_(c), qty_(qty), initialFixing_(initialFixing) {
    registerWith(c);
}

Real IndexedCoupon::nominal() const {
    if (index_)
        return qty_ * index_->fixing(fixingDate_);
    else
        return qty_ * initialFixing_;
}

Real IndexedCoupon::rate() const { return c_->rate(); }

DayCounter IndexedCoupon::dayCounter() const { return c_->dayCounter(); }

IndexedCouponLeg::IndexedCouponLeg(const Leg& underlyingLeg, const Real qty, const boost::shared_ptr<Index>& index)
    : underlyingLeg_(underlyingLeg), qty_(qty), index_(index), initialFixing_(Null<Real>()), fixingDays_(0),
      fixingCalendar_(NullCalendar()), fixingConvention_(Preceding) {
    QL_REQUIRE(index, "IndexedCouponLeg: index required");
}

IndexedCouponLeg& IndexedCouponLeg::withInitialFixing(const Real initialFixing) {
    initialFixing_ = initialFixing;
    return *this;
}

IndexedCouponLeg& IndexedCouponLeg::withValuationSchedule(const Schedule& valuationSchedule) {
    valuationSchedule_ = valuationSchedule;
    return *this;
}

IndexedCouponLeg& IndexedCouponLeg::withFixingDays(const Size fixingDays) {
    fixingDays_ = fixingDays;
    return *this;
}

IndexedCouponLeg& IndexedCouponLeg::withFixingCalendar(const Calendar& fixingCalendar) {
    fixingCalendar_ = fixingCalendar;
    return *this;
}

IndexedCouponLeg& IndexedCouponLeg::withFixingConvention(const BusinessDayConvention& fixingConvention) {
    fixingConvention_ = fixingConvention;
    return *this;
}

IndexedCouponLeg& IndexedCouponLeg::inArrearsFixing(const bool inArrearsFixing) {
    inArrearsFixing_ = inArrearsFixing;
    return *this;
}

IndexedCouponLeg::operator Leg() const {
    Leg resultLeg;
    resultLeg.reserve(underlyingLeg_.size());

    QL_REQUIRE(valuationSchedule_.empty() || valuationSchedule_.size() == underlyingLeg_.size() + 1,
               "IndexedCouponLeg: valuation schedule size ("
                   << valuationSchedule_.size() << ") inconsistent with underlying leg size (" << underlyingLeg_.size()
                   << ") + 1");

    for (Size i = 0; i < underlyingLeg_.size(); ++i) {
        auto cpn = boost::dynamic_pointer_cast<Coupon>(underlyingLeg_[i]);
        QL_REQUIRE(cpn, "IndexedCouponLeg: coupon required");

        Date fixingDate;
        if (valuationSchedule_.empty())
            fixingDate = inArrearsFixing_ ? cpn->accrualEndDate() : cpn->accrualStartDate();
        else
            fixingDate = inArrearsFixing_ ? valuationSchedule_.date(i + 1) : valuationSchedule_.date(i);
        fixingDate = fixingCalendar_.advance(fixingDate, -static_cast<int>(fixingDays_), Days, fixingConvention_);

        if (i == 0 && initialFixing_ != Null<Real>())
            resultLeg.push_back(boost::make_shared<IndexedCoupon>(cpn, qty_, initialFixing_));
        else
            resultLeg.push_back(boost::make_shared<IndexedCoupon>(cpn, qty_, index_, fixingDate));
    }

    return resultLeg;
}

} // namespace QuantExt

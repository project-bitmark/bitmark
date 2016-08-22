// Original Code: Copyright (c) 2014 The Bitcoin Core Developers
// Modified Code: Copyright (c) 2014 Project Bitmark
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "core.h"
#include "main.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(main_tests)

BOOST_AUTO_TEST_CASE(subsidy_limit_test)
{
    uint64_t nSum = 0;
    for (int nHeight = 0; nHeight < 31520000; nHeight += 1000) {
        uint64_t nSubsidy = GetBlockValue(NULL, 0, true);
        BOOST_CHECK(nSubsidy <= 20 * COIN);
        nSum += nSubsidy * 1000;
        BOOST_CHECK(MoneyRange(nSum));
    }
    BOOST_CHECK(nSum == 2757989473108000ULL);
}

BOOST_AUTO_TEST_SUITE_END()

// Original Code: Copyright (c) 2011-2014 The Bitcoin Core Developers
// Modified Code: Copyright (c) 2014 Project Bitmark
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "uint256.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(Checkpoints_tests)

BOOST_AUTO_TEST_CASE(sanity)
{
    uint256 p11571 = uint256("ec7fa58c5659476f5bafc1ff70d41188a73cdaa6deffb0fc42b7054815225f21");
    uint256 p17500 = uint256("812f6ba1879ef23d3210c831b2572b411ee272f16317a541461fddfda5d2ebbf");
    BOOST_CHECK(Checkpoints::CheckBlock(11571, p11571));
    BOOST_CHECK(Checkpoints::CheckBlock(17500, p17500));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(11571, p17500));
    BOOST_CHECK(!Checkpoints::CheckBlock(17500, p11571));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(11571+1, p17500));
    BOOST_CHECK(Checkpoints::CheckBlock(17500+1, p11571));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 46519);
}    

BOOST_AUTO_TEST_SUITE_END()

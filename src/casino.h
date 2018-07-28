// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Ybtc Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef YBTC_POW_H
#define YBTC_POW_H

#include "consensus/params.h"

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckCasino(uint256 hash, unsigned int nBits, const Consensus::Params&);

#endif // YBTC_POW_H

pragma solidity ^0.4.21;


contract CASINO {
    uint256 constant private CHAIN_PHASE_PLAYER = 2;
    uint256 constant private MAX_REGISTER = 256;
    uint256 constant private REGISTER_REWARD = 20;
    uint256 constant private WINNER_BUFFER_SIZE = 128;
    uint256 constant private TOTAL_SUPPLY = 21000000;

    uint256 public totalSupply;
    uint256 public phaseHeight;
    uint256[MAX_REGISTER] public addresses;
    uint256[MAX_REGISTER] public balances;
    uint256[WINNER_BUFFER_SIZE * CHAIN_PHASE_PLAYER] public winners;

    constructor() public {
        phaseHeight = 0;
        addresses[0] = uint256(msg.sender);
        winners[0] = 0;
        balances[0] = TOTAL_SUPPLY;
    }

    function refill() public returns (uint256) {
        uint256 low = TOTAL_SUPPLY;
        uint256 index = MAX_REGISTER;
        uint256 usender = uint256(msg.sender);
        for ( uint256 i = 0 ; i < MAX_REGISTER; ++i ) {
            if ( addresses[i] == usender ) {
                balances[i] += REGISTER_REWARD;
                return balances[i];
            }
            if ( balances[i] < low ) {
                index = i;
                low = balances[i];
            }
        }

        if ( index < MAX_REGISTER ) {
            balances[index] = REGISTER_REWARD;
            addresses[index] = usender;
            return balances[index];
        }

        return 0;
    }

    function getTotalPlayer() public view returns (uint256) {
        uint256 ret = 0;
        for ( uint256 i = 0; i < MAX_REGISTER; ++i) {
            if ( balances[i] > 0 ) ret++;
        }
        return ret;
    }

    function isWinnerId(uint256 seq, uint256 winnerList) private pure returns (bool) {
        uint256 w = winnerList;
        for ( uint256 m = 0; m < CHAIN_PHASE_PLAYER; ++m ) {
            uint256 u = (w & 0xffff);
            if ( seq == u) return true;
            w = (w >> 32);
        }
        return false;
    }

    function setNextWinners(uint256 currentPhase, uint256 winnerList) public returns (bool) {

        require(currentPhase == phaseHeight);

        uint256 j = 0;
        uint256 k = 0;

        phaseHeight += 1;
        uint256 phaseIndex = ( phaseHeight % WINNER_BUFFER_SIZE );

        for ( uint256 i = 0; i < MAX_REGISTER; ++i ) {

            if ( balances[i] == 0 ) continue;

            balances[i] -= 1;
            k += 1;

            if ( isWinnerId(k, winnerList) ) {
                winners[ (phaseIndex*CHAIN_PHASE_PLAYER) + j] = i;
                j += 1;
                if ( j >= CHAIN_PHASE_PLAYER ) return true;
            }
        }

        return true;
    }

    function isWinnerMe(uint256 currentPhase) public view returns (uint256) {
        uint256 win = 0;
        if (currentPhase > phaseHeight || currentPhase + WINNER_BUFFER_SIZE <= phaseHeight) {
            return win;
        }
        uint256 usender = uint256(msg.sender);
        for ( uint256 i = 0 ; i < CHAIN_PHASE_PLAYER; ++i ) {
            win = winners[ (currentPhase % WINNER_BUFFER_SIZE) * 2 + i];
            if ( addresses[win] == usender ) {
                win++;
                return win;
            }
        }

        return win;
    }

    function balanceOf() public view returns (uint256) {
        uint256 usender = uint256(msg.sender);
        for ( uint256 i = 0 ; i < MAX_REGISTER; ++i ) {
            if ( addresses[i] == usender ) {
                return balances[i];
            }
        }
        return 0;
    }
}


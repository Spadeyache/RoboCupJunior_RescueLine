#pragma once

#include "config.h"

// Majority-vote ring buffer for XIAO command debouncing.
// A command is only confirmed once it reaches FILTER_THRESHOLD votes
// in the last FILTER_QUEUE_SIZE frames.
//
// U-turn fires when BOTH left AND right colours are glimpsed — a pure
// single-direction turn will never show the opposite side.
class CommandFilter {
public:
    uint8_t votesUturn  = 0;
    uint8_t votesLeft   = 0;
    uint8_t votesRight  = 0;
    uint8_t votesRed    = 0;
    uint8_t votesSilver = 0;

    CommandFilter() { clear(); }

    void clear() {
        for (int i = 0; i < FILTER_QUEUE_SIZE; i++) _queue[i] = 0;
        _head = 0;
        votesUturn = votesLeft = votesRight = votesRed = votesSilver = 0;
    }

    uint8_t update(uint8_t rawCmd) {
        _queue[_head] = rawCmd;
        _head = (_head + 1) % FILTER_QUEUE_SIZE;

        votesUturn = votesLeft = votesRight = votesRed = votesSilver = 0;
        for (int i = 0; i < FILTER_QUEUE_SIZE; i++) {
            switch (_queue[i]) {
                case 1: votesUturn++; votesLeft++; votesRight++; break;
                case 2: votesLeft++;   break;
                case 3: votesRight++;  break;
                case 4: votesRed++;    break;
                case 5: votesSilver++; break;
            }
        }

        if (votesRed    >= FILTER_THRESHOLD) return 4;
        if (votesSilver >= FILTER_THRESHOLD) return 5;

        bool isUturn = (votesUturn >= 2)
                    || (votesLeft  >= 6 && votesRight >= 1)
                    || (votesRight >= 6 && votesLeft  >= 1)
                    || (votesLeft  >= 4 && votesRight >= 4);
        if (isUturn)                        return 1;

        if (votesLeft  >= FILTER_THRESHOLD) return 2;
        if (votesRight >= FILTER_THRESHOLD) return 3;

        return 0;
    }

private:
    uint8_t _queue[FILTER_QUEUE_SIZE];
    uint8_t _head = 0;
};

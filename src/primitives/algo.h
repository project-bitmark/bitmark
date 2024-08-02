#ifndef BITCOIN_PRIMITIVES_ALGO_H
#define BITCOIN_PRIMITIVES_ALGO_H

#include <sstream>

const int NUM_ALGOS = 8;
const int nSSF = 720 / NUM_ALGOS; // interval for ssf updates

enum class Algo {
    UNKNOWN = -1,
    SCRYPT = 0,
    SHA256D = 1,
    YESCRYPT = 2,
    ARGON2 = 3,
    X17 = 4,
    LYRA2REv2 = 5,
    EQUIHASH = 6,
    CRYPTONIGHT = 7
};

inline std::ostream& operator<<(std::ostream& os, const Algo val)
{
    switch (val) {
    case Algo::SCRYPT:
        os << "Scrypt";
        break;
    case Algo::SHA256D:
        os << "Sha256D";
        break;
    case Algo::YESCRYPT:
        os << "Yescrypt";
        break;
    case Algo::ARGON2:
        os << "Argon2";
        break;
    case Algo::X17:
        os << "X17";
        break;
    case Algo::LYRA2REv2:
        os << "Lyra2Rev2";
        break;
    case Algo::EQUIHASH:
        os << "Equihash";
        break;
    case Algo::CRYPTONIGHT:
        os << "Cryptonight";
        break;
    case Algo::UNKNOWN:
    default:
        os << "unknown";
    }
    return os;
};


inline std::string ToString(const Algo algo)
{
    std::stringstream stream;

    stream << algo;

    return stream.str();
}

// Based on tests with general purpose CPUs,
//       ( Except for SHA256 which was designed for simplicity and suited for ASICs,
//       so given a factor of 16 decrease in weight. )
//   Weighing gives more value to hashes from some algos over others,
//      because, for example a Cryptonight hash is much more computationally expensive
//      than a SHA256d hash.
//   Weights should ultimately reflect the market value of hashes by different algorithms;
//      this will vary constantly (and more significantly long-term with hardware developement)
//   As of June, 2018 these values are closely reflective of market values seen on
//      nicehash.com and miningrigrentals.com
inline unsigned int GetAlgoWeight(const Algo algo)
{
    unsigned int weight = 8000; // scrypt, lyra2rev2 and 17 share this value.
    switch (algo) {
    case Algo::SHA256D:
        weight = 1;
        break;
    case Algo::ARGON2:
        weight = 4000000;
        break;
    case Algo::EQUIHASH:
        weight = 8000000;
        break;
    case Algo::CRYPTONIGHT:
        weight = 8000000;
        break;
    case Algo::YESCRYPT:
        weight = 800000;
        break;
    case Algo::UNKNOWN:
    default:
        break;
    }
    return weight;
}

#endif

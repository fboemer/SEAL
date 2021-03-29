// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#ifdef SEAL_USE_INTEL_HEXL
#include "seal/util/locks.h"
#include <unordered_map>
#include "intel-hexl/intel-hexl.hpp"

namespace intel
{
    namespace seal_ext
    {
        struct HashPair
        {
            template <class T1, class T2>
            size_t operator()(const std::pair<T1, T2> &p) const
            {
                auto hash1 = std::hash<T1>{}(std::get<0>(p));
                auto hash2 = std::hash<T2>{}(std::get<1>(p));
                return hash_combine(hash1, hash2);
            }

            static size_t hash_combine(size_t lhs, size_t rhs)
            {
                lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
                return lhs;
            }
        };

        static std::unordered_map<std::pair<uint64_t, uint64_t>, intel::hexl::NTT, HashPair> ntt_cache_;

        static seal::util::ReaderWriterLocker ntt_cache_locker_;

        static intel::hexl::NTT get_ntt(size_t N, uint64_t p, uint64_t root)
        {
            std::pair<uint64_t, uint64_t> key{ N, p };

            // Enable shared access of NTT already present
            {
                seal::util::ReaderLock reader_lock(ntt_cache_locker_.acquire_read());
                auto ntt_it = ntt_cache_.find(key);
                if (ntt_it != ntt_cache_.end())
                {
                    return ntt_it->second;
                }
            }

            // Deal with NTT not yet present
            seal::util::WriterLock write_lock(ntt_cache_locker_.acquire_write());

            // Check ntt_cache for value (maybe added by another thread)
            auto ntt_it = ntt_cache_.find(key);
            if (ntt_it == ntt_cache_.end())
            {
                ntt_it = ntt_cache_.emplace(std::move(key), intel::hexl::NTT(N, p, root)).first;
            }
            return ntt_it->second;
        }

        static void compute_forward_ntt(
            seal::util::CoeffIter operand, size_t N, uint64_t p, uint64_t root, uint64_t input_mod_factor,
            uint64_t output_mod_factor)
        {
            get_ntt(N, p, root).ComputeForward(operand, operand, input_mod_factor, output_mod_factor);
        }

        static void compute_inverse_ntt(
            seal::util::CoeffIter operand, size_t N, uint64_t p, uint64_t root, uint64_t input_mod_factor,
            uint64_t output_mod_factor)
        {
            get_ntt(N, p, root).ComputeInverse(operand, operand, input_mod_factor, output_mod_factor);
        }

    } // namespace seal_ext
} // namespace intel
#endif

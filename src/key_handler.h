#pragma once
#include <vector>

const std::pair<std::vector<unsigned char>, std::vector<unsigned char>> generate_keypair();
std::vector<unsigned char> compute_shared_key(
        const std::vector<unsigned char>& my_sk,
        const std::vector<unsigned char>& other_pk);
        
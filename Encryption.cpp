#include "Encryption.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

    // ==== SHA-256 常量 =========================================================

    // SHA-256算法的常量数组K
    constexpr std::array<std::uint32_t, 64> SHA256_K = {
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U,
        0x923f82a4U, 0xab1c5ed5U, 0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
        0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U, 0xe49b69c1U, 0xefbe4786U,
        0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U,
        0x06ca6351U, 0x14292967U, 0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
        0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U, 0xa2bfe8a1U, 0xa81a664bU,
        0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU,
        0x5b9cca4fU, 0x682e6ff3U, 0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
        0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
    };

    // SHA-256辅助函数（右旋转）
    inline std::uint32_t rotr(std::uint32_t x, std::uint32_t n) {
        return (x >> n) | (x << (32U - n));
    }

    // SHA-256选择函数（ch）
    inline std::uint32_t ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
        return (x & y) ^ ((~x) & z);
    }

    // SHA-256多数函数（maj）
    inline std::uint32_t maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
        return (x & y) ^ (x & z) ^ (y & z);
    }

    // SHA-256 Σ0函数
    inline std::uint32_t Sigma0(std::uint32_t x) {
        return rotr(x, 2U) ^ rotr(x, 13U) ^ rotr(x, 22U);
    }

    // SHA-256 Σ1函数
    inline std::uint32_t Sigma1(std::uint32_t x) {
        return rotr(x, 6U) ^ rotr(x, 11U) ^ rotr(x, 25U);
    }

    // SHA-256 σ0函数
    inline std::uint32_t sigma0(std::uint32_t x) {
        return rotr(x, 7U) ^ rotr(x, 18U) ^ (x >> 3U);
    }

    // SHA-256 σ1函数
    inline std::uint32_t sigma1(std::uint32_t x) {
        return rotr(x, 17U) ^ rotr(x, 19U) ^ (x >> 10U);
    }

    // SHA-256哈希算法主函数
    std::array<std::uint32_t, 8> sha256(const std::vector<std::uint8_t>& data) {
        // 初始哈希值（H0）
        std::array<std::uint32_t, 8> state = {
            0x6a09e667U,
            0xbb67ae85U,
            0x3c6ef372U,
            0xa54ff53aU,
            0x510e527fU,
            0x9b05688cU,
            0x1f83d9abU,
            0x5be0cd19U
        };

        std::vector<std::uint8_t> padded = data;
        std::uint64_t bitLength = static_cast<std::uint64_t>(padded.size()) * 8ULL;

        // 填充：先加一个1（0x80），然后加0直到长度满足要求
        padded.push_back(0x80);
        while ((padded.size() % 64U) != 56U) {
            padded.push_back(0x00);
        }

        // 添加原始消息长度（以比特为单位）
        for (int i = 7; i >= 0; --i) {
            padded.push_back(static_cast<std::uint8_t>((bitLength >> (i * 8)) & 0xFFU));
        }

        // 处理每个512位（64字节）块
        for (std::size_t offset = 0; offset < padded.size(); offset += 64U) {
            std::uint32_t w[64];
            // 将16个32位字复制到w[0..15]
            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<std::uint32_t>(padded[offset + i * 4]) << 24U) |
                    (static_cast<std::uint32_t>(padded[offset + i * 4 + 1]) << 16U) |
                    (static_cast<std::uint32_t>(padded[offset + i * 4 + 2]) << 8U) |
                    (static_cast<std::uint32_t>(padded[offset + i * 4 + 3]));
            }
            // 扩展w[16..63]
            for (int i = 16; i < 64; ++i) {
                w[i] = sigma1(w[i - 2]) + w[i - 7] + sigma0(w[i - 15]) + w[i - 16];
            }

            // 初始化工作变量
            std::uint32_t a = state[0];
            std::uint32_t b = state[1];
            std::uint32_t c = state[2];
            std::uint32_t d = state[3];
            std::uint32_t e = state[4];
            std::uint32_t f = state[5];
            std::uint32_t g = state[6];
            std::uint32_t h = state[7];

            // 主循环
            for (int i = 0; i < 64; ++i) {
                std::uint32_t T1 = h + Sigma1(e) + ch(e, f, g) + SHA256_K[i] + w[i];
                std::uint32_t T2 = Sigma0(a) + maj(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + T1;
                d = c;
                c = b;
                b = a;
                a = T1 + T2;
            }

            // 更新状态
            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
            state[5] += f;
            state[6] += g;
            state[7] += h;
        }

        return state;
    }

    // ==== Base64 编码/解码 ================================================================

    // Base64字母表
    const char* BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    // Base64编码
    std::string encodeBase64(const std::vector<std::uint8_t>& data) {
        std::string result;
        std::size_t i = 0;

        // 每次处理3个字节
        while (i + 2 < data.size()) {
            std::uint32_t triple = (static_cast<std::uint32_t>(data[i]) << 16U) |
                (static_cast<std::uint32_t>(data[i + 1]) << 8U) |
                (static_cast<std::uint32_t>(data[i + 2]));
            result.push_back(BASE64_ALPHABET[(triple >> 18U) & 0x3FU]);
            result.push_back(BASE64_ALPHABET[(triple >> 12U) & 0x3FU]);
            result.push_back(BASE64_ALPHABET[(triple >> 6U) & 0x3FU]);
            result.push_back(BASE64_ALPHABET[triple & 0x3FU]);
            i += 3;
        }

        // 处理不足3个字节的情况
        if (i < data.size()) {
            std::uint32_t triple = static_cast<std::uint32_t>(data[i]) << 16U;
            result.push_back(BASE64_ALPHABET[(triple >> 18U) & 0x3FU]);
            if (i + 1 < data.size()) {
                triple |= static_cast<std::uint32_t>(data[i + 1]) << 8U;
                result.push_back(BASE64_ALPHABET[(triple >> 12U) & 0x3FU]);
                result.push_back(BASE64_ALPHABET[(triple >> 6U) & 0x3FU]);
                result.push_back('=');  // 填充一个'='
            }
            else {
                result.push_back(BASE64_ALPHABET[(triple >> 12U) & 0x3FU]);
                result.push_back('=');
                result.push_back('=');  // 填充两个'='
            }
        }

        return result;
    }

    // Base64解码
    std::vector<std::uint8_t> decodeBase64(const std::string& text) {
        std::vector<std::uint8_t> result;
        std::array<int, 256> decodeTable{};
        decodeTable.fill(-1);

        // 构建解码表
        for (int i = 0; i < 64; ++i) {
            decodeTable[static_cast<unsigned char>(BASE64_ALPHABET[i])] = i;
        }

        std::uint32_t buffer = 0;
        int bitsCollected = 0;

        for (char ch : text) {
            if (ch == '=') {
                break;  // 遇到填充字符则停止
            }
            int value = decodeTable[static_cast<unsigned char>(ch)];
            if (value < 0) {
                continue;  // 跳过无效字符
            }
            buffer = (buffer << 6U) | static_cast<std::uint32_t>(value);
            bitsCollected += 6;
            if (bitsCollected >= 8) {
                bitsCollected -= 8;
                result.push_back(static_cast<std::uint8_t>((buffer >> bitsCollected) & 0xFFU));
            }
        }

        return result;
    }

} // namespace

// ==== Encryption 工具实现 ===================================================

// HMAC-SHA256算法
std::array<std::uint8_t, 32> Encryption::hmacSha256(const std::string& key,
    const std::vector<std::uint8_t>& data) {
    constexpr std::size_t BLOCK_SIZE = 64;
    std::vector<std::uint8_t> keyBlock(BLOCK_SIZE, 0x00);

    // 如果密钥长度超过块大小，先对密钥进行哈希
    if (key.size() > BLOCK_SIZE) {
        auto hashed = sha256(std::vector<std::uint8_t>(key.begin(), key.end()));
        for (std::size_t i = 0; i < 8; ++i) {
            keyBlock[i * 4] = static_cast<std::uint8_t>((hashed[i] >> 24U) & 0xFFU);
            keyBlock[i * 4 + 1] = static_cast<std::uint8_t>((hashed[i] >> 16U) & 0xFFU);
            keyBlock[i * 4 + 2] = static_cast<std::uint8_t>((hashed[i] >> 8U) & 0xFFU);
            keyBlock[i * 4 + 3] = static_cast<std::uint8_t>(hashed[i] & 0xFFU);
        }
    }
    else {
        std::memcpy(keyBlock.data(), key.data(), key.size());
    }

    // 创建内填充和外填充
    std::vector<std::uint8_t> oKeyPad(BLOCK_SIZE, 0x5c);
    std::vector<std::uint8_t> iKeyPad(BLOCK_SIZE, 0x36);

    for (std::size_t i = 0; i < BLOCK_SIZE; ++i) {
        oKeyPad[i] ^= keyBlock[i];
        iKeyPad[i] ^= keyBlock[i];
    }

    // 计算内层哈希
    std::vector<std::uint8_t> innerData = iKeyPad;
    innerData.insert(innerData.end(), data.begin(), data.end());

    auto innerHashState = sha256(innerData);
    std::vector<std::uint8_t> innerHash(32);
    for (std::size_t i = 0; i < 8; ++i) {
        innerHash[i * 4] = static_cast<std::uint8_t>((innerHashState[i] >> 24U) & 0xFFU);
        innerHash[i * 4 + 1] = static_cast<std::uint8_t>((innerHashState[i] >> 16U) & 0xFFU);
        innerHash[i * 4 + 2] = static_cast<std::uint8_t>((innerHashState[i] >> 8U) & 0xFFU);
        innerHash[i * 4 + 3] = static_cast<std::uint8_t>(innerHashState[i] & 0xFFU);
    }

    // 计算外层哈希
    std::vector<std::uint8_t> outerData = oKeyPad;
    outerData.insert(outerData.end(), innerHash.begin(), innerHash.end());

    auto outerHashState = sha256(outerData);
    std::array<std::uint8_t, 32> hmac{};
    for (std::size_t i = 0; i < 8; ++i) {
        hmac[i * 4] = static_cast<std::uint8_t>((outerHashState[i] >> 24U) & 0xFFU);
        hmac[i * 4 + 1] = static_cast<std::uint8_t>((outerHashState[i] >> 16U) & 0xFFU);
        hmac[i * 4 + 2] = static_cast<std::uint8_t>((outerHashState[i] >> 8U) & 0xFFU);
        hmac[i * 4 + 3] = static_cast<std::uint8_t>(outerHashState[i] & 0xFFU);
    }

    return hmac;
}

// PBKDF2-HMAC-SHA256密钥派生函数
std::vector<std::uint8_t> Encryption::pbkdf2HmacSha256(const std::string& password,
    const std::string& salt,
    std::uint32_t iterations,
    std::size_t dkLen) {
    if (iterations == 0 || dkLen == 0) {
        throw std::invalid_argument("PBKDF2 参数非法。");
    }

    std::vector<std::uint8_t> derivedKey(dkLen);
    std::size_t hashLen = 32;
    std::size_t blockCount = (dkLen + hashLen - 1) / hashLen;

    for (std::size_t block = 1; block <= blockCount; ++block) {
        // 构建盐值块（salt + block index）
        std::vector<std::uint8_t> saltBlock(salt.begin(), salt.end());
        saltBlock.push_back(static_cast<std::uint8_t>((block >> 24U) & 0xFFU));
        saltBlock.push_back(static_cast<std::uint8_t>((block >> 16U) & 0xFFU));
        saltBlock.push_back(static_cast<std::uint8_t>((block >> 8U) & 0xFFU));
        saltBlock.push_back(static_cast<std::uint8_t>(block & 0xFFU));

        auto u = hmacSha256(password, saltBlock);
        std::array<std::uint8_t, 32> f = u;

        // 迭代计算
        for (std::uint32_t iter = 1; iter < iterations; ++iter) {
            std::vector<std::uint8_t> uData(u.begin(), u.end());
            u = hmacSha256(password, uData);
            for (std::size_t i = 0; i < f.size(); ++i) {
                f[i] ^= u[i];
            }
        }

        // 复制到输出密钥
        std::size_t offset = (block - 1) * hashLen;
        std::size_t toCopy = std::min(hashLen, dkLen - offset);
        std::memcpy(derivedKey.data() + offset, f.data(), toCopy);
    }

    return derivedKey;
}

// Base64编码（公共接口）
std::string Encryption::toBase64(const std::vector<std::uint8_t>& data) {
    return encodeBase64(data);
}

// Base64解码（公共接口）
std::vector<std::uint8_t> Encryption::fromBase64(const std::string& text) {
    return decodeBase64(text);
}

// XOR加密/解密（按字节异或）
std::vector<std::uint8_t> Encryption::xorBuffer(const std::vector<std::uint8_t>& buffer,
    const std::vector<std::uint8_t>& key) {
    if (key.empty()) {
        throw std::invalid_argument("密钥为空。");
    }
    std::vector<std::uint8_t> result(buffer.size());
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        result[i] = buffer[i] ^ key[i % key.size()];  // 循环使用密钥
    }
    return result;
}

// ==== 对外接口 =============================================================

// 密码哈希（使用PBKDF2-HMAC-SHA256）
std::string Encryption::hashPassword(const std::string& password,
    const std::string& salt,
    std::uint32_t iterations) {
    auto derived = pbkdf2HmacSha256(password, salt, iterations, 32);
    return toBase64(derived);
}

// 密码验证
bool Encryption::verifyPassword(const std::string& password,
    const std::string& salt,
    const std::string& expectedHash,
    std::uint32_t iterations) {
    auto hashBytes = fromBase64(expectedHash);
    auto derived = pbkdf2HmacSha256(password, salt, iterations, hashBytes.size());
    if (derived.size() != hashBytes.size()) {
        return false;
    }
    // 安全比较：计算字节差值的或运算
    std::uint8_t diff = 0;
    for (std::size_t i = 0; i < derived.size(); ++i) {
        diff |= static_cast<std::uint8_t>(derived[i] ^ hashBytes[i]);
    }
    return diff == 0;
}

// XOR加密字符串（先XOR再Base64）
std::string Encryption::xorEncrypt(const std::string& data, const std::string& key) {
    std::vector<std::uint8_t> buffer(data.begin(), data.end());
    std::vector<std::uint8_t> keyBuffer(key.begin(), key.end());
    auto encrypted = xorBuffer(buffer, keyBuffer);
    return toBase64(encrypted);
}

// XOR解密字符串（先Base64解码再XOR）
std::string Encryption::xorDecrypt(const std::string& cipher, const std::string& key) {
    auto cipherBytes = fromBase64(cipher);
    std::vector<std::uint8_t> keyBuffer(key.begin(), key.end());
    auto decrypted = xorBuffer(cipherBytes, keyBuffer);
    return std::string(decrypted.begin(), decrypted.end());
}

// 生成随机盐值
std::string Encryption::generateSalt() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<std::uint32_t> dist(0, 255);

    std::vector<std::uint8_t> salt(16);
    for (auto& byte : salt) {
        byte = static_cast<std::uint8_t>(dist(gen));
    }
    return toBase64(salt);
}

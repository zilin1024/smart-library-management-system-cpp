#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

/**
 * @class Encryption
 * @brief 工具层加密组件，提供口令哈希、对称加解密与随机盐生成等功能。
 *
 * 说明：
 * - 当前实现主要面向教学示例，未使用平台专用硬件加速。
 * - hashPassword 采用 PBKDF2-HMAC-SHA256 (简化版) 迭代，适用于一般演示。
 * - xorEncrypt/xorDecrypt 为轻量级对称演示算法，不适用于生产用途。
 */
class Encryption {
public:
    /**
     * @brief 使用 PBKDF2-HMAC-SHA256 生成口令哈希。
     * @param password 明文口令。
     * @param salt 随机盐。
     * @param iterations 迭代次数，默认 10_000。
     * @return Base64 编码的哈希结果。
     */
    static std::string hashPassword(const std::string& password,
        const std::string& salt,
        std::uint32_t iterations = 10000);

    /**
     * @brief 校验口令是否匹配哈希。
     * @param password 待验证的明文口令。
     * @param salt 随机盐。
     * @param expectedHash 期望的哈希（Base64）。
     * @param iterations 迭代次数。
     * @return 匹配返回 true。
     */
    static bool verifyPassword(const std::string& password,
        const std::string& salt,
        const std::string& expectedHash,
        std::uint32_t iterations = 10000);

    /**
     * @brief XOR 对称加密。
     * @param data 待加密数据。
     * @param key 密钥。
     * @return 加密后的字节序列（Base64 编码）。
     */
    static std::string xorEncrypt(const std::string& data, const std::string& key);

    /**
     * @brief XOR 对称解密。
     * @param cipher Base64 编码的密文。
     * @param key 密钥。
     * @return 解密得到的明文。
     */
    static std::string xorDecrypt(const std::string& cipher, const std::string& key);

    /**
     * @brief 生成随机盐（16 字节，Base64 编码）。
     */
    static std::string generateSalt();

private:
    static std::vector<std::uint8_t> pbkdf2HmacSha256(const std::string& password,
        const std::string& salt,
        std::uint32_t iterations,
        std::size_t dkLen);

    static std::array<std::uint8_t, 32> hmacSha256(const std::string& key,
        const std::vector<std::uint8_t>& data);

    static std::string toBase64(const std::vector<std::uint8_t>& data);
    static std::vector<std::uint8_t> fromBase64(const std::string& text);
    static std::vector<std::uint8_t> xorBuffer(const std::vector<std::uint8_t>& buffer,
        const std::vector<std::uint8_t>& key);
};

#endif // ENCRYPTION_H

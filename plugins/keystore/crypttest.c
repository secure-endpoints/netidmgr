/*
 * Copyright (c) 2009 Secure Endpoints Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include<windows.h>
#include<wincrypt.h>
#include<stdio.h>

void
hexdump(unsigned char * buf, size_t cb)
{
    size_t i;
    static char nibbles[] =
        { '0', '1', '2', '3', '4', '5', '6', '7',
          '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    printf("Hexdump of object %p (length: %d)\n", buf, cb);

    for (i=0; i < cb; i += 16) {
        size_t j;

        printf("%04X : ", i);

        for (j=i; j < cb && j < i+16; j++) {
            printf("%c%c ",
                   nibbles[(buf[j] >> 4) & 0xf],
                   nibbles[buf[j] & 0xf]);

            if ((j%16)==7)
                printf("- ");
        }

        for (; j < i+16; j++) {
            printf("   ");

            if ((j%16)==7)
                printf("- ");
        }

        printf("   ");

        for (j=i; j < cb && j < i+16; j++) {
            if (buf[j] > 32 && buf[j] < 127)
                printf("%c", buf[j]);
            else
                printf(".");
            if ((j%16)==7)
                printf("-");
        }

        printf("\n");
    }
}

#define PERR(s) printf(s " GetLastError=0x%8X\n", GetLastError())
#define CCall(f,s) if (!f) {PERR(s); goto done; }

void
encrypt(char * instr, char * password, void **pbuf, size_t *pcb)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTHASH hEncHash = 0;
    HCRYPTKEY  hKey = 0;
    DWORD cbData;
    DWORD cbHash;
    void * hash = NULL;

    *pcb = 0;

    CCall(CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT | CRYPT_SILENT),
          "Acquire context failed!");

    CCall(CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash),
          "Failed to create hash!");

    CCall(CryptHashData(hHash, password, strlen(password), 0),
          "Failed to hash data!");

    CCall(CryptDeriveKey(hProv, CALG_RC4, hHash, CRYPT_NO_SALT,
                         &hKey),
          "Failed to derive key!");

    CCall(CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hEncHash),
          "Failed to create secondary hash!");

    cbData = strlen(instr) + 1;
    CCall(CryptEncrypt(hKey,
                       hEncHash,
                       TRUE,     /* Final */
                       0,
                       NULL,
                       &cbData,
                       0),
          "Failed to determine size of encrypted buffer");

    *pcb = cbData;
    *pbuf = malloc(cbData);
    memcpy(*pbuf, instr, strlen(instr) + 1);
    cbData = strlen(instr) + 1;
    CCall(CryptEncrypt(hKey,
                       hEncHash,
                       TRUE,    /*  */
                       0,
                       *pbuf,
                       &cbData,
                       (DWORD) *pcb),
          "Failed to encrypt data!");

    cbData = sizeof(cbHash);
    CCall(CryptGetHashParam(hEncHash,
                            HP_HASHSIZE,
                            &cbHash,
                            &cbData,
                            0),
          "Failed to get hash size!");

    hash = malloc(cbHash);
    CCall(CryptGetHashParam(hEncHash,
                            HP_HASHVAL,
                            hash,
                            &cbHash,
                            0),
          "Failed to get hash!");

    printf("Hash:\n");
    hexdump(hash, cbHash);

 done:

    if (hash)
        free(hash);

    if (hHash)
        CryptDestroyHash(hHash);

    if (hEncHash)
        CryptDestroyHash(hEncHash);

    if (hKey)
        CryptDestroyKey(hKey);

    if (hProv)
        CryptReleaseContext(hProv, 0);

}

void
decrypt(void * buf, size_t cb, char * password, char ** outstr)
{
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY  hKey = 0;
    HCRYPTHASH hDecHash = 0;
    DWORD cbData;
    DWORD cbHash;
    void * hash = NULL;

    *outstr = NULL;

    CCall(CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                              CRYPT_VERIFYCONTEXT | CRYPT_SILENT),
          "Acquire context failed!");

    CCall(CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash),
          "Failed to create hash!");

    CCall(CryptHashData(hHash, password, strlen(password), 0),
          "Failed to hash data!");

    CCall(CryptDeriveKey(hProv, CALG_RC4, hHash, CRYPT_NO_SALT,
                         &hKey),
          "Failed to derive key!");

    CCall(CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hDecHash),
          "Failed to create dec hash!");

    *outstr = malloc(cb);
    memcpy(*outstr, buf, cb);

    cbData = cb;
    CCall(CryptDecrypt(hKey, hDecHash, TRUE, 0, *outstr, &cbData),
          "Failed to decrypt!");

    printf("cbData = %d\n", cbData);

    cbData = sizeof(cbHash);
    CCall(CryptGetHashParam(hDecHash,
                            HP_HASHSIZE,
                            &cbHash,
                            &cbData,
                            0),
          "Failed to get hash size!");

    hash = malloc(cbHash);
    CCall(CryptGetHashParam(hDecHash,
                            HP_HASHVAL,
                            hash,
                            &cbHash,
                            0),
          "Failed to get hash!");

    printf("Hash:\n");
    hexdump(hash, cbHash);



 done:

    if (hash)
        free(hash);

    if (hProv)
        CryptReleaseContext(hProv, 0);

    if (hHash)
        CryptDestroyHash(hHash);

    if (hKey)
        CryptDestroyKey(hKey);
}

int main(int argc, char ** argv)
{

    void * ciphertext = NULL;
    size_t cb_cipertext = 0;
    char *outstr = NULL;

    encrypt("Hello world! This is a slightly longer string that is going to be decrypted."
            "I made it long just for kicks although I verified that it works for shorter"
            "plaintexts",
            "MyPassword",
            &ciphertext, &cb_cipertext);

    if (ciphertext) {
        hexdump((unsigned char *) ciphertext, cb_cipertext);
    }

    decrypt(ciphertext, cb_cipertext, "MyPassword", &outstr);

    if (outstr) {
        hexdump(outstr, cb_cipertext);
    }

    if (ciphertext)
        free(ciphertext);

    if (outstr)
        free(outstr);

    return 0;
}

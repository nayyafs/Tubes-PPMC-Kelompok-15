
#define BLOCK_SIZE 25
typedef unsigned char BYTE;
typedef unsigned long long SIZE;

#define CRYPTO_KEYBYTES 16
#define CRYPTO_NSECBYTES 0
#define CRYPTO_NPUBBYTES 12
#define CRYPTO_ABYTES 16
#define CRYPTO_NOOVERLAP 1

unsigned char plaintext[100] = "TolonglahheyLAPET";
unsigned char key[CRYPTO_KEYBYTES] = {0x66, 0x44, 0x22, 0x11, 0x33, 0x55, 0x77, 0x99, 0xBB, 0xDD, 0xFF, 0xEE, 0xCC, 0xAA, 0x88, 0x66};
unsigned char nonce[CRYPTO_NPUBBYTES] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34};

unsigned char ciphertext[100 + CRYPTO_ABYTES];
unsigned char decrypted[100];
unsigned char ad[100] = "AssociatedData";

unsigned long long ciphertext_len;
unsigned long long decrypted_len;

int ret;



void setup() {
  Serial.begin(115200);
  
}

void loop() {
    Serial.println((char*) plaintext);
    delay(1000);
}
/*
Last Update
*   Dimas - 26 Apr - 02.30
*       - Berhasil Subscribe message dr topic ESP32_Insulin_Checker_topic
*       - Berhasil Publish Message ke topic ESP32_Insulin_Pump_topic
*/

#include <WiFi.h>
#include <PubSubClient.h>
#define trigPin 18
#define echoPin 19
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// ---------------------------------- ENCRYPTION ----------------------------------
//---------------------------------- elephant_200.h --------------------------------------
#define BLOCK_SIZE 25
typedef unsigned char BYTE;
typedef unsigned long long SIZE;

//----------------------------------- api.h ---------------------------------------
//-----------COBA1
#define CRYPTO_KEYBYTES 16
#define CRYPTO_NSECBYTES 0
#define CRYPTO_NPUBBYTES 12
#define CRYPTO_ABYTES 16
#define CRYPTO_NOOVERLAP 1

//------------COBA2
// #define CRYPTO_KEYBYTES 32
// #define CRYPTO_NPUBBYTES 16
// #define CRYPTO_ABYTES 16
// #define CRYPTO_NSECBYTES 0
// #define CRYPTO_NOOVERLAP 1

// ----------------------------- keccak.c -------------------------------------
#define maxNrRounds 18
#define nrLanes 25
#define index(x, y) (((x)%5)+5*((y)%5))
#define ROL8(a, offset) ((offset != 0) ? ((((BYTE)a) << offset) ^ (((BYTE)a) >> (sizeof(BYTE)*8-offset))) : a)

const BYTE KeccakRoundConstants[maxNrRounds] = {
    0x01, 0x82, 0x8a, 0x00, 0x8b, 0x01, 0x81, 0x09, 0x8a,
    0x88, 0x09, 0x0a, 0x8b, 0x8b, 0x89, 0x03, 0x02, 0x80
};

const unsigned int KeccakRhoOffsets[nrLanes] = {
    0, 1, 6, 4, 3, 4, 4, 6, 7, 4, 3, 2, 3, 1, 7, 1, 5, 7, 5, 0, 2, 2, 5, 0, 6
};

void theta(BYTE *A)
{
    unsigned int x, y;
    BYTE C[5], D[5];

    for(x=0; x<5; x++) {
        C[x] = 0;
        for(y=0; y<5; y++)
            C[x] ^= A[index(x, y)];
    }
    for(x=0; x<5; x++)
        D[x] = ROL8(C[(x+1)%5], 1) ^ C[(x+4)%5];
    for(x=0; x<5; x++)
        for(y=0; y<5; y++)
            A[index(x, y)] ^= D[x];
}

void rho(BYTE *A)
{
    for(unsigned int x=0; x<5; x++)
        for(unsigned int y=0; y<5; y++)
            A[index(x, y)] = ROL8(A[index(x, y)], KeccakRhoOffsets[index(x, y)]);
}

void pi(BYTE *A)
{
    BYTE tempA[25];

    for(unsigned int x=0; x<5; x++)
        for(unsigned int y=0; y<5; y++)
            tempA[index(x, y)] = A[index(x, y)];
    for(unsigned int x=0; x<5; x++)
        for(unsigned int y=0; y<5; y++)
            A[index(0*x+1*y, 2*x+3*y)] = tempA[index(x, y)];
}

void chi(BYTE *A)
{
    unsigned int x, y;
    BYTE C[5];

    for(y=0; y<5; y++) {
        for(x=0; x<5; x++)
            C[x] = A[index(x, y)] ^ ((~A[index(x+1, y)]) & A[index(x+2, y)]);
        for(x=0; x<5; x++)
            A[index(x, y)] = C[x];
    }
}

void iota(BYTE *A, unsigned int indexRound)
{
    A[index(0, 0)] ^= KeccakRoundConstants[indexRound];
}

void KeccakP200Round(BYTE *state, unsigned int indexRound)
{
    theta(state);
    rho(state);
    pi(state);
    chi(state);
    iota(state, indexRound);
}

void permutation(BYTE* state)
{
    for(unsigned int i=0; i<maxNrRounds; i++)
        KeccakP200Round(state, i);
}

//--------------------------------- encrypt.c --------------------------------
BYTE rotl(BYTE b)
{
    return (b << 1) | (b >> 7);
}

int constcmp(const BYTE* a, const BYTE* b, SIZE length)
{
    BYTE r = 0;

    for (SIZE i = 0; i < length; ++i)
        r |= a[i] ^ b[i];
    return r;
}

void lfsr_step(BYTE* output, BYTE* input)
{
    BYTE temp = rotl(input[0]) ^ rotl(input[2]) ^ (input[13] << 1);
    for(SIZE i = 0; i < BLOCK_SIZE - 1; ++i)
        output[i] = input[i + 1];
    output[BLOCK_SIZE - 1] = temp;
}

void xor_block(BYTE* state, const BYTE* block, SIZE size)
{
    for(SIZE i = 0; i < size; ++i)
        state[i] ^= block[i];
}


void get_ad_block(BYTE* output, const BYTE* ad, SIZE adlen, const BYTE* npub, SIZE i)
{
    SIZE len = 0;
    // First block contains nonce
    // Remark: nonce may not be longer then BLOCK_SIZE
    if(i == 0) {
        memcpy(output, npub, CRYPTO_NPUBBYTES);
        len += CRYPTO_NPUBBYTES;
    }

    const SIZE block_offset = i * BLOCK_SIZE - (i != 0) * CRYPTO_NPUBBYTES;
    // If adlen is divisible by BLOCK_SIZE, add an additional padding block
    if(i != 0 && block_offset == adlen) {
        memset(output, 0x00, BLOCK_SIZE);
        output[0] = 0x01;
        return;
    }
    const SIZE r_outlen = BLOCK_SIZE - len;
    const SIZE r_adlen  = adlen - block_offset;
    // Fill with associated data if available
    if(r_outlen <= r_adlen) { // enough AD
        memcpy(output + len, ad + block_offset, r_outlen);
    } else { // not enough AD, need to pad
        if(r_adlen > 0) // ad might be nullptr
            memcpy(output + len, ad + block_offset, r_adlen);
        memset(output + len + r_adlen, 0x00, r_outlen - r_adlen);
        output[len + r_adlen] = 0x01;
    }
}


void get_c_block(BYTE* output, const BYTE* c, SIZE clen, SIZE i)
{
    const SIZE block_offset = i * BLOCK_SIZE;
    // If clen is divisible by BLOCK_SIZE, add an additional padding block
    if(block_offset == clen) {
        memset(output, 0x00, BLOCK_SIZE);
        output[0] = 0x01;
        return;
    }
    const SIZE r_clen  = clen - block_offset;
    // Fill with ciphertext if available
    if(BLOCK_SIZE <= r_clen) { // enough ciphertext
        memcpy(output, c + block_offset, BLOCK_SIZE);
    } else { // not enough ciphertext, need to pad
        if(r_clen > 0) // c might be nullptr
            memcpy(output, c + block_offset, r_clen);
        memset(output + r_clen, 0x00, BLOCK_SIZE - r_clen);
        output[r_clen] = 0x01;
    }
}

void crypto_aead_impl(
    BYTE* c, BYTE* tag, const BYTE* m, SIZE mlen, const BYTE* ad, SIZE adlen,
    const BYTE* npub, const BYTE* k, int encrypt)
{
    // Compute number of blocks
    const SIZE nblocks_c  = 1 + mlen / BLOCK_SIZE;
    const SIZE nblocks_m  = (mlen % BLOCK_SIZE) ? nblocks_c : nblocks_c - 1;
    const SIZE nblocks_ad = 1 + (CRYPTO_NPUBBYTES + adlen) / BLOCK_SIZE;
    const SIZE nb_it = (nblocks_c + 1 > nblocks_ad - 1) ? nblocks_c + 1 : nblocks_ad - 1;

    // Storage for the expanded key L
    BYTE expanded_key[BLOCK_SIZE] = {0};
    memcpy(expanded_key, k, CRYPTO_KEYBYTES);
    permutation(expanded_key);

    // Buffers for storing previous, current and next mask
    BYTE mask_buffer_1[BLOCK_SIZE] = {0};
    BYTE mask_buffer_2[BLOCK_SIZE] = {0};
    BYTE mask_buffer_3[BLOCK_SIZE] = {0};
    memcpy(mask_buffer_2, expanded_key, BLOCK_SIZE);

    BYTE* previous_mask = mask_buffer_1;
    BYTE* current_mask = mask_buffer_2;
    BYTE* next_mask = mask_buffer_3;

    // Buffer to store current ciphertext/AD block
    BYTE buffer[BLOCK_SIZE];

    // Tag buffer and initialization of tag to zero
    BYTE tag_buffer[BLOCK_SIZE] = {0};
    get_ad_block(tag_buffer, ad, adlen, npub, 0);

    SIZE offset = 0;
    for(SIZE i = 0; i < nb_it; ++i) {
        // Compute mask for the next message
        lfsr_step(next_mask, current_mask);
        if(i < nblocks_m) {
            // Compute ciphertext block
            memcpy(buffer, npub, CRYPTO_NPUBBYTES);
            memset(buffer + CRYPTO_NPUBBYTES, 0, BLOCK_SIZE - CRYPTO_NPUBBYTES);
            xor_block(buffer, current_mask, BLOCK_SIZE);
            xor_block(buffer, next_mask, BLOCK_SIZE);
            permutation(buffer);
            xor_block(buffer, current_mask, BLOCK_SIZE);
            xor_block(buffer, next_mask, BLOCK_SIZE);
            const SIZE r_size = (i == nblocks_m - 1) ? mlen - offset : BLOCK_SIZE;
            xor_block(buffer, m + offset, r_size);
            memcpy(c + offset, buffer, r_size);
        }


        if(i > 0 && i <= nblocks_c) {
            // Compute tag for ciphertext block
            get_c_block(buffer, encrypt ? c : m, mlen, i - 1);
            xor_block(buffer, previous_mask, BLOCK_SIZE);
            xor_block(buffer, next_mask, BLOCK_SIZE);
            permutation(buffer);
            xor_block(buffer, previous_mask, BLOCK_SIZE);
            xor_block(buffer, next_mask, BLOCK_SIZE);
            xor_block(tag_buffer, buffer, BLOCK_SIZE);
        }

        // If there is any AD left, compute tag for AD block 
        if(i + 1 < nblocks_ad) {
            get_ad_block(buffer, ad, adlen, npub, i + 1);
            xor_block(buffer, next_mask, BLOCK_SIZE);
            permutation(buffer);
            xor_block(buffer, next_mask, BLOCK_SIZE);
            xor_block(tag_buffer, buffer, BLOCK_SIZE);
        }

        // Cyclically shift the mask buffers
        // Value of next_mask will be computed in the next iteration
        BYTE* const temp = previous_mask;
        previous_mask = current_mask;
        current_mask = next_mask;
        next_mask = temp;

        offset += BLOCK_SIZE;
    }
    // Compute tag
    xor_block(tag_buffer, expanded_key, BLOCK_SIZE);
    permutation(tag_buffer);
    xor_block(tag_buffer, expanded_key, BLOCK_SIZE);
    memcpy(tag, tag_buffer, CRYPTO_ABYTES);
}

// Remark: c must be at least mlen + CRYPTO_ABYTES long
int crypto_aead_encrypt(
  unsigned char *c, unsigned long long *clen,
  const unsigned char *m, unsigned long long mlen,
  const unsigned char *ad, unsigned long long adlen,
  const unsigned char *nsec,
  const unsigned char *npub,
  const unsigned char *k)
{
    (void)nsec;
    *clen = mlen + CRYPTO_ABYTES;
    BYTE tag[CRYPTO_ABYTES];
    crypto_aead_impl(c, tag, m, mlen, ad, adlen, npub, k, 1);
    memcpy(c + mlen, tag, CRYPTO_ABYTES);
    return 0;
}

int crypto_aead_decrypt(
  unsigned char *m, unsigned long long *mlen,
  unsigned char *nsec,
  const unsigned char *c, unsigned long long clen,
  const unsigned char *ad, unsigned long long adlen,
  const unsigned char *npub,
  const unsigned char *k)
{
    (void)nsec;
    if(clen < CRYPTO_ABYTES)
        return -1;
    *mlen = clen - CRYPTO_ABYTES;
    BYTE tag[CRYPTO_ABYTES];
    crypto_aead_impl(m, tag, c, *mlen, ad, adlen, npub, k, 0);
    return (constcmp(c + *mlen, tag, CRYPTO_ABYTES) == 0) ? 0 : -1;
}

//---------------------------------- int main  --------------------------------------
unsigned char plaintext[100];
unsigned char key[CRYPTO_KEYBYTES] = {0x66, 0x44, 0x22, 0x11, 0x33, 0x55, 0x77, 0x99, 0xBB, 0xDD, 0xFF, 0xEE, 0xCC, 0xAA, 0x88, 0x66};
unsigned char nonce[CRYPTO_NPUBBYTES] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34, 0x56, 0x78, 0x90, 0x12, 0x34};

unsigned char ciphertext[100 + CRYPTO_ABYTES];
unsigned char decrypted[100];
unsigned char ad[100] = "AssociatedData";
unsigned long long ciphertext_len;
unsigned long long decrypted_len;
int ret;

//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// ---------------------------------- ESP32 Connect -------------------------------
// const char* ssid = "Wifi";
// const char* password = "12345678";
const char* ssid = "Dimasrifky";
const char* password = "dinanfamily";
const char* mqtt_server = "broker.mqtt-dashboard.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-Checker-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("ESP32_Insulin_Status_topic", "Checker Online");
      client.publish("ESP32_Insulin_Pump_topic", "");
      // ... and resubscribe
      client.subscribe("ESP32_Insulin_Checker_topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}



//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
// ---------------------------------- MAIN CODE -----------------------------------
void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    msg[i] = (char)payload[i];
    Serial.print(msg[i]);
  }
  Serial.println();

  if (strcmp(msg, "READ") == 0) {
    // Read distance from HCSR04
    long duration, distance;
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    duration = pulseIn(echoPin, HIGH);
    distance = duration * 0.034 / 2;
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    // Publish the distance as an ASCII string
    char distance_str[100];
    itoa(distance, distance_str, 10);

    // client.publish("ESP32_Insulin_Pump_topic", distance_str);

    plaintext[0] = distance_str[0];
    Serial.print("Plaintext   : ");
    Serial.println((char*) plaintext);
    delay(100);

    ret = crypto_aead_encrypt(ciphertext, &ciphertext_len, plaintext, strlen((char *) plaintext), ad, strlen((char *) ad), NULL, nonce, key);
    if (ret != 0) {
      Serial.println("Error: Encryption failed");
      return;
    }

    // Convert the ciphertext to a string of hexadecimal characters
    char ciphertext_str[(2 * CRYPTO_ABYTES) + 1];
    for (int i = 0; i < ciphertext_len; i++) {
      sprintf(ciphertext_str + (2 * i), "%02X", ciphertext[i]);
    }
    ciphertext_str[2 * ciphertext_len] = '\0';

    // Publish the ciphertext as an ASCII string
    client.publish("ESP32_Insulin_Pump_topic", ciphertext_str);
  }
}

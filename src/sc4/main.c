
#include "stm32f4xx_hal.h"
#include "usb_device.h"
#include "init.h"
#include "tweetnacl.h"
#include "tweetnacl-aux.h"
#include "hardware.h"
#include "utils.h"
#include "stm32f4xx_hal_flash_ex.h"
#include "b58.h"

unsigned char cmd[512];
void show_mac();

void user_tick_handler() {
  // This routine is called once every millisecond by an interrupt
  // service routine.  User ISR code goes here.
}

// Global storage for currently active key
// _ssk is 64 bytes because TweetNaCl keeps a copy of the spk
// appended to the ssk

u8 _ssk[64];
u8 spk[32];
u8 esk[32];
u8 epk[32];

void _loadkey(u8* seed) {
  u8 hash[64];
  crypto_hash(hash, seed, 32);
  memcpy(_ssk, hash, 32);
  memset(_ssk + 32, 0, 32);
  crypto_sign_keypair_from_seed(spk, _ssk);
  crypto_hash(hash, _ssk, 32);
  memcpy(esk, hash, 32);
  spk2epk(epk, spk);
}

void show_current_key() {
  printf("Currently loaded key:\n");
  printf("SPK: ");
  printh(spk, 32);
  newline();
  printf("EPK: ");
  printh(epk, 32);
  newline();
}

int rdp_enabled() {
  FLASH_OBProgramInitTypeDef opts;
  HAL_FLASHEx_OBGetConfig(&opts);
  return opts.RDPLevel != OB_RDP_LEVEL_0;
}

void show_keys() {
  if (rdp_enabled()) {
    printf("Can't display key seeds because read protection is enabled.\n");
  } else {
    printf("Seeds:\n");
    for (int i=0; i<10; i++) {
      printf("%d: ", i);
      printh((u8*)FLASH_USER_START_ADDR + (i*32), 32);
      newline();
    }
  }
  printf("Keys:\n");
  for (int i=0; i<10; i++) {
    printf("%d: ", i);
    printh((u8*)FLASH_USER_START_ADDR + ((i+10)*32), 32);
    newline();
  }
}

void provision(int n);

void loadkey(int n) {
  if (n < 0 || n > 9) {
    printf("Key ID must be between 0 and 9\n");
    show_current_key();
    return;
  }
  u8 *seed = (u8*)FLASH_USER_START_ADDR + (n*32);
  if (seed[0] == 0xFF) provision(n);
  printf("Load key %d\n", n);
  _loadkey(seed);
  show_current_key();
}

void provision(int n) {
  if (n < 0 || n > 9) {
    print("Key ID must be between 0 and 9\n");
    show_current_key();
    return;
  }
  printf("Provisioning key %d...\n", n);
  u8 seeds[20][32];
  memcpy(seeds, (u8*)FLASH_USER_START_ADDR, sizeof(seeds));
  randombytes(seeds[n], 32);
  // High bit zero indicates this key has been provisioned
  seeds[n][0] = seeds[n][0] & 0x7F;
  _loadkey(seeds[n]);
  memcpy(seeds[n+10], spk, 32);
  write_flash((u8*)seeds, 0, sizeof(seeds));
  printf("Done.\n");
}

void sign(u8 *bytes, int cnt) {
  u8 hash[64];
  u8 sig[crypto_hash_BYTES + 64];

  long long unsigned int smlen;
  crypto_hash(hash, bytes, cnt);

  char buf[132];
  sprintf(buf, "Sign: ");
  sprinth(buf+5, hash, 32);
  sprintf(buf+5+64, "         OK?");
  lcd_print(buf);
  
  printf("Hash:\n");
  printh(hash, 32); newline();
  printh(hash+32, 32); newline();
  
  int buttons;
  while (!(buttons = user_buttons())) {}
  if (buttons != 1) {
    set_led(RED);
    lcd_print("\n\\2 Cancelled");
    printf("Cancelled\n");
    delay(1000);
    show_banner();
    set_led(OFF);
    return;
  }
  lcd_print("\n\\2 Signing");
  set_led(GREEN);
  printf("Sigature:\n");
  crypto_sign(sig, &smlen, hash, crypto_hash_BYTES, _ssk);
  printh(sig, 32); newline();
  printh(sig+32, 32); newline();
  printh(sig+64, 32); newline();
  printh(sig+96, 32); newline();
  show_banner();
  set_led(OFF);
}

void diffie_helman(char *pkh) {
  u8 pk[32];
  hex2binary(pk, pkh, 32);
  printf("DH: ");
  printh(pk, 32);
  newline();
  u8 k[32];
  crypto_scalarmult(k, esk, pk);
  printh(k, 32);
  newline();
}

int hash_cnt = 0;

u8 serial_read_wait() {
  while (!serial_available());
  return serial_read();
}

int crypto_hash_stream_read_block(u8* buf) {
  if (!hash_cnt) return 0;
  int i;
  char msg[128];
  sprintf(msg, "Hashing %d bytes", hash_cnt);
  lcd_print(msg);
  print(".");     // Indicate ready to receive a block
  if (hash_cnt>=128) {
    FOR(i, 128) buf[i]=serial_read_wait();
    hash_cnt -= 128;
    return 128;
  } else {
    FOR(i, hash_cnt) buf[i]=serial_read_wait();
    int cnt = hash_cnt;
    hash_cnt = 0;
    return cnt;
  }
}

void hash_data(u8* s) {
  sscanf((char*)s, "%d", &hash_cnt);
  hash_state hs;
  u8 hash[64];
  crypto_hash_stream(hash, &hs);
  show_banner();

  char b58buf[128];  // Actually only need 96 characters worst case
  size_t b58len;
  bool flag = b58enc(b58buf, &b58len, hash, 64);
  if (!flag) sprintf(b58buf, "FAIL");
  printf("%s\n", b58buf);
  b58buf[84]=0;
  lcd_print(b58buf);
}

void randi(u8 *s) {
  int cnt;
  sscanf((char*)s, "%d", &cnt);
  printf("Random %d\n", cnt);
  for (int i=0; i<cnt; i++) printf("%08X ", read_rng());
  printf("\n");
}

void erase_keys() {
  printf("Are you sure you want to erase all of the keys on this device?\n");
  printf("This cannot be undone.  (y/n)\n");
  readln(cmd, sizeof(cmd));
  if (cmd[0] != 'y') {
    printf("Aborted\n");
    return;
  }
  erase_user_flash();
  printf("Done\n");
  show_mac();
}

void enable_rdp() {
  printf("Enabling read protection.\n");
  printf("WARNING: This cannot be undone without losing all\n");
  printf("of the keys currently stored on this device.\n");
  printf("Are you sure you want to proceed? (y/n)\n");

  readln(cmd, sizeof(cmd));
  if (cmd[0] != 'y') {
    printf("Aborted\n");
    return;
  }

  FLASH_OBProgramInitTypeDef opts;
  opts.OptionType = OPTIONBYTE_RDP;
  opts.RDPLevel = OB_RDP_LEVEL_1;
  HAL_FLASH_OB_Unlock();
  HAL_FLASHEx_OBProgram(&opts);
  HAL_FLASH_OB_Launch();
  HAL_FLASH_OB_Lock();
  printf("Read protection has been enabled.\n");
  printf("See the documentation for instructions on how to disable it.\n");
}

void show_mac() {
  printf("SC4 HSM UID: ");
  printh((u8 *)STM32_UUID, 12);
  newline();
  if (*(u8*)FLASH_USER_START_ADDR == 0xFF) {
    printf("No keys have been provisioned!\n");
    printf("Automatically provisioning a default key\n");
    loadkey(0);
  } else {
    show_current_key();
  }
  printf("Read protection is %s\n", rdp_enabled() ? "ENABLED" : "DISABLED");
  printf("Type ? for a list of commands\n");
}

void help() {
  print("k: Show available keys\n");
  print("l[n]: Load key N\n");
  print("P[n]: Provision key N\n");
  print("R: Enable read protection\n");
  print("s[string]: Sign string with currently loaded key\n");
  print("d[key]: Generate a diffie-hellman key\n");
  print("h[N]: Compute the SHA512 hash of N bytes of data\n");
  print("r[N]: Display N raw random numbers\n");
  print("p[string]: Print string on the built-in display\n");
  print("n: Show random noise on built-in display\n");
  print("m: Moire pattern demo\n");
  print("E: Erase all keys\n");
  print("S: Run TinyScheme\n");
  print("X: System reset\n");
  print("0-7: Turn LED on/off\n");
}

void usb_hid_init();
void usb_cdc_init();
void u2f_channel_process_ready();

void show_u2f_banner() {
  lcd_print("\n\\2 SC4-U2F");
}

void u2f() {
  usb_hid_init();
  show_u2f_banner();
  while (1) {
    if (user_buttons()) {
      for (int i=0; i<10; i++) while (user_buttons());
      usb_cdc_init();
      show_banner();
      return;
    }
    u2f_channel_process_ready();
  }
}

void loop() {
  while(!serial_available()) {
    if (user_buttons()) {
      for (int i=0; i<10; i++) while (user_buttons());
      u2f();
    }
  }
  
  set_led(YELLOW);
  int cnt = readln(cmd, sizeof(cmd));  
  set_led(GREEN);
  delay(100);
  set_led(OFF);
  switch (cmd[0]) {
  case '\0': show_mac(); break;
  case 'k': show_keys() ; break;
  case 'l': loadkey(cmd[1] - '0'); break;
  case 'P': provision(cmd[1] - '0'); break;
  case 'R': enable_rdp(); break;
  case 's': sign(cmd + 1, cnt - 1); break;
  case 'd': diffie_helman((char *)(cmd + 1)); break;
  case 'h': hash_data(cmd +1); break;
  case 'r': randi(cmd+1); break;
  case 'p': lcd_print((char *)(cmd+1)); break;
  case 'n': lcd_noise(); break;
  case 'm': moire(); break;
  case 'E': erase_keys(); break;
  case 'S': scheme_main(); break;
  case 'X': system_reset(); break;
  case 'u' : u2f(); break;
  case '0'...'7': set_led(cmd[0]-'0'); break;
  case '?' : help(); break;
  default: printf("Error\n");
  }
  printf("Ready\n");
}

int main() {
  sc4_hsm_init();
  set_led(YELLOW);
  lcd_print("\n Initializing...");
  _loadkey((u8*)FLASH_USER_START_ADDR);
  show_banner();
  set_led(GREEN);
  delay(100);
  set_led(OFF);
  usb_cdc_init();
  while (1) loop();
}

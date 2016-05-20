
#include "stm32f4xx_hal.h"
#include "usb_device.h"
#include "utils.h"
#include "init.h"
#include "tweetnacl.h"
#include "hardware.h"
#include "utils.h"

unsigned char cmd[512];

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

void show_keys() {
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
  print("Provisioning key %d...\n", n);
  u8 seeds[20][32];
  memcpy(seeds, (u8*)FLASH_USER_START_ADDR, sizeof(seeds));
  randombytes(seeds[n], 32);
  // High bit zero indicates this key has been provisioned
  seeds[n][0] = seeds[n][0] & 0x7F;
  _loadkey(seeds[n]);
  memcpy(seeds[n+10], spk, 32);
  write_flash((u8*)seeds, 0, sizeof(seeds));
  show_keys();
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

void randi(u8 *s) {
  int cnt;
  sscanf((char*)s, "%d", &cnt);
  printf("Random %d\n", cnt);
  for (int i=0; i<cnt; i++) printf("%08X ", read_rng());
  printf("\n");
}

// RDP = ReaD Protection
// AA = Level 0, no protection
// CC = Level 2, full protection (irreversible)
// Any other value (canonical 55) = Level 1, partial protection

#include "stm32f4xx_hal_flash_ex.h"

void rdp(u8 c) {

  FLASH_OBProgramInitTypeDef opts;

  HAL_FLASHEx_OBGetConfig(&opts);

  int rdpl = opts.RDPLevel;
  int _rdpl = (rdpl == OB_RDP_LEVEL_0 ? 0 :
	       (rdpl == OB_RDP_LEVEL_1 ? 1 :
		(rdpl == OB_RDP_LEVEL_2 ? 2 :
		 -1)));

  printf("Current RDP level: %x (%d)\n", rdpl, _rdpl);

  if (!c) return;

  int level;
  switch (c) {
  case '0': level = OB_RDP_LEVEL_0; break;
  case '1': level = OB_RDP_LEVEL_1; break;
  default:
    printf("RDP level must be 0 or 1\n");
    return;
  }

  printf("Setting RDP %x\n", level);

  int status = HAL_FLASH_OB_Unlock();
  if (status != HAL_OK) {
    printf("HAL_FLASH_OB_Unlock failed");
    return;
  }

  opts.OptionType = OPTIONBYTE_RDP;
  opts.RDPLevel = level;
  status = HAL_FLASHEx_OBProgram(&opts);
  if (status != HAL_OK) {
    printf("HAL_FLASH_OBProgram failed");
    return;
  }
  
  printf("Setting RDP level.  DO NOT DISCONNECT!!!\n");
  lcd_print("\\2  DO NOT\nDISCONNECT");
  set_led(RED);
  status = HAL_FLASH_OB_Launch();
  if (status == HAL_OK) {
    printf("Done.  It is now safe to disconnect.\n");
    show_banner();
    set_led(GREEN);
    delay(500);
    set_led(OFF);
  } else {
    printf("*** FAILED!!! ***\n");
  }

  HAL_FLASHEx_OBGetConfig(&opts);
  printf("New RDP level: %x\n", opts.RDPLevel);

  status = HAL_FLASH_OB_Lock();
  if (status != HAL_OK) printf("HAL_FLASH_OB_Lock failed!\n");
}

void system_reset() {
  NVIC_SystemReset();
}

void show_mac() {
  printf("SC4 HSM UID: ");
  printh((u8 *)STM32_UUID, 12);
  newline();
  if (*(u8*)FLASH_USER_START_ADDR == 0xFF) {
    printf("No keys have not been provisioned!\n");
    printf("Automatically provisioning a default key\n");
    loadkey(0);
  } else {
    show_current_key();
  }
  printf("Type ? for a list of commands\n");
}

void help() {
  print("l[n]: Load key N\n");
  print("P[n]: Provision key N\n");
  print("k: Show available keys\n");
  print("s[string]: Sign string with currently loaded key\n");
  print("d[key]: Generate a diffie-hellman key\n");
  print("r[N]: Display N raw random numbers\n");
  print("p[string]: Print string on the built-in display\n");
  print("n: Show random noise on built-in display\n");
  print("m: Moire pattern demo\n");
  print("E: Erase all keys\n");
  print("S: Run TinyScheme\n");
  print("R: System reset\n");
  print("0-7: Turn LED on/off\n");
  print("X[n]: Enable read protection at level n.\n");
  print("*** Warning! Enabling/disabling read protection can be dangerous!\n");
  print("*** Read the documentation BEFORE attempt to use this feature!\n");
}

void loop() {
  int cnt = readln(cmd, sizeof(cmd));
  
  switch (cmd[0]) {
  case '\0': show_mac(); break;
  case '0'...'7': set_led(cmd[0]-'0'); break;
  case 'k': show_keys() ; break;
  case 'l': loadkey(cmd[1] - '0'); break;
  case 'P': provision(cmd[1] - '0'); break;
  case 's': sign(cmd + 1, cnt - 1); break;
  case 'd': diffie_helman((char *)(cmd + 1)); break;
  case 'r': randi(cmd+1); break;
  case 'p': lcd_print((char *)(cmd+1)); break;
  case 'n': lcd_noise(); break;
  case 'm': moire(); break;
  case 'E': erase_user_flash(); break;
  case 'S': scheme_main(); break;
  case 'R': system_reset(); break;
  case 'X': rdp(cmd[1]); break;
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
  delay(500);
  set_led(OFF);
  while (1) loop();
}

/*
	umd_dump.prx
 */

// headerの読み込み
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <pspsdk.h>
#include <pspreg.h>
#include <kubridge.h>
#include <pspctrl.h>
#include <pspumd.h>
#include <pspthreadman_kernel.h>
#include <psprtc.h>

#include <systemctrl.h>
#include <systemctrl_se.h>
#include <pspkernel.h>
#include <kubridge.h>
#include <psploadexec_kernel.h>

#include "main.h"

// モジュールの定義
PSP_MODULE_INFO( "UMD_DUMP", PSP_MODULE_KERNEL, 0, 0 );

// マクロの定義
#define CHEACK_KEY (PSP_CTRL_SELECT | PSP_CTRL_START | PSP_CTRL_UP | PSP_CTRL_RIGHT | \
    PSP_CTRL_DOWN | PSP_CTRL_LEFT | PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_TRIANGLE | \
    PSP_CTRL_CIRCLE | PSP_CTRL_CROSS | PSP_CTRL_SQUARE | PSP_CTRL_NOTE | PSP_CTRL_HOME)

#define XOR_KEY (PSP_CTRL_DISC)

#define MAX_NUMBER_OF_THREADS 64

#define INI_PATH "/umd_dump.ini"
#define EBOOT_PATH "ms0:/psp/game/iso_tool/"

int scePower_0442D852(int unknown);
int get_registry_value(const char *dir, const char *name, int *val);
int set_registry_value(const char *dir, const char *name, int *val);

int module_start( SceSize arglen, void *argp );
int module_stop( void );

int main_thread( SceSize arglen, void *argp );
void main_menu(void);
int yes_no(char *str, int boot);
void ThreadsStatChange( bool stat, SceUID thlist[], int thnum );
void get_button(SceCtrlData  *data);
void wait_button_up(void);
unsigned int read_config(const char *file_name);
unsigned int opt_check(const char* word);
int read_line_file(SceUID fp, char* line, int num);
static void *p_malloc(u32 size);
static s32 p_mfree(void *ptr);
void boot();
int get_file_data(int* pos, int* size, int* size_pos, const char* buf, char *name);

SEConfig config;
int stop_flag;
SceUID st_thlist_first[MAX_NUMBER_OF_THREADS];
int st_thnum_first;
SceUID thlist[MAX_NUMBER_OF_THREADS];
int thnum;

SceCtrlData data;
char eboot[256];

int main_thread( SceSize arglen, void *argp )
{
  unsigned int key;
  char *temp;
  char path[256];

  strcpy(path, argp);
  temp = strrchr(path, '/');
  temp[0] = '\0';
  strcat(path, INI_PATH);

  key = read_config(path);

  while(stop_flag == 0){
    sceKernelDelayThread( 50000 );
    sceCtrlPeekBufferPositive( &data, 1 );
    data.Buttons ^= XOR_KEY;
    if((data.Buttons & key) == key)
      main_menu();
  }

  return 0;
}

#define PRINT_SCREEN()                                                           \
    pspDebugScreenInit();                                                           \
    pspDebugScreenSetXY(10,10);                                                     \
    pspDebugScreenKprintf("umd_dump.prx Ver. 1.31            by takka");            \
    pspDebugScreenSetXY(10,12);                                                     \
    pspDebugScreenKprintf("TYPE %s, ID %s, SIZE %d, BUFFER %d", umd_type, umd_id, umd_sector, max_buf); \
    pspDebugScreenSetXY(10,13);                                                     \
    pspDebugScreenKprintf("NAME %s", out_path);                                     \

void main_menu(void)
{
  SceUID fd_i;
  SceUID fd_o;
  char *buf;
  int time_old;
  int time = 0;
  char umd_type[16];
  char umd_id[11];
  char umd_data[48];
  int umd_sector;
  int now_sector;
  char out_path[256];
  char ren_path[256];
  char msg[256];
  int read_sec;
  int ret;
  int i;
  int type;
  int max_buf = 512;

  u64 start_tick;
  u64 now_tick;
  u64 old_tick = 0;
  pspTime date1;
  pspTime date2;
  pspTime date3;
  int spd;

  int pos;
  int size;
  int size_pos;
  int rewrite = 0;

  // ボタンから手が離れるまで待つ
  wait_button_up();

  if(sceUmdCheckMedium() == 0)
    return;

  sceUmdActivate(1, "disc0:");
  sceUmdWaitDriveStat(PSP_UMD_READY);

//  sceUmdDeactivate(1, "disc0:");
//  sceIoUnassign("umd:");
//  sceIoAssign("flash0", "lflash0:0,0", "flashfat0:", IOASSIGN_RDWR, NULL, 0);

  // メモリ確保
  do
  {
    buf = p_malloc(max_buf * 0x800);
    if(buf == NULL)
    {
      max_buf -= 2;
      if(max_buf <= 0)
        return;
    }
  }
  while(buf == NULL);

  // UMD情報取得
  fd_i = sceIoOpen("disc0:/UMD_DATA.BIN", PSP_O_RDONLY, 0777);
  sceIoRead(fd_i, umd_data, 48);
  sceIoClose(fd_i);

  i = 0x21;
  type = 0;
  while(i < 0x24)
  {
    if(umd_data[i] == 'G')
      type |= 0x01;
    if(umd_data[i] == 'V')
      type |= 0x02;
    if(umd_data[i] == 'A')
      type |= 0x04;
    i++;
  }

  if(type == 0x01)
  {
    strcpy(umd_type,"GAME");
    strcpy(out_path, "ms0:/iso/");
  }
  else
  {
    strcpy(umd_type,"VIDEO/AUDIO");
    strcpy(out_path, "ms0:/iso/video/");
    switch(type)
    {
      case 0x03:
        memcpy(&umd_data[0x21], "V\0\0", 3);
        break;
      case 0x05:
        memcpy(&umd_data[0x21], "A\0\0", 3);
        break;
      case 0x07:
        memcpy(&umd_data[0x21], "VA\0", 3);
        break;
    }

    ///UMD_DATA.BIN のセクタ番号を取得
    fd_i = sceIoOpen("umd:", PSP_O_RDONLY, 0777);
    sceIoLseek(fd_i, 20, SEEK_SET);
    sceIoRead(fd_i, buf, 44);
    sceIoClose(fd_i);
    get_file_data(&pos, &size, &size_pos, buf, "UMD_DATA.BIN");

  }

  fd_i = sceIoOpen("umd:", PSP_O_RDONLY, 0777);
  sceIoLseek(fd_i, 16, SEEK_SET);
  sceIoRead(fd_i, buf, 1);
  sceIoClose(fd_i);


  // 0x800Cから9byteがメディアタイプ
//  memcpy(umd_type, &buf[0x008], 9);
//  umd_type[9] = '\0';

  // 0x50から4byteがセクタ数
  memcpy(&umd_sector, &buf[0x050], 4);

  // 0x8373から10byteがUMD ID
  memcpy(umd_id, &buf[0x373], 10);
  umd_id[10] = '\0';

  strcpy(ren_path, out_path);
  strcat(ren_path, "01234565789012345678901234567890123456789012345678901234567890123456789");

  strcat(out_path, umd_id);
  strcat(out_path, ".iso");

  // XMBの動作停止
  sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thlist, MAX_NUMBER_OF_THREADS, &thnum );
  ThreadsStatChange( false, thlist, thnum );

  // 画面表示
  PRINT_SCREEN();

  ret = yes_no("START ?", 1);

  if(ret == 0)
  {
    // XMBの動作復旧
    ThreadsStatChange( true, thlist, thnum );
    // メモリ解放
    p_mfree(buf);
    return;
  }
  else if(ret == -1)
  {
    // XMBの動作復旧
    ThreadsStatChange( true, thlist, thnum );
    p_mfree(buf);
    boot();
    return;
  }

  // オートパワーオフの設定を無効にする
  get_registry_value("/CONFIG/SYSTEM/POWER_SAVING", "suspend_interval", &time_old);
  set_registry_value("/CONFIG/SYSTEM/POWER_SAVING", "suspend_interval", &time);

  // XMBの動作停止
//  sceKernelGetThreadmanIdList( SCE_KERNEL_TMID_Thread, thlist, MAX_NUMBER_OF_THREADS, &thnum );
  ThreadsStatChange( true, thlist, thnum );
  sceKernelSuspendAllUserThreads();

  // 再度画面表示
  PRINT_SCREEN();

  if((type > 0x01) && (type & 0x01))
    rewrite = yes_no("Rewrite UMD_DATA.BIN ? (GVA/GV/GA -> VA/V/A)", 0);

  fd_i = sceIoOpen("umd:", PSP_O_RDONLY, 0777);
  fd_o = sceIoOpen(ren_path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

  now_sector = 0;
  sceRtcGetCurrentTick(&start_tick);
  old_tick = start_tick;
  read_sec = sceIoRead(fd_i, buf, max_buf);

  while(read_sec > 0)
  {
    ret = sceIoWrite(fd_o, buf, read_sec * 0x800);
    if(ret != read_sec * 0x800)
    {
      pspDebugScreenSetXY(10,19);
      pspDebugScreenKprintf("Write ERROR !");
      ret = 0;
      goto LABEL_ERR;
    }
    now_sector += read_sec;
    read_sec = sceIoRead(fd_i, buf, max_buf);
    if((read_sec < 0) && (now_sector < umd_sector))
    {
      pspDebugScreenSetXY(10,19);
      pspDebugScreenKprintf("Read ERROR !");
      ret = 0;
      goto LABEL_ERR;
    }

    sprintf(msg, "%d / %d Sector %dPercent End", now_sector, umd_sector, (now_sector * 100 /umd_sector));
    pspDebugScreenSetXY(10,15);
    pspDebugScreenKprintf(msg);

    sceRtcGetCurrentTick(&now_tick);
    now_tick -= start_tick;
    spd = max_buf * 2 * 1000 / ((now_tick - old_tick) / 1000);
    old_tick = now_tick;
    sceRtcSetTick(&date1, &now_tick);
    now_tick = now_tick * umd_sector / now_sector;
    sceRtcSetTick(&date2, &now_tick);
    now_tick -= old_tick;
    sceRtcSetTick(&date3, &now_tick);
    sprintf(msg, "NOW %02d:%02d:%02d / EST %02d:%02d:%02d / ETA %02d:%02d:%02d",
        date1.hour, date1.minutes, date1.seconds,
        date2.hour, date2.minutes, date2.seconds,
        date3.hour, date3.minutes, date3.seconds);
    pspDebugScreenSetXY(10,16);
    pspDebugScreenKprintf(msg);

    sprintf(msg, "SPEED %4dKB/s", spd);
    pspDebugScreenSetXY(10,17);
    pspDebugScreenKprintf(msg);

    get_button(&data);
    if((data.Buttons) == PSP_CTRL_CROSS)
    {
      if(yes_no("STOP ?", 0) == 1)
      {
        ret = 0;
        goto LABEL_ERR;
      }
    }
  }

  ret = 1;

  if(rewrite == 1)
  {
    sceIoLseek(fd_o, pos, SEEK_SET);
    sceIoWrite(fd_o, umd_data, 48);
  }

LABEL_ERR:
  sceIoClose(fd_i);
  sceIoClose(fd_o);

  if(ret == 1)
  {
    pspDebugScreenSetXY(10,19);
    pspDebugScreenKprintf("Complete");
    sceIoRemove(out_path);
    sceIoRename(ren_path, out_path);
  }
  else
    sceIoRemove(ren_path);

  // メモリ解放
  p_mfree(buf);

  // オートパワーオフの設定を元に戻す
  set_registry_value("/CONFIG/SYSTEM/POWER_SAVING", "suspend_interval", &time_old);

  while(yes_no("REBOOT", 0) == 0);

  // XMBの動作復旧
  ThreadsStatChange( true, thlist, thnum );

  // 動作が完全に復旧しないのでリブートを行う
  scePower_0442D852(50000);
}

int yes_no(char *str, int boot)
{
  int ret;

  pspDebugScreenSetXY(10,21);
  pspDebugScreenKprintf(str);

  pspDebugScreenSetXY(10,22);
  pspDebugScreenKprintf("YES ... O / NO ... X");

  if(boot == 1)
  {
    pspDebugScreenSetXY(10,23);
    pspDebugScreenKprintf("[] .. BOOT %sEBOOT.PBP", eboot);
  }


  wait_button_up();

  ret = 0;
  while(1)
  {
    get_button(&data);
    if((data.Buttons) == PSP_CTRL_CROSS)
    {
      ret = 0;
      break;
    }
    else if((data.Buttons) == PSP_CTRL_CIRCLE)
    {
      ret = 1;
      break;
    }
    else if((data.Buttons) == PSP_CTRL_SQUARE)
    {
      if(boot == 1)
      {
        ret = -1;
        break;
      }
    }
  }

  pspDebugScreenSetXY(10,21);
  pspDebugScreenKprintf("                                                             ");

  pspDebugScreenSetXY(10,22);
  pspDebugScreenKprintf("                    ");
  wait_button_up();

  return ret;
}

void get_button(SceCtrlData *data)
{
  sceCtrlPeekBufferPositive( data, 1 );
  data->Buttons &= CHEACK_KEY;
}

void wait_button_up(void)
{
  while((data.Buttons & CHEACK_KEY) != 0)
  {
    sceCtrlPeekBufferPositive( &data, 1 );
  }
}


int module_start( SceSize arglen, void *argp )
{
  SceUID thid;

  stop_flag = 0;
  thid = sceKernelCreateThread( "UMD_DUMP", main_thread, 30, 0x6000, PSP_THREAD_ATTR_NO_FILLSTACK, 0 );
  if( thid ) sceKernelStartThread( thid, arglen, argp );

  return 0;
}

int module_stop( void )
{
  stop_flag = 1;
  return 0;
}

int read_line_file(SceUID fp, char* line, int num)
{
  char buff[num];
  char* end;
  int len;
  int tmp;

  tmp = 0;
  len = sceIoRead(fp, buff, num);
  // エラーの場合
  if(len == 0)
    return -1;

  end = strchr(buff, '\n');

  // \nが見つからない場合
  if(end == NULL)
  {
    buff[num - 1] = '\0';
    strcpy(line, buff);
    return len;
  }

  end[0] = '\0';
  if((end != buff) && (end[-1] == '\r'))
  {
    end[-1] = '\0';
    tmp = -1;
  }

  strcpy(line, buff);
  sceIoLseek(fp, - len + (end - buff) + 1, SEEK_CUR);
  return end - buff + tmp;
}

void ThreadsStatChange( bool stat, SceUID thlist[], int thnum )
{
  int ( *request_stat_func )( SceUID ) = NULL;
  int i, j;
  SceUID selfid = sceKernelGetThreadId();

  if( stat ){
    request_stat_func = sceKernelResumeThread;
  } else{
    request_stat_func = sceKernelSuspendThread;
  }

  SceKernelThreadInfo status;

  for( i = 0; i < thnum; i++ ){
    bool no_target = false;
    for( j = 0; j < st_thnum_first; j++ ){
      if( thlist[i] == st_thlist_first[j] || selfid == thlist[i] ){
        no_target = true;
        break;
      }
    }

    sceKernelReferThreadStatus(thlist[i], &status);
    if( ! no_target ) ( *request_stat_func )( thlist[i] );
  }
}


unsigned int read_config(const char * file_name)
{
  SceUID fp;
  SceIoStat stat;
  char line[1024];
  char buffer[1024];
  int ptr = 0;
  int buf_ptr = 0;
  int button = 0;

  sceIoGetstat(file_name, &stat);
  fp = sceIoOpen(file_name, PSP_O_RDONLY, 0777);
  if(fp != 0)
  {
    read_line_file(fp, line, 256);
    if(kuKernelInitApitype() == PSP_INIT_APITYPE_VSH1)
      read_line_file(fp, eboot, 256);
    else
      read_line_file(fp, line, 256);
    read_line_file(fp, eboot, 256);
    sceIoClose(fp);

    while(line[ptr] != 0)
    {
      buf_ptr = 0;
      while(line[ptr] != 0)
      {
        if(line[ptr] != ' ')
        {
          buffer[buf_ptr] = line[ptr];
          if((line[ptr] == '|') || (line[ptr] == 0x0d) || (line[ptr] == '\0'))
          {
            ptr++;
            break;
          }
          buf_ptr++;
        }
        ptr++;
      }
      buffer[buf_ptr] = '\0';
      button |= opt_check(buffer);
    }
  }

  if(button == 0)
  {
    if(kuKernelInitApitype() == PSP_INIT_APITYPE_VSH1)
      button =PSP_CTRL_HOME;
    else
      button =PSP_CTRL_NOTE;
  }

  if(eboot[0] == '\0')
    strcpy(eboot, EBOOT_PATH);

  return button;
}

unsigned int opt_check(const char* word)
{
  unsigned int button_data[] =
  {
      PSP_CTRL_CIRCLE,
      PSP_CTRL_CROSS,
      PSP_CTRL_SQUARE,
      PSP_CTRL_TRIANGLE,
      PSP_CTRL_UP,
      PSP_CTRL_DOWN,
      PSP_CTRL_RIGHT,
      PSP_CTRL_LEFT,
      PSP_CTRL_RTRIGGER,
      PSP_CTRL_LTRIGGER,
      PSP_CTRL_VOLUP,
      PSP_CTRL_VOLDOWN,
      PSP_CTRL_SCREEN,
      PSP_CTRL_NOTE,
      PSP_CTRL_SELECT,
      PSP_CTRL_START,
      PSP_CTRL_DISC,
      PSP_CTRL_HOLD,
      PSP_CTRL_WLAN_UP,
      PSP_CTRL_HOME,
      0
  };

  char* button_name[] =
  {
      "PSP_CTRL_CIRCLE",
      "PSP_CTRL_CROSS",
      "PSP_CTRL_SQUARE",
      "PSP_CTRL_TRIANGLE",
      "PSP_CTRL_UP",
      "PSP_CTRL_DOWN",
      "PSP_CTRL_RIGHT",
      "PSP_CTRL_LEFT",
      "PSP_CTRL_RTRIGGER",
      "PSP_CTRL_LTRIGGER",
      "PSP_CTRL_VOLUP",
      "PSP_CTRL_VOLDOWN",
      "PSP_CTRL_SCREEN",
      "PSP_CTRL_NOTE",
      "PSP_CTRL_SELECT",
      "PSP_CTRL_START",
      "PSP_CTRL_DISC",
      "PSP_CTRL_HOLD",
      "PSP_CTRL_WLAN_UP",
      "PSP_CTRL_HOME",
      ""
  };

  int num = 0;

  while(button_data[num] != 0)
  {
    if(strcasecmp(word, button_name[num]) == 0)
      return button_data[num];
    num++;
  }
  return 0;
}

static void *p_malloc(u32 size)
{
  u32 *p;
  SceUID h_block;

  if(size == 0)
    return NULL;

  h_block = sceKernelAllocPartitionMemory(2, "block", 0, size + sizeof(h_block), NULL);

  if(h_block <= 0)
    return NULL;

  p = (u32 *)sceKernelGetBlockHeadAddr(h_block);
  *p = h_block;

  return (void *)(p + 1);
}

static s32 p_mfree(void *ptr)
{
  return sceKernelFreePartitionMemory((SceUID)*((u32 *)ptr - 1));
}

int get_registry_value(const char *dir, const char *name, int *val)
{
  int ret = 0;
  struct RegParam reg;
  REGHANDLE h;

  memset(&reg, 0, sizeof(reg));
  reg.regtype = 1;
  reg.namelen = strlen("/system");
  reg.unk2 = 1;
  reg.unk3 = 1;
  strcpy(reg.name, "/system");
  if(sceRegOpenRegistry(&reg, 2, &h) == 0)
  {
    REGHANDLE hd;
    if(!sceRegOpenCategory(h, dir, 2, &hd))
    {
      REGHANDLE hk;
      unsigned int type, size;

      if(!sceRegGetKeyInfo(hd, name, &hk, &type, &size))
      {
        if(!sceRegGetKeyValue(hd, hk, val, 4))
        {
          ret = 1;
          sceRegFlushCategory(hd);
        }
      }
      sceRegCloseCategory(hd);
    }
    sceRegFlushRegistry(h);
    sceRegCloseRegistry(h);
  }

  return ret;
}

int set_registry_value(const char *dir, const char *name, int *val)
{
  int ret = 0;
  struct RegParam reg;
  REGHANDLE h;

  memset(&reg, 0, sizeof(reg));
  reg.regtype = 1;
  reg.namelen = strlen("/system");
  reg.unk2 = 1;
  reg.unk3 = 1;
  strcpy(reg.name, "/system");
  if(sceRegOpenRegistry(&reg, 2, &h) == 0)
  {
    REGHANDLE hd;
    if(!sceRegOpenCategory(h, dir, 2, &hd))
    {
      if(!sceRegSetKeyValue(hd, name, val, 4))
      {
        ret = 1;
        sceRegFlushCategory(hd);
      }
      sceRegCloseCategory(hd);
    }
    sceRegFlushRegistry(h);
    sceRegCloseRegistry(h);
  }

  return ret;
}

void boot()
{
  char path[512];
  struct SceKernelLoadExecVSHParam param;
  SceIoStat stat;

  memset(&param, 0, sizeof(param));
  param.size = sizeof(param);
  param.args = strlen(eboot)+1;
  param.argp = eboot;
  param.key = "game";

  strcpy(path, eboot);
  strcat(path, "EBOOT.PBP");


  if(sceIoGetstat(path, &stat) >= 0)
    sctrlKernelLoadExecVSHMs1(path, &param);
}

int get_file_data(int* pos, int* size, int* size_pos, const char* buf, char *name)
{
  int ptr = 0;
  int len;
  int ret;

  len = strlen(name) - 1;

  while(strncasecmp(&buf[ptr], &name[1], len) != 0)
  {
    while(buf[ptr++] != name[0])
      if(ptr > 44 * 0x800)
        return -1;
  }

  ptr--;

  // ファイル名 - 0x1f にファイル先頭アドレス
  memcpy(pos, &buf[ptr - 0x1f], 4);
  *pos *= 0x800;

  // ファイル名 - 0x17 にファイルサイズ
  memcpy(size, &buf[ptr - 0x17], 4);

  // ファイルサイズの位置
  *size_pos = 20 * 0x800 + ptr - 0x17;

  ret = *pos;

  return ret;
}

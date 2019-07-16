//{#pragma region Includes
  #include "includes/core.h"
  #include "includes/definitions.h"
  #include "includes/structures.h"
  #include "includes/declarations.h"
//}#pragma endregion Includes

//{#pragma region Odroid
  static odroid_gamepad_state gamepad;
  odroid_battery_state battery_state;
//}#pragma endregion Odroid

//{#pragma region Global
  int STEP = 1;
  int OPTION = 0;
  bool SAVED = false;
  int8_t USER;
  bool RESTART = false;
//}#pragma endregion Global

//{#pragma region Emulator and Directories
  char EMULATORS[COUNT][30] = {
    "THEME",
    "NINTENDO ENTERTAINMENT SYSTEM",
    "NINTENDO GAME BOY",
    "NINTENDO GAME BOY COLOR",
    "SEGA MASTER SYSTEM",
    "SEGA GAME GEAR",
    "COLECOVISION",
    "SINCLAIR ZX SPECTRUM 48K",
    "ATARI 2600",
    "ATARI 7800",
    "COMMODORE 64",
    "NEC TURBOGRAFX 16"
  };

  char DIRECTORIES[COUNT][10] = {
    "",
    "nes",
    "gb",
    "gbc",
    "sms",
    "gg",
    "col",
    "spectrum",
    "a26",
    "a78",
    "c64",
    "nec"
  };
//}#pragma endregion Emulator and Directories

//{#pragma region Buffer
  unsigned short buffer[40000];
//}#pragma endregion Buffer

/*
  APPLICATION
*/
//{#pragma region Main
  void app_main(void)
  {
    nvs_flash_init();
    odroid_system_init();

    int startHeap = esp_get_free_heap_size();
    printf("A HEAP:0x%x\n", startHeap); 

    // Audio
    odroid_audio_init(16000);
    odroid_settings_Volume_set(4);   

    // Display
    ili9341_init();  

    // Joystick
    odroid_input_gamepad_init();

    // Battery
    odroid_input_battery_level_init();    

    // Theme
    get_theme();
    GUI = THEMES[USER];
    SYSTEMS[0].x = GAP/3;
    for(int n = 1; n < COUNT; n++) {
      if(n == 1) {
        SYSTEMS[n].x = GAP/3+NEXT;
      } else if(n == 2) {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP);
      } else {
        SYSTEMS[n].x = GAP/3+NEXT+(GAP*(n-1));
      }
    };  

    ili9341_prepare();
    ili9341_clear(0);

    printf("==============\n%s\n==============\n", "RETRO ESP32");
    switch(esp_reset_reason()) {
      case ESP_RST_POWERON:
        RESTART = false;
      break;
      case ESP_RST_SW:
        RESTART = true;
      break;
      default:
        RESTART = false;
      break;
    }
    //RESTART ? restart() : splash();

    draw_background();
    draw_systems();
    draw_text(16,16,EMULATORS[STEP],false,true);

    STEP == 0 ? draw_themes() : get_files();
    if(STEP != 0) {animate(1);}

    xTaskCreate(launcher, "launcher", 8192, NULL, 5, NULL);
  }
//}#pragma endregion Main

/*
  METHODS
*/
//{#pragma region Debounce
  void debounce(int key) {
    draw_battery();
    while (gamepad.values[key]) odroid_input_gamepad_read(&gamepad);    
  }
//}#pragma endregion Debounce

//{#pragma region Text
  int get_letter(char letter) {
    int dx = 0;
    char upper[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!-'&?.,/()[] ";
    char lower[] = "abcdefghijklmnopqrstuvwxyz0123456789!-'&?.,/()[] ";
    for(int n = 0; n < strlen(upper); n++) {
      if(letter == upper[n] || letter == lower[n]) {
        return letter != ' ' ? n * 5 : 0;
        break;
      }
    }
    return dx;
  }

  void draw_text(short x, short y, char *string, bool ext, bool current) {
    int length = !ext ? strlen(string) : strlen(string)-(strlen(DIRECTORIES[STEP])+1);
    int size = 5;
    for(int n = 0; n < length; n++) {                                     
      int dx = get_letter(string[n]);
      int i = 0;       
      for(int r = 0; r < (size); r++) {
        if(string[n] != ' ') {
          for(int c = dx; c < (dx+size); c++) {
            buffer[i] = characters[r][c] == 0 ? GUI.bg : current ? WHITE : GUI.fg;
            i++;
          }                                            
        }                                       
      }
      if(string[n] != ' ') {
        ili9341_write_frame_rectangleLE(x, y, size, size, buffer);                            
      }
      x+= string[n] != ' ' ? 6 : 2;
    }  
  }
//}#pragma endregion Text

//{#pragma region Mask
  void draw_mask(int x, int y, int w, int h){
    for (int i = 0; i < w * h; i++) buffer[i] = GUI.bg;
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
  }

  void draw_background() {
    int w = 320;
    int h = 60;
    for (int i = 0; i < 4; i++) draw_mask(0, i*h, w, h);
    draw_battery();
  }
//}#pragma endregion Mask

//{#pragma region Theme
  void draw_themes() {
    int x = ORIGIN.x;                     
    int y = POS.y + 46;
    int filled = 0;
    int count = 22;
    for(int n = USER; n < count; n++){
      if(filled < ROMS.limit) {
        draw_mask(x,y,100,16);
        draw_text(x,y,THEMES[n].name,false, n == USER ? true : false);    
        y+=20;
        filled++;            
      }
    }
    int slots = (ROMS.limit - filled);
    for(int n = 0; n < slots; n++) {
      draw_mask(x,y,100,16);
      draw_text(x,y,THEMES[n].name,false,false);    
      y+=20;    
    }
  }

  void get_theme() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );                    

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);

    err = nvs_get_i8(handle, "USER", &USER);
    switch (err) {
      case ESP_OK:
        break;
      case ESP_ERR_NVS_NOT_FOUND:
        USER = 0;
        break;
      default :
        USER = 0;
    }   
    nvs_close(handle);
  }

  void set_theme(int8_t i) {
    nvs_handle handle;
    nvs_open("storage", NVS_READWRITE, &handle);
    nvs_set_i8(handle, "USER", i);
    nvs_commit(handle);  
    nvs_close(handle);
    get_theme(); 
  }

  void update_theme() {
    GUI = THEMES[USER];
    set_theme(USER);
    draw_background();
    draw_mask(0,0,320,64);
    draw_systems();
    draw_text(16,16,EMULATORS[STEP], false, true);
    draw_themes();  
  }
//}#pragma endregion Theme

//{#pragma region GUI
  void draw_systems() {
    for(int e = 0; e < COUNT; e++) {
      int i = 0;                            
      int x = SYSTEMS[e].x;
      int y = POS.y;
      int w = 32;
      int h = 32;
      if(x > 0 && x < 288) {
        for(int r = 0; r < 32; r++) {
          for(int c = 0; c < 32; c++) { 
            buffer[i] = SYSTEMS[e].system[r][c] == WHITE ? WHITE : GUI.bg;                                  
            i++;
          }      
        }
        ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      }
    }
  }

  void draw_media(int x, int y, bool current) {
    int offset = STEP * 16;
    int i = 0;
    if(current) {
      for(int h = 0; h < 16; h++) {  
        for(int w = offset; w < (offset+16); w++) {                                 
          buffer[i] = media[h][w] == WHITE ? GUI.fg : GUI.bg;
          i++;    
        }
      }                   
      ili9341_write_frame_rectangleLE(x+1, y+1, 16, 16, buffer);
    }
    i = 0;
    for(int h = 0; h < 16; h++) {  
      for(int w = offset; w < (offset+16); w++) {                                 
        buffer[i] = media[h][w] == WHITE ? current ? WHITE : GUI.fg : GUI.bg;
        i++;    
      }
    }
    current ? 
    ili9341_write_frame_rectangleLE(x-1, y-1, 16, 16, buffer) :
    ili9341_write_frame_rectangleLE(x, y, 16, 16, buffer);
  }

  void draw_battery() {
    #ifdef BATTERY
      odroid_input_battery_level_read(&battery_state);

      int i = 0;
      int x = SCREEN.w - 32;
      int y = 8;
      int h = 0;
      int w = 0;

      draw_mask(x,y,16,16);
      for(h = 0; h < 16; h++) {  
        for(w = 0; w < 16; w++) {                                 
          buffer[i] = battery[h][w] == WHITE ? WHITE : GUI.bg;
          i++;    
        }
      }            
      ili9341_write_frame_rectangleLE(x, y, 16, 16, buffer);

      x += 2;
      y += 6;
      w = (int)battery_state.percentage/10 > 0 ? ((int)battery_state.percentage/10) : 10;
      h = 4;
      i = 0;

      int color[11] = {24576,24576,64288,64288,65504,65504,65504,26592,26592,26592,26592};


      for(int c = 0; c < h; c++) {
        for(int n = 0; n <= w; n++) {
          buffer[i] = color[w];
          i++;
        }                                  
      }
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
    #endif
  }

  void draw_numbers() {
    int x = 296;
    int y = POS.y + 48;
    int h = 5;
    int w = 0;
    char count[10];
    sprintf(count, "(%d/%d)", (ROMS.offset+1), ROMS.total);
    w = strlen(count)*5;
    x -= w;
    draw_mask(x-5,y,w+5,h);
    draw_text(x,y,count,false,false);  
  }

  void draw_launcher() {
    draw_background();
    draw_text(16,16,EMULATORS[STEP], false, true);
    int i = 0;                            
    int x = GAP/3;
    int y = POS.y;
    int w = 32;
    int h = 32;  
    for(int r = 0; r < 32; r++) {
      for(int c = 0; c < 32; c++) { 
        buffer[i] = SYSTEMS[STEP].system[r][c] == WHITE ? WHITE : GUI.bg;
        i++;
      }      
    }
    ili9341_write_frame_rectangleLE(x, y, w, h, buffer); 

    y += 48;
    draw_media(x,y-6,true);
    draw_options();
  }

  void draw_options() {
    has_save_file(ROM.name);
    //printf("\nROM NAME:%s\nROM PATH:%s\n%s\n",ROM.name,ROM.path,SAVED ? "Has Save File" : "No Save File");

    int x = GAP/3 + 32;
    int y = POS.y + 48;  
    int w = 5;
    int h = 5;
    int i = 0;
    int offset = 0;
    if(SAVED) {
      // resume               
      i = 0;  
      offset = 5;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 0 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Resume",false,OPTION == 0?true:false);
      // restart
      i = 0;  
      y+=20;
      offset = 10;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 1 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Restart",false,OPTION == 1?true:false);
      // restart
      i = 0;  
      y+=20;
      offset = 20;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? OPTION == 2 ? WHITE : GUI.fg : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Delete Save",false,OPTION == 2?true:false);       
    } else {
      // run
      i = 0;  
      offset = 0;
      for(int r = 0; r < 5; r++){for(int c = 0; c < 5; c++) {
        buffer[i] = icons[r+offset][c] == WHITE ? WHITE : GUI.bg;i++;
      }}
      ili9341_write_frame_rectangleLE(x, y, w, h, buffer);
      draw_text(x+10,y,"Run",false,true);
    } 
  }
//}#pragma endregion GUI  

//{#pragma region Files
  inline static void swap(char** a, char** b)
  {
      char* t = *a;
      *a = *b;
      *b = t;
  }

  static int strcicmp(char const *a, char const *b)
  {
      for (;; a++, b++)
      {
          int d = tolower((int)*a) - tolower((int)*b);
          if (d != 0 || !*a) return d;
      }
  }

  static int partition (char* arr[], int low, int high)
  {
      char* pivot = arr[high];
      int i = (low - 1);

      for (int j = low; j <= high- 1; j++)
      {
          if (strcicmp(arr[j], pivot) < 0)
          {
              i++;
              swap(&arr[i], &arr[j]);
          }
      }
      swap(&arr[i + 1], &arr[high]);
      return (i + 1);
  }     
  void quick_sort(char* arr[], int low, int high)
  {
      if (low < high)
      {
          int pi = partition(arr, low, high);

          quick_sort(arr, low, pi - 1);
          quick_sort(arr, pi + 1, high);
      }
  }

  void sort_files(char** files)
  {
      bool swapped = true;

      if (ROMS.total > 1)
      {
          quick_sort(files, 0, ROMS.total - 1);
      }
  }
  void get_files() {
    odroid_sdcard_open("/sd");
    const int MAX_FILES = 1024;
    char** result = (char**)malloc(MAX_FILES * sizeof(void*));

    DIR *directory;   
    struct dirent *file;                  
    char path[256] = "/sd/roms/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    strcpy(ROM.path, path);    
    bool files = !(directory = opendir(path)) ? false : true;

    ROMS.total = 0;

    if(files) {
      while ((file = readdir(directory)) != NULL) {

        int rom_length = strlen(file->d_name);
        int ext_lext = strlen(DIRECTORIES[STEP]);
        bool extenstion = strcmp(&file->d_name[rom_length - ext_lext], DIRECTORIES[STEP]) == 0 && file->d_name[0] != '.';
        if(extenstion) {
          size_t len = strlen(file->d_name);
          result[ROMS.total] = (char*)malloc(len + 1);
          strcpy(result[ROMS.total], file->d_name);
          ROMS.total++;
        } 

      }
    }
    closedir(directory);
    sort_files(result);   

    if(ROMS.total > 0) {
      draw_files(result);
      draw_numbers();      
    } else {
      char message[100] = "no games available";
      int center = ceil((320/2)-((strlen(message)*5)/2));
      draw_text(center,134,message,false,false);
    }
  }

  void draw_files(char** files) {
    
    int x = ORIGIN.x;                     
    int y = POS.y + 48;
    int game = ROMS.offset ;

    for (int i = 0; i < 4; i++) draw_mask(0, y+(i*40)-6, 320, 40);
    printf("\nROMS.offset:%d ROMS.limit:%d ROMS.total:%d\n", ROMS.offset, ROMS.limit, ROMS.total);
    for(int n = 0; n < ROMS.total; n++) {
      if(game < (ROMS.limit+ROMS.offset) && n >= game && game < ROMS.total) {                                      
        //printf("%d: %s\n", game, files[n]);
        draw_media(x,y-6,game == ROMS.offset ? true : false);
        draw_text(x+24,y,files[n],true,game == ROMS.offset ? true : false);  
        if(game == ROMS.offset) {
          strcpy(ROM.name, files[n]);
          int i = strlen(ROM.path); ROM.path[i] = '/'; 
          ROM.path[i + 1] = 0;
          strcat(ROM.path, ROM.name);
          ROM.ready = true;
        }
        y+=20;         
        game++;
      }
    }
  }

  void has_save_file(char *name) {
    SAVED = false;                                  
    DIR *directory;   
    struct dirent *file;                  
    char path[256] = "/sd/odroid/data/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    directory = opendir(path); 
    gets(name);
    while ((file = readdir(directory)) != NULL) {
      char tmp[256] = "";
      strcat(tmp, file->d_name);
      tmp[strlen(tmp)-4] = '\0';
      gets(tmp);
      if(strcmp(name, tmp) == 0){
        SAVED = true;
      }
    }
    closedir(directory);
  }
//}#pragma endregion Files  

//{#pragma region Animations
  void animate(int dir) {     
    int y = POS.y + 46;                        
    for (int i = 0; i < 4; i++) draw_mask(0, y+(i*40)-6, 320, 40);                 
    int sx[4][13] = {
      {8,8,4,4,4,3,3,3,3,2,2,2,2}, // 48
      {30,26,20,20,18,18,16,16,12,12,8,8,4} // 208 30+26+20+20+18+18+16+16+12+12+8+8+4
    };  
    for(int i = 0; i < 13; i++) {    
      if(dir == -1) {
        // LEFT
        for(int e = 0; e < COUNT; e++) {
          SYSTEMS[e].x += STEP == (e-1) ? sx[1][i] : sx[0][i];
        }
      } else {
        // RIGHT
        for(int e = 0; e < COUNT; e++) {
          SYSTEMS[e].x -= STEP == e ? sx[1][i] : sx[0][i];
        }      
      }
      draw_mask(0,32,320,32);
      draw_systems();
    }
    draw_mask(0,0,SCREEN.w - 32,32);
    draw_text(16,16,EMULATORS[STEP], false, true);  
    STEP == 0 ? draw_themes() : get_files();
  }
//}#pragma endregion Animations  

//{#pragma region Boot Screens
  void splash() {
    draw_background();
    sleep(1);
    int w = 128;
    int h = 18;
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);
    int i = 0;
    for(int r = 0; r < h; r++) {
      i = 0;
      for(int c = 0; c < w; c++) {
        buffer[i] = logo[r][c] == 0 ? GUI.bg : GUI.fg;
        i++;
      }
      ili9341_write_frame_rectangleLE(x, y+r, w, 1, buffer);
      usleep(10000);    
    }

    sleep(2);

    for(int r = 0; r < h; r++) {
      i = 0;
      for(int c = 0; c < w; c++) {
        buffer[i] = GUI.bg;
        i++;
      }
      ili9341_write_frame_rectangleLE(x, y+r, w, 1, buffer);
      usleep(10000);    
    }  
    sleep(1);
  }

  void boot() {
    draw_background();
    char message[100] = "retro esp32";
    int width = strlen(message)*5;
    int center = ceil((320/2)-(width/2));
    int y = 118;
    draw_text(center,y,message,false,false);

    y+=10;
    for(int n = 0; n < (width+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(center+n, y, 1, 5, buffer);
      usleep(10000);
    }
  }

  void restart() {
    draw_background();
    char message[100] = "restarting";
    int h = 5;
    int w = strlen(message)*h;  
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);  
    draw_text(x,y,message,false,false);

    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    }  
  }
//}#pragma endregion Boot Screens  

//{#pragma region ROM Options
  void rom_run(bool resume) {
    draw_background();;
    char *message = !resume ? "loading..." : "hold start";

    int h = 5;
    int w = strlen(message)*h;  
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);  
    draw_text(x,y,message,false,false);
    odroid_system_application_set(PROGRAMS[STEP-1]);
    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    } 
    odroid_system_application_set(PROGRAMS[STEP-1]);
    usleep(10000);
    esp_restart();
  }

  void rom_resume() {
    draw_background();                 
    char message[100] = "resuming...";
    int h = 5;
    int w = strlen(message)*h;  
    int x = (SCREEN.w/2)-(w/2);
    int y = (SCREEN.h/2)-(h/2);  
    draw_text(x,y,message,false,false);
    odroid_system_application_set(PROGRAMS[STEP-1]);
    y+=10;
    for(int n = 0; n < (w+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(x+n, y, 1, 5, buffer);
      usleep(15000);
    }

    odroid_system_application_set(PROGRAMS[STEP-1]);
    usleep(10000);
    esp_restart();  
  }

  void rom_delete_save() {
    draw_background();                  
    char message[100] = "deleting...";
    int width = strlen(message)*5;
    int center = ceil((320/2)-(width/2));
    int y = 118;
    draw_text(center,y,message,false,false);

    y+=10;
    for(int n = 0; n < (width+10); n++) {
      for(int i = 0; i < 5; i++) {
        buffer[i] = GUI.fg;
      }
      ili9341_write_frame_rectangleLE(center+n, y, 1, 5, buffer);
      usleep(15000);
    }  

    //printf("\n***********\nSEARHING FOR:%s.sav\n***********\n", ROM.name);

    DIR *directory;   
    struct dirent *file;                  
    char path[256] = "/sd/odroid/data/";
    strcat(&path[strlen(path) - 1], DIRECTORIES[STEP]);
    directory = opendir(path); 
    gets(ROM.name);
    while ((file = readdir(directory)) != NULL) {
      char tmp[256] = "";
      char file_to_delete[256] = "";
      strcat(tmp, file->d_name);
      sprintf(file_to_delete, "%s/%s", path, file->d_name);
      tmp[strlen(tmp)-4] = '\0';
      gets(tmp);
      if(strcmp(ROM.name, tmp) == 0){
        //printf("\n***********\nFOUND: %s\n***********\n", file_to_delete);

        struct stat st;
        if (stat(file_to_delete, &st) == 0) {                               
          //printf("\n***********\nDELETING: %s\n***********\n", file_to_delete);
          unlink(file_to_delete);
          LAUNCHER = false;
          draw_background();
          draw_systems();
          draw_text(16,16,EMULATORS[STEP],false,true); 
          STEP == 0 ? draw_themes() : get_files();
        }      
      }
    }  
    //closedir(path);
  }
//}#pragma endregion ROM Options

//{#pragma region Launcher
  static void launcher() {
    draw_battery();                            
    while (true) {
      /*
        Get Gamepad State
      */
      odroid_input_gamepad_read(&gamepad);

      /*
        LEFT
      */
      if(gamepad.values[ODROID_INPUT_LEFT]) {
        if(!LAUNCHER) {
          STEP--;
          if( STEP < 0 ) { STEP = 0; } else { 
            ROMS.offset = 0; 
            ROMS.total = 0;                               
            animate(-1); 
          }                       
        }
        debounce(ODROID_INPUT_LEFT);
      }
      /*
        RIGHT
      */
      if(gamepad.values[ODROID_INPUT_RIGHT]) {
        if(!LAUNCHER) {
          STEP++;
          if( STEP > COUNT-1 ) { STEP = COUNT-1; } else { 
            ROMS.offset = 0; 
            ROMS.total = 0; 
            animate(1); 
          }    
        }
        debounce(ODROID_INPUT_RIGHT);
      }  
      /*
        UP
      */
      if (gamepad.values[ODROID_INPUT_UP]) {
        if(!LAUNCHER) {
          if(STEP == 0) {
            USER--;
            if( USER < 0 ) { USER = 21; }
            draw_themes();
          }
          if(STEP != 0) {
            ROMS.offset--;
            if( ROMS.offset < 0 ) { ROMS.offset = ROMS.total - 1; }
            get_files();
          }
        } else {
          if(SAVED) {
            OPTION--;
            if( OPTION < 0 ) { OPTION = 2; }
            draw_options();
          }
        }
        debounce(ODROID_INPUT_UP);
      }
      /*
        DOWN
      */
      if (gamepad.values[ODROID_INPUT_DOWN]) {
        if(!LAUNCHER) {
          if(STEP == 0) {
            USER++;
            if( USER > 21 ) { USER = 0; }
            draw_themes();
          }
          if(STEP != 0) {
            ROMS.offset++;
            if( ROMS.offset > ROMS.total - 1 ) { ROMS.offset = 0; }
            get_files();
          }            
        } else {
          if(SAVED) {
            OPTION++;
            if( OPTION > 2 ) { OPTION = 0; }
            draw_options();
          } 
        }
        debounce(ODROID_INPUT_DOWN);       
      }  
      /*
        BUTTON A
      */
      if (gamepad.values[ODROID_INPUT_A]) {
        if(STEP == 0) {                                            
          update_theme();
        } else {
          if (ROM.ready && !LAUNCHER) {
            OPTION = 0;
            LAUNCHER = true;
            draw_launcher();
          } else {
            odroid_settings_RomFilePath_set(ROM.path);
            switch(OPTION) {
              case 0:
                SAVED ? rom_resume() : rom_run(false);
              break;
              case 1:
                rom_run(true);
              break;
              case 2:
                rom_delete_save();
              break;
            }
          }     
        }
        debounce(ODROID_INPUT_A);
      }
      /*
        BUTTON B
      */
      if (gamepad.values[ODROID_INPUT_B]) {
        if (ROM.ready && LAUNCHER) {        
          LAUNCHER = false;
          draw_background();
          draw_systems();
          draw_text(16,16,EMULATORS[STEP],false,true); 
          STEP == 0 ? draw_themes() : get_files();
        }
        debounce(ODROID_INPUT_B);
      }  

      /*
        START + SELECT (MENU)
      */
      if (gamepad.values[ODROID_INPUT_MENU]) {
        usleep(10000);
        esp_restart();
        debounce(ODROID_INPUT_MENU);
      }      
    }                               
  }
//}#pragma endregion Launcher  
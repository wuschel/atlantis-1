#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <curses.h>

static int xof, yof;

static int json_getint(cJSON *json, const char *key, int def) {
    json = cJSON_GetObjectItem(json, key);
    return json ? json->valueint : def;
}

static const char * json_getstr(cJSON *json, const char *key, const char *def) {
    json = cJSON_GetObjectItem(json, key);
    return json ? json->valuestring : def;
}

void finish(int sig) {
    erase();
    endwin();
    exit(sig);
}

void run(void) {
    initscr();
    signal(SIGINT, finish);      /* arrange interrupts to terminate */
    start_color();
    init_color(COLOR_YELLOW, 1000, 1000, 0);
    attrset(COLOR_PAIR(COLOR_BLACK));
    bkgd(' ' | COLOR_PAIR(COLOR_BLACK));
    bkgdset(' ' | COLOR_PAIR(COLOR_BLACK));
    keypad(stdscr, TRUE);  /* enable keyboard mapping */
    nonl();         /* tell curses not to do NL->CR/NL on output */
    cbreak();       /* take input chars one at a time, no wait for \n */
    noecho();       /* don't echo input */
    scrollok(stdscr, FALSE);
    wclear(stdscr);
    for (;;) {
        int c = getch();     /* refresh, accept single keystroke of input */
        switch (c) {
            case 'Q':
              finish(0);
              break;
        }
    }
}

void print_map(cJSON *json, FILE *F) {
    int w = 1, h = 1;
    int fno = 0;
    cJSON *region, *regions = cJSON_GetObjectItem(json, "regions");
    cJSON *faction, *factions = cJSON_GetObjectItem(json, "factions");
    char * mapdata;

    faction = cJSON_GetObjectItem(json, "faction");
    if (faction) {
        fno = cJSON_GetObjectItem(faction, "id")->valueint;
    }
    for (region=regions->child; region; region=region->next) {
        int x = json_getint(region, "x", 0);
        int y = json_getint(region, "y", 0);
        if (x>=w) w=x+1;
        if (y>=h) h=y+1;
    }
    mapdata = (char *)malloc(w*h*sizeof(char));
    memset(mapdata, ' ', w*h*sizeof(char));
    for (region=regions->child; region; region=region->next) {
        int x = (json_getint(region, "x", 0) + xof) % w;
        int y = (json_getint(region, "y", 0) + yof) % h;
        const char * terrain = json_getstr(region, "terrain", 0);
        mapdata[x+y*h] = terrain[0];
    }
}

int main (int argc, char **argv) {
    FILE *in = stdin, *out = stdout;
    long len;
    char *data;
    cJSON *json;
    printf("argc: %d\n", argc);
    if (argc>1) {
        in = fopen(argv[1], "r");
        if (!in || errno) {
            perror("could not open input file");
            return errno;
        }
    }
    if (argc>2) {
        out = fopen(argv[2], "w");
        if (!out || errno) {
            perror("could not open output file");
            return errno;
        }
    }
    fseek(in,0,SEEK_END);
    len=ftell(in);
    fseek(in,0,SEEK_SET);
    data = (char *)malloc(len+1);
    if (data) {
        fread(data,1,len,in);
    }
    if (in!=stdin) fclose(in);

    json = cJSON_Parse(data);
    print_map(json, out);
    run();
    free(data);
    cJSON_Delete(json);
    if (out!=stdout) fclose(out);

    return 0;
}
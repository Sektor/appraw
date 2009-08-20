/***************************************************************************
 *   appraw - .desktop file patching tool                                  *
 *   (c) 2009 Anton Olkhovik <ant007h@gmail.com>                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

//#define PATH_PREFIX     "" //debug
#define PATH_PREFIX     "/usr/share/applications/"
#define PATH_EXTENSION  ".desktop"
#define LINE_BUF_SIZE   512
#define FILE_BUF_ADD    512

#define ACTION_INSERT	1
#define ACTION_REMOVE	2
#define ACTION_SHOW	3

char linebuf[LINE_BUF_SIZE];

void print_help()
{
    printf("appraw - .desktop file patching tool\n");
    printf("Usage: appraw action app_name\n");
    printf("action:\n");
    printf("-i resources\n");
    printf("    Modify Exec field of app's .desktop file to launch it\n");
    printf("    with help of fsoraw and request specified resources.\n");
    printf("    resources=res1,res2,...\n");
    printf("-d\n");
    printf("    Operates as '-i Display'.\n");
    printf("-c\n");
    printf("    Operates as '-i CPU'.\n");
    printf("-r\n");
    printf("    Remove fsoraw call from Exec field of app's .desktop file.\n");
    printf("-s\n");
    printf("    Show Exec field of app's .desktop file and quit.\n");
    printf("-h, --help\n");
    printf("    Print help and quit.\n");
    printf("Examples:\n");
    printf("    appraw -i CPU,Display tangogps\n");
    printf("    appraw -i Display tangogps\n");
    printf("    appraw -d mokomaze\n");
    printf("    appraw -r mokomaze\n");
}

int strpos(char *haystack, char *needle)
{
    char *p = strstr(haystack, needle);
    if (p) return p - haystack;
    return -1; //Not found = -1.
}

int file_size(FILE *f)
{
    int pos;
    int end;

    pos = ftell (f);
    fseek (f, 0, SEEK_END);
    end = ftell (f);
    fseek (f, pos, SEEK_SET);

    return end;
}

char* getfullapp(char* app)
{
    char *fullapp = (char*)malloc(strlen(PATH_PREFIX) + strlen(app) + strlen(PATH_EXTENSION) + 1);
    strcpy(fullapp, PATH_PREFIX);
    strcat(fullapp, app);
    strcat(fullapp, PATH_EXTENSION);
    return fullapp;
}

int action(char* app, char* resources, int act_id)
{
    int res=0;
    int exec_found=0;
    int mod_ok=0;
    char *fullapp = getfullapp(app);
    FILE *f = fopen(fullapp, "r");
    if (!f)
    {
        printf("Can't open file for reading:\n%s\n", fullapp);
        res=1;
    }
    else
    {
        printf("* Analyzing input file:\n%s\n", fullapp);
        char* filebuf = (char*)malloc(file_size(f) + FILE_BUF_ADD);
        filebuf[0]=0;
        while(fgets(linebuf,LINE_BUF_SIZE,f))
        {
            if (strpos(linebuf, "Exec=") == 0)
            {
                printf("* 'Exec' field found:\n%s\n", linebuf);
                exec_found=1;

                if (act_id==ACTION_INSERT)
                {
                    if (strpos(linebuf, "Exec=fsoraw ") == 0)
                    {
                        printf("This .desktop file is already fsoraw-enabled\n");
                        printf("Nothing was modified.\n");
                        break;
                    }
                    else
                    {
                        int len1 = strlen(filebuf);

                        strcat(filebuf, "Exec=fsoraw -r ");
                        strcat(filebuf, resources);
                        strcat(filebuf, " -- ");
                        strcat(filebuf, linebuf + strlen("Exec="));

                        printf("* Patched 'Exec' field:\n");
                        printf("%s\n", filebuf + len1);
                        mod_ok=1;
                    }
                }

                else if (act_id==ACTION_REMOVE)
                {
                    int use_fsoraw=0;
                    if (strpos(linebuf, "Exec=fsoraw ") == 0)
                    {
                        use_fsoraw=1;
                        int tail_n = strpos(linebuf, " -- ");
                        if (tail_n>=0)
                        {
                            int len1 = strlen(filebuf);

                            strcat(filebuf, "Exec=");
                            strcat(filebuf, linebuf + tail_n + strlen(" -- "));

                            printf("* Unpatched 'Exec' field:\n");
                            printf("%s\n", filebuf + len1);
                            mod_ok=1;
                        }
                    }

                    if (!mod_ok)
                    {
                        if (use_fsoraw)
                        {
                            printf("This .desktop file uses 'fsoraw' but it was patched not by this program\n");
                            printf("Nothing was modified.\n");
                        }
                        else
                        {
                            printf("This .desktop file not uses 'fsoraw' or uses it in unknown manner\n");
                            printf("Nothing was modified.\n");
                        }
                        break;
                    }
                }

                else if (act_id==ACTION_SHOW)
                {
                    if (strpos(linebuf, "Exec=fsoraw ") == 0)
                        printf("* fsoraw-enabled: YES\n");
                    else
                        printf("* fsoraw-enabled: no\n");
                    break;
                }
            }
            else
                strcat(filebuf, linebuf);
        }
        fclose(f);

        if (!exec_found)
            printf("'Exec' field not found.\n");
        else if (mod_ok)
        {
            //printf("%s", filebuf); //debug
            f = fopen(fullapp, "w+");
            if (!f)
            {
                printf("Can't open file for writing.\n");
                res=1;
            }
            else
            {
                fprintf(f, "%s", filebuf);
                fclose(f);
                printf("* File modified successfully.\n");
            }
        }
        free(filebuf);
    }
    free(fullapp);
    return res;
}

void parse_command_line(int argc, char *argv[])
{
    if (argc<=1)
    {
        print_help();
        exit(0);
    }

    for (int i=1; i<argc; i++)
    {
        if ( (!strcmp(argv[i],"-h")) || (!strcmp(argv[i],"--help")) )
        {
            print_help();
            exit(0);
        }
        else if (!strcmp(argv[i],"-i"))
        {
            if (i+2<argc)
            {
                char *resources = argv[i+1];
                char *app = argv[i+2];
                action(app, resources, ACTION_INSERT); //
                exit(0);
            }
            else
            {
                printf("Not enough actual parameters for '-i' action\n");
                exit(1);
            }
        }
        else if (!strcmp(argv[i],"-c"))
        {
            if (i+1<argc)
            {
                char resources[] = "CPU";
                char *app = argv[i+1];
                action(app, resources, ACTION_INSERT); //
                exit(0);
            }
            else
            {
                printf("Not enough actual parameters for '-c' action\n");
                exit(1);
            }
        }
        else if (!strcmp(argv[i],"-d"))
        {
            if (i+1<argc)
            {
                char resources[] = "Display";
                char *app = argv[i+1];
                action(app, resources, ACTION_INSERT); //
                exit(0);
            }
            else
            {
                printf("Not enough actual parameters for '-d' action\n");
                exit(1);
            }
        }
        else if (!strcmp(argv[i],"-r"))
        {
            if (i+1<argc)
            {
                char *app = argv[i+1];
                action(app, NULL, ACTION_REMOVE); //
                exit(0);
            }
            else
            {
                printf("Not enough actual parameters for '-r' action\n");
                exit(1);
            }
        }
        else if (!strcmp(argv[i],"-s"))
        {
            if (i+1<argc)
            {
                char *app = argv[i+1];
                action(app, NULL, ACTION_SHOW); //
                exit(0);
            }
            else
            {
                printf("Not enough actual parameters for '-s' action\n");
                exit(1);
            }
        }
        else
        {
            printf("Unknown argument '%s'\n", argv[i]);
            exit(1);
        }
    }
}

int main(int argc, char* argv[])
{
    parse_command_line(argc, argv);
    return 0;
}

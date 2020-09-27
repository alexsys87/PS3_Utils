/* 
    (c) 2013 Estwald/Hermes <www.elotrolado.net>

    EXTRACTPS3ISO is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    EXTRACTPS3ISO is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    apayloadlong with EXTRACTPS3ISO.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <windows.h>

#include <filesystem>

#include "download.h"
#include "ConsoleColor.h"
#include "ConsoleUtils.h"

#define ISODCL(from, to) (to - from + 1)

static int isonum_731 (unsigned char *p)
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}

static int isonum_733 (unsigned char *p)
{
	return (isonum_731 (p));
}

static int isonum_721 (char *p)
{
	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

struct iso_primary_descriptor {
	unsigned char type			            [ISODCL (  1,   1)]; /* 711 */
	unsigned char id				        [ISODCL (  2,   6)];
	unsigned char version			        [ISODCL (  7,   7)]; /* 711 */
	unsigned char unused1			        [ISODCL (  8,   8)];
	unsigned char system_id			        [ISODCL (  9,  40)]; /* aunsigned chars */
	unsigned char volume_id			        [ISODCL ( 41,  72)]; /* dunsigned chars */
	unsigned char unused2			        [ISODCL ( 73,  80)];
	unsigned char volume_space_size		    [ISODCL ( 81,  88)]; /* 733 */
	unsigned char unused3			        [ISODCL ( 89, 120)];
	unsigned char volume_set_size		    [ISODCL (121, 124)]; /* 723 */
	unsigned char volume_sequence_number	[ISODCL (125, 128)]; /* 723 */
	unsigned char logical_block_size		[ISODCL (129, 132)]; /* 723 */
	unsigned char path_table_size		    [ISODCL (133, 140)]; /* 733 */
	unsigned char type_l_path_table		    [ISODCL (141, 144)]; /* 731 */
	unsigned char opt_type_l_path_table 	[ISODCL (145, 148)]; /* 731 */
	unsigned char type_m_path_table		    [ISODCL (149, 152)]; /* 732 */
	unsigned char opt_type_m_path_table	    [ISODCL (153, 156)]; /* 732 */
	unsigned char root_directory_record	    [ISODCL (157, 190)]; /* 9.1 */
	unsigned char volume_set_id		        [ISODCL (191, 318)]; /* dunsigned chars */
	unsigned char publisher_id		        [ISODCL (319, 446)]; /* achars */
	unsigned char preparer_id		        [ISODCL (447, 574)]; /* achars */
	unsigned char application_id		    [ISODCL (575, 702)]; /* achars */
	unsigned char copyright_file_id		    [ISODCL (703, 739)]; /* 7.5 dchars */
	unsigned char abstract_file_id		    [ISODCL (740, 776)]; /* 7.5 dchars */
	unsigned char bibliographic_file_id	    [ISODCL (777, 813)]; /* 7.5 dchars */
	unsigned char creation_date		        [ISODCL (814, 830)]; /* 8.4.26.1 */
	unsigned char modification_date		    [ISODCL (831, 847)]; /* 8.4.26.1 */
	unsigned char expiration_date		    [ISODCL (848, 864)]; /* 8.4.26.1 */
	unsigned char effective_date		    [ISODCL (865, 881)]; /* 8.4.26.1 */
	unsigned char file_structure_version	[ISODCL (882, 882)]; /* 711 */
	unsigned char unused4			        [ISODCL (883, 883)];
	unsigned char application_data		    [ISODCL (884, 1395)];
	unsigned char unused5			        [ISODCL (1396, 2048)];
};

struct iso_directory_record {
	unsigned char length			        [ISODCL (1, 1)];    /* 711 */
	unsigned char ext_attr_length		    [ISODCL (2, 2)];    /* 711 */
	unsigned char extent			        [ISODCL (3, 10)];   /* 733 */
	unsigned char size			            [ISODCL (11, 18)];  /* 733 */
	unsigned char date			            [ISODCL (19, 25)];  /* 7 by 711 */
	unsigned char flags			            [ISODCL (26, 26)];
	unsigned char file_unit_size		    [ISODCL (27, 27)];  /* 711 */
	unsigned char interleave			    [ISODCL (28, 28)];  /* 711 */
	unsigned char volume_sequence_number	[ISODCL (29, 32)];  /* 723 */
	unsigned char name_len		[1];                            /* 711 */
	unsigned char name			[1];
};

struct iso_path_table{
	unsigned char  name_len[2];	/* 721 */
	char extent[4];	        	/* 731 */
	char parent[2];	            /* 721 */
	char name[1];
};

#define SWAP16(x) ((((uint16_t)(x))>>8) | ((x) << 8))

static void UTF16_to_UTF8(uint16_t *stw, uint8_t *stb)
{
    while(SWAP16(stw[0])) {
        if((SWAP16(stw[0]) & 0xFF80) == 0) {
            *(stb++) = SWAP16(stw[0]) & 0xFF;   // utf16 00000000 0xxxxxxx utf8 0xxxxxxx
        } else if((SWAP16(stw[0]) & 0xF800) == 0) { // utf16 00000yyy yyxxxxxx utf8 110yyyyy 10xxxxxx
            *(stb++) = ((SWAP16(stw[0])>>6) & 0xFF) | 0xC0; *(stb++) = (SWAP16(stw[0]) & 0x3F) | 0x80;
        } else if((SWAP16(stw[0]) & 0xFC00) == 0xD800 && (SWAP16(stw[1]) & 0xFC00) == 0xDC00 ) { // utf16 110110ww wwzzzzyy 110111yy yyxxxxxx (wwww = uuuuu - 1) 
                                                                             // utf8 1111000uu 10uuzzzz 10yyyyyy 10xxxxxx  
            *(stb++)= (((SWAP16(stw[0]) + 64)>>8) & 0x3) | 0xF0; *(stb++)= (((SWAP16(stw[0])>>2) + 16) & 0x3F) | 0x80; 
            *(stb++)= ((SWAP16(stw[0])>>4) & 0x30) | 0x80 | ((SWAP16(stw[1])<<2) & 0xF); *(stb++)= (SWAP16(stw[1]) & 0x3F) | 0x80;
            stw++;
        } else { // utf16 zzzzyyyy yyxxxxxx utf8 1110zzzz 10yyyyyy 10xxxxxx
            *(stb++)= ((SWAP16(stw[0])>>12) & 0xF) | 0xE0; *(stb++)= ((SWAP16(stw[0])>>6) & 0x3F) | 0x80; *(stb++)= (SWAP16(stw[0]) & 0x3F) | 0x80;
        } 
        
        stw++;
    }
    
    *stb= 0;
}

#define MAX_ISO_PATHS 4096

typedef struct {
    int parent;
    char *name;
} dir_iso;


static void get_iso_path(dir_iso* directory_iso, char *path, int indx)
{
    char aux[0x420];

    path[0] = 0;

    if(!indx) {path[0] = '/'; path[1] = 0; return;}

    while(1) {
        strcpy_s(aux, 0x420, directory_iso[indx].name);
        strcat_s(aux, path);
        strcpy_s(path, 0x420, aux);
       
        indx = directory_iso[indx].parent - 1;
        if(indx == 0) break;     
    }
}

void MakeDir(std::string dest_path)
{
    namespace fs = std::filesystem;
    std::replace(dest_path.begin(), dest_path.end(), '/', '\\'); // replace all '/' to '\\'
    fs::create_directories(dest_path);
}

int ExtractIso(std::string iso_path, std::string dest_path)
{
    FILE* dest_file = NULL;
    FILE* iso_file  = NULL;

    uint8_t* sectors1 = NULL;
    uint8_t* sectors2 = NULL;
    uint8_t* sectors3 = NULL;

    static char dest_file_path[0x420];
    static uint16_t  wdest_file_path[1024];
    static char dest_folder[0x420];

    char* iso_file_path = new char[0x420];
    memset(iso_file_path, 0, 0x420);
    memcpy(iso_file_path, iso_path.c_str(), iso_path.size());

    char* dest_folder_path = new char[0x420];
    memset(dest_folder_path, 0, 0x420);
    memcpy(dest_folder_path, dest_path.c_str(), dest_path.size());

    struct iso_primary_descriptor sect_desc;
    struct iso_directory_record* idr;
    int idx = -1;
    int end_dir_rec = 0;
    int fail = 0;

    size_t dest_path_len = strlen(dest_folder_path);

    errno_t err = fopen_s(&iso_file, iso_file_path, "rb");
    if (err != 0) {
        printf("Error!: Cannot open ISO file\n\n");
        return -1;
    }

    if (_fseeki64(iso_file, 0x8800, SEEK_SET) < 0) {
        printf("Error!: in sect_descriptor fseek\n\n");
        return -1;
    }

    if (fread((void*)&sect_desc, 1, 2048, iso_file) != 2048) {
        printf("Error!: reading sect_descriptor\n\n");
        return -1;
    }

    if (!(sect_desc.type[0] == 2 && !strncmp((const char*)&sect_desc.id[0], "CD001", 5))) {
        printf("Error!: UTF16 descriptor not found\n\n");
        return -1;
    }

    uint32_t toc = isonum_733(&sect_desc.volume_space_size[0]);

    if ((((uint64_t)toc) * 2048ULL) > (GetDiskFreeSpaceX(dest_folder_path) - 0x100000ULL)) {
        printf("Error!: Insufficient Disk Space in Destination\n");
        return -1;
    }

    uint32_t lba0 = isonum_731(&sect_desc.type_l_path_table[0]); // lba
    uint32_t size0 = isonum_733(&sect_desc.path_table_size[0]); // tamaño
    //printf("lba0 %u size %u %u\n", lba0, size0, ((size0 + 2047)/2048) * 2048);

    if (_fseeki64(iso_file, lba0 * 2048, SEEK_SET) < 0) {
        printf("Error!: in path_table fseek\n\n");
        return -1;
    }

    static dir_iso* directory_iso = (dir_iso*)malloc((MAX_ISO_PATHS + 1) * sizeof(dir_iso));

    if (!directory_iso) {
        printf("Error!: in directory_is malloc()\n\n");
        return -1;
    }

    memset(directory_iso, 0, (MAX_ISO_PATHS + 1) * sizeof(dir_iso));

    sectors1 = (uint8_t*)malloc(((size0 + 2047) / 2048) * 2048);

    if (!sectors1) {
        printf("Error!: in sectors malloc()\n\n");
        return -1;
    }

    sectors2 = (uint8_t*)malloc(2048 * 2);

    if (!sectors2) {
        printf("Error!: in sectors2 malloc()\n\n");
        return -1;
    }

    sectors3 = (uint8_t*)malloc(128 * 2048);

    if (!sectors3) {
        printf("Error!: in sectors3 malloc()\n\n");
        return -1;
    }

    if (fread((void*)sectors1, 1, size0, iso_file) != size0) {
        printf("Error!: reading path_table\n\n");
        return -1;
    }

    uint32_t p = 0;

    dest_folder[0] = 0;

    idx = 0;

    directory_iso[idx].name = NULL;

    if (!verbose) Progress(0, 0); //printf("\rPercent done: %u%% \r", 0);
        
    uint32_t flba = 0;

    while (p < size0 && !fail) {

        uint32_t lba;

        uint32_t snamelen = isonum_721((char*)&sectors1[p]);
        if (snamelen == 0) p = ((p / 2048) * 2048) + 2048;
        p += 2;
        lba = isonum_731(&sectors1[p]);
        p += 4;
        uint32_t parent = isonum_721((char*)&sectors1[p]);
        p += 2;

        memset(wdest_file_path, 0, 512 * 2);
        memcpy(wdest_file_path, &sectors1[p], snamelen);

        UTF16_to_UTF8(wdest_file_path, (uint8_t*)dest_file_path);

        if (idx >= MAX_ISO_PATHS) {
            printf("Too much folders (max %i)\n\n", MAX_ISO_PATHS);
            return -1;
        }

        directory_iso[idx].name = (char*)malloc(strlen(dest_file_path) + 2);
        if (!directory_iso[idx].name) {
            printf("Error!: in directory_iso.name malloc()\n\n");
            return -1;
        }

        strcpy_s(directory_iso[idx].name, MAX_ISO_PATHS, "/");
        strcat_s(directory_iso[idx].name, MAX_ISO_PATHS, dest_file_path);

        directory_iso[idx].parent = parent;

        get_iso_path(directory_iso, dest_folder, idx);

        strcat_s(dest_folder_path, 0x420, dest_folder);

        MakeDir(dest_folder_path);

        dest_folder_path[dest_path_len] = 0;

        if (verbose) printf("%s\n", dest_folder);

        uint32_t file_lba = 0;
        uint64_t file_size = 0;

        char file_aux[0x420];

        file_aux[0] = 0;

        int q2 = 0;
        int size_directory = 0;

        while (!end_dir_rec && !fail) {

            if (_fseeki64(iso_file, ((uint64_t)lba) * 2048ULL, SEEK_SET) < 0) {
                printf("Error!: in directory_record fseek\n\n");
                fail = 1; break;
            }

            memset(sectors2 + 2048, 0, 2048);

            if (fread((void*)sectors2, 1, 2048, iso_file) != 2048) {
                printf("Error!: reading directory_record sector\n\n");
                fail = 1; break;
            }

            int q = 0;

            if (q2 == 0) {
                idr = (struct iso_directory_record*) & sectors2[q];
                if ((int)idr->name_len[0] == 1 && idr->name[0] == 0 && lba == isonum_731((unsigned char*)idr->extent) && idr->flags[0] == 0x2) {
                    size_directory = isonum_733((unsigned char*)idr->size);

                }
                else {
                    printf("Error!: Bad first directory record! (LBA %i)\n\n", lba);
                    fail = 1; break;
                }
            }

            int signal_idr_correction = 0;

            while (!fail) {


                if (signal_idr_correction) {
                    signal_idr_correction = 0;
                    q -= 2048; // sector correction
                    // copy next sector to first
                    memcpy(sectors2, sectors2 + 2048, 2048);
                    memset(sectors2 + 2048, 0, 2048);
                    lba++;

                    q2 += 2048;

                }

                if (q2 >= size_directory) { end_dir_rec = 1; break; }

                idr = (struct iso_directory_record*) & sectors2[q];

                if (idr->length[0] != 0 && (idr->length[0] + q) > 2048) {

                    if (_fseeki64(iso_file, lba * 2048 + 2048, SEEK_SET) < 0) {
                        printf("Error!: in directory_record fseek\n\n");
                        fail = 1; break;
                    }

                    if (fread((void*)(sectors2 + 2048), 1, 2048, iso_file) != 2048) {
                        printf("Error!: reading directory_record sector\n\n");
                        fail = 1; break;
                    }

                    signal_idr_correction = 1;

                }

                if (idr->length[0] == 0 && (2048 - q) > 255) { end_dir_rec = 1; break; }

                if ((idr->length[0] == 0 && q != 0) || q == 2048) {

                    lba++;
                    q2 += 2048;

                    if (q2 >= size_directory) { end_dir_rec = 1; break; }

                    if (_fseeki64(iso_file, (((uint64_t)lba) * 2048ULL), SEEK_SET) < 0) {
                        printf("Error!: in directory_record fseek\n\n");
                        fail = 1; break;
                    }

                    if (fread((void*)(sectors2), 1, 2048, iso_file) != 2048) {
                        printf("Error!: reading directory_record sector\n\n");
                        fail = 1; break;
                    }
                    memset(sectors2 + 2048, 0, 2048);

                    q = 0;
                    idr = (struct iso_directory_record*) & sectors2[q];

                    if (idr->length[0] == 0 || ((int)idr->name_len[0] == 1 && !idr->name[0])) { end_dir_rec = 1; break; }

                }

                if ((int)idr->name_len[0] > 1 && idr->flags[0] != 0x2 &&
                    idr->name[idr->name_len[0] - 1] == '1' && idr->name[idr->name_len[0] - 3] == ';') { // skip directories

                    memset(wdest_file_path, 0, 512 * 2);
                    memcpy(wdest_file_path, idr->name, idr->name_len[0]);

                    UTF16_to_UTF8(wdest_file_path, (uint8_t*)dest_file_path);

                    if (file_aux[0]) {
                        if (strcmp(dest_file_path, file_aux)) {

                            printf("Error!: in batch file %s\n\n", file_aux);
                            fail = 1; break;
                        }

                        file_size += (uint64_t)(uint32_t)isonum_733(&idr->size[0]);
                        if (idr->flags[0] == 0x80) {// get next batch file
                            q += idr->length[0];
                            continue;
                        }

                        file_aux[0] = 0; // stop batch file

                    }
                    else {

                        file_lba = isonum_733(&idr->extent[0]);
                        file_size = (uint64_t)(uint32_t)isonum_733(&idr->size[0]);
                        if (idr->flags[0] == 0x80) {
                            strcpy_s(file_aux, 0x420, dest_file_path);
                            q += idr->length[0];
                            continue;  // get next batch file
                        }
                    }

                    size_t len = strlen(dest_file_path);

                    dest_file_path[len - 2] = 0; // break ";1" string

                    len = strlen(dest_folder);
                    strcat_s(dest_folder, 0x420, "/");
                    strcat_s(dest_folder, 0x420, dest_file_path);

                    if (verbose) {
                        //if (file_size < 1024ULL)
                        //    printf("extracting: %s LBA %u size %u Bytes\n", dest_file_path, file_lba, (uint32_t)file_size);
                        //else if (file_size < 0x100000LL)
                        //    printf("extracting: %s LBA %u size %u KB\n", dest_file_path, file_lba, (uint32_t)(file_size / 1024));
                        //else
                        //    printf("extracting: %s LBA %u size %u MB\n", dest_file_path, file_lba, (uint32_t)(file_size / 0x100000LL));
                        printf("extracting: %s\n", dest_file_path);
                    }

                    strcat_s(dest_folder_path, 0x420, dest_folder);

                    err = fopen_s(&dest_file, dest_folder_path, "wb");

                    if (err == 0) {

                        uint32_t count = 0, percent = (uint32_t)(file_size / 0x40000ULL);
                        if (percent == 0) percent = 1;

                        clock_t t_one, t_two;

                        t_one = clock();

                        while (file_size > 0 && !fail) {
                            uint32_t fsize;

                            t_two = clock();

                            if (((t_two - t_one) >= CLOCKS_PER_SEC / 2)) {
                                t_one = t_two;
                                if (verbose) printf("\rWriting file... %u %%", count * 100 / percent);
                            }

                            if (file_size > 0x40000) fsize = 0x40000;
                            else fsize = (uint32_t)file_size;

                            count++;

                            if (_fseeki64(iso_file, (((uint64_t)file_lba) * 2048ULL), SEEK_SET) < 0) {
                                printf("Error!: in ISO file fseek\n\n");
                                fail = 1; break;
                            }

                            if (fread((void*)sectors3, 1, (int)fsize, iso_file) != (int)fsize)
                            {
                                fail = 1; break;
                            }

                            if (fwrite((void*)sectors3, 1, (int)fsize, dest_file) != fsize) {
                                printf("\nError!: writing ISO file\n\n");
                                fail = 1; break;
                            }

                            file_size -= (uint64_t)fsize;

                            file_lba += (fsize + 2047) / 2048;
                            flba += (fsize + 2047) / 2048;

                            //if (!verbose) printf("\rPercent done: %u%% \r", (uint32_t)(((uint64_t)flba) * 100ULL / ((uint64_t)toc)));
                            if (!verbose) Progress(flba, toc);
                        }

                        if (verbose) printf("\r                             \r");

                        fclose(dest_file); dest_file = NULL;
                    }
                    else {

                        printf("\nError!: creating extract file\n\n");
                        fail = 1; break;

                    }

                    dest_folder_path[dest_path_len] = 0;
                    dest_folder[len] = 0;


                }

                q += idr->length[0];
            }

            lba++;
            q2 += 2048;
            if (q2 >= size_directory) { end_dir_rec = 1; break; }

        }

        end_dir_rec = 0;
        p += snamelen;
        if (snamelen & 1) p++;

        idx++;

    }

    if (!verbose && !fail) Progress(100, 100); //printf("\rPercent done: %u%% \n", 100);

    if (iso_file)  fclose(iso_file);
    if (dest_file) fclose(dest_file);

    if (sectors1) free(sectors1);
    if (sectors2) free(sectors2);
    if (sectors3) free(sectors3);

    for (int n = 0; n <= idx; n++)
        if (directory_iso[n].name) { free(directory_iso[n].name); directory_iso[n].name = NULL; }

    if (directory_iso) free(directory_iso);

    delete[] dest_folder_path;
    delete[] iso_file_path;

    return 0;
}

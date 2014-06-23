#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <elf.h>
#include <limits.h>
#include <unistd.h>

//nobody cares about 32bit cubins
#define Elf_Ehdr        Elf64_Ehdr
#define Elf_Shdr        Elf64_Shdr
#define Elf_Phdr        Elf64_Phdr
#define Elf_Sym         Elf64_Sym

uint8_t *ReadFile(uint8_t *filename, uint32_t *size)
{
    if(!filename || !size)
    {
        goto exit;
    }
    FILE *file        = NULL;
    uint32_t file_len = 0;
    file              = fopen(filename, "rb");
    if(!file)
    {
        goto exit;
    }
    fseek(file, 0, SEEK_END);
    file_len = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t *bin = (uint8_t *)malloc(file_len + 1);
    if(!bin)
    {
        goto fail_malloc;
    }
    if(!fread(bin, file_len, 1, file))
    {
        goto fail_read;
    }
    *size = file_len;
    return bin;
fail_read:
    free(bin);
fail_malloc:
    fclose(file);
exit:
    return NULL;
}
int WriteFile(uint8_t *filename, char *data, uint32_t size)
{
    int ret = 0;
    FILE *f = fopen(filename,"w");
    if(!f)
    {
        printf("Can't open %s\n",filename);
        return 1;
    }
    uint32_t n = fwrite(data, 1, size, f);
    if(n<size)
    {
        printf("Can't write %s\n",filename);
        ret = 1;
    }
    fclose(f);
    return ret;
}
int main(int argc, char **argv)
{
    uint64_t b = 0x0;
    uint32_t s = 0;
    uint32_t e = 0;
    uint32_t m = 0;
    char    *n = ".text.bench";
    char    *o = NULL;
    char    *i = NULL;
    char    *d = "./data/cubin/fuz/";
    int      c = 0;
    while((c = getopt (argc, argv, "b:s:e:m:o:i:n:d:")) != -1)
    {
        switch(c)
        {
            case 'b':{sscanf(optarg,"%lx",&b);  break;}
            case 's':{s = atoi(optarg);         break;}
            case 'e':{e = atoi(optarg);         break;}
            case 'm':{m = atoi(optarg);         break;}
            case 'o':{o = optarg;               break;}
            case 'i':{i = optarg;               break;}
            case 'n':{n = optarg;               break;}
            case 'd':{d = optarg;               break;}
        }
    }
    printf("base:  0x%016lx\n",b);
    printf("start: %d\n",s);
    printf("end:   %d\n",e);
    printf("mode:  ");
    switch(m)
    {
        case 3:  {printf("shift-fill up to e zeros in b\n"); break;}
        case 2:  {printf("f|= 1<<i;\n");                     break;}
        case 1:  {printf("f = 1<<i;\n");                     break;}
        case 0:
        default: {printf("f = i++; \n");                     break;}
    }
    if(e<=s && s!=0 && e!=0)
    {
        printf("End <= Start\n");
        return 1;
    }
    if(!i)
    {
        printf("No input  file\n");
        return 1;
    }
    if(!o)
    {
        printf("No output file\n");
        return 1;
    }
    uint32_t size   = 0;
    char     *cubin = ReadFile(i,&size);
    char     *out   = malloc(size);
    char     *pos   = 0;
    memcpy(out,cubin,size);
    Elf_Ehdr *eh    = (Elf_Ehdr*)out;
    Elf_Shdr *sh    = (Elf_Shdr*)(out+eh->e_shoff);
    char     *shstr = (char*)eh + sh[eh->e_shstrndx].sh_offset;
    int j;
    for(j=0;j<eh->e_shnum;j++)
    {
        Elf_Shdr *hdr = &sh[j];
        char *name    = (char*)(shstr+hdr->sh_name);
        if(strcmp(name,n)==0)
        {
            pos = (char*)eh + hdr->sh_offset;
            break;
        }
    }
    if(!pos)
    {
        printf("No section\n");
        return 1;
    }
    uint64_t x = 0;
    uint64_t f = 0;
    uint64_t l = e-s;
    printf("l=%ld\n",l);
    switch(m)
    {
        case 0:{x=0;break;}
        case 1:
        case 2:
        case 3:{x=1;break;}
    }
    if(mkdir(d,0777))
    {
        printf("Can't create %s\n",d);
    }
    j = 0;
    while(1)
    {
        switch(m)
        {
            case 3:
            {
                if(x==((uint64_t)1)<<e)
                {
                    return 0;
                }
                f = b|x;
                x <<= 1;
                if(f == b)
                {
                    continue;
                }
                break;
            }
            case 2:
            {
                if(x==(((uint64_t)1)<<(l)))
                {
                    return 0;
                }
                f |= x<<s;
                x <<= 1;
                break;
            }
            case 1:
            {
                if(x==(((uint64_t)1)<<(l)))
                {
                    return 0;
                }
                f = x<<s;
                x <<= 1;
                break;
            }
            default:
            case 0:
            {
                if(x==(((uint64_t)1)<<(l+1)))
                {
                    return 0;
                }
                f = x<<s;
                x++;
                break;
            }
        }
        uint64_t y = b|f;
        printf("f = 0x%016lx\n",f);
        printf("y = 0x%016lx\n",y);
        char filename[1024] = {0};
        sprintf(filename,"%s%016lx.cubin",d,y);
        *((uint64_t*)pos) = y;
        WriteFile(filename,out,size);
        j++;
    }
    return 0;
}
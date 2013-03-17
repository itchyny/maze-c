/*
 * file: maze.c
 * author: itchyny
 * Last Change: 2013/03/17 16:23:51.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* strcpy */
#include <err.h> /* warnx */
#include <time.h> /* time */
#include <stdint.h> /* uint8_t, uint32_t */
#include <unistd.h> /* getopt */

#ifdef HAVE_CONFIG_H
#  include <config.h> /* PACKAGE, PACKAGE_STRING */
#else
#  define HAVE_SYS_IOCTL_H
#  define HAVE_IOCTL
#  define HAVE_SYS_TIME_H
#  define HAVE_GETTIMEOFDAY
#endif /* HAVE_CONFIG_H */
#ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h> /* ioctl, TIOCGWINSZ */
#endif /* HAVE_SYS_IOCTL_H */
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h> /* gettimeofday */
#endif /* HAVE_SYS_TIME_H */

#ifndef PACKAGE
#  define PACKAGE "maze"
#endif /* PACKAGE */
#ifndef PACKAGE_STRING
#  define PACKAGE_STRING "maze 0.0.0"
#endif /* PACKAGE_STRING */

#define MAJOR_VERSION 0
#define MINOR_VERSION 0
#define REVISION_VERSION 0
#define COMPRESS_VERSION 0

#define ALL_VERSION ((MAJOR_VERSION << 24) | (MINOR_VERSION << 16)\
                 | (REVISION_VERSION << 8) | COMPRESS_VERSION)

#define MAZEID (251 | (238 << 8) | (190 << 16) | (187 << 24))

#define TTY_PATH "/dev/tty"

#define CHECK_ALLOCATE(var)\
  if (var == NULL) {\
    fprintf(stderr, PACKAGE ": cannot allocate variable `" #var "'\n");\
    exit(EXIT_FAILURE);\
  }

extern int optind, opterr; /* unistd.h */
extern char *optarg; /* unistd.h */

uint32_t HALFSIZEX, HALFSIZEY, SIZEX, SIZEY;

uint8_t undesided, wall, road, start, goal;
uint8_t ** maze;
uint8_t * _maze;
uint8_t ** desided;
uint8_t * _desided;
uint32_t * memox;
uint32_t * memoy;
uint32_t memoindex;

char aflag, nflag, eflag, cflag, Cflag, uflag, dflag;

void terminalsize(void)
{
#ifdef HAVE_IOCTL
  struct winsize size;
#endif /* HAVE_IOCTL */
  FILE * console;
  uint32_t terminalwidth, terminalheight;
  terminalwidth = 110; terminalheight = 34;
#ifdef HAVE_IOCTL
  if ((console = fopen(TTY_PATH, "r")) == NULL &&
       ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) {
    terminalwidth = size.ws_col;
    terminalheight = size.ws_row;
  } else if (ioctl(fileno(console), TIOCGWINSZ, &size) == 0) {
    terminalwidth = size.ws_col;
    terminalheight = size.ws_row;
  }
#endif /* HAVE_IOCTL */
  HALFSIZEX = (terminalheight - 2) / 2;
  HALFSIZEY = (terminalwidth - 2) / 4;
}

void desideone(uint32_t i, uint32_t j, char r) {
  if (0 <= i && i < SIZEX && 0 <= j && j < SIZEY && maze[i][j] < road) {
    if (r) {
      maze[i][j] = road;
      if (!desided[i][j]) {
        desided[i][j] = 1;
      }
    } else if (maze[i][j] < wall && !desided[i][j]) {
      maze[i][j] = wall;
      if (!desided[i][j]) {
        desided[i][j] = 1;
      }
    }
  }
}

void desideroad(uint32_t i, uint32_t j) {
  desideone(i - 1, j - 1, 0);
  desideone(i - 1, j, 0);
  desideone(i - 1, j + 1, 0);
  desideone(i, j - 1, 0);
  desideone(i, j, 1);
  desideone(i, j + 1, 0);
  desideone(i + 1, j - 1, 0);
  desideone(i + 1, j, 0);
  desideone(i + 1, j + 1, 0);
}

void desidewall(uint32_t i, uint32_t j) {
  desideone(i, j, 0);
}

void printelemaa(char x) {
  if (x == wall) {
    printf("XX");
  } else if (x == start) {
    printf("S ");
  } else if (x == goal) {
    printf(" G");
  } else {
    printf("  ");
  }
}

void printeleman(char x) {
  printf("%d,", x <= goal ? x : undesided);
}

void printelemone(char x) {
  if (x == wall) {
    printf("\x1b[7m  ");
  } else if (x == start) {
    printf("\x1b[0mS ");
  } else if (x == goal) {
    printf("\x1b[0m G");
  } else {
    printf("\x1b[0m  ");
  }
}

void printelem(uint32_t i, uint32_t j) {
  if (!eflag && aflag) {
    printelemaa(maze[i][j]);
  } else if (!eflag && nflag) {
    printeleman(maze[i][j]);
  } else if ((j ? maze[i][j - 1] : undesided) == maze[i][j]) {
    printf("  ");
  } else {
    printelemone(maze[i][j]);
  }
}

void printmaze(void) {
  uint32_t i, j, imin, imax, jmin, jmax;
  imin = jmin = nflag ? 0 : 1;
  imax = nflag ? SIZEX : SIZEX - 1;
  jmax = nflag ? SIZEY : SIZEY - 1;
  if (!nflag) printf("\n");
  for (i = imin; i < imax; ++i) {
    if (!nflag) printf("  ");
    for (j = jmin; j < jmax; ++j) {
      printelem(i, j);
    }
    if (!eflag && (aflag || nflag)) {
      printf("\n");
    } else {
      printf("\x1b[0m\n");
    }
  }
  if (!nflag) printf("\n");
  fflush(stdout);
}

#define NEW_FRAME(id, var1, var2) \
  frame[id] = var1;\
  frame[id + 256] = var2;
#define RESTORE_FRAME(id) \
  ++codeindex;\
  *(frame[id]) = \
    (code[codeindex]) | (code[codeindex + 1] << 8) |\
    (code[codeindex + 2] << 16) | (code[codeindex + 3] << 24);\
  codeindex += 4;\
  *(frame[id + 256]) = \
    (code[codeindex]) | (code[codeindex + 1] << 8) |\
    (code[codeindex + 2] << 16) | (code[codeindex + 3] << 24);\
  codeindex += 4;
void decompress1(uint8_t * code, uint32_t length) {
  uint32_t i, j, k, len, resutore1, restore2,
           mazeidentifier, version,
           major, minor, revision, compress_ver,
           sizex, sizey, size, codeindex,
           flagid1, flagid2;
  uint8_t identifier;
  uint32_t ** frame;
  uint8_t ** newmaze;
  uint8_t * _newmaze;
  uint8_t ** __maze;
  uint8_t * ___maze;
  char rest, restmax, b;
  frame = (uint32_t **) calloc(256 * 2, sizeof(uint32_t *));
  CHECK_ALLOCATE(frame);
  mazeidentifier = version = sizex = sizey = size = 0;
  flagid1 = flagid2 = 0;
  NEW_FRAME(187, &mazeidentifier, &version);
  NEW_FRAME('Z', &sizex, &sizey);
  NEW_FRAME('F', &flagid1, &flagid2);
  SIZEX = sizex; SIZEY = sizey;
  for (codeindex = 0; codeindex < length; ) {
    identifier = code[codeindex];
    if (identifier && frame[identifier] != NULL && codeindex + 9 < length) {
      RESTORE_FRAME(identifier);
    } else {
      codeindex += 9;
    }
    if (identifier == 0) break;
  }
  if (mazeidentifier != MAZEID) {
    fprintf(stderr, PACKAGE
        ": invalid id: %08x (varid id: %08x)\n", mazeidentifier, MAZEID);
    exit(EXIT_FAILURE);
  }
  compress_ver = version & 255; revision = (version >> 8) & 255;
  minor = (version >> 16) & 255; major = (version >> 24) & 255;
  if (compress_ver > COMPRESS_VERSION) {
    fprintf(stderr, PACKAGE
            ": too large version: %d.%d.%d %d"
            " (current verion: %d.%d.%d %d)\n",
            major, minor, revision, compress_ver,
            MAJOR_VERSION, MINOR_VERSION, REVISION_VERSION, COMPRESS_VERSION);
  }
  if (sizex < 3 || sizey < 3) {
    fprintf(stderr, PACKAGE ": too small size\n");
    exit(EXIT_FAILURE);
  }
  if (sizex > 0x1ffff || sizey > 0x1ffff) {
    fprintf(stderr, PACKAGE ": too large size\n");
    exit(EXIT_FAILURE);
  }
  newmaze = (uint8_t **) calloc(sizex, sizeof(uint8_t *));
  CHECK_ALLOCATE(newmaze);
  size = sizex * sizey;
  _newmaze = (uint8_t *) calloc(size, sizeof(uint8_t));
  CHECK_ALLOCATE(_newmaze);
  for (i = 0; i < sizex; ++i) {
    newmaze[i] = _newmaze + i * sizey;
  }
  if (!aflag && !nflag) {
    aflag = !!(flagid1 & 2);
    nflag = !!(flagid1 & 1);
  }
  restmax = rest = 7;
  i = j = k = 0;
  for (; codeindex < length; ++codeindex) {
    rest = restmax;
    do {
      _newmaze[k++] = (code[codeindex] >> rest) & 1 ? road : wall;
      if (k == size) {
        codeindex = length;
        rest = 0;
      }
    } while (rest--);
  }
  for (i = 0; i < length; i += 9) {
    if (0 < code[i] && code[i] < 4 && i + 9 <= length) {
      newmaze[code[i + 1] | (code[i + 2] << 8)
                          | (code[i + 3] << 16)
                          | (code[i + 4] << 24)]
             [code[i + 5] | (code[i + 6] << 8)
                          | (code[i + 7] << 16)
                          | (code[i + 8] << 24)]
               = (code[i] == 2 ? start :
                  code[i] == 3 ? goal : undesided);
    }
    if (code[i] == 0) break;
  }
  for (i = 0; i < sizex; ++i) {
    for (j = 0; j < sizey; ++j) {
    }
  }
  __maze = maze; ___maze = _maze;
  maze = newmaze; _maze = _newmaze;
  SIZEX = sizex; SIZEY = sizey;
  if (!dflag) printmaze();
  maze = __maze; _maze = ___maze;
  if (dflag && maze) {
    b = 1;
    for (i = 0; i < sizex; ++i) {
      for (j = 0; j < sizey; ++j) {
        b = b && newmaze[i][j] == maze[i][j];
        if (newmaze[i][j] != maze[i][j]) {
          printf("no match: i: %d, j: %d, prev: %d, new: %d\n",
                  i, j, maze[i][j], newmaze[i][j]);
        }
      }
    }
    if (b) {
      printf("true\n");
    } else {
      printf("false\n");
    }
  }
  free(_newmaze);
  free(newmaze);
  free(frame);
}

uint8_t decompress2arr[32] =
  {   0,   1,   4,   5,  16,  17,  20,  21,
     64,  65,  68,  69,  80,  81,  84,  85,
    170, 171, 174, 175, 186, 187, 190, 191,
    234, 235, 238, 239, 250, 251, 254, 255 };
uint8_t compress2arr[256];
#define decompress2arr2len (50)
uint8_t decompress2arr2[decompress2arr2len] =
  { 0, 0, 174, 187, 174, 251, 186, 187, 187, 187,
    187, 190, 187, 235, 187, 238, 187, 239, 187, 251,
    190, 187, 190, 238, 235, 238, 238, 174, 238, 186,
    238, 187, 238, 190, 238, 238, 238, 250, 238, 251,
    239, 174, 239, 238, 251, 187, 251, 238, 255, 255 };
#define zero3 (32 + 2 + decompress2arr2len / 2)
#define zero4 (zero3 + 1)
#define ff4 (zero4 + 1)
uint8_t decompress2arr3[62] =
  { 2, 3, 6, 7, 22, 26, 27, 33,
    35, 36, 39, 70, 86, 88, 90, 91,
    93, 95, 106, 107, 110, 111, 127, 128,
    172, 173, 176, 177, 180, 181, 192, 193,
    196, 197, 200, 201, 203, 208, 209, 212,
    213, 232, 233, 236, 237, 240 };
uint8_t compress2arr3[256];
void decompress2(uint8_t * code, uint32_t length) {
  uint8_t * result;
  uint8_t c;
  uint32_t resultindex, allocatelen, counter, i, j;
  counter = resultindex = 0;
  for (i = 0; i < length; ++i) {
    if (34 <= code[i] && code[i] < 34 + decompress2arr2len / 2) {
      ++counter;
    } else if (code[i] == zero3) {
      counter += 2;
    } else if (code[i] == zero4 || code[i] == ff4) {
      counter += 3;
    } else if (code[i] == 32) {
      counter -= 2; i += 2;
    } else if (code[i] == 33) {
      --counter; ++i;
    }
  }
  result = (uint8_t *) calloc(allocatelen = length + counter + 9,
      sizeof(uint8_t));
  CHECK_ALLOCATE(result);
  for (i = 0; i < length; ++i) {
    if (code[i] < 32) {
      result[resultindex++] = decompress2arr[code[i]];
    } else if (code[i] == 32 && i + 2 < length) {
      result[resultindex++] = code[i + 1] | (code[i + 2] << 5);
      i += 2;
    } else if (code[i] == 33 && i + 1 < length) {
      result[resultindex++] = decompress2arr3[code[i + 1]];
      ++i;
    } else if (34 <= code[i] && code[i] < 34 + decompress2arr2len / 2) {
      result[resultindex++] = decompress2arr2[(code[i] - 34) * 2];
      result[resultindex++] = decompress2arr2[(code[i] - 34) * 2 + 1];
    } else if (code[i] == zero3) {
      result[resultindex++] = 0; result[resultindex++] = 0;
      result[resultindex++] = 0;
    } else if (code[i] == zero4) {
      result[resultindex++] = 0; result[resultindex++] = 0;
      result[resultindex++] = 0; result[resultindex++] = 0;
    } else if (code[i] == ff4) {
      result[resultindex++] = 255; result[resultindex++] = 255;
      result[resultindex++] = 255; result[resultindex++] = 255;
    }
  }
  if (dflag) {
    for (i = 0; i < resultindex; ++i) {
      printf("%u ", result[i]);
    }
    printf("\n");
    printf("len: %d\n", resultindex);
    if (allocatelen < resultindex) {
      printf("invalid len: %d < %d\n", allocatelen, resultindex);
    }
  }
  decompress1(result ,resultindex);
  free(result);
}

void decompress3(uint8_t * code, uint32_t length) {
  uint8_t * result;
  uint32_t resultindex, allocatelen, i;
  if (length > 0xffffff) {
    fprintf(stderr, PACKAGE ": too large size\n");
    exit(EXIT_FAILURE);
  }
  resultindex = 0;
  result = (uint8_t *) calloc(allocatelen = length, sizeof(uint8_t));
  CHECK_ALLOCATE(result);
  for (i = 0; i < length; ++i) {
    if ('0' <= code[i] && code[i] <= '9') {
      result[resultindex++] = code[i] - '0';
    } else if ('A' <= code[i] && code[i] <= 'Z') {
      result[resultindex++] = code[i] - 'A' + 10;
    } else if ('a' <= code[i] && code[i] <= 'z') {
      result[resultindex++] = code[i] - 'a' + 10 + 26;
    } else if (code[i] != '\n') {
      fprintf(stderr, PACKAGE ": invarid character: '%c'\n", code[i]);
      free(result);
      exit(EXIT_FAILURE);
    }
  }
  if (dflag) {
    for (i = 0; i < resultindex; ++i) {
      printf("%d ", result[i]);
    }
    printf("\n");
    printf("len: %d\n", resultindex);
    if (allocatelen < resultindex) {
      printf("invalid len: %d < %d\n", allocatelen, resultindex);
    }
  }
  decompress2(result, resultindex);
  free(result);
}

void compress3(uint8_t * code, uint32_t length) {
  uint8_t * result;
  uint32_t resultindex, allocatelen, i;
  resultindex = 0;
  result = (uint8_t *) calloc(allocatelen = length, sizeof(uint8_t));
  CHECK_ALLOCATE(result);
  for (i = 0; i < length; ++i) {
    result[resultindex++]
      = code[i] < 10 ? code[i] + '0'
      : code[i] < 10 + 26 ? code[i] + 'A' - 10
      : code[i] + 'a' - 10 - 26;
  }
  if (cflag) {
    printf("%s\n", result);
  }
  if (dflag) {
    printf("len: %d\n", resultindex);
    decompress3(result, resultindex);
    if (allocatelen < resultindex) {
      printf("invalid len: %d < %d\n", allocatelen, resultindex);
    }
  }
  free(result);
}

void compress2(uint8_t * code, uint32_t length) {
  uint8_t * result;
  uint32_t resultindex, allocatelen, counter, i, j;
  char done;
  counter = resultindex = 0;
  for (i = 0; i < 32; ++i) {
    compress2arr[decompress2arr[i]] = i;
  }
  for (i = 0; i < 62; ++i) {
    compress2arr3[decompress2arr3[i]] = i + 1;
  }
  for (i = 0; i < length; ++i) {
    if (compress2arr[code[i]] == 0 && code[i] != 0) ++counter;
  }
  result = (uint8_t *) calloc(allocatelen = length + counter * 2,
      sizeof(uint8_t));
  CHECK_ALLOCATE(result);
  for (i = 0; i < length; ++i) {
    if (i + 3 < length) {
      done = 0;
      if (code[i] == 0 && code[i + 1] == 0 && code[i + 2] == 0) {
        done = 1;
        if (code[i + 3] == 0) {
          result[resultindex++] = zero4;
          ++i;
        } else {
          result[resultindex++] = zero3;
        }
        i += 2;
      }
      if (done) continue;
      if (code[i] == 255 && code[i + 1] == 255
          && code[i + 2] == 255 && code[i + 3] == 255) {
        done = 1;
        result[resultindex++] = ff4;
        i += 3;
      }
      if (done) continue;
    }
    if (i + 1 < length) {
      done = 0;
      for (j = 0; j < decompress2arr2len; j += 2) {
        if (code[i] == decompress2arr2[j] &&
            code[i + 1] == decompress2arr2[j + 1]) {
          done = 1;
          result[resultindex++] = j / 2 + 34;
          ++i;
          break;
        }
      }
      if (done) continue;
    }
    if (compress2arr[code[i]] || code[i] == 0) {
      result[resultindex++] = compress2arr[code[i]];
    } else if (compress2arr3[code[i]]) {
      result[resultindex++] = 33;
      result[resultindex++] = compress2arr3[code[i]] - 1;
    } else {
      result[resultindex++] = 32;
      result[resultindex++] = code[i] & 31;
      result[resultindex++] = (code[i] >> 5) & 31;
    }
  }
  if (dflag) {
    for (i = 0; i < resultindex; ++i) {
      printf("%d ", result[i]);
    }
    printf("\n");
    printf("len: %d\n", resultindex);
    if (allocatelen < resultindex) {
      printf("invalid len: %d < %d\n", allocatelen, resultindex);
    }
  }
  compress3(result, resultindex);
  free(result);
}

#define STORE_INT(var)\
  result[resultindex++] = var & 255;\
  result[resultindex++] = (var >> 8) & 255;\
  result[resultindex++] = (var >> 16) & 255;\
  result[resultindex++] = (var >> 24) & 255;
#define STORE_FRAME(id, num1, num2)\
  result[resultindex++] = id;\
  STORE_INT(num1);\
  STORE_INT(num2);
void compress1(void) {
  uint32_t i, j, resultindex, counter, flagid1;
  uint8_t * result;
  uint8_t mazeij;
  char rest, restmax;
  if (dflag) {
    printf("sizex: %d\n", SIZEX);
    printf("sizey: %d\n", SIZEY);
    printf("len: %d\n", SIZEX * SIZEY);
  }
  counter = 0;
  for (i = 0; i < SIZEX; ++i)
    for (j = 0; j < SIZEY; ++j)
      if ((mazeij = maze[i][j]) != road && mazeij != wall)
        ++counter;
  result = (uint8_t *) calloc((counter + 5) * 9
                             + (SIZEX * SIZEY) / 8, sizeof(uint8_t));
  CHECK_ALLOCATE(result);
  resultindex = 0;
  restmax = rest = 7;
  STORE_FRAME(187, MAZEID, ALL_VERSION);
  STORE_FRAME('Z', SIZEX, SIZEY);
  if (aflag || nflag) {
    flagid1 = aflag ? 2 : 0; flagid1 |= nflag ? 1 : 0;
    STORE_FRAME('F', flagid1, 0);
  }
  for (i = 0; i < SIZEX; ++i) {
    for (j = 0; j < SIZEY; ++j) {
      if ((mazeij = maze[i][j]) != road && mazeij != wall) {
        STORE_FRAME(mazeij == start ? 2 :
                    mazeij == goal ? 3 : 1, i, j);
      }
    }
  }
  STORE_FRAME(0, 0, 0);
  for (i = 0; i < SIZEX; ++i) {
    for (j = 0; j < SIZEY; ++j) {
      result[resultindex] |= (maze[i][j] == wall ? 0 : 1) << rest;
      if (rest) {
        --rest;
      } else {
        rest = restmax;
        ++resultindex;
      }
    }
  }
  ++resultindex;
  if (dflag) {
    for (i = 0; i < resultindex; ++i) {
      printf("%d ", result[i]);
    }
    printf("\n");
    printf("len: %d\n", resultindex);
  }
  compress2(result, resultindex);
  free(result);
}

char canmove(uint32_t i, uint32_t j) {
  return maze[i][j + 2] < road ||
         maze[i][j - 2] < road ||
         maze[i + 2][j] < road ||
         maze[i - 2][j] < road;
}

char tempx[4];
char tempy[4];
void mazebranch(uint32_t i, uint32_t j) {
  uint32_t n, m, k, l;
  m = 4;
  desideroad(i, j);
  l = 0;
  while (m) {
    m = 0;
    if (maze[i][j + 2] < road) { tempx[m] = 0; tempy[m] = 1; ++m; }
    if (maze[i][j - 2] < road) { tempx[m] = 0; tempy[m] = - 1; ++m; }
    if (maze[i + 2][j] < road) { tempx[m] = 1; tempy[m] = 0; ++m; }
    if (maze[i - 2][j] < road) { tempx[m] = - 1; tempy[m] = 0; ++m; }
    if (m) {
      n = rand() % m;
      desideroad(i += tempx[n], j += tempy[n]);
      desideroad(i += tempx[n], j += tempy[n]);
      memox[memoindex] = i;
      memoy[memoindex++] = j;
    }
    if (++l > SIZEX || l > SIZEY) {
      m = 0;
    }
  }
  for (k = m = 0; m < memoindex; ++m) {
    if (canmove(memox[m], memoy[m])) {
      memox[k] = memox[m];
      memoy[k] = memoy[m];
      ++k;
    }
  }
  memoindex = k;
  if (memoindex > 0) {
    n = rand() % (memoindex / 7 + 1);
    if (n % 2) {
      n += memoindex / 7 * 6;
    }
    mazebranch(memox[n], memoy[n]);
  }
}

void usage(void)
{
  (void)fprintf(stderr,
      "usage: "
      PACKAGE
      " ["
      " -w width |"
      " -h height |"
      " -[ane] |"
      " -[cC] |"
      " -u compressed_maze |"
      " -d |"
      " -v"
      " ]\n");
  fflush(stderr);
  exit(EXIT_FAILURE);
}

void version(void)
{
  printf(PACKAGE_STRING " (compress version: %d)\n", COMPRESS_VERSION);
  fflush(stdout);
  exit(EXIT_SUCCESS);
}

void initialize(int argc, char *argv[]) {
  char * endptr;
  uint32_t ch;
  undesided = 0; wall = 1; road = 2; start = 3; goal = 4;
  while ((ch = getopt(argc, argv, ":w:h:anecCu:dv")) != -1) {
    switch (ch) {
      case 'w': HALFSIZEY = strtol(optarg, &endptr, 10) + 1; break;
      case 'h': HALFSIZEX = strtol(optarg, &endptr, 10) + 1; break;
      case 'a': aflag = 1; nflag = 0; break;
      case 'n': nflag = 1; aflag = 0; break;
      case 'e': eflag = 1; aflag = dflag = 0; break;
      case 'c': cflag = 1; break;
      case 'C': Cflag = cflag = 1; break;
      case 'u': uflag = 1;
                decompress3((uint8_t *)optarg, strlen(optarg));
                exit(EXIT_SUCCESS);
                break;
      case 'd': dflag = 1; break;
      case 'v': version(); break; /* exit */
      case '?': warnx("illegal option -- %c", optopt); /* fall through */
      default: usage(); /* exit */
    }
  }
  SIZEX = HALFSIZEX * 2 + 1;
  SIZEY = HALFSIZEY * 2 + 1;
  if (HALFSIZEX < 2 || HALFSIZEY < 2) {
    fprintf(stderr, PACKAGE ": too small size\n");
    exit(EXIT_FAILURE);
  }
  if (HALFSIZEX > 0xffff || HALFSIZEY > 0xffff) {
    fprintf(stderr, PACKAGE ": too large size\n");
    exit(EXIT_FAILURE);
  }
}

void allocate(void) {
  uint32_t i, j;
  maze = (uint8_t **) calloc(SIZEX, sizeof(uint8_t *));
  CHECK_ALLOCATE(maze);
  _maze = (uint8_t *) calloc(SIZEX * SIZEY, sizeof(uint8_t));
  CHECK_ALLOCATE(_maze);
  for (i = 0; i < SIZEX; ++i) {
    maze[i] = _maze + i * SIZEY;
  }
  desided = (uint8_t **) calloc(SIZEX, sizeof(uint8_t *));
  CHECK_ALLOCATE(desided);
  _desided = (uint8_t *) calloc(SIZEX * SIZEY, sizeof(uint8_t));
  CHECK_ALLOCATE(_desided);
  for (i = 0; i < SIZEX; ++i) {
    desided[i] = _desided + i * SIZEY;
  }
  memox = (uint32_t *) calloc(HALFSIZEX * HALFSIZEY, sizeof(uint32_t));
  CHECK_ALLOCATE(memox);
  memoy = (uint32_t *) calloc(HALFSIZEX * HALFSIZEY, sizeof(uint32_t));
  CHECK_ALLOCATE(memoy);
  memoindex = 0;
}

void deallocate(void) {
  free(_maze);
  free(maze);
  free(_desided);
  free(desided);
  free(memox);
  free(memoy);
}

void mazeroutine(void) {
  uint32_t i, j;
  for (i = 0; i < SIZEX; ++i) {
    desideroad(i, 0);
    desideroad(i, SIZEY - 1);
  }
  for (j = 0; j < SIZEY; ++j) {
    desideroad(0, j);
    desideroad(SIZEX - 1, j);
  }
  maze[2][1] = start;
  maze[SIZEX - 3][SIZEY - 2] = goal;
  mazebranch(2, 2);
  if (!cflag || Cflag) {
    printmaze();
  }
  if (cflag) {
    compress1();
  }
}

int main(int argc, char *argv[]) {
#ifdef HAVE_GETTIMEOFDAY
  struct timeval s;
  gettimeofday(&s, NULL);
  srand((unsigned int)(time(NULL) + s.tv_usec));
#else
  srand((unsigned int)time(NULL));
#endif /* HAVE_GETTIMEOFDAY */
  terminalsize();
  initialize(argc, argv);
  allocate();
  mazeroutine();
  deallocate();
  return 0;
}

